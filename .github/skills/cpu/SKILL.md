---
name: cpu
description: Skill for CPU execution, CPU manager logic, and processor coordination in the MO2 emulator.
---

# CPU Skill

Use for execution paths, core state, and instruction flow tied to CPU behavior.

MO2-specific priorities:
- Keep the existing FCFS and Round-Robin scheduling behavior intact while wiring in memory-aware execution.
- Only trigger page faults and symbol-table residency checks while a process is actively assigned to a CPU core; do not fault while the process is merely waiting in the ready queue.
- If an instruction faults, restart it after the page is resident instead of partially executing it.
- Respect the MO2 execution model for READ/WRITE/DECLARE/ADD/PRINT flows and ensure they behave consistently with the spec examples.
- Preserve CPU accounting and process state transitions while adding memory-related delays or faults.
- When debugging CPU behavior, verify both the success path and a failure path such as an access violation or a page fault restart.
