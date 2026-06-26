# OS_Emulator

## Features

### Command recognition
- TODO

### Console UI implementation
- TODO

### Command interpreter implementation
- TODO

### Process representation
A process is represented by the `Process` class which is defined in the Process Library.

#### Class Members
- `private` members
  - `instruction_list` - a `std::vector<std::unique_ptr<Instruction>>`
  containing the instructions that make up the process.

- `public` members
  - `current_instruction` - index of the current instruction.
  - `id` - the process id, if set to `-1` the process is considered invalid.
  - `core_id` - the id of the core it is assigned to
  - `state` - the state of the process, `READY` `RUNNING` `WAITING` `FINISHED`
  - `process_name` - a string representing the process name

#### Process Execution
The `Kernel` is responsible for simulating a `tick` cycle. It calls the `CPUManager`'s tick handler. The `CPUManager` then calls `CPUCore`'s tick handler. 

The `CPUCore`'s tick handler calls the process's `execute_next_instruction` method, which is responsible for updating its `current_instruction` index. It also automatically changes its `state` to `FINISHED` if the process is finished, i.e. `current_instruction >= (instruction_list.size())`.

During the cycle, a `LogEntry` object is passed through the call chain:

`Kernel -> CPUManager -> CPUCore -> Process -> Instruction`

This `LogEntry` object is modified by the `Process` and `Instruction` classes to update execution details such as the timestamp, current instruction, and log message.

The `LogEventType` field is particularly important because it allows the `CPUManager` to signal an interrupt to the `Kernel`. In this emulator, interrupts are primarily used to handle logging events.

### Scheduler implementation
- TODO

# Instructions on how to run