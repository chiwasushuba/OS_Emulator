---
name: prompt-engineering
description: Skill for prompt refinement, scope clarification, instruction shaping, and response-format tuning for MO2 emulator work.
---

# Prompt Engineering Skill

Use this skill when a prompt is vague, overly broad, or needs cleaner wording before routing to another agent.

Focus on:

- Clarifying intent.
- Narrowing scope.
- Improving instruction quality.
- Shaping the expected response format.

For MO2 emulator tasks, prefer prompts that explicitly name the subsystem and the spec contract:
- Ask for the relevant subsystem boundary first: kernel, memory, process, scheduler, cpu, or cli.
- Include the MO2 spec summary as the source of truth, especially for new commands, config parameters, paging rules, and access-violation behavior.
- Make the expected output concrete: touched files, behavior change, validation performed, and which edge cases were exercised.
- Avoid vague requests like “add MO2 support” when the prompt should name the specific command or invariant being implemented.
