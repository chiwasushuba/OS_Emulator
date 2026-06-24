#include "cpu.h"

CPUCore::CPUCore(int id) : core_id(id) {}

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
    current_process = nullptr;
}

bool CPUCore::tick(LogEntry& log) {
    if (current_process == nullptr) {
        return false;
    }

    return current_process->execute_next_instruction(log);
}