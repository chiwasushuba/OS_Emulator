#include "memory_allocator.h"
#include "os_process.h"   // get_current_time()
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

FirstFitAllocator::FirstFitAllocator(size_t maximumSize)
    : maximumSize(maximumSize), allocatedSize(0) {
    memory.resize(maximumSize);
    allocationMap.resize(maximumSize);
    initializeMemory();
}

void FirstFitAllocator::initializeMemory() {
    std::fill(memory.begin(), memory.end(), '.'); // '.' = unallocated memory
    std::fill(allocationMap.begin(), allocationMap.end(), false);
}

bool FirstFitAllocator::isRangeFree(size_t index, size_t size) const {
    if (index + size > maximumSize) return false;
    for (size_t i = index; i < index + size; ++i) {
        if (allocationMap[i]) return false;
    }
    return true;
}

void FirstFitAllocator::markRange(size_t index, size_t size, bool value) {
    std::fill(allocationMap.begin() + index, allocationMap.begin() + index + size, value);
}

void* FirstFitAllocator::allocate(size_t size, int pid, const std::string& process_name) {
    std::lock_guard<std::mutex> lock(mem_mutex);

    if (size == 0 || size > maximumSize) return nullptr;

    // First-fit scan: first index whose whole [i, i+size) range is free
    for (size_t i = 0; i + size <= maximumSize; ++i) {
        if (!allocationMap[i] && isRangeFree(i, size)) {
            markRange(i, size, true);
            std::fill(memory.begin() + i, memory.begin() + i + size, '#');

            allocatedSize += size;
            allocatedBlocks.push_back(MemoryBlock{i, i + size, pid, process_name});

            return &memory[i];
        }
    }

    return nullptr; // no hole big enough - scheduler keeps the process in the ready queue
}

void FirstFitAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) return;

    std::lock_guard<std::mutex> lock(mem_mutex);

    size_t index = static_cast<char*>(ptr) - &memory[0];

    auto it = std::find_if(allocatedBlocks.begin(), allocatedBlocks.end(),
        [index](const MemoryBlock& b) { return b.start == index; });

    if (it == allocatedBlocks.end()) return; // unknown block, ignore

    size_t size = it->end - it->start;
    markRange(index, size, false);
    std::fill(memory.begin() + index, memory.begin() + index + size, '.');

    allocatedSize -= size;
    allocatedBlocks.erase(it);
}

std::string FirstFitAllocator::visualizeMemory() {
    std::lock_guard<std::mutex> lock(mem_mutex);
    return std::string(memory.begin(), memory.end());
}

size_t FirstFitAllocator::get_maximum_size() const {
    return maximumSize;
}

size_t FirstFitAllocator::get_allocated_size() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    return allocatedSize;
}

size_t FirstFitAllocator::get_free_size() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    return maximumSize - allocatedSize;
}

size_t FirstFitAllocator::get_num_processes_in_memory() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    return allocatedBlocks.size();
}

std::vector<MemoryBlock> FirstFitAllocator::get_allocated_blocks() const {
    std::lock_guard<std::mutex> lock(mem_mutex);
    std::vector<MemoryBlock> blocks = allocatedBlocks;
    std::sort(blocks.begin(), blocks.end(),
        [](const MemoryBlock& a, const MemoryBlock& b) { return a.start < b.start; });
    return blocks;
}

// Writes memory_stamp_<qq>.txt with timestamp, process count, external
// fragmentation, and an ASCII layout of memory from the top address down to 0.
//
// External fragmentation here = total free bytes currently in memory
// (max-overall-mem - allocated). Since every process takes a fixed
// mem-per-proc size, any free space left over is unusable "fragmentation".
void FirstFitAllocator::generate_memory_stamp(uint64_t quantum_cycle, const std::string& output_dir) const {
    std::vector<MemoryBlock> blocks = get_allocated_blocks();
    size_t used = get_allocated_size();
    size_t freeBytes = maximumSize - used;

    std::ostringstream ss;
    ss << "Timestamp: (" << get_current_time() << ")\n";
    ss << "Number of processes in memory: " << blocks.size() << "\n";
    ss << "Total external fragmentation in KB: " << (freeBytes / 1024) << "\n\n";

    ss << "----end---- = " << maximumSize << "\n\n";

    // Print top-down (highest address first), matching the mockup layout.
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        ss << it->end << "\n";
        ss << it->process_name << "\n";
        ss << it->start << "\n\n";
    }

    ss << "----start---- = 0\n";

    std::string filename = output_dir + "memory_stamp_" + std::to_string(quantum_cycle) + ".txt";
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << ss.str();
    } else {
        std::cerr << "Failed to write memory stamp file: " << filename << "\n";
    }
}
