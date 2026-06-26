#pragma once
#include <atomic>
#include "cpu.h"
#include "core.h"
#include "console.h"
#include "command_handler.h"
#include "commands.h"
#include "scheduler.h"

class Kernel {
    private:
        std::atomic<bool> is_running;
        void main_loop();
        std::unique_ptr<CPUManager> cpu_manager = nullptr;
        ProcessLogger process_logger;
        ProcessManager process_manager;
        std::unique_ptr<Scheduler> scheduler = nullptr;

        Config config;                                     // loaded from config.txt
        std::unique_ptr<ProcessGenerator> process_generator = nullptr;
    public:
        Kernel();
        void start();
        void shutdown();
        void handle_logging(const LogEntry& log);

        // Called by scheduler-start / scheduler-stop CLI commands
        void start_scheduler();
        void stop_scheduler();

        //Triggered by the report-util command 
        void generate_report_file();
};
