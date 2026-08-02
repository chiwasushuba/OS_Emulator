# MCO2 — Remaining Features, Fixes & Bugs

> **Generated**: 2026-08-02 · **Purpose**: Exhaustive checklist for another model to implement.
> Each item includes: what's missing, where to change, and how to solve it.

---

## Status Legend

| Icon | Meaning |
|------|---------|
| 🔴 | **Not implemented** — feature doesn't exist yet |
| 🟡 | **Stubbed / partial** — skeleton exists but logic is empty |
| 🟢 | **Working** — implemented and functional |
| 🐛 | **Bug / risk** — existing code has a correctness issue |

---

## 1. Configuration — New MO2 Parameters 🔴

**Files**: [config.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/config/config.h), [config.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/config/config.cpp)

**Problem**: The config parser only reads MO1 parameters (`num-cpu`, `scheduler`, `quantum-cycles`, `batch-process-freq`, `min-ins`, `max-ins`, `delays-per-exec`). The 4 new MO2 parameters are not parsed or stored.

**What to add**:
| Parameter | Type | Constraints |
|-----------|------|-------------|
| `max-overall-mem` | `uint32_t` | Power of 2, range `[2^6, 2^16]` i.e. `[64, 65536]` |
| `mem-per-frame` | `uint32_t` | Power of 2, must evenly divide `max-overall-mem` |
| `min-mem-per-proc` | `uint32_t` | Power of 2, range `[64, 65536]` |
| `max-mem-per-proc` | `uint32_t` | Power of 2, `>= min-mem-per-proc`, `<= max-overall-mem` |

**How to solve**:
1. Add 4 new member variables to the `Config` class.
2. Add parsing branches in the config file reader loop for these keys.
3. Add validation:
   - All must be powers of 2 (`(val & (val - 1)) == 0 && val != 0`).
   - Range check `[64, 65536]`.
   - `min-mem-per-proc <= max-mem-per-proc`.
   - `max-overall-mem % mem-per-frame == 0`.
   - `max-mem-per-proc <= max-overall-mem`.
4. Add getter methods for each parameter.

---

## 2. Memory Manager — Core Logic 🟡

**Files**: [memory_manager.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/memory_manager.h), [memory_manager.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/memory_manager.cpp)

**Problem**: Class exists but all methods are stubs that return `false`/`0`/do nothing.

**What to implement**:

### 2a. `initialize(total_memory, frame_size)`
- Calculate `num_frames = total_memory / frame_size`.
- Initialize the `FrameTable` with `num_frames`.
- Initialize the `BackingStore`.
- Store `frame_size` for later page-size calculations.

### 2b. `allocate_frames(process_id, num_frames)`
- For demand paging, this should **NOT** allocate all frames upfront.
- Instead, register the process as having `num_frames` pages in its virtual address space.
- Create a `PageTable` for the process with `num_pages = num_frames`.
- Store a map: `process_id → PageTable`.

### 2c. `handle_page_fault(process_id, page_number)` ← **Critical path**
- Check if a free frame exists via `FrameTable::find_free_frame()`.
- If yes: allocate frame, update process's `PageTable`, load page data from backing store (or zero-initialize if new).
- If no free frame: **select a victim** (implement FIFO or LRU):
  - Pick the oldest/least-recently-used frame.
  - If the frame's page is dirty, write it to the backing store first.
  - Free the victim frame, invalidate the victim's page table entry.
  - Then allocate the freed frame to the requesting process.
- Return `true` on success.

### 2d. `deallocate_frames(process_id)`
- Free all frames owned by the process in the frame table.
- Remove the process's page table.
- Clean up backing store entries for the process.

### 2e. `get_memory_usage()` / stats methods
- Return used frame count × frame size.
- Add methods for: total memory, free memory, active/inactive pages, pages paged in/out counters.

### 2f. Add to `MemoryManager`:
- `std::unordered_map<int, PageTable> process_page_tables_`
- `size_t pages_paged_in_` counter (increment on every page load).
- `size_t pages_paged_out_` counter (increment on every eviction write-back).
- A page replacement tracker (e.g., `std::queue<FrameRef>` for FIFO, or `std::list` for LRU).
- `bool is_page_resident(process_id, page_number)` — check if a specific page is in memory.
- `int translate_address(process_id, virtual_address)` — virtual-to-physical translation.

---

## 3. Backing Store — File I/O 🟡

**Files**: [backing_store.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/backing_store.h), [backing_store.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/backing_store.cpp)

**Problem**: Class exists but all methods are empty stubs. No actual file I/O.

**How to solve**:

