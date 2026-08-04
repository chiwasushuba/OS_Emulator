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
    // True when the last tick was spent actually running a process rather than
    // idle or stalled handling a page fault. See tick().
    bool last_tick_productive = false;

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

    // Real accumulated tick counts, for vmstat. active = ticks the core spent
    // "actually executing instructions" (the spec's wording); total - active =
    // idle, which now correctly includes ticks lost to page-fault handling.
    uint64_t get_total_ticks() const { return total_ticks; }
    uint64_t get_active_ticks() const { return active_ticks; }

    bool was_productive() const { return last_tick_productive; }
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