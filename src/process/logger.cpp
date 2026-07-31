#include "os_process.h"

void ProcessLogger::append(int process_id, const LogEntry& entry)
{
    std::lock_guard<std::mutex> lock(logger_mutex);
    process_logs[process_id].push_back(entry);
}

std::vector<LogEntry> ProcessLogger::get_logs(int process_id)
{
    std::lock_guard<std::mutex> lock(logger_mutex);

    auto it = process_logs.find(process_id);
    if (it == process_logs.end()) {
        return {};
    }

    return it->second; // returns a copy
}

std::vector<int> ProcessLogger::get_process_ids()
{
    std::lock_guard<std::mutex> lock(logger_mutex);

    std::vector<int> ids;
    ids.reserve(process_logs.size());

    for (const auto& entry : process_logs) {
        ids.push_back(entry.first);
    }

    return ids;
}