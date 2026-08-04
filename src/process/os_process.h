#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <mutex>
#include <algorithm>

class Process;

struct Operand
{
	bool isVariable;
	std::string variable;
	uint16_t immediate;

	static Operand Imm(uint16_t value)
	{
		return Operand{false, "", value};
	}
};

enum class ProcessState
{
	READY,
	RUNNING,
	WAITING,
	FINISHED,
	TERMINATED
};

enum class SleepState
{
	SLEEPING,
	AWAKE
};

struct ProcessSnapshot
{
	int id;
	std::string name;
	int core_id;
	int current_instruction;
	size_t total_instructions;
	size_t mem_size;
	std::string created_at;
};

enum class LogEventType
{
	NONE,
	INSTRUCTION_FINISHED,
	PROCESS_STARTED,
	LOG
};

// Logging the snapshot of a process
struct LogEntry
{
	int pid = -1;
	int core_id = -1;
	int current_instruction = 0;
	int total_instructions = 0;
	std::string timestamp = "";
	std::string message = "";
	std::string process_name = "";
	LogEventType event_type = LogEventType::NONE;
};

// Creates and owns all processes
class ProcessManager
{
private:
	std::unordered_map<int, std::unique_ptr<Process>> processes;
	int next_pid = 1;

	mutable std::mutex pm_mutex;

public:
	// Creates a new process object, adds it to the process map, and returns the pid
	int create_process(const std::string &name);
	// Get process pointer (nullptr if not found)
	Process *get_process(int pid);
	Process *get_process(const std::string &process_name);
	// Get all active PIDs for screen -ls
	std::vector<int> get_active_pids() const;
	std::vector<int> get_finished_pids() const;
	std::vector<int> get_all_pids() const;
	std::vector<ProcessSnapshot> get_active_processes() const;
};

// Gets called by core component to generate reports
class ProcessLogger
{
private:
	std::unordered_map<int, std::vector<LogEntry>> process_logs;
	std::mutex logger_mutex;

public:
	void append(int process_id, const LogEntry &entry); // append a new entry
	std::vector<LogEntry> get_logs(int process_id);		// get logs for a pid
	std::vector<int> get_process_ids();					// Returns list of all unique process ids in logger
};

// Instruction base class
class Instruction
{
public:
	virtual ~Instruction() = default;
	// Pass the process as a context, so that it can access the process data for logs
	// returns logentry since it "simulated" an operation
	virtual bool execute(Process &context, LogEntry &log) = 0;

	// Most basic instructions complete in 1 step, but SLEEP or FOR might take longer, thus needing parameter tracking
	virtual bool is_completed() const { return true; };

	// Overridden by instructions with state
	virtual void reset() {}

protected:
	uint16_t resolve_operand(const Process &ctx, const Operand &op) const;
};

// Process Control Block PCB, no need for mutex since there would be only one universal scheduler
class Process
{
private:
	std::vector<std::unique_ptr<Instruction>> instruction_list;
	std::unordered_map<std::string, uint16_t> symbol_table;

public: // public for easier manipulation by the scheduler
	int current_instruction = 0;
	int id = -1;
	int core_id = -1;
	ProcessState state = ProcessState::READY;
	std::string process_name;

	size_t mem_size = 0;
	size_t page_size = 0;
	// NOTE: the process deliberately keeps NO residency cache of its own. The
	// memory manager owns the frame table and is the only thing that knows when
	// a page has been evicted; a local copy would go stale the moment another
	// process stole the frame, and reads would silently miss the page-in.
	std::function<bool(size_t, bool)> page_fault_handler;

	// A process owns an ADDRESS SPACE, not bytes. Its data physically lives in
	// the memory manager's frames (or in the backing store while paged out), and
	// is reached through these handlers, which translate a virtual address via
	// this process's page table. mem_size below is the size of that address
	// space - the bound that decides what is an access violation.
	std::function<bool(size_t, uint16_t &)> read_memory_handler;
	std::function<bool(size_t, uint16_t)> write_memory_handler;
	// Atomic fault-in + access, in ONE allocator call. Returns
	// IMemoryAllocator::AccessResult. This is what READ/WRITE use; the two
	// handlers above remain for callers that only need a plain translated access.
	std::function<int(size_t, uint16_t &, bool)> access_memory_handler;

