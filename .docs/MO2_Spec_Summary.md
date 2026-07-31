# CSOPESY MO2 — Multitasking OS with Memory Management
### Spec Summary & Implementation Guide

This is a continuation of MO1 (CPU scheduler emulator). MO2 adds a **memory manager with demand paging**, a **backing store**, and new **process instructions for memory access**. Everything from MO1 must still work.

---

## 1. What's Carried Over from MO1
- Main menu CLI (`root:\>` prompt), ASCII banner, `initialize`, `screen`, `scheduler-start`, `scheduler-stop`, `report-util`, `exit`.
- FCFS / Round-Robin CPU scheduler across `num-cpu` cores.
- Process screens (`screen -r <name>` to attach/view a process).
- `config.txt`-driven configuration (must run `initialize` before any other command works).

## 2. New Commands to Implement

| Command | Purpose |
|---|---|
| `process-smi` | Summarized memory view — like `nvidia-smi`. Shows CPU util %, total memory usage (used/total), memory util %, and a list of running processes with their memory footprint. |
| `vmstat` (or `vmstat -s`) | Detailed memory/CPU stats: total/used/free memory, active/inactive memory, idle/active/total CPU ticks, pages paged in, pages paged out. |
| `screen -s <name> <mem_size>` | Create a process **with an explicit memory size** (bytes, power of 2, range [2^6, 2^16]). Invalid size → `"invalid memory allocation"`. |
| `screen -c <name> <mem_size> "<instructions>"` | Create a process with **user-defined instructions** (semicolon-separated, 1–50 instructions). Wrong count → `"invalid command"`. |
| `screen -r <name>` (updated) | Attach to a process screen. Now must also report if the process was **shut down due to a memory access violation**, printing the violating address and timestamp. |

## 3. Config File — New Parameters (on top of MO1's)

| Parameter | Meaning |
|---|---|
| `max-overall-mem` | Total simulated physical memory available (bytes, power of 2 in [2^6, 2^16]). |
| `mem-per-frame` | Size of one frame/page (bytes). `total frames = max-overall-mem / mem-per-frame`. |
| `min-mem-per-proc` | Minimum memory a scheduler-generated process can be given. |
| `max-mem-per-proc` | Maximum memory a scheduler-generated process can be given. |

For processes auto-generated via `scheduler-start`/`scheduler-test`, memory size `M` is randomly rolled between `min-mem-per-proc` and `max-mem-per-proc`. Number of pages needed: `P = M / mem-per-frame`.

## 4. Memory Manager — Demand Paging

