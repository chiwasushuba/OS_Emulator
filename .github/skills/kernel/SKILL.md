---
name: kernel
description: Skill for kernel control flow, boot sequencing, and cross-cutting OS behavior in the MO2 emulator.
---

# Kernel Skill

Use for work that affects how the emulator coordinates its core services.

MO2-specific priorities:
- Extend config parsing to include the new MO2 parameters: max-overall-mem, mem-per-frame, min-mem-per-proc, and max-mem-per-proc.
- Validate config values consistently and reject malformed or invalid ranges instead of silently proceeding with bad data.
- Coordinate initialization so the memory subsystem, process subsystem, and scheduler all come up with a coherent state.
- Keep the kernel responsible for system-wide invariants such as frame availability, total memory sizing, and startup rejection of commands before initialization.
- Make the kernel behavior match the MO2 spec’s build order and edge cases rather than introducing ad hoc defaults.
- When changing kernel logic, verify that MO1 behavior still works alongside the new memory-management features.
