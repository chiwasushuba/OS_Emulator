#include <thread>
#include <iostream>
#include "config.h"
#include "kernel.h"
#include "core.h"

Kernel::Kernel() {
    this->is_running = false;
}

void Kernel::handle_logging(const LogEntry& log) {
    this->process_logger.append(log.pid, log);
    std::cout << "log.pid: " << log.pid << "\n";
    std::cout << "log.core_id: " << log.core_id << "\n";
    std::cout << "log.current_instruction: " << log.current_instruction << "\n";
    std::cout << "log.message: " << log.message << "\n";
    std::cout << "log.process_name: " << log.process_name << "\n";
    std::cout << "log.timestamp: " << log.timestamp << "\n";
    std::cout << "log.total_instructions: " << log.total_instructions << "\n";
}

void Kernel::main_loop() {
    int curr_cycle = 0;
    while(this->is_running) {
        // increment tick for cpu and scheduler
        cpu_manager->tick([this](const LogEntry& log) {
            this->handle_logging(log);
        });
        
        curr_cycle++;
        std::cout << "Tick " << curr_cycle << "\n";
        if (curr_cycle > 50) {
            ProcessViewer view = ProcessViewer(this->process_manager, this->process_logger);
            view.list_processes();
            view.print_log(1);
            this->is_running = false;
        }
    }
}

void Kernel::start() {
    std::cout << "Hello, World!\n";
    this->is_running = true;

    // Load config variables
    Config config;
    loadConfig("../../config.txt", config);
    // Pass config variables to:
    // initialize scheduler
    // initialize cpu manager
    this->cpu_manager = std::make_unique<CPUManager>(config.num_cpu, config.delays_per_exec);

    std::cout << "num_cpu:\t\t" << config.num_cpu << "\n";
    std::cout << "scheduler:\t\t" << config.scheduler << "\n";
    std::cout << "quantum_cycles:\t\t" << config.quantum_cycles << "\n";
    std::cout << "batch_process_freq:\t" << config.batch_process_freq << "\n";
    std::cout << "min_ins:\t\t" << config.min_ins << "\n";
    std::cout << "max_ins:\t\t" << config.max_ins << "\n";
    std::cout << "delays_per_exec:\t" << config.delays_per_exec << "\n";
    
    Process* test_job = create_dummy_test_process(this->process_manager, "test_process");

    // Grab core 0 by reference and assign the process to it
    if (!cpu_manager->get_cores().empty()) {
        cpu_manager->get_cores()[0].assign_process(test_job);
        std::cout << "Successfully assigned " << test_job->process_name 
                  << " (PID: " << test_job->id << ") to Core 0!\n";
    }

    std::thread clock_thread(&Kernel::main_loop, this);

    // Initialize blocking cli

    clock_thread.join();
    std::cout << "Goodbye, World!";
}