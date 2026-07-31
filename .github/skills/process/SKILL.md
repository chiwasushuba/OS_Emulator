---
name: process
description: Skill for process lifecycle, state tracking, logging, and process representation in the MO2 emulator.
---

# Process Skill

Use for process creation, transitions, identifiers, and process-level reporting.

MO2-specific priorities:
- Track each process’s memory footprint, page table, symbol table, and termination reason so screen -r can report the correct state.
- Support the new process creation flows: screen -s for explicit memory size and screen -c for instruction strings with memory-aware semantics.
- Implement the symbol table as a fixed 64-byte / 32-variable structure; later declarations beyond that limit should be silently ignored.
- Support READ/WRITE semantics and clamp uint16 values into the valid range [0, 65535].
- Record access violations with the timestamp and offending address so the process screen can report them later.
- Preserve MO1 process behavior such as naming, screen attachment, and lifecycle transitions while adding the MO2 memory and fault states.
- Treat process termination as a first-class state transition, including freeing frames and invalidating any backing-store state for that process.
