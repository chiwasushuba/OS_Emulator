// Process generators: test helpers and the ProcessGenerator class for scheduler-start/stop

#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "os_process.h"
#include "core.h"
#include "scheduler.h"

Process* test_for_loop(ProcessManager& pm, const std::string& name) {
    int pid = pm.create_process(name);
    Process* proc = pm.get_process(pid);
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;
    
    std::vector<std::unique_ptr<Instruction>> loop_body;
    loop_body.push_back(std::make_unique<AddInstruction>(1, 1, 5)); // Add to register/variable
    loop_body.push_back(std::make_unique<PrintInstruction>("Loop cycle executed. Delaying."));
    loop_body.push_back(std::make_unique<SleepInstruction>(2));     // Sleep for 2 ticks inside the loop

    proc->add_instruction(std::make_unique<ForInstruction>(std::move(loop_body), 3));
    return proc;
}

Process* test_deep_for_loops(ProcessManager& pm, const std::string& name) {
    int pid = pm.create_process(name);
    Process* proc = pm.get_process(pid);
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;

    // Deepest level
    std::vector<std::unique_ptr<Instruction>> level4;
    level4.push_back(
        std::make_unique<PrintInstruction>("LEVEL 4 EXECUTED")
    );

    // Level 3
    std::vector<std::unique_ptr<Instruction>> level3;
    level3.push_back(
        std::make_unique<ForInstruction>(std::move(level4), 3)
    );

    // Level 2
    std::vector<std::unique_ptr<Instruction>> level2;
    level2.push_back(
        std::make_unique<ForInstruction>(std::move(level3), 3)
    );

    // Level 1
    std::vector<std::unique_ptr<Instruction>> level1;
    level1.push_back(
        std::make_unique<ForInstruction>(std::move(level2), 3)
    );

    // Root loop
    proc->add_instruction(
        std::make_unique<ForInstruction>(std::move(level1), 3)
    );

    return proc;
}

Process* test_nested_for_loops(ProcessManager& pm, const std::string& name) {
    int pid = pm.create_process(name);
    Process* proc = pm.get_process(pid);
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;

    std::vector<std::unique_ptr<Instruction>> outer_loop;
    std::vector<std::unique_ptr<Instruction>> inner_loop;
    
    outer_loop.push_back(std::make_unique<PrintInstruction>("OUTER LOOP PRINT"));
    inner_loop.push_back(std::make_unique<PrintInstruction>("INNER LOOP PRINT"));
    outer_loop.push_back(std::make_unique<ForInstruction>(std::move(inner_loop), 3));
    proc->add_instruction(std::make_unique<ForInstruction>(std::move(outer_loop), 3));

    return proc;
}    

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

// ============================================================================
// ProcessGenerator — drives scheduler-start / scheduler-stop
// ============================================================================


ProcessGenerator::ProcessGenerator(ProcessManager& pm, Scheduler& sched, const Config& cfg)
    : process_manager(pm), scheduler(sched), config(cfg)
{}

std::string ProcessGenerator::make_process_name(int number) const {
    std::ostringstream oss;
    // Determine minimum width: at least 2 digits (p01, p02, ..., p99, p100, ...)
    int width = 2;
    if (number >= 100)   width = 3;
    if (number >= 1000)  width = 4;
    if (number >= 10000) width = 5;

    oss << "p" << std::setw(width) << std::setfill('0') << number;
    return oss.str();
}

void ProcessGenerator::generate_one_process() {
    std::string name = make_process_name(next_process_number++);

    int pid = process_manager.create_process(name);
    Process* proc = process_manager.get_process(pid);
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;

    // Randomize instruction count between [min_ins, max_ins]
    std::uniform_int_distribution<uint64_t> dist(config.min_ins, config.max_ins);
    uint64_t num_instructions = dist(rng);

    // Fill with dummy PrintInstructions (lightweight placeholder instructions)
    for (uint64_t i = 0; i < num_instructions; ++i) {
        proc->add_instruction(
            std::make_unique<PrintInstruction>("Hello world from " + name)
        );
    }

    scheduler.add_process(proc);
}


void ProcessGenerator::tick() {
    if (!generating_.load()) {
        return;
    }

    ticks_since_last_generate++;

    if (ticks_since_last_generate >= config.batch_process_freq) {
        generate_one_process();
        ticks_since_last_generate = 0;
    }
}

void ProcessGenerator::start() {
    if (generating_.load()) {
        std::cout << "Process generation is already running.\n";
        return;
    }
    generating_.store(true);
    ticks_since_last_generate = 0;
    std::cout << "Process generation started (every "
              << config.batch_process_freq << " CPU cycle(s)).\n";
}

void ProcessGenerator::stop() {
    if (!generating_.load()) {
        std::cout << "Process generation is not running.\n";
        return;
    }
    generating_.store(false);
    std::cout << "Process generation stopped. "
              << (next_process_number - 1) << " process(es) were generated.\n";
}

bool ProcessGenerator::is_generating() const {
    return generating_.load();
}
