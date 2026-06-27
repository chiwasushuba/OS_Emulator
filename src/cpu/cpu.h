#pragma once
#include <cstdint>
#include <memory>
#include <functional>
#include "os_process.h"

class CPUCore {
private:
    int core_id;
    Process* current_process = nullptr;
    int delays_per_exec;
    int ticks_since_last_exec;
    uint64_t total_ticks = 0;
    uint64_t active_ticks = 0;

public:
    explicit CPUCore(int id, int delay=0);

    int get_id() const;

    bool is_idle() const;

    void assign_process(Process* process);

    Process* get_process() const;

    void remove_process();

    // Execute one CPU tick, called by kernel
    bool tick(LogEntry& log);

    double get_utilization() const;
};

class CPUManager {
private:
    int num_cores;
    uint64_t delay_per_exec;
    std::vector<CPUCore> cores;
public:
    CPUManager(int num_cores, uint64_t delay_per_exec);

    double get_global_utilization() const;

    // kernel gets cpu through get_cores, which points to the mem where the array of cores live.
    std::vector<CPUCore>& get_cores();

    // pass in log handler here
    void tick(std::function<void(const LogEntry&)> raise_interrupt);
    
    int get_cores_used() const;
    int get_cores_available() const;
};