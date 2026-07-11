#include <thread>
#include <iostream>
#include "config.h"
#include "kernel.h"
#include "core.h"
#include "os_process.h"
#include "constants.h"

Kernel::Kernel() {
    this->is_running.store(false);
    this->is_initialized.store(false);
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

        // Every quantum-cycles ticks, snapshot memory to memory_stamp_<qq>.txt
        take_memory_snapshot_if_due();

        // Pace the loop - without this, 1 "cycle" is microseconds, which makes
        // quantum-cycles, batch-process-freq, and memory snapshots fire far
        // too fast to be meaningful (or observable) in a live demo.
        std::this_thread::sleep_for(Timing::TICK_DURATION);
    }
}

void Kernel::take_memory_snapshot_if_due() {
    if (!memory_allocator || config.quantum_cycles == 0) {
        return;
    }

    global_tick_counter++;

    if (global_tick_counter % config.quantum_cycles == 0) {
        quantum_cycle_counter++;
        memory_allocator->generate_memory_stamp(quantum_cycle_counter, "../../");
    }
}

void Kernel::initialize_subsystems() {
    if (this->is_initialized.load()) {
        std::cout << "Error: System has already been initialized.\n";
        return;
    }

    std::cout << "Initializing system subsystems...\n\n";
    // Initialize cpu manager
    this->cpu_manager = std::make_unique<CPUManager>(config.num_cpu, config.delays_per_exec);

    // Initialize memory manager (first-fit flat allocator)
    this->memory_allocator = std::make_unique<FirstFitAllocator>(config.max_overall_mem);

    // Initialize scheduler
    if (this->config.scheduler == "rr") {
        this->scheduler = std::make_unique<RoundRobinScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator, this->config.mem_per_proc, this->config.quantum_cycles);
    } else if (this->config.scheduler == "fcfs") {
        this->scheduler = std::make_unique<FCFSScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator, this->config.mem_per_proc);
    } else {
        // Fallback if config has an invalid name
        std::cout << "Warning: Unknown scheduler '" << this->config.scheduler << "' in config.txt. Defaulting to FCFS.\n";
        this->scheduler = std::make_unique<FCFSScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator, this->config.mem_per_proc);
    }

    // Initialize the process generator (controlled by scheduler-start / scheduler-stop)
    this->process_generator = std::make_unique<ProcessGenerator>(this->process_manager, *this->scheduler, this->config);

    std::cout << "---config loaded with the following values---\n";
    std::cout << "num_cpu:\t\t" << this->config.num_cpu << "\n";
    std::cout << "scheduler:\t\t" << this->config.scheduler << "\n";
    std::cout << "quantum_cycles:\t\t" << this->config.quantum_cycles << "\n";
    std::cout << "batch_process_freq:\t" << this->config.batch_process_freq << "\n";
    std::cout << "min_ins:\t\t" << this->config.min_ins << "\n";
    std::cout << "max_ins:\t\t" << this->config.max_ins << "\n";
    std::cout << "delays_per_exec:\t" << this->config.delays_per_exec << "\n";
    std::cout << "max_overall_mem:\t" << this->config.max_overall_mem << "\n";
    std::cout << "mem_per_frame:\t\t" << this->config.mem_per_frame << "\n";
    std::cout << "mem_per_proc:\t\t" << this->config.mem_per_proc << "\n";
    std::cout << "\n";

    this->is_initialized.store(true);
    // Start the CPU clock on a background thread
    this->clock_thread = std::thread(&Kernel::main_loop, this);
}

void Kernel::start() {
    this->is_running.store(true);

    // Load config variables into member
    loadConfig("../../config.txt", this->config);

    // Build the CLI chain: CommandHandler -> Console
    CommandHandler handler(this);
    this->console = std::make_unique<Console>(&handler);

    // Run the blocking CLI on the main thread
    this->console->run();

    // CLI exited — wait for the clock thread to finish
    if (this->clock_thread.joinable()) {
        this->clock_thread.join();
    }
}

void Kernel::handle_command(const CommandPacket& packet) {
    if (packet.type != CommandType::INITIALIZE && packet.type != CommandType::EXIT) {
        if (!is_initialized.load()) {
            log_error_not_initialized();
            return;
        }
    }

    switch (packet.type) {
        case CommandType::INITIALIZE:
            this->initialize_subsystems();
            break;

        case CommandType::START_SCHEDULER:
            if (process_generator) process_generator->start();
            break;

        case CommandType::STOP_SCHEDULER:
            if (process_generator) process_generator->stop();
            break;

        case CommandType::REPORT:
            this->generate_report_file();
            break;

        case CommandType::SCREEN:
            // Delegate smoothly to internal screen logic using the sub-action enum
            this->execute_screen_subsystem(packet.screen_action, packet.payload);
            break;

        case CommandType::EXIT:
            this->shutdown();
            break;

        case CommandType::UNKNOWN:
        default:
            std::cout << "Error: Kernel received unhandled or invalid command packet.\n";
            break;
    }
}

void Kernel::execute_screen_subsystem(ScreenAction action, const std::string& payload) {
    if (action == ScreenAction::NONE) {
        return;
    }
    if (action != ScreenAction::NONE && action != ScreenAction::LIST) {
        this->console->clearScreen();
    }
    ProcessViewer viewer(this->process_manager, this->process_logger, *(this->cpu_manager));
    switch (action) {
        case ScreenAction::LIST:
            viewer.list_processes();
            break;
        case ScreenAction::CREATE:
            {
                Process* created_process = process_generator.get()->generate_one_process(payload);
                viewer.view_process(created_process->process_name);
            }
            break;
        case ScreenAction::READ:
            viewer.view_process(payload);
            break;
        default: std::cout << "Error: Invalid screen action packet.\n"; break;
    }
}
void Kernel::shutdown() {
    this->is_running.store(false);
}

void Kernel::start_scheduler() {
    if (!is_initialized.load()) {   // Technically redundant since CLI handles it, but just to be safe...
        log_error_not_initialized(); 
        return;
    }
    if (process_generator) {
        process_generator->start();
    }
}

void Kernel::stop_scheduler() {
    if (!is_initialized.load()) { 
        log_error_not_initialized(); 
        return;
    }
    if (process_generator) {
        process_generator->stop();
    }
}

void Kernel::generate_report_file() {
    if (!is_initialized.load()) { 
        log_error_not_initialized(); 
        return;
    }
    // Instantiate the ReportGenerator with access to core manager
    ReportGenerator reporter(this->process_manager, *(this->cpu_manager));

    // Call file generator implementation
    reporter.generate_report("../../csopesy_report.txt");
}
