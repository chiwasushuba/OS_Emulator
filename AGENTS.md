# OS_Emulator Agent Routing

All prompts in this workspace should first route through `project-orchestrator`.

Routing rules:

- `project-orchestrator` is the top-level router for every prompt.
- If the request is about prompt quality, wording, scope, or response shape, route it to `prompt-engineer` first.
- If the request is about OS implementation work, route it to `os-developer` after prompt refinement.
- `research` is for read-only discovery.
- `self` is only for focused follow-up work in the current context.
- `project-orchestrator` may hand work back and forth between `prompt-engineer` and `os-developer` until the prompt is stable and the task is correctly scoped.

Keep the orchestration loop simple: refine the prompt, choose the right role, delegate the smallest useful slice of work, and reconcile the result before moving on.
