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

std::string get_current_time_hms() {
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t time_now = system_clock::to_time_t(now);

    std::tm local_tm = *std::localtime(&time_now);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%H:%M:%S");

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
    // "Variable declaration commands cannot execute if the symbol table segment
    // is not in physical memory. Thus, a page fault also occurs." The segment is
    // faulted in here; the declaration itself is deferred to the next tick.
    if (!fault_stall_pending) {
        if (context.touch_symbol_table() == Process::MEM_ACCESS_FAULTED) {
            fault_stall_pending = true;
            context.page_fault_stalled = true;
            log.message = "PAGE FAULT: DECLARE (symbol table segment)";
            return false;
        }
    } else {
        fault_stall_pending = false;
    }

    std::stringstream ss;
    ss << std::right << std::setw(10) << "DECLARE: ";
    if (context.declare_variable(var, value)) {
        ss << "Declared var " << this->var << " with value " << this->value;
    } else {
        // The 64-byte symbol table holds at most 32 uint16 variables; past that
        // the spec says succeeding declarations are ignored.
        ss << "Ignored " << this->var << " - symbol table full ("
           << Process::MAX_VARIABLES << " variables max)";
    }
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

// Renders an address the way the spec's messages do: 0x + uppercase hex.
static std::string hex_addr(uint32_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}

bool ReadInstruction::execute(Process& context, LogEntry& log) {
    initialize_entry(context, log);

    // Bounds check FIRST. Address translation precedes demand paging: an address
    // outside the process's own space has no page-table entry at all, so asking the
    // pager for it returns false forever and the instruction retries indefinitely
    // instead of faulting the process. A uint16 needs 2 bytes: address and address+1.
    if (!context.is_address_valid(address) || !context.is_address_valid(address + 1)) {
        context.terminate_with_violation(hex_addr(address), get_current_time_hms());
        log.message = "ACCESS VIOLATION: READ at " + hex_addr(address);
        log.event_type = LogEventType::LOG;
        return true;
    }

    // Address is genuinely ours, so demand-page it in and load it - both in a
    // SINGLE atomic allocator call. Splitting fault-in from the access let
    // another core steal the frame in between, and made a page-straddling uint16
    // unserviceable whenever physical memory held fewer frames than the access
    // needed, retrying forever and pinning this core.
    uint16_t val = 0;
    if (!fault_stall_pending) {
        int result = context.access_memory(address, val, false);
        if (result == Process::MEM_ACCESS_INVALID) {
            // No page-table entry despite passing the bounds check: the process
            // has no allocation at all. Treat it as the violation it is rather
            // than spinning.
            context.terminate_with_violation(hex_addr(address), get_current_time_hms());
            log.message = "ACCESS VIOLATION: READ at " + hex_addr(address);
            log.event_type = LogEventType::LOG;
            return true;
        }
        if (result == Process::MEM_ACCESS_FAULTED) {
            // The page has now been brought in and the value read, but the spec
            // wants the instruction restarted once a valid frame is found. Burn
            // this tick on fault handling and complete on the next one.
            fault_stall_pending = true;
            fault_stall_value = val;
            context.page_fault_stalled = true;
            log.message = "PAGE FAULT: READ at " + hex_addr(address);
            return false;
        }
    } else {
        val = fault_stall_value;
        fault_stall_pending = false;
    }
    context.set_variable(var, val);

    std::stringstream ss;
    ss << std::right << std::setw(10) << "READ: ";
    ss << var << " = " << val << " from " << hex_addr(address);
    log.message = ss.str();
    return true;
}

bool WriteInstruction::execute(Process& context, LogEntry& log) {
    initialize_entry(context, log);

    // Bounds check FIRST - see the note in ReadInstruction::execute.
    if (!context.is_address_valid(address) || !context.is_address_valid(address + 1)) {
        context.terminate_with_violation(hex_addr(address), get_current_time_hms());
        log.message = "ACCESS VIOLATION: WRITE at " + hex_addr(address);
        log.event_type = LogEventType::LOG;
        return true;
    }

    // Resolve and clamp value to uint16 range (resolve_operand already returns uint16_t)
    uint16_t resolved = resolve_operand(context, value);

    // Atomic fault-in + store - see the note in ReadInstruction::execute. The
    // store has already landed by the time a fault is reported, so the retry
    // tick simply completes the instruction.
    if (!fault_stall_pending) {
        int result = context.access_memory(address, resolved, true);
        if (result == Process::MEM_ACCESS_INVALID) {
            context.terminate_with_violation(hex_addr(address), get_current_time_hms());
            log.message = "ACCESS VIOLATION: WRITE at " + hex_addr(address);
            log.event_type = LogEventType::LOG;
            return true;
        }
        if (result == Process::MEM_ACCESS_FAULTED) {
            fault_stall_pending = true;
            context.page_fault_stalled = true;
            log.message = "PAGE FAULT: WRITE at " + hex_addr(address);
            return false;
        }
    } else {
        fault_stall_pending = false;
    }

    std::stringstream ss;
    ss << std::right << std::setw(10) << "WRITE: ";
    ss << resolved << " to " << hex_addr(address);
    log.message = ss.str();
    return true;
}
