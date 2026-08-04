#include <iostream>
#include "os_process.h"

bool Process::execute_next_instruction(LogEntry& log) {
    // Check if trying to execute on a finished or violated process
    if (is_finished() || access_violation) {
        if (access_violation) {
            state = ProcessState::TERMINATED;
        } else {
            state = ProcessState::FINISHED;
        }
        mark_finished_time();
        return false;
    }

    // Cleared every tick; an instruction sets it when it had to have a page
    // faulted in, and the CPU core reads it right after this returns to decide
    // whether the tick counted as executing an instruction.
    page_fault_stalled = false;

    initialize_entry(*this, log);
    Instruction* inst = instruction_list[current_instruction].get();

    // Execute current instruction
    // std::cout << "\nDEBUG: Executing instruction " << current_instruction << " for process " << process_name << "\n";
    bool should_log = inst->execute(*this, log);

    // Check if instruction caused an access violation
    if (access_violation) {
        state = ProcessState::TERMINATED;
        mark_finished_time();
        // CPUManager only forwards a log when the event type is set, so without
        // this the violation never reaches the process log and `process-smi`
        // inside the screen shows nothing about why the process died.
        log.event_type = LogEventType::LOG;
        return should_log;  // still log the violation message
    }

    if (inst->is_completed()) {
        log.event_type = LogEventType::INSTRUCTION_FINISHED;
    }

    // Page-faulting instructions must be retried on the next tick instead of
    // advancing to the following instruction.
    if (!should_log && log.message.rfind("PAGE FAULT:", 0) == 0) {
        return false;
    }

    // Move to next instruction only if current one completed and did not hit a page-fault-like retry case.
    if (inst->is_completed()) {
        current_instruction++;
    }

    // Check if process has finished after execution
    if (is_finished()) {
        state = ProcessState::FINISHED;
        mark_finished_time();
    }

    return should_log;
}

void Process::mark_finished_time() {
    if (finished_at.empty()) {
        finished_at = get_current_time();
    }
}

void Process::add_instruction(std::unique_ptr<Instruction> new_instruction) {
    instruction_list.push_back(std::move(new_instruction));
}