	// Access violation state
	bool access_violation = false;
	std::string violation_address = "";
	std::string violation_timestamp = "";

	// Timestamps shown by screen -ls / report-util. The listing used to print
	// "now" for every row, so every process appeared to have been created at the
	// instant the command was typed.
	std::string created_at = "";
	std::string finished_at = "";

	// Set for exactly one tick when a memory instruction had to have a page
	// faulted in. The CPU core reads it to decide whether the tick counted as
	// executing an instruction (it did not - it was spent handling the fault),
	// which is what keeps utilization honest under memory pressure.
	bool page_fault_stalled = false;

	// Symbol table limit: the segment is a fixed 64 bytes and a uint16 costs
	// 2 bytes, so 64 / 2 = 32 variables.
	static constexpr size_t SYMBOL_TABLE_BYTES = 64;
	static constexpr size_t MAX_VARIABLES = SYMBOL_TABLE_BYTES / sizeof(uint16_t);

	// Mirrors IMemoryAllocator::AccessResult. Duplicated rather than included
	// because the memory subsystem already depends on this header, and including
	// it back would make the dependency circular.
	static constexpr int MEM_ACCESS_INVALID = -1;
	static constexpr int MEM_ACCESS_OK = 0;
	static constexpr int MEM_ACCESS_FAULTED = 1;

	void set_memory_handlers(std::function<bool(size_t, uint16_t &)> reader,
							 std::function<bool(size_t, uint16_t)> writer)
	{
		read_memory_handler = std::move(reader);
		write_memory_handler = std::move(writer);
	}

	// Read/write a uint16 at a virtual byte address. Both return false if the
	// page is not resident, which the caller turns into a page fault + retry.
	bool read_memory(size_t address, uint16_t &out) const
	{
		return read_memory_handler ? read_memory_handler(address, out) : false;
	}

	bool write_memory(size_t address, uint16_t value)
	{
		return write_memory_handler ? write_memory_handler(address, value) : false;
	}

	void set_access_memory_handler(std::function<int(size_t, uint16_t &, bool)> handler)
	{
		access_memory_handler = std::move(handler);
	}

	// Faults in and accesses a uint16 in one atomic allocator call. Returns
	// IMemoryAllocator::AccessResult (-1 invalid / 0 served / 1 served-after-fault).
	int access_memory(size_t address, uint16_t &value, bool is_write)
	{
		if (!access_memory_handler)
			return -1;
		return access_memory_handler(address, value, is_write);
	}

	bool execute_next_instruction(LogEntry &log); // should, LogEntry& log call logging
	// Stamps finished_at the first time the process reaches FINISHED/TERMINATED.
	void mark_finished_time();
	void set_page_size(size_t page_size_bytes) { page_size = page_size_bytes; }
	size_t get_page_size() const { return page_size; }
	void set_page_fault_handler(std::function<bool(size_t, bool)> handler)
	{
		page_fault_handler = std::move(handler);
	}
	// Asks the memory manager for the page. It returns immediately if the page
	// already holds a frame; otherwise this IS the page fault - it picks a
	// victim, evicts it to the backing store, and loads this page in.
	bool ensure_page_resident(size_t page_number, bool for_write = false)
	{
		return page_fault_handler ? page_fault_handler(page_number, for_write) : false;
	}

