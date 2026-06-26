#pragma once
#include <atomic>
#include <thread>
#include "cpu.h"
#include "core.h"
#include "console.h"
#include "command_handler.h"
#include "commands.h"
#include "scheduler.h"

class Kernel {
    private:
        std::atomic<bool> is_running;
        std::atomic<bool> is_initialized;
        void main_loop();
        std::unique_ptr<CPUManager> cpu_manager = nullptr;
        ProcessLogger process_logger;
        ProcessManager process_manager;
        std::unique_ptr<Scheduler> scheduler = nullptr;
        std::thread clock_thread;

        Config config;                                     // loaded from config.txt
        std::unique_ptr<ProcessGenerator> process_generator = nullptr;
    public:
        Kernel();
        void start();
        void shutdown();
        void handle_logging(const LogEntry& log);
        
        // Initialization functions
        void initialize_subsystems();
        void log_error_not_initialized() const {
            std::cout << "Error: You must run 'initialize' before executing this command.\n";
        }
        bool get_initialized_status() const { return is_initialized.load(); }
        
        // Called by scheduler-start / scheduler-stop CLI commands
        void start_scheduler();
        void stop_scheduler();

        //Triggered by the report-util command 
        void generate_report_file();
};
