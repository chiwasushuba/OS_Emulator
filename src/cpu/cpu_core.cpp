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
    current_process->core_id = -1;
    current_process = nullptr;
}

bool CPUCore::tick(LogEntry& log) {
    // technically we dont use the bool anymore but no time to refactor
    bool should_log = true;
    if (current_process == nullptr) {
        ticks_since_last_exec = 0;
        return false;
    }

    ticks_since_last_exec++;

    if (ticks_since_last_exec >= delays_per_exec || delays_per_exec == 0) {
        should_log = current_process->execute_next_instruction(log);
        ticks_since_last_exec = 0;
    } else {
        should_log = false; 
    }

    return should_log;
}