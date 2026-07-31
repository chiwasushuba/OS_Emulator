#pragma once
#include "memory_allocator.h"   // IMemoryAllocator, MemoryBlock
#include <vector>
#include <unordered_map>
#include <deque>
#include "../compat/mutex_compat.h"

// A single Page Table Entry: which physical frame (if any) currently holds
// this page. If the page isn't resident, it has been written to the backing
// store and frame == -1.
struct PageTableEntry {
    int frame = -1;
    bool on_backing_store = false;
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
class PagingAllocator : public IMemoryAllocator {
public:
    PagingAllocator(size_t maximumSize, size_t frameSize);
    ~PagingAllocator() override = default;

    void* allocate(size_t size, int pid, const std::string& process_name) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
    void generate_memory_stamp(uint64_t quantum_cycle, const std::string& output_dir = "../../") const override;

    // Reporting helpers - mirrors FirstFitAllocator's surface so the rest of
    // the codebase (report_generator, process_viewer) doesn't need to care
    // which allocator implementation is actually in use.
    size_t get_maximum_size() const override;
    size_t get_allocated_size() const override;
    size_t get_free_size() const override;
    size_t get_num_processes_in_memory() const;

    size_t get_num_paged_in() const override;
    size_t get_num_paged_out() const override;

private:
    size_t maximumSize;
    size_t frameSize;
    size_t numFrames;

    // Frame table: for each physical frame, which pid/page currently
    // occupies it (-1 / -1 if free). This is the physical side of the
    // page -> frame mapping.
    std::vector<int> frameOwnerPid;
    std::vector<int> frameOwnerPage;

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
