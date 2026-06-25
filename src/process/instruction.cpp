#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "os_process.h"

// Helper function
std::string get_current_time() {
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t time_now = system_clock::to_time_t(now);

    std::tm local_tm = *std::localtime(&time_now);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S%p");

    return oss.str();
}

// Helper function for generating the entry, be sure to call this before passing the log to execute
void initialize_entry(Process& context, LogEntry& log) {
    log.core_id = context.core_id;
    log.current_instruction = context.current_instruction;
    log.pid = context.id;
    log.process_name = context.process_name;
    log.timestamp = get_current_time();
    log.total_instructions = context.total_instructions();
    log.event_type = LogEventType::NONE;
}

bool AddInstruction::execute(Process& context, LogEntry& log) {
    this->var_1 = this->var_2 + this->var_3;
    std::stringstream ss;
    ss << std::right << std::setw(10) << "ADD: ";
    ss << this->var_1 << " = " << this->var_2 << " + " << this->var_3;
    log.message = ss.str();
    return true;
}

bool SubtractInstruction::execute(Process& context, LogEntry& log) {
    this->var_1 = this->var_2 - this->var_3;
    
    std::stringstream ss;
    ss << std::right << std::setw(10) << "SUBTRACT: ";
    ss << this->var_1 << " = " << this->var_2 << " - " << this->var_3;
    log.message = ss.str();
    return true;
}

bool PrintInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    ss << std::right << std::setw(10) << "PRINT: ";
    ss << this->msg;
    if (this->x != "") {
        ss << " " << this->x;
    }
    log.message = ss.str();
    return true;
}

bool DeclareInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    ss << std::right << std::setw(10) << "DECLARE: ";
    ss << "Declared var " << this->var << " with value " << this->value;
    log.message = ss.str();
    return true;
}

bool ForInstruction::execute(Process& context, LogEntry& log) {
    if (completed) return true;
    if (nestedInstructions.empty() || repeatCount == 0) {
        completed = true;
        return true;
    }

    std::stringstream ss;
    std::stringstream header;
    std::stringstream splitter;
    header << std::right << std::setw(10) << "FOR:\n";
    splitter << std::right << std::setw(35) << "";

    // Get the current sub-instruction to run on this CPU tick
    auto& currentInst = nestedInstructions[currentInstructionIndex];    
    bool instLogged = currentInst->execute(context, log);
    

    // If the nested instruction is finished, advance our pointers
    if (currentInst->is_completed()) {
        currentInstructionIndex++;
        log.event_type = LogEventType::INSTRUCTION_FINISHED;

        // If we finished all instructions in the block, complete one loop iteration
        if (currentInstructionIndex >= nestedInstructions.size()) {
            currentInstructionIndex = 0; // Reset to start of block
            currentIteration++;
            
            if (!log.message.empty()) {
                log.message += "\n";
            }

            ss << std::right << std::setw(10) << "IT: "
            << currentIteration
            << "/"
            << repeatCount;

            log.message = header.str() + splitter.str() + log.message + splitter.str() + ss.str();

            
            // reset nested FOR and SLEEP (stateful instructions)
            for (auto& inst : nestedInstructions) {
                inst->reset();
            }
        }
    }

    // Check if the entire loop structure is done
    if (currentIteration >= repeatCount) {
        completed = true;
    }

    return instLogged;
}

bool ForInstruction::is_completed() const {
    return this->completed;
}

void ForInstruction::reset()
{
    currentIteration = 0;
    currentInstructionIndex = 0;
    completed = false;

    for (auto& inst : nestedInstructions)
    {
        inst->reset();
    }
}

bool SleepInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    ss << std::right << std::setw(10) << "SLEEP: ";

    if (state == SleepState::AWAKE) {
        state = SleepState::SLEEPING;
        log.event_type = LogEventType::LOG;
        ss << "Sleep started for " << (int)original_ticks << " ticks";
        log.message = ss.str();
        return true;
    }
    else if (state == SleepState::SLEEPING) {
        if (remaining_ticks > 0) {
            remaining_ticks--;
            return false;
        }

        if (remaining_ticks == 0) {
            state = SleepState::AWAKE;
            ss << "Sleep ended";
            log.message = ss.str();
            return true;
        }
    }
    return false;
}

bool SleepInstruction::is_completed() const {
    return this->remaining_ticks == 0 && state == SleepState::AWAKE;
}

void SleepInstruction::reset() {
    remaining_ticks = original_ticks;
    state = SleepState::AWAKE;
}

