#include "os_process.h"

bool Process::execute_next_instruction(LogEntry& log) {
    if (is_finished()) {
        state = ProcessState::FINISHED;
        return false;
    }

    initialize_entry(*this, log);

    Instruction* inst = instruction_list[current_instruction].get();

    // Execute current instruction
    bool should_log = inst->execute(*this, log);

    // Move to next instruction only if current one completed
    if (inst->is_completed()) {
        current_instruction++;
    }

    // Check if process has finished
    if (is_finished()) {
        state = ProcessState::FINISHED;
    }

    return should_log;
}

void Process::add_instruction(std::unique_ptr<Instruction> new_instruction) {
    instruction_list.push_back(std::move(new_instruction));
}