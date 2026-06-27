# OS_Emulator

## Project Setup
This project uses **CMake** as its build system. CMake generates the necessary build files for your platform and compiler.

### Requirements

Make sure you have the following installed:

- A C/C++ compiler:
  - GCC / Clang (Linux/macOS)
  - MSVC (Windows)
- CMake (3.16 or newer recommended)
- MinGW (GCC toolchain)

### Build and Run
From the project root directory, run the following commands:

**1. Configure the build system.**
```
cmake -G "MinGW Makefiles" -B build -S .
```
**2. Compile the project**
```
cmake --build build
```
**3. Run the application**
```
cd build/src
os_emulator.exe
```

You can change the output folder and use any generator you prefer.

For Windows users, you can use the provided batch files `build.bat` to configure and build, then `run.bat` to compile the program.

## Features

### Command recognition
Command recognition is handled by the `CommandHandler` class.

The `CommandHandler` is responsible for tokenizing, validating, and mapping a command input into a `CommandPacket` object.

The first token is identified as the command keyword, while the remaining tokens are treated as arguments.

### Console UI implementation
The console's main I/O loop lives on a separate thread from the main loop that simulates a `tick` throughout the OS emulator. This allows the CLI to provide a smooth experience despite having multiple processes running in the background.

### Command interpreter implementation
Once a command has been tokenized, validated, and mapped, the `CommandHandler` executes the command by calling the `Kernel`'s local command handler with the `CommandPacket` as the argument.

The `CommandPacket` contains the following elements:
- `CommandType` - an enumeration of command types supported by the kernel.
- `ScreenAction` - an enumeration for the specific actions for the "screen" command.
- `payload` - a payload string, currently being used solely by the "screen" command.

The kernel interprets the information contained in the `CommandPacket` and dispatches the request to the appropriate subsystem to perform the requested operation.

The interaction flow is as follows:
```
  Kernel
    ↓ creates
  Console
    ↓ forwards user input to
  CommandHandler
    ↓ parses into
  CommandPacket
    ↓ passes to
  Kernel
    ↓ dispatches to
  Kernel Subsystems
```
### Process representation
A process is represented by the `Process` class which is defined in the Process Library.

#### Class Members
- `private` members
  - `instruction_list` - a `std::vector<std::unique_ptr<Instruction>>`
  containing the instructions that make up the process.

- `public` members
  - `current_instruction` - index of the current instruction.
  - `id` - the process id, if set to `-1` the process is considered invalid.
  - `core_id` - the id of the core it is assigned to.
  - `state` - the state of the process, `READY` `RUNNING` `WAITING` `FINISHED`
  - `process_name` - a string representing the process name.

#### Process Execution
The `Kernel` is responsible for simulating a `tick` cycle. It calls the `CPUManager`'s tick handler. The `CPUManager` then calls `CPUCore`'s tick handler. 

The `CPUCore`'s tick handler calls the process's `execute_next_instruction` method, which is responsible for updating its `current_instruction` index. It also automatically changes its `state` to `FINISHED` if the process is finished, i.e. `current_instruction >= (instruction_list.size())`.

During the cycle, a `LogEntry` object is passed through the call chain:

`Kernel -> CPUManager -> CPUCore -> Process -> Instruction`

This `LogEntry` object is modified by the `Process` and `Instruction` classes to update execution details such as the timestamp, current instruction, and log message.

The `LogEventType` field is particularly important because it allows the `CPUManager` to signal an interrupt to the `Kernel`. In this emulator, interrupts are primarily used to handle logging events.

#### Scheduler implementation
The `Kernel` is responsible for ticking the active scheduling algorithm on every clock cycle. The base `Scheduler` class defines an abstract contract interface requiring any concrete scheduler to implement `add_process` and `tick`.

Both scheduler implementations utilize a First-In, First-Out queue (`std::queue<Process*>`) to manage ready processes waiting for CPU allocation.

##### 1. First-Come, First-Served (FCFS) Scheduler
The `FCFSScheduler` is a non-preemptive algorithm. During a scheduling cycle (`tick`):
* It monitors active CPU cores and checks if their assigned processes have completed (`ProcessState::FINISHED`).
* Completed processes are immediately detached, freeing the core.
* If a core is idle and the ready queue is not empty, the scheduler pops the process at the front of the queue and assigns it to the core. A process assigned to a core runs continuously without interruption until finished.

##### 2. Round Robin (RR) Scheduler
The `RoundRobinScheduler` is a preemptive algorithm that supports time-sharing. It introduces a `quantum` parameter and a `core_cycles` tracker:
* The `core_cycles` vector tracks the cycle count for each active core.
* During a `tick` cycle, if a core is not idle, the scheduler increments the core's cycle counter.
* If the elapsed cycles on a core reach or exceed the `quantum` threshold:
  1. The running process is preempted (detached from the core).
  2. Its state is updated back to `ProcessState::READY`.
  3. It is appended to the back of the ready queue.
  4. The core's cycle tracker is reset to `0`, making the core available for the next process.

##### Lifecycle State Transitions
The scheduling loop drives process states through the following state machine transitions:

`READY (in Queue) -> RUNNING (on Core) -> READY (if Preempted by RR) -> FINISHED (detached from Core)`