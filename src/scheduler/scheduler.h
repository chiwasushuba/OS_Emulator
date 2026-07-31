#pragma once
#include <queue>
#include <vector>
#include <unordered_map>
#include "os_process.h"
#include "cpu.h"
#include "memory_allocator.h"

class Scheduler {
protected:
    CPUManager& cpu_manager;
    ProcessManager& process_manager;
    IMemoryAllocator& memory_allocator;
public:
    Scheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc)
        : cpu_manager(cpu_m), process_manager(proc_m), memory_allocator(mem_alloc) {}
    
    virtual ~Scheduler() = default;

    // Called when a new process is created
    virtual void add_process(Process* process) = 0;

    // Called every CPU tick to perform scheduling (assigning processes to cores)
    virtual void tick() = 0;
};

class RoundRobinScheduler : public Scheduler {
private:
    std::queue<Process*> ready_queue;
    uint64_t quantum;
    std::vector<uint64_t> core_cycles;

    // Tracks the memory block currently held by each resident process (by pid),
    // so memory is only allocated once on first admission and freed on FINISH -
    // NOT re-allocated every time a quantum preemption puts it back in the queue.
    std::unordered_map<int, void*> process_memory_ptr;

public:
    RoundRobinScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc, uint64_t quantum);
    
    void add_process(Process* process) override;
    void tick() override;
};

class FCFSScheduler : public Scheduler {
private:
    std::queue<Process*> ready_queue;

    // Tracks the memory block currently held by each resident process (by pid),
    // so memory is allocated on first admission and freed when the process
    // finishes. Mirrors the same pattern used in RoundRobinScheduler.
    std::unordered_map<int, void*> process_memory_ptr;

public:
    FCFSScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc);
    
    void add_process(Process* process) override;
    void tick() override;
};