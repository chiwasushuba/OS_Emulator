// TODO: Implement Random Process Generation

#include <memory>
#include <string>
#include <vector>
#include "os_process.h"

/**
 * Creates a dynamically allocated test process filled with a variety of instructions.
 * * @param pid The process ID to assign.
 * @param name A descriptive string name for the process.
 * @return Process* Pointer to the constructed process (stored on the heap).
 */
Process* create_dummy_test_process(ProcessManager& pm, const std::string& name) {
    // 1. Instantiate the base Process object on the heap
    int pid = pm.create_process(name);
    Process* proc = pm.get_process(pid);
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;

    // 2. Add sample simple instructions
    proc->add_instruction(std::make_unique<DeclareInstruction>("counter", 0));
    proc->add_instruction(std::make_unique<PrintInstruction>("Initializing loop simulation..."));

    // 3. Build instructions to inject inside the FOR loop
    std::vector<std::unique_ptr<Instruction>> loop_body;
    loop_body.push_back(std::make_unique<AddInstruction>(1, 1, 5)); // Add to register/variable
    loop_body.push_back(std::make_unique<PrintInstruction>("Loop cycle executed. Delaying."));
    loop_body.push_back(std::make_unique<SleepInstruction>(2));     // Sleep for 2 ticks inside the loop

    // 4. Wrap the loop body instructions inside a ForInstruction (runs 3 times)
    proc->add_instruction(std::make_unique<ForInstruction>(std::move(loop_body), 3));

    // 5. Add a final sign-off instruction
    proc->add_instruction(std::make_unique<PrintInstruction>("Simulation Complete. Core spinning down."));

    return proc;
}