	// Faults in every page spanned by [address, address + length). An unaligned
	// uint16 can straddle a page boundary, so one call may touch two pages.
	// Returns false if any of them could not be brought in - the caller must
	// then retry the whole instruction on a later tick, per the spec's
	// "page fault handling continuously occurs until a valid page has been
	// returned, before an instruction is performed."
	bool ensure_bytes_resident(size_t address, size_t length, bool for_write)
	{
		if (page_size == 0 || !page_fault_handler)
			return true; // paging not wired up (e.g. a unit-test process)

		size_t first_page = address / page_size;
		size_t last_page = (address + length - 1) / page_size;
		for (size_t page = first_page; page <= last_page; ++page)
		{
			if (!ensure_page_resident(page, for_write))
				return false;
		}
		return true;
	}

	// The symbol table segment lives at the base of the process's address space.
	// Per the spec, a variable declaration cannot execute while that segment is
	// swapped out - it page-faults just like a READ/WRITE does.
	bool ensure_symbol_table_resident()
	{
		return ensure_bytes_resident(0, SYMBOL_TABLE_BYTES, true);
	}

	// Same idea, but reports WHETHER a fault had to be serviced, which
	// ensure_symbol_table_resident() cannot - it returns true both when the
	// segment was already resident and when it had just been paged in, so
	// DECLARE could never actually stall on it. Returns MEM_ACCESS_*.
	// The segment is 64 bytes and mem-per-frame is at least 64, so it is always
	// exactly page 0 and touching address 0 covers all of it.
	int touch_symbol_table()
	{
		if (!access_memory_handler)
			return MEM_ACCESS_OK; // paging not wired up (e.g. a unit-test process)
		uint16_t probe = 0;
		int result = access_memory_handler(0, probe, false);
		// No page table at all means the process has not been admitted to memory
		// yet; let the declaration through rather than stalling forever.
		return (result == MEM_ACCESS_INVALID) ? MEM_ACCESS_OK : result;
	}

	bool symbol_table_full() const { return symbol_table.size() >= MAX_VARIABLES; }

	void add_instruction(std::unique_ptr<Instruction> new_instruction);

	bool is_address_valid(uint32_t address) const
	{
		return address < mem_size;
	}

	bool declare_variable(const std::string &name, uint16_t value)
	{
		// Re-declaring an existing variable reuses its slot, so it stays legal
		// even once the 32-variable symbol table is full.
		auto it = symbol_table.find(name);
		if (it != symbol_table.end())
		{
			it->second = value;
			return true;
		}
		if (symbol_table.size() >= MAX_VARIABLES)
		{
			return false;
		}
		symbol_table[name] = value;
		return true;
	}

	bool set_variable(const std::string &name, uint16_t value)
	{
		auto it = symbol_table.find(name);
		if (it != symbol_table.end())
		{
			it->second = value;
			return true;
		}
		if (symbol_table.size() >= MAX_VARIABLES)
			return false;
		symbol_table[name] = value;
		return true;
	}

	bool get_variable(const std::string &name, uint16_t &value) const
	{
		auto it = symbol_table.find(name);
		if (it == symbol_table.end())
			return false;

		value = it->second;
		return true;
	}

	const std::unordered_map<std::string, uint16_t> &get_all_variables() const
	{
		return symbol_table;
	}

	size_t total_instructions() const
	{
		return instruction_list.size();
	}

	bool is_finished() const
	{
		return current_instruction >= (instruction_list.size()) || state == ProcessState::TERMINATED;
	}

	void terminate_with_violation(const std::string &address, const std::string &timestamp)
	{
		access_violation = true;
		violation_address = address;
		violation_timestamp = timestamp;
		state = ProcessState::TERMINATED;
	}
};

// DECLARE(var, value)
//
// Per the spec, a declaration cannot execute while the symbol table segment is
// swapped out - it page-faults exactly like READ/WRITE does, and stalls a tick.
class DeclareInstruction : public Instruction
{
private:
	std::string var;
	uint16_t value;
	bool fault_stall_pending = false;

public:
	DeclareInstruction(std::string var_name, uint16_t val)
		: var(var_name), value(val) {};

	bool execute(Process &context, LogEntry &log) override;
	bool is_completed() const override { return !fault_stall_pending; }
	void reset() override { fault_stall_pending = false; }
};

