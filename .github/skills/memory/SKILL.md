---
name: memory
description: Skill for allocation, paging, memory layout, and memory-management policy in the MO2 emulator.
---

# Memory Skill

Use for memory allocation and paging behavior.

MO2-specific priorities:
- Model physical memory as fixed-size frames derived from max-overall-mem and mem-per-frame, with frame counts based on the spec’s sizing rules.
- Implement per-process page tables and a process memory layout that includes the symbol table segment and the user-accessible address space.
- Support demand paging with page faults, frame allocation, victim selection, eviction, and load-on-demand behavior.
- Persist evicted pages in the backing store file and ensure clean/dirty page handling is handled safely.
- Enforce the MO2 memory rules for process allocation sizes: powers of two in the inclusive range [64, 65536] bytes, and scheduler-generated sizes that respect min/max memory limits.
- Enforce process-local bounds for READ/WRITE: addresses outside the process’s own allocated space are access violations, not cross-process reads or writes.
- Handle symbol-table residency and the 64-byte / 32-variable limit carefully; declarations past that limit should be ignored rather than throwing an error.
- Verify the implementation against both the worked example and failure cases such as access violations, invalid memory sizes, and page-fault restarts.