- Physical memory is divided into fixed-size **frames** (`mem-per-frame` bytes each).
- Each process has its own virtual address space, divided into pages of the same size.
- Pages are loaded into frames **on demand** — not all at once.
- **Page fault**: process accesses a virtual page not currently resident in a frame.
  - Demand pager finds a free frame, or if none exist, picks a **victim frame** (page replacement algorithm — e.g., FIFO or LRU, your choice — check if the spec/instructor mandates one) and evicts it to the **backing store**.
  - The needed page is then loaded from the backing store (or created if it's new) into the freed/available frame.
- **Backing store**: simulated as a plain text file `csopesy-backing-store.txt` that can be inspected at any time — this is where evicted pages live when not in physical memory.
- Memory allocation and page-fault handling **only occur when a process is currently assigned a CPU core** (i.e., only during its execution slice, not while merely waiting in queue).
- Instructions can only execute once **all referenced pages are resident** — if a page fault occurs mid-instruction, that instruction is **restarted** after the fault is resolved. This repeats until successful.
- Variable declarations similarly can't happen if the **symbol table page** isn't resident — this also triggers a page fault.

## 5. Process Memory Layout

- **Symbol table segment**: fixed **64 bytes**, holds up to **32 variables** (each `uint16` = 2 bytes). Once full, further `DECLARE`-type instructions are silently ignored.
- Variables belong to the process and persist (in memory) until the process terminates.
- Memory addresses are **hexadecimal** and **emulated** — not real RAM addresses.

## 6. New Instructions

### `READ(var, memory_address)`
- Reads a `uint16` from the given hex address into `var`.
- If that address was never written to, returns `0`.

### `WRITE(memory_address, value)`
- Writes a `uint16` value to the given hex address.
- Value is clamped to `[0, 65535]` (max uint16).

### Access Violations
- Reading/writing an address **outside the process's allocated memory space** → **access violation error**, and the process is **immediately terminated** (like a real segfault).
- On `screen -r <name>` for a process that died this way, print:
  ```
  Process <name> shut down due to memory access violation error that occurred at <HH:MM:SS>. <hex address> invalid.
  ```

### Example (from spec)
```
screen -c process2 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\"Result: \" + varC)"
```
1. `DECLARE varA 10` → varA = 10
2. `DECLARE varB 5` → varB = 5
3. `ADD varA varA varB` → varA = 15
4. `WRITE 0x500 varA` → writes 15 to address 0x500
5. `READ varC 0x500` → varC = 15
6. `PRINT("Result: " + varC)` → prints "Result: 15"

## 7. process-smi Sample Output
```
-----------------------------------------------
| PROCESS-SMI V01.00 Driver Version: 01.00    |
-----------------------------------------------
CPU-Util: 100%
Memory Usage: 1245MiB / 4795MiB
Memory Util: 26%

===============================================
Running processes and memory usage:
-----------------------------------------------
process05 134MiB
process06 134MiB
process07 977MiB
-----------------------------------------------
root:\>
```

## 8. vmstat Sample Fields
```
<N> K total memory
<N> K used memory
<N> K active memory
<N> K inactive memory
<N> K free memory
...
<N> pages paged in
<N> pages paged out
...
```
(Match the general shape/labels of Linux `vmstat -s`; not every Linux field is required, focus on the ones explicitly listed in section 6 of the spec: total, used, free, idle/active/total CPU ticks, pages paged in/out.)

## 9. Build Order Suggestion
1. Update `config.txt` parsing to read the 4 new MO2 parameters.
2. Implement the frame table / physical memory model (`max-overall-mem / mem-per-frame` frames).
3. Implement per-process page table + symbol table segment (64 bytes / 32 vars).
4. Implement `READ`/`WRITE` instructions with clamping + bounds checking.
5. Implement page-fault handling: victim selection, eviction to `csopesy-backing-store.txt`, load-on-demand, instruction restart.
6. Wire page faults into the scheduler tick loop (only fault while a process has a CPU).
7. Implement access-violation detection → terminate process, log timestamp+address.
8. Update `screen -r` to report violation shutdowns.
9. Implement `process-smi` and `vmstat` display commands.
10. Update `screen -s` (mem size + range validation) and add `screen -c` (instruction string parsing, 1–50 instruction count validation).
11. Regression-test all MO1 features still work (FCFS/RR scheduling, scheduler-start/stop, report-util, etc.).

## 10. Edge Cases to Handle

### Config / Startup
- Any command other than `initialize` (and maybe `exit`) typed before initialization → must be rejected/ignored with a message, not crash.
- `config.txt` missing a parameter, malformed, or containing out-of-range values (e.g., `num-cpu = 0` or `> 128`) — decide whether to reject at startup or clamp; be consistent and don't silently proceed with garbage.
- `max-overall-mem` not evenly divisible by `mem-per-frame` — decide how you'll round (floor the frame count) and document it.
- `min-mem-per-proc > max-mem-per-proc` — invalid config, should be guarded against.
- Memory values that are **not powers of 2**, or outside `[2^6, 2^16]`, in config — should probably still be validated the same way manual allocations are.

### `screen -s` / `screen -c` Memory Allocation
- Memory size **not a power of 2** → `"invalid memory allocation"`.
- Memory size **below 64 bytes or above 65536** → `"invalid memory allocation"`.
- Memory size exactly at the boundary (`64` or `65536`) — should be **accepted**, not rejected (inclusive range).
- Creating a process with a name that **already exists** (running or finished) — MO1 behavior should apply; decide/confirm reject vs. allow duplicate.
- `screen -c` instruction string with **0 instructions** or **more than 50** → `"invalid command"`.
- `screen -c` with malformed instructions (bad syntax, unknown opcode, mismatched quotes/semicolons) — needs its own parse-error handling, distinct from the count-based `"invalid command"`.
- Requesting a process whose required memory **exceeds `max-overall-mem` entirely** (i.e., process can never fit even alone) — should probably be rejected outright rather than infinite-loop trying to page it in.
- Not enough **total** frames across the system for all processes' minimum footprint (e.g., symbol table page) simultaneously — starvation scenario; make sure it doesn't deadlock.

### Variables / Symbol Table
- Declaring a variable **after the 32-variable / 64-byte limit** is reached → instruction is **silently ignored** (not an error, not a crash) — easy to accidentally throw an error here instead.
- Redeclaring a variable that already exists — decide: overwrite value, or ignore, or error (spec doesn't say explicitly; pick one and be consistent).
- Using a variable in `ADD`/`WRITE`/`PRINT` that was **never declared** — needs defined behavior (auto-declare with 0? error? spec implies variables must be declared first).
- Arithmetic overflow: `ADD`/`SUBTRACT` producing a result `> 65535` or `< 0` → must be **clamped** to uint16 range, not wrapped or overflowed silently.

### READ / WRITE Instructions
- `WRITE` to an address **within the process's allocated space but not yet touched** → must default-initialize to 0 conceptually, not crash, before being overwritten.
- `READ` from an address **never written** → returns `0` (per spec), not garbage/undefined.
- `WRITE` value **outside uint16 range** (e.g., negative, or > 65535) — must be clamped to `[0, 65535]` before storing.
- `READ`/`WRITE` to an address **outside the process's own allocated memory space** (even if that address is "valid" for another process) → **access violation**, process terminated immediately, timestamp + address logged.
- Access violation occurring **mid-instruction-string** in a `screen -c` batch — remaining instructions in that batch must **not** execute after termination.
- Malformed hex address (e.g., missing `0x` prefix, non-hex characters) — needs graceful handling, not a crash.

### Paging / Backing Store
- Page fault occurring when **no frames are free and no victim is currently evictable** (e.g., all frames pinned/in-use this cycle) — must not deadlock; define a fallback.
- Same page faulted **repeatedly in a tight loop** (e.g., thrashing between two processes needing more frames than available) — system should keep making progress, not hang forever undetected.
- Evicting a **dirty page** (written to) vs a **clean page** — dirty pages must be written back to `csopesy-backing-store.txt` before eviction; clean/unmodified pages arguably don't need a re-write.
- Reading `csopesy-backing-store.txt` for a page that was **never actually evicted yet** — should return a sensible default (e.g., zeroed page), not error.
- Concurrent access to `csopesy-backing-store.txt` from multiple CPU cores/processes evicting/loading pages **at the same time** — needs synchronization (file lock / mutex) to avoid corruption.
- Process **terminates (normally or via violation)** — its frames must be freed and its backing-store entries cleaned up/invalidated so they aren't reused incorrectly by another process.
- A page fault happening on a process that **loses its CPU core mid-fault-resolution** (preempted by Round Robin quantum expiry) — instruction must correctly resume/restart after the fault is resolved on its next turn, not skip or duplicate.

### Scheduler Interaction
- Round Robin quantum expiring **exactly while a page fault is being serviced** — must not double-count cycles or lose the process's place in the ready queue.
- A process that page-faults on **every single instruction** (e.g., pathological/adversarial input) with a very high `delays-per-exec` — verify it doesn't stall the whole scheduler or other cores.
- `scheduler-start` generating random processes whose rolled memory size `M` (between `min-mem-per-proc` and `max-mem-per-proc`) isn't cleanly divisible by `mem-per-frame` — decide how partial-page rounding works (usually round up to a whole page).

### Commands / Display
- `process-smi` / `vmstat` called **before `initialize`** → should be rejected like any other command.
- `process-smi` / `vmstat` called when **zero processes are running** → should show 0%/empty list gracefully, not divide-by-zero on utilization %.
- `screen -r <name>` for a process that's **still running, finished normally, doesn't exist, or died from violation** — four distinct messages/behaviors, easy to conflate two of them.
- Very long or duplicate process names, or names with special characters — decide on validation rules and apply them consistently across `screen -s`, `screen -c`, and `screen -r`.

## 11. Grading Notes
- Assessed via a **black-box quiz** — test cases given live, demoed via recorded video (.MP4), no recompiling allowed during the quiz (only `config.txt` parameter changes).
- Deliverables: **source code** (+ README with name & run instructions, or GitHub link) and a **PPT technical report** covering command recognition, process/memory representation, scheduler implementation, and memory management (paging + backing store).
- Grading is per test case: no points (no workaround) / partial points (workaround exists) / full points (passes directly).
