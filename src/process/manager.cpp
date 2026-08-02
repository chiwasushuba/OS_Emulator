#include "os_process.h"

int ProcessManager::create_process(const std::string& name) {
    std::lock_guard<std::mutex> lock(pm_mutex);
    int pid = next_pid++;

    auto process = std::make_unique<Process>();

    process->id = pid;
    process->process_name = name;

    processes[pid] = std::move(process);

    return pid;
}

Process* ProcessManager::get_process(int pid) {
    std::lock_guard<std::mutex> lock(pm_mutex);
    auto it = processes.find(pid);
    if (it == processes.end()) {
        return nullptr;
    }
    return it->second.get();
}

Process* ProcessManager::get_process(const std::string& process_name) {
    std::lock_guard<std::mutex> lock(pm_mutex);
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        Process* process = it->second.get();
        if (process->process_name == process_name) {
            return process;
        }
    }

    return nullptr;
}

std::vector<int> ProcessManager::get_active_pids() const {
    std::lock_guard<std::mutex> lock(pm_mutex);
    std::vector<int> pids;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        Process* process = it->second.get();
        if (process->core_id != -1 && process->state != ProcessState::FINISHED && process->state != ProcessState::TERMINATED) {
            pids.push_back(it->first);
        }
    }
    return pids;
}

std::vector<int> ProcessManager::get_finished_pids() const {
    std::lock_guard<std::mutex> lock(pm_mutex);
    std::vector<int> pids;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        Process* process = it->second.get();
        if (process->state == ProcessState::FINISHED || process->state == ProcessState::TERMINATED) {
            pids.push_back(it->first);
        }
    }
    return pids;
}

std::vector<int> ProcessManager::get_all_pids() const {
    std::lock_guard<std::mutex> lock(pm_mutex);
    std::vector<int> pids;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        pids.push_back(it->first);
    }
    return pids;
}

std::vector<ProcessSnapshot> ProcessManager::get_active_processes() const {
    std::lock_guard<std::mutex> lock(pm_mutex); // 🔒 Protects the read
    
    std::vector<ProcessSnapshot> active_procs;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        Process* process = it->second.get();
        // Evaluate the criteria strictly inside the lock
        if (process->core_id != -1 && process->state != ProcessState::FINISHED && process->state != ProcessState::TERMINATED) {
            active_procs.push_back(ProcessSnapshot{
                process->id,
                process->process_name,
                process->core_id,
                process->current_instruction,
                process->total_instructions()
            });
        }
    }
    return active_procs; // Returns a thread-safe, static copy
}
