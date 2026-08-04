#include "paging_allocator.h"
#include "os_process.h"   // get_current_time()
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

PagingAllocator::PagingAllocator(size_t maximumSize, size_t frameSize, const std::string& base_dir)
    : maximumSize(maximumSize), frameSize(frameSize),
      backingStoreFile(base_dir + "csopesy-backing-store.txt") {
    numFrames = maximumSize / frameSize;
    frameOwnerPid.assign(numFrames, -1);
    frameOwnerPage.assign(numFrames, -1);

    // Pre-allocate main memory up front, as the spec describes.
    physicalMemory.assign(numFrames * frameSize, 0);

	// Create the backing store file, truncating any leftovers from a previous
	// run. Stale PAGE_OUT lines would otherwise be matched against a fresh
	// run's pids and misreported as pages already on disk.
    std::ofstream backing(backingStoreFile, std::ios::trunc);
    backing.close();
}

int PagingAllocator::obtainFrame() {
    // 1. Prefer a genuinely free frame.
    for (size_t f = 0; f < numFrames; ++f) {
        if (frameOwnerPid[f] == -1) return static_cast<int>(f);
    }

    // 2. Memory is full: evict the oldest-resident page (FIFO policy) to
    // the backing store to free up its frame.
    if (frameFifo.empty()) return -1; // only possible if numFrames == 0

    int victimFrame = frameFifo.front();
    frameFifo.pop_front();

    int victimPid = frameOwnerPid[victimFrame];
    int victimPage = frameOwnerPage[victimFrame];

    if (victimPid != -1) {
        auto victimTableIt = pageTables.find(victimPid);
        if (victimTableIt != pageTables.end() && victimPage >= 0 && static_cast<size_t>(victimPage) < victimTableIt->second.size()) {
            victimTableIt->second[victimPage].frame = -1;
            victimTableIt->second[victimPage].on_backing_store = true;
            victimTableIt->second[victimPage].is_dirty = false;
        }
    }
    numPagedOut++;

    // Move the victim's bytes out of main memory and into the backing store,
    // then blank the frame - the page now exists only on "disk".
    writeToBackingStore(victimPid, victimPage, victimFrame);
    clear_frame_locked(victimFrame);

    frameOwnerPid[victimFrame] = -1;
    frameOwnerPage[victimFrame] = -1;

    return victimFrame;
}

void PagingAllocator::clear_frame_locked(int frame) {
    if (frame < 0 || static_cast<size_t>(frame) >= numFrames) return;
    size_t start = static_cast<size_t>(frame) * frameSize;
    std::fill(physicalMemory.begin() + start,
              physicalMemory.begin() + start + frameSize,
              static_cast<uint8_t>(0));
}

size_t PagingAllocator::translate_locked(int pid, size_t vaddr) const {
    auto it = pageTables.find(pid);
    if (it == pageTables.end()) return SIZE_MAX;

    size_t page = vaddr / frameSize;
    if (page >= it->second.size()) return SIZE_MAX;

    int frame = it->second[page].frame;
    if (frame < 0) return SIZE_MAX; // page fault - not resident

    return static_cast<size_t>(frame) * frameSize + (vaddr % frameSize);
}

bool PagingAllocator::read_memory(int pid, size_t vaddr, uint16_t& out) const {
    std::lock_guard<std::mutex> lock(mem_mutex);

    // A uint16 is 2 bytes and may straddle a page boundary when unaligned, so
    // both halves are translated independently - they can sit in frames that
    // are nowhere near each other, which is the point of paging.
    size_t lo = translate_locked(pid, vaddr);
    size_t hi = translate_locked(pid, vaddr + 1);
    if (lo == SIZE_MAX || hi == SIZE_MAX) return false;

    out = static_cast<uint16_t>(physicalMemory[lo]) |
          static_cast<uint16_t>(physicalMemory[hi] << 8);
    return true;
}

bool PagingAllocator::write_memory(int pid, size_t vaddr, uint16_t value) {
    std::lock_guard<std::mutex> lock(mem_mutex);

    size_t lo = translate_locked(pid, vaddr);
    size_t hi = translate_locked(pid, vaddr + 1);
    if (lo == SIZE_MAX || hi == SIZE_MAX) return false;

    physicalMemory[lo] = static_cast<uint8_t>(value & 0xFF);
    physicalMemory[hi] = static_cast<uint8_t>((value >> 8) & 0xFF);

    // Both touched pages are now dirty.
    auto it = pageTables.find(pid);
    if (it != pageTables.end()) {
        for (size_t addr : {vaddr, vaddr + 1}) {
            size_t page = addr / frameSize;
            if (page < it->second.size()) it->second[page].is_dirty = true;
        }
    }
    return true;
}

std::string PagingAllocator::dump_frame(size_t frame) const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    if (frame >= numFrames) return {};

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    size_t start = frame * frameSize;
    for (size_t i = 0; i < frameSize; ++i)
        hex << std::setw(2) << static_cast<unsigned>(physicalMemory[start + i]);
    return hex.str();
}

