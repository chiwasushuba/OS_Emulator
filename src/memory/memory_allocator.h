#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include "../compat/mutex_compat.h"

// A contiguous chunk of memory currently owned by a process.
// start is inclusive, end is exclusive (end - start == size allocated).
struct MemoryBlock {
    size_t start;
    size_t end;
    int pid;
    std::string process_name;
};

// Same shape as the professor's IMemoryAllocator, extended with pid/name so
// the allocator can produce the memory_stamp_<qq>.txt reports on its own.
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;

    // Returns a pointer identifying the allocated block, or nullptr if there
    // is no free space big enough (caller must NOT retry immediately -
    // that's the scheduler's job, e.g. requeue at the tail).
    virtual void* allocate(size_t size, int pid, const std::string& process_name) = 0;

    // Frees the block previously returned by allocate(). No-op on nullptr.
    virtual void deallocate(void* ptr) = 0;

    // Raw ASCII dump of memory ('.' = free, '#' = allocated).
    virtual std::string visualizeMemory() = 0;

    // Writes "memory_stamp_<quantum_cycle>.txt" to output_dir.
    virtual void generate_memory_stamp(uint64_t quantum_cycle, const std::string& output_dir = "../../") const = 0;

    // Reporting helpers — overridden by concrete allocators
    virtual size_t get_maximum_size() const { return 0; }
    virtual size_t get_allocated_size() const { return 0; }
    virtual size_t get_free_size() const { return 0; }
    virtual size_t get_num_paged_in() const { return 0; }
    virtual size_t get_num_paged_out() const { return 0; }
};

// First-fit flat memory allocator: scans memory from address 0 upward and
// places a process in the first hole big enough to hold it. No paging, no
// backing store - if nothing fits, allocate() returns nullptr.
class FirstFitAllocator : public IMemoryAllocator {
private:
    size_t maximumSize;
    size_t allocatedSize;
    std::vector<char> memory;
    std::vector<bool> allocationMap;
    std::vector<MemoryBlock> allocatedBlocks;
    mutable std::mutex mem_mutex;

    void initializeMemory();
    // NOTE: unlike the professor's canAllocateAt (which only checked the start
    // index), this checks that EVERY byte in [index, index+size) is free -
    // otherwise first-fit could silently stomp on another process's memory.
    bool isRangeFree(size_t index, size_t size) const;
    void markRange(size_t index, size_t size, bool value);

public:
    explicit FirstFitAllocator(size_t maximumSize);
    ~FirstFitAllocator() override = default;

    void* allocate(size_t size, int pid, const std::string& process_name) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;
    void generate_memory_stamp(uint64_t quantum_cycle, const std::string& output_dir = "../../") const override;

    // Reporting helpers
    size_t get_maximum_size() const override;
    size_t get_allocated_size() const override;
    size_t get_free_size() const override;
    size_t get_num_paged_in() const override { return 0; }
    size_t get_num_paged_out() const override { return 0; }
    size_t get_num_processes_in_memory() const;
    std::vector<MemoryBlock> get_allocated_blocks() const; // sorted ascending by start address
};
