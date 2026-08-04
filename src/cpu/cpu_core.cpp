#include "cpu.h"

CPUCore::CPUCore(int id, int delay) : core_id(id), delays_per_exec(delay) {
    ticks_since_last_exec = 0;
}

int CPUCore::get_id() const {
    return core_id;
}

bool CPUCore::is_idle() const {
    return current_process == nullptr;
}

void CPUCore::assign_process(Process* process) {
    current_process = process;

    if (current_process) {
        current_process->core_id = core_id;
        current_process->state = ProcessState::RUNNING;
    }
}

Process* CPUCore::get_process() const {
    return current_process;
}

void CPUCore::remove_process() {
    if (current_process) {
        current_process->core_id = -1;
    }
    current_process = nullptr;
    ticks_since_last_exec = 0;
    last_tick_productive = false;
}

bool CPUCore::tick(LogEntry& log) {
    // technically we dont use the bool anymore but no time to refactor
    bool should_log = true;
    
    // Put here so that its counted everytime (even without a process)
    total_ticks++;
    if (current_process == nullptr) {
        ticks_since_last_exec = 0;
        last_tick_productive = false;
        return false;
    }

    ticks_since_last_exec++;

    // Execution delay implementation
    if (ticks_since_last_exec >= delays_per_exec || delays_per_exec == 0) {
        should_log = current_process->execute_next_instruction(log);
        ticks_since_last_exec = 0;

        // A tick spent servicing a page fault is NOT a tick spent executing an
        // instruction - the process made no progress, it waited on the demand
        // pager. Counting it as active reported 100% utilization for a system
        // thrashing so hard that only one process could actually run, which is
        // exactly the situation the spec's test case checks for.
        last_tick_productive = !current_process->page_fault_stalled;
    } else {
        // Busy-waiting out delays-per-exec still occupies the core: the spec
        // says the process "remains in the CPU", so this counts as productive.
        should_log = false;
        last_tick_productive = true;
    }

    if (last_tick_productive) {
        active_ticks++;
    }

    return should_log;
}

double CPUCore::get_utilization() const {
    if (total_ticks == 0) return 0.0;
    return (static_cast<double>(active_ticks) / total_ticks) * 100.0;
}