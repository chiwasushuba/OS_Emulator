#include <cstdint>
#include "cpu.h"

CPUManager::CPUManager(int num_cores, uint64_t delay_per_exec)
    : num_cores(num_cores), delay_per_exec(delay_per_exec) 
{
    // Populate the vector with the specified number of cores
    for (int i = 0; i < num_cores; ++i) {
        cores.emplace_back(i, delay_per_exec); // Calls the CPUCore(int id) constructor
    }
}

double CPUManager::get_global_utilization() const {
    if (cores.empty()) return 0.0;

    double total_utilization = 0.0;
    for (const auto& core : cores) {
        total_utilization += core.get_utilization();
    }

    return total_utilization / cores.size();
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

int CPUManager::get_cores_used() const {
    int used_count = 0;
    for (const auto& core : cores) {
        // A core is used if it currently owns/is running a process
        if (core.get_process() != nullptr) {
            used_count++;
        }
    }
    return used_count;
}

int CPUManager::get_cores_available() const {
    int available_count = 0;
    for (const auto& core : cores) {
        // A core is available if it has no process assigned (idle)
        if (core.get_process() == nullptr) {
            available_count++;
        }
    }
    return available_count;
}