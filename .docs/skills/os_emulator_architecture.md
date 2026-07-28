# OS Emulator Codebase Architecture Skill

## Architecture Overview

This C++20 project emulates a multitasking OS with CLI, CPU scheduling, and memory management.
The executable entry point is `src/main.cpp` → `Kernel::start()`.

## Module Map

| Module | Directory | Key Classes | Notes |
|---|---|---|---|
| **CLI** | `src/cli/` | `Console`, `CommandHandler`, `ui` | Handles user input, routes to `Kernel::handle_command()` |
| **Config** | `src/config/` | `Config` struct, `loadConfig()`, `validateConfig()` | Parses `config.txt` (space-separated key-value). File path: `../../config.txt` relative to build dir |
| **Core** | `src/core/` | `ProcessGenerator`, `ProcessViewer`, `ReportGenerator` | Business logic layer between Kernel and subsystems |
| **CPU** | `src/cpu/` | `CPUManager`, `CPUCore` | Tick-based execution. `CPUManager` tracks utilization and idle/active ticks |
| **Kernel** | `src/kernel/` | `Kernel` | Main loop on background thread. Owns all subsystems. Drives `cpu_manager->tick()`, `process_generator->tick()`, `scheduler->tick()` |
| **Memory** | `src/memory/` | `IMemoryAllocator`, `FirstFitAllocator`, `PagingAllocator` | `IMemoryAllocator` is the interface. Both allocators are thread-safe (internal mutex) |
| **Process** | `src/process/` | `Process`, `ProcessManager`, `ProcessLogger`, `Instruction` subclasses | Instructions: Print, Declare, Add, Subtract, Sleep, For (nestable) |
| **Scheduler** | `src/scheduler/` | `Scheduler` (base), `FCFSScheduler`, `RoundRobinScheduler` | Both schedulers integrate with `IMemoryAllocator` for memory-aware scheduling |

## Key Architectural Patterns

### 1. Scheduler Memory Integration Pattern
Both `FCFSScheduler` and `RoundRobinScheduler` follow this pattern:
- Maintain an `std::unordered_map<int, void*> process_memory_ptr` mapping pid → allocated memory pointer
- On **first admission**: call `memory_allocator.allocate(mem_per_proc, p->id, p->process_name)`
- On **process finish**: call `memory_allocator.deallocate(mem_it->second)` and erase from map
- On **allocation failure**: requeue process at tail of ready queue
- **Queue scanning**: Scan up to `ready_queue.size()` entries to find an admittable process (prevents starvation)

### 2. RR-Specific: Preemption + Memory Residency
- Preempted processes keep their memory (NOT deallocated)
- Check `process_memory_ptr.find(p->id) != end()` to detect resident processes (skip re-allocation)
- Only deallocate on `ProcessState::FINISHED`

### 3. Memory Allocator Selection (in Kernel)
```cpp
if (config.mem_per_frame < config.max_overall_mem) {
    // Paging allocator: frame-based with FIFO eviction
    memory_allocator = make_unique<PagingAllocator>(max_overall_mem, mem_per_frame);
} else {
    // Flat first-fit: contiguous allocation
    memory_allocator = make_unique<FirstFitAllocator>(max_overall_mem);
}
```

### 4. Kernel Main Loop Order
```
1. cpu_manager->tick()           // execute instructions on all cores
2. process_generator->tick()     // generate new processes (if scheduler-start active)
3. scheduler->tick()             // assign ready processes to idle cores
4. take_memory_snapshot_if_due() // periodic memory_stamp_<qq>.txt
5. sleep(TICK_DURATION)          // pace the loop (20ms default)
```

### 5. Config File Format
File: `config.txt` (path `../../config.txt` from build directory)
```
num-cpu 4
scheduler "rr"
quantum-cycles 5
batch-process-freq 1
min-ins 100
max-ins 100
delays-per-exec 0
max-overall-mem 16384
mem-per-frame 16
mem-per-proc 4096
```
- Scheduler values are quoted strings: `"fcfs"` or `"rr"`
- Validated by `validateConfig()` with range checks

## Build System
- CMake-based with per-module static libraries
- Each module has its own `CMakeLists.txt` with `add_library()`
- `kernel` is the root library linking all others
- `os_emulator` executable links only to `kernel`
- Build: `cmake -G "MinGW Makefiles" -B build -S . && cmake --build build`
- Run: `build/src/os_emulator.exe`

## Common Pitfalls
1. **File paths are relative to build dir** — `../../config.txt`, `../../csopesy_report.txt`, etc.
2. **Process ownership** — `ProcessManager` owns `Process` objects via `unique_ptr`. Schedulers hold raw `Process*` pointers.
3. **Thread safety** — Main loop runs on `clock_thread`, CLI runs on main thread. `ProcessManager` has `pm_mutex`, allocators have `mem_mutex`.
4. **Header organization** — No separate `.h` files for schedulers; all declared in `scheduler.h`. Process types all in `os_process.h`.