// ADD(var1, var2/value, var3/value)
class AddInstruction : public Instruction
{
private:
	std::string destination = 0;
	Operand lhs = Operand::Imm(0);
	Operand rhs = Operand::Imm(0);

public:
	AddInstruction(std::string var_1, Operand var_2, Operand var_3)
		: destination(var_1), lhs(var_2), rhs(var_3) {};

	bool execute(Process &context, LogEntry &log) override;
};

// SUBTRACT(var1, var2/value, var3/value)
class SubtractInstruction : public Instruction
{
private:
	std::string destination = 0;
	Operand lhs = Operand::Imm(0);
	Operand rhs = Operand::Imm(0);

public:
	SubtractInstruction(std::string var_1, Operand var_2, Operand var_3)
		: destination(var_1), lhs(var_2), rhs(var_3) {}

	bool execute(Process &context, LogEntry &log) override;
};

// PRINT(msg)
class PrintInstruction : public Instruction
{
private:
	std::string msg;
	std::string x;

public:
	PrintInstruction(std::string message, std::string var = "")
		: msg(message), x(var) {}

	bool execute(Process &context, LogEntry &log) override;
};

class PrintExpressionInstruction : public Instruction
{
private:
	std::string prefix;
	std::string variable_name;

public:
	PrintExpressionInstruction(std::string message, std::string variable)
		: prefix(std::move(message)), variable_name(std::move(variable)) {}

	bool execute(Process &context, LogEntry &log) override;
};

// SLEEP(X)
class SleepInstruction : public Instruction
{
private:
	uint8_t remaining_ticks;
	uint8_t original_ticks;
	SleepState state;

public:
	SleepInstruction(uint8_t ticks)
		: remaining_ticks(ticks), original_ticks(ticks), state(SleepState::AWAKE)
	{
	}
	// Returns true if should log, false if not (might refactor if execution actually does something in the future)
	bool execute(Process &context, LogEntry &log) override;
	bool is_completed() const override;
	void reset() override;
};

// FOR([instructions], repeats) -> Nesting supported!
class ForInstruction : public Instruction
{
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

	bool execute(Process &context, LogEntry &log) override;
	bool is_completed() const override;
	void reset() override;
};

// READ(var, hex_address) — loads uint16 from process memory into variable
//
// access_memory() completes the access even when it had to fault a page in, so
// the loaded value is cached in fault_stall_value; the instruction then burns
// one tick without completing (the spec's visible "restart the instruction once
// a valid frame is found") and finishes from the cache on the next tick.
// Bounded at one retry per fault, so unlike the old fault-then-access-separately
// scheme it can never livelock. The state lives here rather than on the Process
// because a FOR loop advances on is_completed(), and it must not step past a
// nested instruction that is still waiting on its page.
class ReadInstruction : public Instruction
{
private:
	std::string var;
	uint32_t address;
	bool fault_stall_pending = false;
	uint16_t fault_stall_value = 0;

public:
	ReadInstruction(std::string var_name, uint32_t addr)
		: var(std::move(var_name)), address(addr) {}
	bool execute(Process &context, LogEntry &log) override;
	bool is_completed() const override { return !fault_stall_pending; }
	void reset() override
	{
		fault_stall_pending = false;
		fault_stall_value = 0;
	}
};

// WRITE(hex_address, value_or_var) — stores uint16 into process memory
class WriteInstruction : public Instruction
{
private:
	uint32_t address;
	Operand value;
	bool fault_stall_pending = false;

public:
	WriteInstruction(uint32_t addr, Operand val)
		: address(addr), value(val) {}
	bool execute(Process &context, LogEntry &log) override;
	bool is_completed() const override { return !fault_stall_pending; }
	void reset() override { fault_stall_pending = false; }
};

// Helper funtions
void initialize_entry(Process &context, LogEntry &log);
std::string get_current_time();
// HH:MM:SS only - the format the spec's access-violation message asks for.
std::string get_current_time_hms();
