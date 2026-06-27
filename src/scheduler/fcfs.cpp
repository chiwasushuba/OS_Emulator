// TODO: Implement fcfs algo
#include "scheduler.h"

FCFSScheduler::FCFSScheduler(CPUManager& cpu_m, ProcessManager& proc_m)
    : Scheduler(cpu_m, proc_m) {}

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
        
        // 1. Check if the core's process just finished
        if (!core.is_idle()) {
            Process* p = core.get_process();
            if (p->state == ProcessState::FINISHED) {
                core.remove_process();
            }
        }
        
        // 2. If the core is idle and there are processes in the queue, assign one.
        // FCFS is non-preemptive, so we never interrupt a running process.
        if (core.is_idle() && !ready_queue.empty()) {
            Process* p = ready_queue.front();
            ready_queue.pop();
            core.assign_process(p);
        }
    }
}