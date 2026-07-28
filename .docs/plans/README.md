# Project Plans

This document tracks the current and future plans for the OS_Emulator project. The project simulates a basic operating system with command interpretation, process scheduling, memory management, and CPU execution cycles.

## Roadmap

The development of the OS Emulator is divided into the following phases:

### Phase 1: Command Interpreter and Console UI
**Goal:** Establish a robust interactive environment for users to interact with the OS emulator.
*   **Command Recognition:** Implement a robust lexer and parser in `CommandHandler` to tokenize input strings and validate commands against a known list (e.g., `screen`, `scheduler-test`, `scheduler-stop`, `report-util`, `clear`, `exit`).
*   **Console UI Implementation:** Improve the `console` and `ui` modules to provide a multi-view console experience. Support different screens like the main menu and process-specific views.
*   **Command Interpreter:** Wire the parsed commands to their respective kernel and manager functions.

### Phase 2: CPU Execution and Process Management
**Goal:** Accurately simulate CPU cycles and process instruction execution.
*   **Process Representation:** Finalize the `Process` class state machine (`READY`, `RUNNING`, `WAITING`, `FINISHED`). Ensure process creation generates a valid stream of instructions based on configuration (`min-ins`, `max-ins`).
*   **CPU Cycles:** Implement tick handlers in `Kernel`, `CPUManager`, and `CPUCore`. Process instructions per tick and handle instruction delays (`delays-per-exec`).
*   **Interrupts and Logging:** Fully integrate the `LogEntry` passing mechanism to bubble up events from `Instruction` to `Kernel` for accurate simulation logs and interrupts.

### Phase 3: Schedulers Implementation
**Goal:** Implement and test process scheduling algorithms.
*   **Scheduler Base Class:** Finalize `scheduler.cpp` and refine the `Scheduler` interface (`add_process`, `tick`).
*   **FCFS (First-Come, First-Served):** Polish `fcfs.cpp`. Ensure non-preemptive logic correctly assigns idle cores to the next process in the ready queue without interrupting running ones.
*   **Round Robin (RR):** Complete `round_robin.cpp`. Implement quantum-based preemption. Ensure processes are context-switched out when their quantum expires, and correctly re-added to the ready queue.

### Phase 4: Memory Management Integration
**Goal:** Simulate physical memory allocation for processes.
*   **Memory Allocators:** Finalize `FirstFitAllocator` and `PagingAllocator`.
*   **Scheduler Integration:** Modify the schedulers so they request memory from `IMemoryAllocator` before transitioning a process from `WAITING`/New to `READY`. If memory is full, the process must wait.
*   **Deallocation:** Ensure that when a process transitions to `FINISHED`, the scheduler correctly deallocates the process's memory block or pages.
*   **Visualization:** Provide command-line tools to visualize the current memory layout and generate memory stamps (`memory_stamp_<quantum_cycle>.txt`).

### Phase 5: Reporting and System Integration
**Goal:** End-to-end testing and system observability.
*   **Reporting:** Output CPU utilization, memory utilization, and process turnaround/waiting times.
*   **Config Validation:** Ensure all parameters from `config.txt` dynamically influence the simulation properly.
*   **System Testing:** Run long `scheduler-test` batches to ensure no deadlocks, memory leaks, or segmentation faults occur in the emulator.
