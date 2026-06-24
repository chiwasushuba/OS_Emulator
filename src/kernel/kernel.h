#pragma once
#include "cpu.h"

class Kernel {
    private:
        bool is_running;
        void main_loop();
        std::unique_ptr<CPUManager> cpu_manager = nullptr;
        ProcessLogger process_logger;
    public:
        Kernel();
        void start();
        void handle_logging(const LogEntry& log);
};