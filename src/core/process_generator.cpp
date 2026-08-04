// Process generators: test helpers and the ProcessGenerator class for scheduler-start/stop

#include <memory>
#include <string>
#include <cmath>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "os_process.h"
#include "core.h"
#include "scheduler.h"

// ============================================================================
// ProcessGenerator — drives scheduler-start / scheduler-stop
// ============================================================================


ProcessGenerator::ProcessGenerator(ProcessManager& pm, Scheduler& sched, const Config& cfg, IMemoryAllocator& alloc)
    : process_manager(pm), scheduler(sched), config(cfg), memory_allocator(alloc)
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
std::unique_ptr<Instruction> create_random_instruction(std::mt19937& rng, const std::string& process_name, int current_depth, size_t mem_size) {
    // MCO2 requires READ/WRITE to appear in scheduler-generated processes - they are
    // what drives page faults, eviction, and backing-store traffic. Types 7 and 8 are
    // allowed at nesting depth too, so loops generate repeated memory pressure.
    int max_type = (current_depth >= 1) ? 5 : 6;
    if (mem_size >= sizeof(uint16_t)) max_type = 8;
    std::uniform_int_distribution<int> type_dist(1, max_type);
    int type = type_dist(rng);
    // A FOR loop can only appear at the top level; re-roll if depth forbids it.
    if (type == 6 && current_depth >= 1) type = 1;

    std::uniform_int_distribution<int> val_dist(0, 65535); // Max uint16_t limit
    std::uniform_int_distribution<int> reg_dist(0, 9);
    std::uniform_int_distribution<int> sleep_dist(1, 5);

    // Pick a 2-byte-aligned address inside the process's own address space, so
    // generated processes exercise paging without tripping an access violation.
    auto random_address = [&]() -> uint32_t {
        size_t max_slot = (mem_size / sizeof(uint16_t)) - 1;
        std::uniform_int_distribution<size_t> addr_dist(0, max_slot);
        return static_cast<uint32_t>(addr_dist(rng) * sizeof(uint16_t));
    };


    switch (type) {
        case 1: 
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
                loop_body.push_back(create_random_instruction(rng, process_name, current_depth + 1, mem_size));
            }
            std::uniform_int_distribution<int> repeat_dist(2, 5);
            return std::make_unique<ForInstruction>(std::move(loop_body), repeat_dist(rng));
        }
        case 7: //read
        {
            return std::make_unique<ReadInstruction>(random_var(rng), random_address());
        }
        case 8: //write
        {
            return std::make_unique<WriteInstruction>(
                random_address(), Operand::Imm(static_cast<uint16_t>(val_dist(rng))));
        }
        default:
            return std::make_unique<PrintInstruction>("Hello world from " + process_name + "!");
    }
}

Process* ProcessGenerator::generate_one_process(std::string name, size_t mem_size_override) {

    int pid = process_manager.create_process(name);
    Process* proc = process_manager.get_process(pid);
    proc->process_name = name;
    proc->state = ProcessState::READY;
    proc->current_instruction = 0;
    proc->set_page_size(config.mem_per_frame);
    // Same wiring screen -s / screen -c do, so scheduler-generated processes
    // demand-page through the allocator instead of bypassing it.
    proc->set_page_fault_handler([this, pid](size_t page_number, bool for_write) {
        return this->memory_allocator.ensure_page_resident(pid, page_number, for_write);
    });
    // Process data lives in the allocator's frames; these translate through
    // this pid's page table to reach it.
    proc->set_memory_handlers(
        [this, pid](size_t vaddr, uint16_t& out) {
            return this->memory_allocator.read_memory(pid, vaddr, out);
        },
        [this, pid](size_t vaddr, uint16_t value) {
            return this->memory_allocator.write_memory(pid, vaddr, value);
        });

    // Fix the memory size BEFORE generating instructions - create_random_instruction
    // draws READ/WRITE addresses from inside proc->mem_size.
    if (mem_size_override > 0) {
        proc->mem_size = mem_size_override;
    } else {
        // Pick a random power-of-2 between min and max
        int min_exp = static_cast<int>(std::log2(config.min_mem_per_proc));
        int max_exp = static_cast<int>(std::log2(config.max_mem_per_proc));
        std::uniform_int_distribution<int> mem_dist(min_exp, max_exp);
        proc->mem_size = static_cast<size_t>(1) << mem_dist(rng);
    }

    // Randomize instruction count between [min_ins, max_ins]
    std::uniform_int_distribution<uint64_t> dist(config.min_ins, config.max_ins);
    uint64_t num_instructions = dist(rng);

    //UPDATED Loop to generate random instructions of any type
    for (uint64_t i = 0; i < num_instructions; ++i) {
        proc->add_instruction(create_random_instruction(this->rng, name, 0, proc->mem_size));
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