### 3a. `initialize(filename)`
- Open/create `csopesy-backing-store.txt` (truncate if exists from a previous run).
- Store file handle or path for later use.

### 3b. `write_page(process_id, page_number, data)`
- Serialize page data to the backing store file.
- Use a consistent format, e.g., one line per page: `PROC_<id>_PAGE_<num>: <hex data bytes>`.
- If an entry already exists for this `(process_id, page_number)`, overwrite it.
- **Thread safety**: Use a `std::mutex` to protect file writes — multiple cores may evict simultaneously.

### 3c. `read_page(process_id, page_number)`
- Search the backing store file for the matching entry.
- Parse and return the data as `std::vector<uint8_t>`.
- If not found (page never evicted), return a **zero-filled vector** of `frame_size` bytes.

### 3d. `remove_process(process_id)`
- Remove all entries for the given process from the backing store.
- Called when a process terminates (normally or via access violation).

> [!TIP]
> Consider using an in-memory `std::unordered_map<key, vector<uint8_t>>` as the primary store, and periodically flushing to the text file. This avoids expensive file I/O on every page fault while keeping the file inspectable.

---

## 4. New Instructions 🔴

**Files**: [instruction.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/process/instruction.h), [instruction.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/process/instruction.cpp)

**Problem**: Only `PrintInstruction` exists. All MO2 instruction types are missing.

### 4a. `DeclareInstruction` — `DECLARE varName value`
- Add variable to the process's symbol table (up to 32 vars / 64 bytes).
- If table is full (32 vars), **silently ignore** (no error, no crash).
- Store as `uint16_t`, clamp value to `[0, 65535]`.
- **Requires**: Symbol table on the process, page fault check on the symbol table page.

### 4b. `ReadInstruction` — `READ(var, memory_address)`
- Read a `uint16_t` from the given hex address in the process's memory space.
- Store the value into `var` in the symbol table.
- If the address was never written to, return `0`.
- If the address is **outside the process's allocated memory** → **access violation** → terminate process immediately.
- **Requires**: Address bounds checking, page fault handling for the target page.

### 4c. `WriteInstruction` — `WRITE(memory_address, value)`
- Write a `uint16_t` value to the given hex address.
- Clamp value to `[0, 65535]`.
- If address is outside bounds → **access violation** → terminate.
- Mark the page as dirty in the page table.
- **Requires**: Address bounds checking, page fault handling, dirty bit.

### 4d. `AddInstruction` — `ADD dest src1 src2`
- `dest = src1 + src2`, all are symbol table variables.
- Clamp result to `[0, 65535]` (no overflow/wrap).

### 4e. `SubtractInstruction` — `SUBTRACT dest src1 src2` _(if spec requires)_
- `dest = src1 - src2`.
- Clamp result to `[0, 65535]` (underflow clamps to 0).

### 4f. `PrintExpressionInstruction` — `PRINT("string" + var)`
- Enhanced print that concatenates string literals with variable values.
- Different from existing `PrintInstruction` which just logs.

### Implementation notes:
- All instructions need a reference/pointer to the process's symbol table and memory space.
- All memory-accessing instructions (`READ`, `WRITE`, `DECLARE`) must check page residency and trigger page faults if needed.
- If a page fault occurs, the instruction must be **restarted** (not skipped) after the fault is resolved.

---

## 5. Process — Symbol Table & Memory Space 🔴

**Files**: [process.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/process/process.h), [process.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/process/process.cpp)

**Problem**: Process has no symbol table, no memory allocation, no page table, and no access violation state.

**What to add to `Process` class**:

```cpp
// Symbol table: max 32 variables, 64 bytes total (each uint16_t = 2 bytes)
std::unordered_map<std::string, uint16_t> symbol_table_;
static constexpr int MAX_VARIABLES = 32;

// Memory
uint32_t memory_size_;           // Allocated memory size in bytes
PageTable* page_table_;          // Pointer to this process's page table
std::vector<uint8_t> memory_;    // Virtual memory space (simulated)

// Access violation tracking
bool access_violation_ = false;
std::string violation_address_;
std::string violation_timestamp_;  // "HH:MM:SS" format
```

**How to solve**:
1. Add `memory_size` parameter to `Process` constructor.
2. Add symbol table with `declare_variable(name, value)`, `get_variable(name)`, `set_variable(name, value)` methods.
3. Add `is_address_valid(hex_address)` — checks if address is within `[0, memory_size)`.
4. Add `terminate_with_violation(address, timestamp)` — sets state to a new `TERMINATED` state, stores violation info.
5. Add a new `ProcessState::TERMINATED` (or reuse FINISHED with a violation flag) in [constants.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/constants.h).