size_t PagingAllocator::get_page_size() const {
    return frameSize;
}

size_t PagingAllocator::get_page_count(int pid) const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end()) return 0;
    return it->second.size();
}

bool PagingAllocator::is_page_resident(int pid, size_t page_number) const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end() || page_number >= it->second.size()) {
        return false;
    }
    return it->second[page_number].frame != -1;
}

bool PagingAllocator::ensure_page_resident(int pid, size_t page_number, bool for_write) {
    std::lock_guard<std::mutex> lock(mem_mutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end() || page_number >= it->second.size()) {
        return false;
    }

    PageTableEntry& entry = it->second[page_number];
    if (entry.frame != -1) {
        if (for_write) {
            entry.is_dirty = true;
        }
        return true;
    }

    int frame = obtainFrame();
    if (frame == -1) {
        return false;
    }

    frameOwnerPid[frame] = pid;
    frameOwnerPage[frame] = static_cast<int>(page_number);
    frameFifo.push_back(frame);

	// A page touched for the first time starts zeroed ("if the memory block
	// isn't initialized, the uint16 value is 0"); one coming back from the
	// backing store gets its saved bytes copied into the new frame.
	clear_frame_locked(frame);
	if (entry.on_backing_store) {
		loadFromBackingStore(pid, static_cast<int>(page_number), frame);
	}
    entry.frame = frame;
    entry.on_backing_store = false;
    entry.is_dirty = for_write;
    numPagedIn++;

    return true;
}

bool PagingAllocator::mark_page_dirty(int pid, size_t page_number) {
    std::lock_guard<std::mutex> lock(mem_mutex);
    auto it = pageTables.find(pid);
    if (it == pageTables.end() || page_number >= it->second.size()) {
        return false;
    }
    it->second[page_number].is_dirty = true;
    return true;
}

void* PagingAllocator::allocate(size_t size, int pid, const std::string& process_name) {
    std::lock_guard<std::mutex> lock(mem_mutex);

    if (size == 0 || numFrames == 0) return nullptr;

    size_t numPages = (size + frameSize - 1) / frameSize; // ceil division

    // NOTE: numPages > numFrames is deliberately allowed. Under demand paging a
    // process's address space is virtual - only the pages it actually touches
    // need a frame, and an instruction touches at most two at once. Rejecting
    // such a process would break the spec's own scenario ("assume the physical
    // memory is full... the demand pager finds a victim frame to be removed").
    // Admission only creates the page table; no frame is claimed until a fault.
    processNames[pid] = process_name;
    pageTables[pid] = std::vector<PageTableEntry>(numPages);

    // Memory isn't one contiguous block under paging, so there's no single
    // address to return. We hand back an opaque handle (the pid) that
    // deallocate() maps back to the page table. See Q5 in the writeup.
    return reinterpret_cast<void*>(static_cast<intptr_t>(pid));
}

void PagingAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) return;

    std::lock_guard<std::mutex> lock(mem_mutex);

    int pid = static_cast<int>(reinterpret_cast<intptr_t>(ptr));

    auto it = pageTables.find(pid);
    if (it == pageTables.end()) return; // unknown handle, ignore

    for (auto& pte : it->second) {
        if (pte.frame != -1) {
            // Wipe the frame on release so the next process to claim it can
            // never read this one's leftovers.
            clear_frame_locked(pte.frame);
            frameOwnerPid[pte.frame] = -1;
            frameOwnerPage[pte.frame] = -1;
            frameFifo.erase(std::remove(frameFifo.begin(), frameFifo.end(), pte.frame),
                             frameFifo.end());
        }
        // Pages that were out on the backing store simply vanish - the
        // process is gone, there's nothing left to page back in.
    }
	for(size_t page = 0; page < it->second.size(); page++)
	{
		removeFromBackingStore(pid, page);
	}
    pageTables.erase(it);
    processNames.erase(pid);
}

std::string PagingAllocator::visualizeMemory() {
    std::lock_guard<std::mutex> lock(mem_mutex);
    std::string out(numFrames, '.');
    for (size_t f = 0; f < numFrames; ++f) {
        if (frameOwnerPid[f] != -1) out[f] = '#';
    }
    return out;
}

size_t PagingAllocator::get_maximum_size() const { return maximumSize; }

size_t PagingAllocator::get_allocated_size() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    size_t used = 0;
    for (int owner : frameOwnerPid) if (owner != -1) used++;
    return used * frameSize;
}

size_t PagingAllocator::get_free_size() const {
    return maximumSize - get_allocated_size();
}

std::map<int, size_t> PagingAllocator::get_resident_bytes_by_pid() const {
    std::lock_guard<std::mutex> lock(mem_mutex);

    // One pass over the frame table - O(frames), and crucially ONE lock. The
    // reporting commands used to ask is_page_resident() per process per page,
    // taking this mutex thousands of times while dozens of cores were trying
    // to service page faults through it; under the stress config that starved
    // the kernel loop badly enough to look like a deadlock.
    std::map<int, size_t> resident;
    for (const auto& entry : pageTables) resident[entry.first] = 0; // include 0-resident procs
    for (size_t f = 0; f < numFrames; ++f) {
        if (frameOwnerPid[f] != -1) resident[frameOwnerPid[f]] += frameSize;
    }
    return resident;
}

