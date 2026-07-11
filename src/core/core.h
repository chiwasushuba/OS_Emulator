#pragma once
#include <string>
#include <vector>
#include "os_process.h"

class ProcessViewer {
private:
    ProcessManager& process_manager;
    ProcessLogger& logger;

public:
    ProcessViewer(ProcessManager& pm, ProcessLogger& log)
        : process_manager(pm), logger(log)
    {}

    void list_processes();
    void print_log(int pid);
};

Process* test_deep_for_loops(ProcessManager& pm, const std::string& name);
Process* test_for_loop(ProcessManager& pm, const std::string& name);
Process* test_nested_for_loops(ProcessManager& pm, const std::string& name);
Process* create_dummy_test_process(ProcessManager& pm, const std::string& name);