---

## 6. `screen -s <name> <mem_size>` — Memory Size Support 🔴

**Files**: [cli_helpers.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cli/cli_helpers.h), [cli_helpers.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cli/cli_helpers.cpp), [os.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/core/os.cpp), [kernel.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/kernel/kernel.cpp)

**Problem**: `parse_screen_command()` parses `screen -s <name>` but does NOT parse a `<mem_size>` argument. `create_process()` in the kernel takes no memory size parameter.

**How to solve**:
1. Update `parse_screen_command()` to optionally parse a 3rd token as `mem_size` for `-s`.
2. Validate `mem_size`:
   - Must be a power of 2.
   - Must be in range `[64, 65536]`.
   - If invalid → print `"invalid memory allocation"` and don't create the process.
3. Pass `mem_size` through `OS::handle_screen_create()` → `Kernel::create_process(name, mem_size)`.
4. In `Kernel::create_process()`, allocate memory via `MemoryManager`.

---

## 7. `screen -c <name> <mem_size> "<instructions>"` — Custom Instructions 🔴

**Files**: Same as §6 plus instruction parsing.

**Problem**: The `-c` subcommand doesn't exist anywhere in the codebase.

**How to solve**:
1. Add `-c` branch to `parse_screen_command()`.
2. Parse: `<name>`, `<mem_size>` (same validation as `-s`), and `"<instructions>"` (quoted string).
3. Split instruction string by semicolons (`;`) to get individual instructions.
4. Validate instruction count: must be `[1, 50]`. If outside range → `"invalid command"`.
5. Create an **instruction parser** function that converts each instruction string into the appropriate `Instruction` subclass:
   - `"DECLARE varA 10"` → `DeclareInstruction("varA", 10)`
   - `"READ varA 0x500"` → `ReadInstruction("varA", 0x500)`
   - `"WRITE 0x500 varA"` → `WriteInstruction(0x500, "varA")`
   - `"ADD varA varA varB"` → `AddInstruction("varA", "varA", "varB")`
   - `"PRINT(\"Result: \" + varC)"` → `PrintExpressionInstruction(...)`
6. Handle malformed instructions gracefully (unknown opcode, bad syntax → parse error message).
7. Pass the parsed instruction list + mem_size to `Kernel::create_process()`.

---

## 8. `screen -r <name>` — Access Violation Reporting 🐛

**Files**: [os.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/core/os.cpp) (screen attach handler)

**Problem**: `screen -r` currently only shows running/finished status. It does NOT check for or report access violations.

**How to solve**:
Add a check when attaching to a process:
```
if (process.access_violation_) {
    print: "Process <name> shut down due to memory access violation error
            that occurred at <HH:MM:SS>. <hex address> invalid."
}
```
Four distinct states to handle:
1. **Process doesn't exist** → `"Process not found."`
2. **Process is still running** → attach to screen, show live output.
3. **Process finished normally** → show completion info.
4. **Process died from access violation** → show violation message with timestamp & address.

---

## 9. `process-smi` Command 🔴

**Files**: [cli.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cli/cli.cpp), [os.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/core/os.cpp), [kernel.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/kernel/kernel.cpp)

**Problem**: Command not registered, not handled, no implementation.

**How to solve**:
1. Register `"process-smi"` in `CLI::command_map`.
2. Add handler in `OS::handle_command()`.
3. Gather data from `Kernel`:
   - **CPU utilization %**: `(busy_cores / total_cores) * 100`.
   - **Memory usage**: `used_memory / total_memory` (from `MemoryManager`).
   - **Running processes list**: each with its memory footprint.
4. Format output to match spec:
```
-----------------------------------------------
| PROCESS-SMI V01.00 Driver Version: 01.00    |
-----------------------------------------------
CPU-Util: <N>%
Memory Usage: <used>MiB / <total>MiB
Memory Util: <N>%

===============================================
Running processes and memory usage:
-----------------------------------------------
<process_name>    <mem>MiB
-----------------------------------------------
```

> [!IMPORTANT]
> Handle edge case: if zero processes running, show `0%` and empty list — do NOT divide by zero.

---

## 10. `vmstat` Command 🔴

**Files**: Same as §9.

**Problem**: Command not registered, not handled, no implementation.

