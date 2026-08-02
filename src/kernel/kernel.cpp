#include <iostream>
#include <iomanip>
#include "config.h"
#include "kernel.h"
#include "core.h"
#include "os_process.h"
#include "instruction_parser.h"
#include "constants.h"
#include "paging_allocator.h"

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

        // Tick the process generator (scheduler-start/stop driven) BEFORE the
        // scheduler assigns cores. Otherwise a process created this tick sits
        // in the ready queue and only gets picked up next tick - a 1-tick
        // admission lag that compounds across every process generated.
        if (process_generator) {
            process_generator->tick();
        }

        // Tick the scheduler to manage queue and assign processes
        if (scheduler) {
            scheduler->tick();
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
    if (!memory_allocator || config.quantum_cycles == 0 || !scheduler_ever_started) {
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

    // Initialize memory manager
	this->memory_allocator = std::make_unique<PagingAllocator>(config.max_overall_mem, config.mem_per_frame);

    // Initialize scheduler
    if (this->config.scheduler == "rr") {
        this->scheduler = std::make_unique<RoundRobinScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator, this->config.quantum_cycles);
    } else if (this->config.scheduler == "fcfs") {
        this->scheduler = std::make_unique<FCFSScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator);
    } else {
        // Fallback if config has an invalid name
        std::cout << "Warning: Unknown scheduler '" << this->config.scheduler << "' in config.txt. Defaulting to FCFS.\n";
        this->scheduler = std::make_unique<FCFSScheduler>(*this->cpu_manager, this->process_manager, *this->memory_allocator);
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
    std::cout << "min_mem_per_proc:\t" << this->config.min_mem_per_proc << "\n";
    std::cout << "max_mem_per_proc:\t" << this->config.max_mem_per_proc << "\n";
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
            if (process_generator) {
                process_generator->start();
                scheduler_ever_started = true;
            }
            break;

        case CommandType::STOP_SCHEDULER:
            if (process_generator) process_generator->stop();
            break;

        case CommandType::REPORT:
            this->generate_report_file();
            break;

        case CommandType::SCREEN:
            // Delegate smoothly to internal screen logic using the sub-action enum
            this->execute_screen_subsystem(packet);
            break;

        case CommandType::PROCESS_SMI:
            this->show_process_smi();
            break;

        case CommandType::VMSTAT:
            this->show_vmstat();
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

void Kernel::execute_screen_subsystem(const CommandPacket& packet) {
    ScreenAction action = packet.screen_action;
    const std::string& payload = packet.payload;
    
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
                // If explicit mem_size was provided, override the random one
                if (packet.mem_size > 0) {
                    created_process->mem_size = packet.mem_size;
                }
                created_process->set_page_size(memory_allocator ? memory_allocator->get_page_size() : 0);
                created_process->set_page_fault_handler([this, created_process](size_t page_number, bool for_write) {
                    return this->memory_allocator ? this->memory_allocator->ensure_page_resident(created_process->id, page_number, for_write) : false;
                });
                created_process->init_memory();
                viewer.view_process(created_process->process_name);
            }
            break;
        case ScreenAction::CREATE_CUSTOM:
            {
                // Parse instructions
                auto instructions = parse_instructions(packet.raw_instructions);
                
                // Validate count (1-50)
                if (instructions.empty() || instructions.size() > 50) {
                    std::cout << "invalid command\n";
                    break;
                }
                
                // Create process
                int pid = process_manager.create_process(payload);
                Process* proc = process_manager.get_process(pid);
                if (!proc) {
                    std::cout << "Error creating process.\n";
                    break;
                }
                proc->process_name = payload;
                proc->mem_size = packet.mem_size;
                proc->set_page_size(memory_allocator ? memory_allocator->get_page_size() : 0);
                proc->set_page_fault_handler([this, pid](size_t page_number, bool for_write) {
                    return this->memory_allocator ? this->memory_allocator->ensure_page_resident(pid, page_number, for_write) : false;
                });
                proc->init_memory();
                proc->state = ProcessState::READY;
                proc->current_instruction = 0;
                
                // Add parsed instructions
                for (auto& inst : instructions) {
                    proc->add_instruction(std::move(inst));
                }
                
                // Add to scheduler
                scheduler->add_process(proc);
                viewer.view_process(proc->process_name);
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
        scheduler_ever_started = true;
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

void Kernel::show_process_smi() {
    int total_cores = config.num_cpu;
    int busy_cores = cpu_manager ? cpu_manager->get_cores_used() : 0;
    int cpu_util = (total_cores > 0) ? (busy_cores * 100 / total_cores) : 0;

    size_t used = memory_allocator ? memory_allocator->get_allocated_size() : 0;
    size_t total = memory_allocator ? memory_allocator->get_maximum_size() : 0;
    int mem_util = (total > 0) ? static_cast<int>(used * 100 / total) : 0;

    auto to_mib = [](size_t bytes) {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    };

    std::cout << "-----------------------------------------------\n";
    std::cout << "| PROCESS-SMI V01.00 Driver Version: 01.00    |\n";
    std::cout << "-----------------------------------------------\n";
    std::cout << "CPU-Util: " << cpu_util << "%\n";
    std::cout << "Memory Usage: " << std::fixed << std::setprecision(2) << to_mib(used)
              << "MiB / " << to_mib(total) << "MiB\n";
    std::cout << "Memory Util: " << mem_util << "%\n\n";

    std::cout << "===============================================\n";
    std::cout << "Running processes and memory usage:\n";
    std::cout << "-----------------------------------------------\n";

    auto active_pids = process_manager.get_active_pids();
    if (active_pids.empty()) {
        std::cout << "(none)\n";
    } else {
        for (int pid : active_pids) {
            Process* p = process_manager.get_process(pid);
            if (p) {
                std::cout << p->process_name << "    " << std::fixed << std::setprecision(2)
                          << to_mib(p->mem_size) << "MiB\n";
            }
        }
    }

    std::cout << "-----------------------------------------------\n";
}

void Kernel::show_vmstat() {
    size_t total = memory_allocator ? memory_allocator->get_maximum_size() : 0;
    size_t used = memory_allocator ? memory_allocator->get_allocated_size() : 0;
    size_t free_mem = total - used;

    auto to_kib = [](size_t bytes) {
        return bytes / 1024;
    };

    // Active = memory used by RUNNING processes
    // Inactive = memory used by non-RUNNING (READY/WAITING) processes
    size_t active = 0, inactive = 0;
    for (int pid : process_manager.get_all_pids()) {
        Process* p = process_manager.get_process(pid);
        if (!p) continue;
        if (p->state == ProcessState::RUNNING) {
            active += p->mem_size;
        } else if (p->state != ProcessState::FINISHED && p->state != ProcessState::TERMINATED) {
            inactive += p->mem_size;
        }
    }

    size_t paged_in = memory_allocator ? memory_allocator->get_num_paged_in() : 0;
    size_t paged_out = memory_allocator ? memory_allocator->get_num_paged_out() : 0;

    // CPU ticks
    uint64_t idle_ticks = 0, active_ticks = 0, total_ticks = 0;
    if (cpu_manager) {
        auto& cores = cpu_manager->get_cores();
        for (const auto& core : cores) {
            double util = core.get_utilization();
            // Approximate from utilization percentage (not perfectly precise but the
            // spec only asks for the general shape of vmstat -s output)
            total_ticks += 100;
            active_ticks += static_cast<uint64_t>(util);
            idle_ticks += static_cast<uint64_t>(100 - util);
        }
    }

    std::cout << to_kib(total)      << " K total memory\n";
    std::cout << to_kib(used)       << " K used memory\n";
    std::cout << to_kib(active)     << " K active memory\n";
    std::cout << to_kib(inactive)   << " K inactive memory\n";
    std::cout << to_kib(free_mem)   << " K free memory\n";
    std::cout << idle_ticks  << " idle cpu ticks\n";
    std::cout << active_ticks << " active cpu ticks\n";
    std::cout << total_ticks << " total cpu ticks\n";
    std::cout << paged_in   << " pages paged in\n";
    std::cout << paged_out  << " pages paged out\n";
}
