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

std::string random_var(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 15);
    return "var_" + std::to_string(dist(rng));
}

Operand random_operand(std::mt19937& rng)
{
    std::uniform_int_distribution<int> coin(0, 1);
    std::uniform_int_distribution<int> val(0, 100);

    Operand op;

    if (coin(rng))
    {
        op.isVariable = true;
        op.variable = random_var(rng);
    }
    else
    {
        op.isVariable = false;
        op.immediate = val(rng);
    }

    return op;
}

// helper function to create a random instruction, used in generate_one_process
std::unique_ptr<Instruction> create_random_instruction(std::mt19937& rng, const std::string& process_name, int current_depth) {
    int max_type = (current_depth >= 3) ? 5 : 6; 
    std::uniform_int_distribution<int> type_dist(1, max_type);
    int type = type_dist(rng);

    std::uniform_int_distribution<int> val_dist(0, 65535); // Max uint16_t limit
    std::uniform_int_distribution<int> reg_dist(0, 9);
    std::uniform_int_distribution<int> sleep_dist(1, 5);


    switch (type) {
        case 1: //print
        // {
        //     std::string var = random_var(rng);

        //     return std::make_unique<PrintInstruction>(
        //         "Value of " + var + ": ",
        //         var
        //     );
        // }
            return std::make_unique<PrintInstruction>("Hello world from " + process_name + "!");
        case 2: //declare
            return std::make_unique<DeclareInstruction>("var_" + std::to_string(reg_dist(rng)), val_dist(rng));
        case 3: //add
        {
            std::string dest = random_var(rng);

            return std::make_unique<AddInstruction>(
                dest,
                random_operand(rng),
                random_operand(rng)
            );
        }
        case 4: //subtract
        {
            std::string dest = random_var(rng);

            return std::make_unique<SubtractInstruction>(
                dest,
                random_operand(rng),
                random_operand(rng)
            );
        } 
        case 5: //sleep
            return std::make_unique<SleepInstruction>(sleep_dist(rng));
        case 6://for loop
        {
            std::vector<std::unique_ptr<Instruction>> loop_body;
            std::uniform_int_distribution<int> loop_len_dist(1, 3);
            int loop_len = loop_len_dist(rng);
            for (int i = 0; i < loop_len; ++i) {
                loop_body.push_back(create_random_instruction(rng, process_name, current_depth + 1));
            }
            std::uniform_int_distribution<int> repeat_dist(2, 5);
            return std::make_unique<ForInstruction>(std::move(loop_body), repeat_dist(rng));
        }
        default: 
            return std::make_unique<PrintInstruction>("Hello world from " + process_name + "!");
    }
}

Process* ProcessGenerator::generate_one_process(std::string name) {

    int pid = process_manager.create_process(name);
    Process* proc = process_manager.get_process(pid);
    proc->process_name = name;
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;

    // Randomize instruction count between [min_ins, max_ins]
    std::uniform_int_distribution<uint64_t> dist(config.min_ins, config.max_ins);
    uint64_t num_instructions = dist(rng);

    // Fill with dummy PrintInstructions (lightweight placeholder instructions)
    //for (uint64_t i = 0; i < num_instructions; ++i) {
    //    proc->add_instruction(
    //        std::make_unique<PrintInstruction>("Hello world from " + name)
    //    );
    //}

    //UPDATED Loop to generate random instructions of any type
    for (uint64_t i = 0; i < num_instructions; ++i) {
        proc->add_instruction(create_random_instruction(this->rng, name, 0));
    }

    scheduler.add_process(proc);

    return proc;
}


void ProcessGenerator::tick() {
    if (!generating_.load()) {
        return;
    }
    
    ticks_since_last_generate++;
    
    if (ticks_since_last_generate >= config.batch_process_freq) {
        std::string name = make_process_name(next_process_number++);
        generate_one_process(name);
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
