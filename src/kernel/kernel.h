#pragma once
#include <atomic>
#include <thread>
#include "cpu.h"
#include "core.h"
#include "console.h"
#include "command_handler.h"
#include "scheduler.h"

enum class CommandType {
    INITIALIZE,
    START_SCHEDULER,
    STOP_SCHEDULER,
    SCREEN,
    REPORT,
    EXIT,
    UNKNOWN
};

enum class ScreenAction {
    LIST,
    CREATE,
    READ,
    NONE
};

struct CommandPacket {
    CommandType type = CommandType::UNKNOWN;
    ScreenAction screen_action = ScreenAction::NONE;
    std::string payload = "";   // for screen command primarily
};

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
        
        // Command implementations
        void initialize_subsystems();
        void start_scheduler();
        void stop_scheduler();
        void generate_report_file();
        void execute_screen_subsystem(const ScreenAction action, const std::string& payload);

    public:
        Kernel();
        void start();
        void shutdown();
        void handle_logging(const LogEntry& log);
        void handle_command(const CommandPacket& packet);
        
        // Initialization functions
        void log_error_not_initialized() const {
            std::cout << "Error: You must run 'initialize' before executing this command.\n";
        }
        bool get_initialized_status() const { return is_initialized.load(); }
};
