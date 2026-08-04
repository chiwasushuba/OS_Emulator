#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
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

uint16_t Instruction::resolve_operand(const Process& ctx, const Operand& op) const {
    if (!op.isVariable)
        return op.immediate;

    uint16_t value = 0;
    if (!ctx.get_variable(op.variable, value))
        return 0; // or handle error

    return value;
}

bool AddInstruction::execute(Process& context, LogEntry& log) {
    // Operation
    uint16_t left = resolve_operand(context, lhs);
    uint16_t right = resolve_operand(context, rhs);
    uint32_t result = left + right;

    if (result > UINT16_MAX)
        result = UINT16_MAX;

    context.set_variable(destination,
                     static_cast<uint16_t>(result));
    
    // Logging
    std::stringstream ss;
    ss << std::right << std::setw(10) << "ADD: ";
    ss << result << " = " << left << " + " << right;
    log.message = ss.str();
    return true;
}

bool SubtractInstruction::execute(Process& context, LogEntry& log) {
    // Operation
    uint16_t left = resolve_operand(context, lhs);
    uint16_t right = resolve_operand(context, rhs);
    uint32_t result = 0;

    if (left >= right) {
        result = static_cast<uint32_t>(left - right);
    }

    context.set_variable(destination,
                     static_cast<uint16_t>(result));

    // Logging
    std::stringstream ss;
    ss << std::right << std::setw(10) << "SUBTRACT: ";
    ss << result << " = " << left << " - " << right;
    log.message = ss.str();
    return true;
}

bool PrintInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    ss << std::right << std::setw(10) << "PRINT: ";
    ss << this->msg;
    uint16_t value;
    if (context.get_variable(x, value)) {
        ss << " " << value;
    }
    log.message = ss.str();
    return true;
}

bool DeclareInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    context.declare_variable(var, value);
    ss << std::right << std::setw(10) << "DECLARE: ";
    ss << "Declared var " << this->var << " with value " << this->value;
    log.message = ss.str();
    return true;
}

bool PrintExpressionInstruction::execute(Process& context, LogEntry& log) {
    std::stringstream ss;
    ss << std::right << std::setw(10) << "PRINT: ";
    ss << prefix;
    uint16_t value = 0;
    if (context.get_variable(variable_name, value)) {
        ss << value;
    }
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

bool ReadInstruction::execute(Process& context, LogEntry& log) {
    initialize_entry(context, log);

    // Bounds check FIRST. Address translation precedes demand paging: an address
    // outside the process's own space has no page-table entry at all, so asking the
    // pager for it returns false forever and the instruction retries indefinitely
    // instead of faulting the process. Needs 2 bytes, so address and address+1.
    if (address % sizeof(uint16_t) != 0 || !context.is_address_valid(address) || !context.is_address_valid(address + sizeof(uint16_t) - 1)) {
        context.terminate_with_violation("0x" + [&]() {
            std::ostringstream oss;
            oss << std::hex << std::uppercase << address;
            return oss.str();
        }(), get_current_time());
        log.message = "ACCESS VIOLATION: READ at 0x" + [&]() {
            std::ostringstream oss;
            oss << std::hex << std::uppercase << address;
            return oss.str();
        }();
        return true;
    }

    // Address is genuinely ours, so demand-page it in.
    size_t page_size = context.get_page_size();
    if (page_size > 0) {
        size_t page = (address / page_size);
        if (!context.is_page_resident(page)) {
            if (!context.ensure_page_resident(page, false)) {
                log.message = "PAGE FAULT: READ at page " + std::to_string(page);
                return false;
            }
        }
    }

    // Read from memory (address is byte offset, each slot is 2 bytes)
    uint16_t val = context.memory_space[address / sizeof(uint16_t)];
    context.set_variable(var, val);

    std::ostringstream addr_ss;
    addr_ss << "0x" << std::hex << std::uppercase << address;
    std::stringstream ss;
    ss << std::right << std::setw(10) << "READ: ";
    ss << var << " = " << val << " from " << addr_ss.str();
    log.message = ss.str();
    return true;
}

bool WriteInstruction::execute(Process& context, LogEntry& log) {
    initialize_entry(context, log);

    // Bounds check FIRST - see the note in ReadInstruction::execute.
    if (address % sizeof(uint16_t) != 0 || !context.is_address_valid(address) || !context.is_address_valid(address + sizeof(uint16_t) - 1)) {
        context.terminate_with_violation("0x" + [&]() {
            std::ostringstream oss;
            oss << std::hex << std::uppercase << address;
            return oss.str();
        }(), get_current_time());
        log.message = "ACCESS VIOLATION: WRITE at 0x" + [&]() {
            std::ostringstream oss;
            oss << std::hex << std::uppercase << address;
            return oss.str();
        }();
        return true;
    }

    // Address is genuinely ours, so demand-page it in.
    size_t page_size = context.get_page_size();
    if (page_size > 0) {
        size_t page = (address / page_size);
        if (!context.is_page_resident(page)) {
            if (!context.ensure_page_resident(page, true)) {
                log.message = "PAGE FAULT: WRITE at page " + std::to_string(page);
                return false;
            }
        }
    }

    // Resolve and clamp value to uint16 range (resolve_operand already returns uint16_t)
    uint16_t resolved = resolve_operand(context, value);

    context.memory_space[address / sizeof(uint16_t)] = resolved;

    std::ostringstream addr_ss;
    addr_ss << "0x" << std::hex << std::uppercase << address;
    std::stringstream ss;
    ss << std::right << std::setw(10) << "WRITE: ";
    ss << resolved << " to " << addr_ss.str();
    log.message = ss.str();
    return true;
}
