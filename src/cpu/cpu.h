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

public:
    explicit CPUCore(int id, int delay=0);

    int get_id() const;

    bool is_idle() const;

    void assign_process(Process* process);

    Process* get_process() const;

    void remove_process();

    // Execute one CPU tick, called by kernel
    bool tick(LogEntry& log);
};

class CPUManager {
private:
    int num_cores;
    uint64_t delay_per_exec;
    std::vector<CPUCore> cores;
public:
    CPUManager(int num_cores, uint64_t delay_per_exec);

    // kernel gets cpu through get_cores, which points to the mem where the array of cores live.
    std::vector<CPUCore>& get_cores();

    // pass in log handler here
    void tick(std::function<void(const LogEntry&)> raise_interrupt);
};