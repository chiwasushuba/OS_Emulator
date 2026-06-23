#include <thread>
#include <iostream>
#include "config.h"
#include "kernel.h"

Kernel::Kernel() {
    this->is_running = false;
}

void Kernel::main_loop() {
    while(this->is_running) {
        // increment tick for cpu and scheduler
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

    std::cout << "num_cpu:\t\t" << config.num_cpu << "\n";
    std::cout << "scheduler:\t\t" << config.scheduler << "\n";
    std::cout << "quantum_cycles:\t\t" << config.quantum_cycles << "\n";
    std::cout << "batch_process_freq:\t" << config.batch_process_freq << "\n";
    std::cout << "min_ins:\t\t" << config.min_ins << "\n";
    std::cout << "max_ins:\t\t" << config.max_ins << "\n";
    std::cout << "delays_per_exec:\t" << config.delays_per_exec << "\n";
    
    std::thread clock_thread(&Kernel::main_loop, this);

    // Initialize blocking cli

    clock_thread.join();
    std::cout << "Goodbye, World!";
}