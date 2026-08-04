#include "scheduler.h"
#include <iostream>

RoundRobinScheduler::RoundRobinScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc, uint64_t quantum)
    : Scheduler(cpu_m, proc_m, mem_alloc), quantum(quantum > 0 ? quantum : 1) {
    // Initialize the core cycles tracker to match the number of cores
    core_cycles.resize(cpu_manager.get_cores().size(), 0);
}

void RoundRobinScheduler::add_process(Process* process) {
    if (process == nullptr) {
        return;
    }
    // A process whose address space exceeds physical memory is still admissible -
    // that is what demand paging is for. Only a sizeless process is rejected.
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

void RoundRobinScheduler::tick() {
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
                core_cycles[i] = 0;
            }
        }

        // 2. Increment cycle count for running process, check if quantum expired
        if (!core.is_idle()) {
            core_cycles[i]++;

            if (core_cycles[i] >= quantum) {
                // Quantum expired, preempt the process.
                // NOTE: memory is NOT released here - the process stays resident
                // in memory while it waits in the ready queue (per spec: memory
                // is only freed when the process finishes execution).
                Process* p = core.get_process();
                core.remove_process();
                p->state = ProcessState::READY;
                ready_queue.push(p);
                core_cycles[i] = 0;

                continue;
            }
        }

        // 3. If core is idle and we have waiting processes, scan the queue
        // for the first process we can actually run right now - either
        // already resident in memory, or admittable into free memory.
        //
        // NOTE: this used to try only ready_queue.front() once per tick. With
        // batch-process-freq generating a new (memory-less) process every
        // tick into the SAME queue as already-resident preempted processes,
        // that let a growing backlog of unadmittable newcomers pile up
        // AHEAD of resident processes in FIFO order, starving them - a core
        // could sit "idle" for many ticks popping doomed ADMIT-FAILs before
        // ever reaching a process it could actually run. Scanning up to the
        // queue's current size (a bounded single pass, so it can't loop
        // forever if nothing in the current queue is runnable) fixes that.
        if (core.is_idle() && !ready_queue.empty()) {
            size_t attempts = ready_queue.size();

            for (size_t attempt = 0; attempt < attempts && core.is_idle(); ++attempt) {
                Process* p = ready_queue.front();
                ready_queue.pop();

                // Already resident in memory (this is a re-admission after preemption)
                if (process_memory_ptr.find(p->id) != process_memory_ptr.end()) {
                    core.assign_process(p);
                    core_cycles[i] = 0; // reset for the new process
                    break;
                }

                // First admission for this process -> needs a fresh memory block
                void* mem = memory_allocator.allocate(p->mem_size, p->id, p->process_name);
                if (mem != nullptr) {
                    process_memory_ptr[p->id] = mem;
                    core.assign_process(p);
                    core_cycles[i] = 0;
                    break;
                } else {
                    // Memory is full and there's no backing store -> send the
                    // process back to the tail of the ready queue and keep scanning.
                    ready_queue.push(p);
                }
            }
        }
    }
}