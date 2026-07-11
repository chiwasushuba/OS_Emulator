#include <cstdint>
#include "cpu.h"

// TODO: Implement CPU manager
// - ticks all CPU cores
// Note: handled here instead of scheduler
// - this enforces encapsulation

CPUManager::CPUManager(int num_cores, uint64_t delay_per_exec)
    : num_cores(num_cores), delay_per_exec(delay_per_exec) 
{
    // Populate the vector with the specified number of cores
    for (int i = 0; i < num_cores; ++i) {
        cores.emplace_back(i); // Calls the CPUCore(int id) constructor
    }
}

std::vector<CPUCore>& CPUManager::get_cores() {
    return this->cores;
}

void CPUManager::tick(std::function<void(const LogEntry&)> raise_interrupt) {
    for (auto& core : this->cores) {
        LogEntry log;
        core.tick(log);

        if (log.event_type == LogEventType::INSTRUCTION_FINISHED || log.event_type == LogEventType::LOG) {
            raise_interrupt(log); 
        }
    }
}   