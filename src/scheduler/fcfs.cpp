#include "scheduler.h"

FCFSScheduler::FCFSScheduler(CPUManager& cpu_m, ProcessManager& proc_m, IMemoryAllocator& mem_alloc)
    : Scheduler(cpu_m, proc_m, mem_alloc) {}

void FCFSScheduler::add_process(Process* process) {
    if (process != nullptr) {
        process->state = ProcessState::READY;
        ready_queue.push(process);
    }
}

void FCFSScheduler::tick() {
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
            }
        }
        
        // 2. If the core is idle and there are processes in the queue, assign one.
        // FCFS is non-preemptive, so we never interrupt a running process.
        // Scan the queue (bounded single pass) to find the first process we
        // can actually admit into memory right now — this prevents a growing
        // backlog of unadmittable processes from starving admittable ones.
        if (core.is_idle() && !ready_queue.empty()) {
            size_t attempts = ready_queue.size();

            for (size_t attempt = 0; attempt < attempts && core.is_idle(); ++attempt) {
                Process* p = ready_queue.front();
                ready_queue.pop();

                // Already resident in memory (defensive — shouldn't happen
                // under FCFS since there's no preemption, but handle it)
                if (process_memory_ptr.find(p->id) != process_memory_ptr.end()) {
                    core.assign_process(p);
                    break;
                }

                // First admission — allocate memory for this process
                void* mem = memory_allocator.allocate(p->mem_size, p->id, p->process_name);
                if (mem != nullptr) {
                    process_memory_ptr[p->id] = mem;
                    core.assign_process(p);
                    break;
                } else {
                    // Memory full — requeue at tail and try the next one
                    ready_queue.push(p);
                }
            }
        }
    }
}