#include "os_process.h"

int ProcessManager::create_process(const std::string& name) {
    int pid = next_pid++;

    auto process = std::make_unique<Process>();

    process->id = pid;
    process->process_name = name;

    processes[pid] = std::move(process);

    return pid;
}

Process* ProcessManager::get_process(int pid) {
    auto it = processes.find(pid);
    if (it == processes.end()) {
        return nullptr;
    }
    return it->second.get();
}

Process* ProcessManager::get_process(const std::string& process_name) {
    for (auto& [pid, process] : processes) {
        if (process->process_name == process_name) {
            return process.get();
        }
    }

    return nullptr;
}

std::vector<int> ProcessManager::get_active_pids() const {
    std::vector<int> pids;
    for (const auto& [pid, process] : processes) {
        if (process->core_id != -1 && process->state != ProcessState::FINISHED) {
            pids.push_back(pid);
        }
    }
    return pids;
}

std::vector<int> ProcessManager::get_finished_pids() const {
    std::vector<int> pids;
    for (const auto& [pid, process] : processes) {
        if (process->state == ProcessState::FINISHED) {
            pids.push_back(pid);
        }
    }
    return pids;
}

std::vector<int> ProcessManager::get_all_pids() const {
    std::vector<int> pids;
    for (const auto& [pid, process] : processes) {
        pids.push_back(pid);
    }
    return pids;
}