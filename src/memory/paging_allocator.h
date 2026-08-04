#pragma once
#include "memory_allocator.h" // IMemoryAllocator, MemoryBlock
#include <vector>
#include <unordered_map>
#include <deque>
#include <map>
#include <mutex>
#include <chrono>
#include <utility>
#include <functional>
#include <cstdint>

// A single Page Table Entry: which physical frame (if any) currently holds
// this page. If the page isn't resident, it has been written to the backing
// store and frame == -1.
struct PageTableEntry
{
	int frame = -1;
	bool on_backing_store = false;
	bool is_dirty = false;
};

// Paging memory allocator.
//
// Main memory is divided into fixed-size frames (config.mem_per_frame).
// A process's fixed mem-per-proc allocation is broken into pages of that
// same size. Pages do NOT need contiguous frames - they can be scattered
// anywhere there's a free frame, which is the whole point of paging.
//
// When memory is full and a page needs a frame, the oldest-resident page
// belonging to ANY process is evicted (FIFO) out to a backing-store file,
// freeing its frame for the incoming page.
class PagingAllocator : public IMemoryAllocator
{
public:
	// base_dir anchors csopesy-backing-store.txt next to config.txt, so it is
	// always where the user expects regardless of the launch directory.
	PagingAllocator(size_t maximumSize, size_t frameSize, const std::string &base_dir = "");
	~PagingAllocator() override = default;

	// Main memory access, translated through the calling process's page table.
	// See IMemoryAllocator for the contract.
	bool read_memory(int pid, size_t vaddr, uint16_t &out) const override;
	bool write_memory(int pid, size_t vaddr, uint16_t value) override;

	// Hex dump of one physical frame - for demoing what a frame actually holds.
	std::string dump_frame(size_t frame) const;

	std::map<int, size_t> get_resident_bytes_by_pid() const override;

	// Forces csopesy-backing-store.txt to match memory exactly. Called by the
	// memory-debug commands so the file is current whenever a human looks at it.
	void flush_backing_store();

	void *allocate(size_t size, int pid, const std::string &process_name) override;
	void deallocate(void *ptr) override;
	std::string visualizeMemory() override;
	void generate_memory_stamp(uint64_t quantum_cycle, const std::string &output_dir = "../../") const override;

	// Reporting helpers - mirrors FirstFitAllocator's surface so the rest of
	// the codebase (report_generator, process_viewer) doesn't need to care
	// which allocator implementation is actually in use.
	size_t get_maximum_size() const override;
	size_t get_allocated_size() const override;
	size_t get_free_size() const override;
	size_t get_num_processes_in_memory() const;

	size_t get_num_paged_in() const override;
	size_t get_num_paged_out() const override;
	size_t get_page_size() const override;
	size_t get_page_count(int pid) const override;
	bool is_page_resident(int pid, size_t page_number) const override;
	bool ensure_page_resident(int pid, size_t page_number, bool for_write = false) override;
	bool mark_page_dirty(int pid, size_t page_number) override;

private:
	size_t maximumSize;
	size_t frameSize;
	size_t numFrames;
	
	const std::string backingStoreFile;
	// Each takes the frame index because the page's bytes live in that frame:
	// swapping out reads them from physical memory, swapping in writes them back.
	void writeToBackingStore(int pid, int page, int frame);
    bool loadFromBackingStore(int pid, int page, int frame);
    void removeFromBackingStore(int pid, int page);

	// THE BACKING STORE. Authoritative in memory, mirrored to the text file.
	// It was originally the file itself, but every page fault then cost three
	// full passes over it (scan to find the page, scan again to drop the line,
	// rewrite) - with dozens of cores faulting every tick that is megabytes of
	// parsing per tick, and the emulator ground to a near halt under the stress
	// config. Keyed by (pid, page); the value is the page's bytes.
	std::map<std::pair<int, int>, std::vector<uint8_t>> backingStore;
	bool backingStoreDirty = false;
	std::chrono::steady_clock::time_point lastBackingStoreFlush{};
	// Rewrites the text file from backingStore. Throttled unless force is set,
	// so the file is never more than a fraction of a second behind.
	void flushBackingStore(bool force);

	// MAIN MEMORY. maximumSize bytes, carved into numFrames frames of frameSize.
	// This is the only place process data physically lives - a process's bytes
	// exist here while its page holds a frame, and in the backing-store file
	// while it does not. Pre-allocated at startup, per the spec ("memory spaces
	// are pre-allocated and free to use by any processes upon startup").
	std::vector<uint8_t> physicalMemory;

	// Frame table: for each physical frame, which pid/page currently
	// occupies it (-1 / -1 if free). This is the physical side of the
	// page -> frame mapping.
	std::vector<int> frameOwnerPid;
	std::vector<int> frameOwnerPage;

	// Virtual -> physical translation. Returns the byte offset into
	// physicalMemory, or SIZE_MAX when the page has no frame (page fault).
	// Caller must already hold mem_mutex.
	size_t translate_locked(int pid, size_t vaddr) const;

	// Zeroes a frame so an incoming page never sees the previous tenant's bytes.
	void clear_frame_locked(int frame);

	// Per-process page table: pid -> one PageTableEntry per page the
	// process owns. Flat, single-level (see Q8 in the writeup).
	std::unordered_map<int, std::vector<PageTableEntry>> pageTables;
	std::unordered_map<int, std::string> processNames;

	// FIFO victim queue: order in which frames were filled, so eviction
	// always takes the oldest-resident page first.
	std::deque<int> frameFifo;

	size_t numPagedIn = 0;
	size_t numPagedOut = 0;

	mutable std::mutex mem_mutex;

	// Returns a free frame index, evicting a FIFO victim first if memory
	// is full. Returns -1 only if numFrames == 0.
	int obtainFrame();
};
