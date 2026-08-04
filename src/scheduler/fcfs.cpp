#include "scheduler.h"
#include <iostream>

FCFSScheduler::FCFSScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc)
    : Scheduler(cpu_m, proc_m, mem_alloc) {}

void FCFSScheduler::add_process(Process* process) {
    if (process == nullptr) {
        return;
    }
    // A process whose address space exceeds physical memory is still admissible -
    // that is what demand paging is for. Only a sizeless process is rejected;
    // FCFS is non-preemptive and never skips the queue head, so an admitted-but-
    // unrunnable process at the front would deadlock everything behind it.
    if (process->mem_size == 0) {
        std::cout << "Process " << process->process_name
                  << " has no memory allocation. Process not admitted.\n";
        process->state = ProcessState::TERMINATED;
        return;
    }
    std::lock_guard<std::mutex> lock(sched_mutex);
    process->state = ProcessState::READY;
    ready_queue.push(process);
}

void FCFSScheduler::tick() {
    std::lock_guard<std::mutex> lock(sched_mutex);
    auto& cores = cpu_manager.get_cores();

    for (size_t i = 0; i < cores.size(); ++i) {
        CPUCore& core = cores[i];
        
        // 1. Check if the core's process just finished -> release its memory
        if (!core.is_idle()) {
            Process* p = core.get_process();
            if (p->state == ProcessState::FINISHED || p->state == ProcessState::TERMINATED) {
                auto mem_it = process_memory_ptr.find(p->id);
                if (mem_it != process_memory_ptr.end()) {
                    memory_allocator.deallocate(mem_it->second);
                    process_memory_ptr.erase(mem_it);
                }
                core.remove_process();
            }
        }
        
        // 2. If the core is idle and there are processes in the queue, assign one.
        // FCFS is non-preemptive, so we never interrupt a running process.
        // We strictly adhere to FCFS by waiting on the head of the queue if
        // there isn't enough memory to admit it yet.
        if (core.is_idle() && !ready_queue.empty()) {
            Process* p = ready_queue.front();

            // Already resident in memory (defensive — shouldn't happen
            // under FCFS since there's no preemption, but handle it)
            if (process_memory_ptr.find(p->id) != process_memory_ptr.end()) {
                ready_queue.pop();
                core.assign_process(p);
            } else {
                // First admission — allocate memory for this process
                void* mem = memory_allocator.allocate(p->mem_size, p->id, p->process_name);
                if (mem != nullptr) {
                    ready_queue.pop();
                    process_memory_ptr[p->id] = mem;
                    core.assign_process(p);
                }
                // Memory full — we do NOT requeue. We leave it at the front of the 
                // queue and wait for memory to free up in subsequent ticks.
            }
        }
    }
}