**How to solve**:
1. Register `"vmstat"` in `CLI::command_map`.
2. Add handler in `OS::handle_command()`.
3. Gather stats from `MemoryManager` and `Kernel`:
   - Total memory (K)
   - Used memory (K)
   - Active memory (K) — memory of currently running processes
   - Inactive memory (K) — memory of processes in ready queue / not running
   - Free memory (K)
   - Idle CPU ticks
   - Active CPU ticks
   - Total CPU ticks
   - Pages paged in (count of page loads)
   - Pages paged out (count of evictions)
4. Format as:
```
<N> K total memory
<N> K used memory
<N> K active memory
<N> K inactive memory
<N> K free memory
...
<N> pages paged in
<N> pages paged out
```

---

## 11. Kernel — Memory Manager Integration 🔴

**Files**: [kernel.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/kernel/kernel.h), [kernel.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/kernel/kernel.cpp)

**Problem**: Kernel has no `MemoryManager` member and doesn't initialize or use memory management.

**How to solve**:
1. Add `MemoryManager memory_manager_` member to `Kernel`.
2. In `Kernel::initialize()`: call `memory_manager_.initialize(max_overall_mem, mem_per_frame)`.
3. In `Kernel::create_process()`:
   - Accept `mem_size` parameter.
   - Calculate `num_pages = mem_size / mem_per_frame` (round up if not evenly divisible).
   - Register the process with the memory manager.
4. On process termination (normal or violation):
   - Call `memory_manager_.deallocate_frames(process_id)`.
5. Expose memory stats to OS layer for `process-smi` and `vmstat`.

---

## 12. CPU Core — Page Fault Handling in Tick Loop 🔴

**Files**: [cpu_core.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cpu/cpu_core.h), [cpu_core.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cpu/cpu_core.cpp)

**Problem**: `CPUCore::tick()` blindly executes the next instruction without checking page residency.

**How to solve**:
1. Before executing an instruction, determine which page(s) it accesses.
2. Check if those pages are resident via `MemoryManager::is_page_resident()`.
3. If not resident → trigger `MemoryManager::handle_page_fault()`.
4. If page fault resolution succeeds → **restart** the instruction (don't advance `current_instruction`).
5. If page fault fails (e.g., process too big for memory entirely) → handle error.
6. Only advance `current_instruction` after a **successful** execution with all pages resident.
7. The CPU core needs a pointer/reference to the `MemoryManager`.

---

## 13. Scheduler — Memory-Aware Scheduling 🔴

**Files**: [scheduler.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/scheduler/scheduler.h), [scheduler.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/scheduler/scheduler.cpp)

**Problem**: Scheduler doesn't consider memory when admitting processes or generating random ones.

**What to fix**:
1. **Random process generation** (`scheduler-test`/`scheduler-start`): roll memory size `M` randomly between `min-mem-per-proc` and `max-mem-per-proc` (powers of 2 only, or round to nearest page boundary).
2. **Round Robin quantum + page faults**: if a page fault occurs during a quantum, the fault resolution consumes tick(s) but the quantum counter should still decrement. If quantum expires mid-fault, the process is preempted and the instruction restarts on its next turn.
3. **Process that can never fit in memory** (`mem_size > max-overall-mem`): reject upfront, don't add to scheduler.

---

## 14. `ProcessState` — Add Violation State 🔴

**File**: [constants.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/constants.h)

**Problem**: `ProcessState` only has `READY`, `RUNNING`, `WAITING`, `FINISHED`. No state for access-violation termination.

**How to solve**:
- Add `TERMINATED` to the enum, or use a separate boolean flag on the `Process` class.
- All code that checks `state == FINISHED` must also consider `TERMINATED` where appropriate (e.g., freeing resources, screen -r display).

---

## 15. Instruction Restart on Page Fault 🔴

**Files**: [cpu_core.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/cpu/cpu_core.cpp), [process.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/process/process.cpp)

**Problem**: No mechanism to "restart" an instruction that failed due to a page fault.

**How to solve**:
- When a page fault occurs during `execute_next_instruction()`:
  - Do NOT increment `current_instruction`.
  - Return a status indicating "page fault occurred, instruction not completed".
  - The CPU core's tick loop should handle the page fault (via memory manager), then on the **next tick**, re-attempt the same instruction.
- Consider adding a return type to `execute_next_instruction()`:
  ```cpp
  enum class ExecutionResult { SUCCESS, PAGE_FAULT, ACCESS_VIOLATION };
  ```

---

## 16. Thread Safety — Backing Store & Frame Table 🐛

**Files**: [backing_store.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/backing_store.cpp), [frame_table.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/frame_table.cpp), [memory_manager.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/memory_manager.cpp)

**Problem**: Multiple CPU cores run concurrently and may trigger page faults simultaneously. No synchronization exists.

**How to solve**:
- Add a `std::mutex` to `MemoryManager` that guards all frame allocation, page fault handling, and backing store operations.
- Use `std::lock_guard<std::mutex>` in every public method of `MemoryManager`.
- The backing store file access must also be serialized.

---

## 17. Dirty Page Tracking 🐛

**Files**: [frame_table.h](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/memory/frame_table.h)

**Problem**: The `Frame` struct has no `is_dirty` flag. `PageTableEntry` has `is_dirty` but it's never set during `WRITE` operations (since `WRITE` doesn't exist yet).

