#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <random>
#include "os_process.h"
#include "config.h"

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
    // Acts as a blocking "screen", separated from UI.
    void view_process(std::string process_name);
};

class Scheduler;

// Generates dummy processes at a configurable frequency, driven by CPU cycle ticks.
// - scheduler-start sets generating_ = true
// - scheduler-stop sets generating_ = false
// - tick() is called once per CPU cycle from the kernel main loop
class ProcessGenerator {
private:
    ProcessManager& process_manager;
    Scheduler& scheduler;
    const Config& config;

    std::atomic<bool> generating_{false};
    int next_process_number = 1;          // sequential counter for p01, p02, ...
    uint64_t ticks_since_last_generate = 0;

    std::mt19937 rng{std::random_device{}()};

    // Builds a sequential name like "p01", "p02", ..., "p1240"
    std::string make_process_name(int number) const;

public:
    ProcessGenerator(ProcessManager& pm, Scheduler& sched, const Config& cfg);

    // Creates a single dummy process with randomized PrintInstruction count
    Process* generate_one_process(std::string name);

    // Called by the kernel once per CPU cycle
    void tick();

    // Called by scheduler-start command
    void start();

    // Called by scheduler-stop command
    void stop();

    bool is_generating() const;
};

class ReportGenerator {
private:
    ProcessManager& process_manager;

public:
    ReportGenerator(ProcessManager& pm);

    // Generates .txt file for the active and finished processes
    void generate_report(const std::string& filename = "csopes_report.txt");
};

Process* test_deep_for_loops(ProcessManager& pm, const std::string& name);
Process* test_for_loop(ProcessManager& pm, const std::string& name);
Process* test_nested_for_loops(ProcessManager& pm, const std::string& name);
Process* create_dummy_test_process(ProcessManager& pm, const std::string& name);
