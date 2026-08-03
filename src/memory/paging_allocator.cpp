#include "paging_allocator.h"
#include "os_process.h"   // get_current_time()
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

PagingAllocator::PagingAllocator(size_t maximumSize, size_t frameSize)
    : maximumSize(maximumSize), frameSize(frameSize) {
    numFrames = maximumSize / frameSize;
    frameOwnerPid.assign(numFrames, -1);
    frameOwnerPage.assign(numFrames, -1);

	// Create backing store file if it does not exist
    std::ofstream backing(backingStoreFile, std::ios::app);
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

	std::cout << "[PAGE OUT] PID: "
          << victimPid
          << " Page: "
          << victimPage
          << " Frame: "
          << victimFrame
          << std::endl;

    if (victimPid != -1) {
        auto victimTableIt = pageTables.find(victimPid);
        if (victimTableIt != pageTables.end() && victimPage >= 0 && static_cast<size_t>(victimPage) < victimTableIt->second.size()) {
            victimTableIt->second[victimPage].frame = -1;
            victimTableIt->second[victimPage].on_backing_store = true;
            victimTableIt->second[victimPage].is_dirty = false;
        }
    }
    numPagedOut++;

    writeToBackingStore(victimPid, victimPage);

    frameOwnerPid[victimFrame] = -1;
    frameOwnerPage[victimFrame] = -1;

    return victimFrame;
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
	std::cout << "[PAGE FAULT] PID: "
          << pid
          << " Page: "
          << page_number
          << " needs a frame"
          << std::endl;

    int frame = obtainFrame();
    if (frame == -1) {
        return false;
    }

    frameOwnerPid[frame] = pid;
    frameOwnerPage[frame] = static_cast<int>(page_number);
    frameFifo.push_back(frame);

	if (entry.on_backing_store) {
		loadFromBackingStore(pid, page_number);
	}
    entry.frame = frame;
    entry.on_backing_store = false;
    entry.is_dirty = for_write;
    numPagedIn++;

	std::cout << "[PAGE IN] PID: "
          << pid
          << " Page: "
          << page_number
          << " -> Frame: "
          << frame
          << std::endl;
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

    // A process that needs more pages than physically exist can NEVER be
    // admitted, no matter how much paging/eviction we do - fail fast.
    if (numPages > numFrames) return nullptr;

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

void PagingAllocator::writeToBackingStore(int pid, int page)
{
	std::cout << "[BACKING STORE WRITE] "
          << "PID: "
          << pid
          << " Page: "
          << page
          << std::endl;
    std::ofstream backing(backingStoreFile, std::ios::app);

    if (!backing.is_open())
        return;

    backing << "PAGE_OUT "
            << "PID=" << pid
            << " PAGE=" << page;

    auto name = processNames.find(pid);

    if (name != processNames.end())
        backing << " PROCESS=" << name->second;

    backing << "\n";
}

bool PagingAllocator::loadFromBackingStore(int pid, int page)
{
    std::ifstream backing(backingStoreFile);

    if (!backing.is_open())
        return false;

    std::string line;

    while (std::getline(backing, line))
    {
        std::stringstream ss(line);

        std::string type;
        int storedPid;
        int storedPage;

        ss >> type;

        if(type != "PAGE_OUT")
            continue;

        ss.ignore(4); // PID=
        ss >> storedPid;

        ss.ignore(6); // PAGE=
        ss >> storedPage;


        if(storedPid == pid && storedPage == page)
        {
			std::cout << "[BACKING STORE READ] "
				<< "PID: "
				<< pid
				<< " Page: "
				<< page
				<< std::endl;
            removeFromBackingStore(pid,page);
            return true;
        }
    }

    return false;
}

void PagingAllocator::removeFromBackingStore(int pid, int page)
{
    std::ifstream input(backingStoreFile);

    if(!input.is_open())
        return;


    std::vector<std::string> lines;
    std::string line;


    while(std::getline(input,line))
    {
        if(line.find(
            "PID=" + std::to_string(pid) +
            " PAGE=" + std::to_string(page)
        ) == std::string::npos)
        {
            lines.push_back(line);
        }
    }

    input.close();


    std::ofstream output(backingStoreFile);

    for(auto& l : lines)
        output << l << "\n";
}