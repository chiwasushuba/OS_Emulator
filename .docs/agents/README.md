# Project Agents

This document outlines the role-based agents available for use in this project's environment. In the Antigravity-compatible model, roles route work and skills hold subsystem knowledge and task-specific behavior.

## Available Agents

- [project-orchestrator](project-orchestrator.md)
- [prompt-engineer](prompt-engineer.md)
- [os-developer](os-developer.md)
- [research](research.md)
- [self](self.md)

## Agent Model

- `project-orchestrator` is the routing role. It decides which skill to use, decomposes work, and validates the result.
- `prompt-engineer` is the prompt-shaping role. It refines prompts, improves scope, and hands the result back to `project-orchestrator`.
- `os-developer` is the main implementation role. It uses the relevant skills to change code and verify behavior.
- `research` is the read-only discovery role.
- `self` inherits the parent agent context when a focused follow-up is needed.

## Managing Agents

Agents can be invoked and managed dynamically using the Antigravity system tools (e.g., `invoke_subagent`, `manage_subagents`).
