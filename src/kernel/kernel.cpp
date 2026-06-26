#include <thread>
#include <iostream>
#include "config.h"
#include "kernel.h"
#include "core.h"

Kernel::Kernel() {
    this->is_running.store(false);
}

void Kernel::handle_logging(const LogEntry& log) {
    this->process_logger.append(log.pid, log);
    // std::cout << "\n------------------HANDLE LOGGING------------------\n";
    // std::cout << "log.pid: " << log.pid << "\n";
    // std::cout << "log.core_id: " << log.core_id << "\n";
    // std::cout << "log.current_instruction: " << log.current_instruction << "\n";
    // std::cout << "log.message: " << log.message << "\n";
    // std::cout << "log.process_name: " << log.process_name << "\n";
    // std::cout << "log.timestamp: " << log.timestamp << "\n";
    // std::cout << "log.total_instructions: " << log.total_instructions << "\n";
}

void Kernel::main_loop() {
    while(this->is_running.load()) {
        // increment tick for cpu
        cpu_manager->tick([this](const LogEntry& log) {
            this->handle_logging(log);
        });

        // Tick the scheduler to manage queue and assign processes
        if (scheduler) {
            scheduler->tick();
        }

        // Tick the process generator (scheduler-start/stop driven)
        if (process_generator) {
            process_generator->tick();
        }
    }
}

void Kernel::start() {
    this->is_running.store(true);

    // Load config variables into member
    loadConfig("../../config.txt", this->config);

    // Initialize cpu manager
    this->cpu_manager = std::make_unique<CPUManager>(config.num_cpu, config.delays_per_exec);

    // Initialize scheduler
    if (this->config.scheduler == "rr") {
        this->scheduler = std::make_unique<RoundRobinScheduler>(*this->cpu_manager, this->process_manager, this->config.quantum_cycles);
    } else {
        // Fallback or other implementations
        this->scheduler = std::make_unique<RoundRobinScheduler>(*this->cpu_manager, this->process_manager, this->config.quantum_cycles);
    }

    // Initialize the process generator (controlled by scheduler-start / scheduler-stop)
    this->process_generator = std::make_unique<ProcessGenerator>(this->process_manager, *this->scheduler, this->config);

    std::cout << "num_cpu:\t\t" << this->config.num_cpu << "\n";
    std::cout << "scheduler:\t\t" << this->config.scheduler << "\n";
    std::cout << "quantum_cycles:\t\t" << this->config.quantum_cycles << "\n";
    std::cout << "batch_process_freq:\t" << this->config.batch_process_freq << "\n";
    std::cout << "min_ins:\t\t" << this->config.min_ins << "\n";
    std::cout << "max_ins:\t\t" << this->config.max_ins << "\n";
    std::cout << "delays_per_exec:\t" << this->config.delays_per_exec << "\n";
    std::cout << "\n";

    // Start the CPU clock on a background thread
    std::thread clock_thread(&Kernel::main_loop, this);

    // Build the CLI chain: Commands -> CommandHandler -> Console
    Commands commands(this);
    CommandHandler handler(&commands);
    Console console(&handler);
    commands.setConsole(&console);

    // Run the blocking CLI on the main thread
    console.run();

    // CLI exited — wait for the clock thread to finish
    clock_thread.join();
    std::cout << "Goodbye!\n";
}

void Kernel::shutdown() {
    this->is_running.store(false);
}

void Kernel::start_scheduler() {
    if (process_generator) {
        process_generator->start();
    }
}

void Kernel::stop_scheduler() {
    if (process_generator) {
        process_generator->stop();
    }
}

void Kernel::generate_report_file() {
    // Instantiate the ReportGenerator with access to core manager
    ReportGenerator reporter(this->process_manager);

    // Call file generator implementation
    reporter.generate_report("csopes_report.txt");
}