**How to solve**:
- Add `bool is_dirty = false` to the `Frame` struct (or rely solely on `PageTableEntry::is_dirty`).
- In `WriteInstruction::execute()`: call `page_table.mark_dirty(page_number)`.
- In victim eviction: check dirty bit. Only write back to backing store if dirty.

---

## 18. Process Cleanup on Termination 🔴

**Files**: [kernel.cpp](file:///C:/Users/Administrator/Desktop/CSOPESY/OS_Emulator/src/kernel/kernel.cpp)

**Problem**: When a process finishes (normally or via access violation), its frames are not freed and backing store entries are not cleaned up.

**How to solve**:
- In the kernel's tick loop, after detecting a process has finished/terminated:
  1. Call `memory_manager_.deallocate_frames(process_id)`.
  2. Call `backing_store_.remove_process(process_id)`.
  3. Remove process from scheduler queues.

---

## 19. Edge Cases & Validation Gaps 🐛

### 19a. Commands before `initialize`
- `process-smi` and `vmstat` must be rejected before initialization (like other commands). Verify all new commands check the initialized flag.

### 19b. Division by zero in utilization calculations
- `process-smi` CPU util and memory util must handle 0 total → show `0%`.

### 19c. `screen -r` for non-existent process
- Verify graceful "Process not found" message, not a crash/nullptr dereference.

### 19d. Duplicate process names
- `screen -s` / `screen -c` with a name that already exists — confirm current behavior (reject or allow) and ensure consistency.

### 19e. Variable not declared
- Using a variable in `ADD`/`WRITE`/`PRINT` that was never `DECLARE`d — define behavior (error or auto-declare with 0).

### 19f. Redeclaring an existing variable
- `DECLARE varA 10` then `DECLARE varA 20` — decide: overwrite or ignore.

### 19g. Malformed hex addresses
- `READ`/`WRITE` with addresses like `"xyz"` instead of `"0x500"` — must not crash.

### 19h. Memory size not evenly divisible by `mem-per-frame`
- For random processes: round up to nearest whole page count. `num_pages = ceil(mem_size / mem_per_frame)`.

---

## 20. MO1 Regression Risks 🟢 → 🐛

**Problem**: MO1 features currently work but could break during MO2 integration.

**Watch for**:
- `create_process()` signature change (adding `mem_size`) — update all callers including `scheduler-test` auto-generation.
- `Process` constructor change — ensure backward compatibility or update all instantiation sites.
- Instruction base class changes — ensure `PrintInstruction` still works.
- `screen -ls` / `report-util` — ensure they handle the new `TERMINATED` state.
- Scheduler tick loop changes — ensure FCFS/RR still function correctly with page-fault logic added.

---

## Summary — Priority Order

| Priority | Task | Section |
|----------|------|---------|
| **P0** | Config: parse 4 new MO2 params | §1 |
| **P0** | Process: add symbol table, memory size, violation state | §5, §14 |
| **P0** | Instructions: DECLARE, READ, WRITE, ADD, PRINT | §4 |
| **P0** | Memory Manager: implement demand paging logic | §2 |
| **P0** | Backing Store: implement file I/O | §3 |
| **P0** | CPU Core: page fault handling in tick loop | §12 |
| **P0** | Instruction restart on page fault | §15 |
| **P1** | Kernel: integrate MemoryManager | §11 |
| **P1** | screen -s: add mem_size param | §6 |
| **P1** | screen -c: implement custom instructions | §7 |
| **P1** | screen -r: access violation reporting | §8 |
| **P1** | Access violation detection & termination | §4b, §4c |
| **P1** | Process cleanup on termination | §18 |
| **P2** | process-smi command | §9 |
| **P2** | vmstat command | §10 |
| **P2** | Scheduler: memory-aware scheduling | §13 |
| **P2** | Thread safety (mutex on memory ops) | §16 |
| **P2** | Dirty page tracking | §17 |
| **P3** | Edge cases & validation | §19 |
| **P3** | MO1 regression testing | §20 |
