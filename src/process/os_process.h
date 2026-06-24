#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>
#include <memory>

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

// Logging the snapshot of a process
struct LogEntry {
    int pid;
    int core_id;
    int current_instruction;
    int total_instructions;
    std::string timestamp;
    std::string message;
    std::string process_name;
};

// Gets called by core component to generate reports
class ProcessLogger {
    private:
        std::unordered_map<int, std::vector<LogEntry>> process_logs;
        std::mutex logger_mutex;
    public:
        void append(int process_id, const LogEntry& entry); // append a new entry
        std::vector<LogEntry> get_logs(int process_id); // get logs for a pid
        std::vector<int> get_process_ids(); // Returns list of all unique process ids in logger
};

class Process;

// Instruction base class
class Instruction {
    public:
        virtual ~Instruction() = default;
        // Pass the process as a context, so that it can access the process data for logs
        // returns logentry since it "simulated" an operation
        virtual LogEntry execute(Process& context) = 0; 
        
        // Most basic instructions complete in 1 step, but SLEEP or FOR might take longer, thus needing parameter tracking
        virtual bool is_completed() const { return true; };
};

// Process Control Block PCB, no need for mutex since there would be only one universal scheduler
class Process {
    private:
        int current_instruction = 0;
        std::vector<std::unique_ptr<Instruction>> instruction_list;
    
        public: // public for easier manipulation by the scheduler
        int id = -1;
        int core_id = -1;
        ProcessState state = ProcessState::READY;
        std::string process_name;
        
        LogEntry execute_next_instruction();    // should call logging
        
        void add_instruction(std::unique_ptr<Instruction> new_instruction);

        size_t total_instructions() const {
            return instruction_list.size();
        }

        bool is_finished() const {
            return current_instruction >= instruction_list.size();
        }
};

// DECLARE(var, value)
class DeclareInstruction : public Instruction {
private:
    std::string var;
    uint16_t value;
public:
    DeclareInstruction(std::string var_name, uint16_t val) 
        : var(var_name), value(val) {};

    LogEntry execute(Process& context) override;
};

// ADD(var1, var2/value, var3/value)
class AddInstruction : public Instruction {
private:
    uint16_t var_1;
    uint16_t var_2;
    uint16_t var_3;
public:
    AddInstruction(uint16_t t, uint16_t s1, uint16_t s2) 
        : var_1(t), var_2(s1), var_3(s2) {};

    LogEntry execute(Process& context) override;
};

// SUBTRACT(var1, var2/value, var3/value)
class SubtractInstruction : public Instruction {
private:
    uint16_t var_1;
    uint16_t var_2;
    uint16_t var_3;
public:
    SubtractInstruction(uint16_t t, uint16_t s1, uint16_t s2) 
        : var_1(t), var_2(s1), var_3(s2) {}

    LogEntry execute(Process& context) override;
};

// PRINT(msg)
class PrintInstruction : public Instruction {
private:
    std::string msg;
public:
    PrintInstruction(std::string message) 
        : msg(message) {}

    LogEntry execute(Process& context) override;  
};

// SLEEP(X)
class SleepInstruction : public Instruction {
private:
    uint8_t remaining_ticks;
public:
    SleepInstruction(uint8_t ticks)
        : remaining_ticks(ticks) {}; 
    LogEntry execute(Process& context) override;
    bool is_completed() const override;
};

// FOR([instructions], repeats) -> Nesting supported!
class ForInstruction : public Instruction {
private:
    std::vector<std::unique_ptr<Instruction>> nestedInstructions;
    uint16_t repeatCount;
public:
    ForInstruction(std::vector<std::unique_ptr<Instruction>> insts, uint16_t repeats)
        : nestedInstructions(std::move(insts)), repeatCount(repeats) {};

    LogEntry execute(Process& context) override;
    bool is_completed() const override;
};
