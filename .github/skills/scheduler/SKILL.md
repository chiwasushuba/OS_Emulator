---
name: scheduler
description: Skill for scheduling policy and dispatch logic, including FCFS, round robin, queueing, and time-slice behavior in the MO2 emulator.
---

# Scheduler Skill

Use for fairness, queueing, and scheduling decisions.

MO2-specific priorities:
- Keep FCFS and Round-Robin behavior intact while integrating memory-management events into the dispatch loop.
- Only resolve page faults when a process is actually running on a CPU core; avoid faulting while the process is waiting in the ready queue.
- Handle restart semantics correctly when a page fault occurs mid-instruction so the instruction is retried rather than partially executed.
- Be careful with RR quantum boundaries so a fault-resolution path does not double-count cycles or lose the process’s place in the queue.
- Ensure scheduler-generated processes receive memory sizes that honor the MO2 config range and are rounded appropriately for paging.
- Verify that a process that repeatedly faults does not stall the entire scheduler and that other cores continue making progress.
