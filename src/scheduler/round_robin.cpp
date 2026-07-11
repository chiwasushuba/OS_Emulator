#include "scheduler.h"

RoundRobinScheduler::RoundRobinScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc, size_t mem_per_proc, uint64_t quantum)
    : Scheduler(cpu_m, proc_m, mem_alloc, mem_per_proc), quantum(quantum) {
    // Initialize the core cycles tracker to match the number of cores
    core_cycles.resize(cpu_manager.get_cores().size(), 0);
}

void RoundRobinScheduler::add_process(Process* process) {
    if (process != nullptr) {
        process->state = ProcessState::READY;
        ready_queue.push(process);
    }
}

void RoundRobinScheduler::tick() {
    auto& cores = cpu_manager.get_cores();

    for (size_t i = 0; i < cores.size(); ++i) {
        CPUCore& core = cores[i];

        // 1. Check if the core's process just finished -> release its memory
        if (!core.is_idle()) {
            Process* p = core.get_process();
            if (p->state == ProcessState::FINISHED) {
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

        // 3. If core is idle and we have waiting processes, assign one
        if (core.is_idle() && !ready_queue.empty()) {
            Process* p = ready_queue.front();
            ready_queue.pop();

            // Already resident in memory (this is a re-admission after preemption)
            if (process_memory_ptr.find(p->id) != process_memory_ptr.end()) {
                core.assign_process(p);
                core_cycles[i] = 0; // reset for the new process
                continue;
            }

            // First admission for this process -> needs a fresh memory block
            void* mem = memory_allocator.allocate(mem_per_proc, p->id, p->process_name);
            if (mem != nullptr) {
                process_memory_ptr[p->id] = mem;
                core.assign_process(p);
                core_cycles[i] = 0;
            } else {
                // Memory is full and there's no backing store -> send the
                // process back to the tail of the ready queue and try again later.
                ready_queue.push(p);
            }
        }
    }
}
