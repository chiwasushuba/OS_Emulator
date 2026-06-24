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

enum class SleepState {
    SLEEPING,
    AWAKE
};

enum class LogEventType {
    NONE,
    INSTRUCTION_FINISHED
};

// Logging the snapshot of a process
struct LogEntry {
    int pid = -1;
    int core_id = -1;
    int current_instruction = 0;
    int total_instructions = 0;
    std::string timestamp = "";
    std::string message = "";
    std::string process_name = "";
    LogEventType event_type = LogEventType::NONE;
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
        virtual bool execute(Process& context, LogEntry& log) = 0; 
        
        // Most basic instructions complete in 1 step, but SLEEP or FOR might take longer, thus needing parameter tracking
        virtual bool is_completed() const { return true; };
};

// Process Control Block PCB, no need for mutex since there would be only one universal scheduler
class Process {
    private:
        std::vector<std::unique_ptr<Instruction>> instruction_list;
    
    public: // public for easier manipulation by the scheduler
        int current_instruction = 0;
        int id = -1;
        int core_id = -1;
        ProcessState state = ProcessState::READY;
        std::string process_name;
        
        bool execute_next_instruction(LogEntry& log);    // should, LogEntry& log call logging
        
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

    bool execute(Process& context, LogEntry& log) override;
};

// ADD(var1, var2/value, var3/value)
class AddInstruction : public Instruction {
private:
    uint16_t var_1 = 0;
    uint16_t var_2 = 0;
    uint16_t var_3 = 0;
public:
    AddInstruction(uint16_t t, uint16_t s1, uint16_t s2) 
        : var_1(t), var_2(s1), var_3(s2) {};

    bool execute(Process& context, LogEntry& log) override;
};

// SUBTRACT(var1, var2/value, var3/value)
class SubtractInstruction : public Instruction {
private:
    uint16_t var_1 = 0;
    uint16_t var_2 = 0;
    uint16_t var_3 = 0;
public:
    SubtractInstruction(uint16_t t, uint16_t s1, uint16_t s2) 
        : var_1(t), var_2(s1), var_3(s2) {}

    bool execute(Process& context, LogEntry& log) override;
};

// PRINT(msg)
class PrintInstruction : public Instruction {
private:
    std::string msg;
public:
    PrintInstruction(std::string message) 
        : msg(message) {}

    bool execute(Process& context, LogEntry& log) override;  
};

// SLEEP(X)
class SleepInstruction : public Instruction {
private:
    uint8_t remaining_ticks;
    SleepState state;
public:
    SleepInstruction(uint8_t ticks); 
    // Returns true if should log, false if not (might refactor if execution actually does something in the future)
    bool execute(Process& context, LogEntry& log) override;
    bool is_completed() const override;
};

// FOR([instructions], repeats) -> Nesting supported!
class ForInstruction : public Instruction {
private:
    std::vector<std::unique_ptr<Instruction>> nestedInstructions;
    uint16_t repeatCount;
    
    // State tracking variables for multi-tick step execution
    uint16_t currentIteration = 0;
    size_t currentInstructionIndex = 0;
    bool completed = false;

public:
    ForInstruction(std::vector<std::unique_ptr<Instruction>> insts, uint16_t repeats)
        : nestedInstructions(std::move(insts)), repeatCount(repeats), 
          currentIteration(0), currentInstructionIndex(0), completed(false) {};

    bool execute(Process& context, LogEntry& log) override;
    bool is_completed() const override;
};

// Helper funtions
void initialize_entry(Process& context, LogEntry& log);
std::string get_current_time();
