#include "scheduler.h"

RoundRobinScheduler::RoundRobinScheduler(CPUManager& cpu_m, ProcessManager& proc_m, uint64_t quantum)
    : Scheduler(cpu_m, proc_m), quantum(quantum) {
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
        
        // 1. Check if the core's process just finished
        if (!core.is_idle()) {
            Process* p = core.get_process();
            if (p->state == ProcessState::FINISHED) {
                core.remove_process();
                core_cycles[i] = 0;
            }
        }
        
        // 2. Increment cycle count for running process, check if quantum expired
        if (!core.is_idle()) {
            core_cycles[i]++;
            
            if (core_cycles[i] >= quantum) {
                // Quantum expired, preempt the process
                Process* p = core.get_process();
                core.remove_process();
                p->state = ProcessState::READY;
                ready_queue.push(p);
                core_cycles[i] = 0;
            }
        }
        
        // 3. If core is idle and we have waiting processes, assign one
        if (core.is_idle() && !ready_queue.empty()) {
            Process* p = ready_queue.front();
            ready_queue.pop();
            core.assign_process(p);
            core_cycles[i] = 0; // reset for the new process
        }
    }
}