#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <random>
#include "os_process.h"
#include "config.h"
#include "cpu.h"
#include "memory_allocator.h"

class ProcessViewer {
private:
    ProcessManager& process_manager;
    ProcessLogger& logger;
    CPUManager& cpu_manager;

public:
    ProcessViewer(ProcessManager& pm, ProcessLogger& log, CPUManager& cpu_m)
        : process_manager(pm), logger(log), cpu_manager(cpu_m)
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
    // Needed so generated processes get a page-fault handler, exactly like the
    // ones created by screen -s / screen -c. Without it their READ/WRITE
    // instructions can never resolve a page.
    IMemoryAllocator& memory_allocator;

    std::atomic<bool> generating_{false};
    int next_process_number = 1;          // sequential counter for p01, p02, ...
    uint64_t ticks_since_last_generate = 0;

    std::mt19937 rng{std::random_device{}()};

    // generate_one_process() is called from the kernel clock thread (tick) AND
    // from the CLI thread (screen -s / screen -c). Without this, both threads
    // mutate rng and next_process_number concurrently, which can hand out the
    // same process name twice and is undefined behaviour on the generator state.
    std::mutex gen_mutex;

    // Builds a sequential name like "p01", "p02", ..., "p1240"
    std::string make_process_name(int number) const;

public:
    ProcessGenerator(ProcessManager& pm, Scheduler& sched, const Config& cfg, IMemoryAllocator& alloc);

    // Creates a single dummy process with a randomized instruction mix.
    // mem_size_override != 0 pins the process's memory size (screen -s <name> <size>);
    // 0 rolls a random power of 2 in [min-mem-per-proc, max-mem-per-proc].
    // The size MUST be known before instructions are generated - READ/WRITE
    // addresses are drawn from inside the process's own address space, so
    // resizing afterwards would turn every generated access into a violation.
    Process* generate_one_process(std::string name, size_t mem_size_override = 0);

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
    CPUManager& cpu_manager;

public:
    ReportGenerator(ProcessManager& pm, CPUManager& cpu_m);

    // Generates .txt file for the active and finished processes
    void generate_report(const std::string& filename = "csopes_report.txt");
};

// helper functions
void format_active_entry(std::stringstream& ss, Process* process);  // depracated

void format_finished_entry(std::stringstream& ss, Process* process);

void format_snapshot_entry(std::stringstream& ss, const ProcessSnapshot& snapshot);