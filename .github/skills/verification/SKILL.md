---
name: verification
description: Skill for regression checks, smoke tests, and validation evidence for MO2 emulator changes.
---

# Verification Skill

Use to confirm behavior after changes and define the narrowest test that proves a fix.

MO2-specific priorities:
- Verify both a happy path and a failure path for every touched invariant.
- For memory work, test the worked example from the spec and at least one failure case such as an invalid memory size or an access violation.
- For CLI work, confirm the new commands produce the expected messages and output shape, especially for process-smi, vmstat, screen -s, screen -c, and screen -r.
- For paging behavior, verify that a faulted instruction restarts correctly once the needed page is resident.
- For regression safety, rerun the existing MO1-style behaviors such as scheduler-start/stop, report-util, and screen attachment to ensure they still work.
- Report evidence plainly: what was executed, what changed, and whether the observed behavior matched the spec.