size_t PagingAllocator::get_num_processes_in_memory() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    return pageTables.size();
}

size_t PagingAllocator::get_num_paged_in() const { return numPagedIn; }
size_t PagingAllocator::get_num_paged_out() const { return numPagedOut; }

void PagingAllocator::generate_memory_stamp(uint64_t quantum_cycle, const std::string& output_dir) const {
    std::lock_guard<std::mutex> lock(mem_mutex);

    size_t used = 0;
    for (int owner : frameOwnerPid) if (owner != -1) used++;
    size_t usedBytes = used * frameSize;

    std::ostringstream ss;
    ss << "Timestamp: (" << get_current_time() << ")\n";
    ss << "Number of processes in memory: " << pageTables.size() << "\n";
    // Under pure paging with fixed-size frames, external fragmentation is
    // 0 by definition: any free frame can satisfy any page request, since
    // a process only ever needs whole frames rather than one contiguous
    // run of bytes. See Q6 in the writeup for where the "lost" space goes
    // instead (internal fragmentation).
    ss << "Total external fragmentation in KB: 0\n\n";
    ss << "Pages paged in (total): " << numPagedIn << "\n";
    ss << "Pages paged out (total): " << numPagedOut << "\n\n";

    for (const auto& entry : pageTables) {
        int pid = entry.first;
        const auto& table = entry.second;
        ss << processNames.at(pid) << " (pid " << pid << "): ";
        for (size_t page = 0; page < table.size(); ++page) {
            ss << "[" << page << "->";
            if (table[page].frame != -1) ss << "F" << table[page].frame;
            else ss << "backing-store";
            ss << "] ";
        }
        ss << "\n";
    }

    std::string filename = output_dir + "memory_stamp_" + std::to_string(quantum_cycle) + ".txt";
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << ss.str();
    } else {
        std::cerr << "Failed to write memory stamp file: " << filename << "\n";
    }
}

// The tag that uniquely identifies one page's record in the store. The trailing
// space matters: without it, "PID=1 PAGE=1" also matches "PID=1 PAGE=10" and
// removeFromBackingStore would drop the wrong page's data.
static std::string page_tag(int pid, int page)
{
    return "PID=" + std::to_string(pid) + " PAGE=" + std::to_string(page) + " ";
}

void PagingAllocator::writeToBackingStore(int pid, int page, int frame)
{
    // Copy the page's real contents straight out of the frame it is being
    // evicted from. This is what makes the store a swap file, not a log.
    std::vector<uint8_t> bytes;
    if (frame >= 0 && static_cast<size_t>(frame) < numFrames) {
        size_t start = static_cast<size_t>(frame) * frameSize;
        bytes.assign(physicalMemory.begin() + start,
                     physicalMemory.begin() + start + frameSize);
    }

    backingStore[{pid, page}] = std::move(bytes);
    backingStoreDirty = true;
    flushBackingStore(false);
}

void PagingAllocator::flushBackingStore(bool force)
{
    if (!backingStoreDirty) return;

    auto now = std::chrono::steady_clock::now();
    if (!force && now - lastBackingStoreFlush < std::chrono::milliseconds(200))
        return;

    std::ofstream backing(backingStoreFile, std::ios::trunc);
    if (!backing.is_open()) return;

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto& entry : backingStore) {
        int pid = entry.first.first;
        int page = entry.first.second;
        auto name = processNames.find(pid);

        out << std::dec << "PAGE_OUT " << page_tag(pid, page)
            << "PROCESS=" << (name != processNames.end() ? name->second : std::string("?"))
            << " DATA=" << std::hex;
        for (uint8_t b : entry.second)
            out << std::setw(2) << static_cast<unsigned>(b);
        out << "\n";
    }
    backing << out.str();

    backingStoreDirty = false;
    lastBackingStoreFlush = now;
}

void PagingAllocator::flush_backing_store()
{
    std::lock_guard<std::mutex> lock(mem_mutex);
    flushBackingStore(true);
}

bool PagingAllocator::loadFromBackingStore(int pid, int page, int frame)
{
    auto it = backingStore.find({pid, page});
    if (it == backingStore.end())
        return false;

    // Copy the saved bytes back into the frame this page was just given.
    if (frame >= 0 && static_cast<size_t>(frame) < numFrames)
    {
        size_t start = static_cast<size_t>(frame) * frameSize;
        size_t count = std::min(it->second.size(), frameSize);
        std::copy(it->second.begin(), it->second.begin() + count,
                  physicalMemory.begin() + start);
    }

    // The page is back in main memory, so it is no longer in the store.
    backingStore.erase(it);
    backingStoreDirty = true;
    flushBackingStore(false);
    return true;
}

void PagingAllocator::removeFromBackingStore(int pid, int page)
{
    if (backingStore.erase({pid, page}) > 0) {
        backingStoreDirty = true;
        flushBackingStore(false);
    }
}