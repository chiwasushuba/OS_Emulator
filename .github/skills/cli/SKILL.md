---
name: cli
description: Skill for command handling, console flow, and user-facing orchestration in the MO2 emulator.
---

# CLI Skill

Use for parser behavior, command dispatch, prompts, and terminal interaction.

MO2-specific priorities:
- Preserve all MO1 commands and the root prompt behavior while adding the MO2 command set.
- Implement the new commands exactly as described in the MO2 spec summary: process-smi, vmstat, screen -s, screen -c, and the updated screen -r reporting.
- Validate memory-size input and instruction-count input using the spec’s required error behavior: invalid memory allocation for bad sizes and invalid command for bad instruction counts.
- Reject commands that arrive before initialization in a consistent, non-crashing way.
- Keep output formatting aligned with the spec examples where possible, especially process-smi and vmstat labels.
- Ensure screen -r reports access-violation shutdowns with the timestamp and offending address.
- Treat the CLI as a contract surface: if a case is ambiguous, prefer the explicit MO2 spec behavior over a “reasonable” interpretation.
