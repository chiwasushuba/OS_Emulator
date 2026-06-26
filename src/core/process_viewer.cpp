#include "os_process.h"
#include "core.h"
#include <iostream>
#include <sstream>

void format_active_entry(std::stringstream& ss, Process* process) {
        ss << process->process_name << "\t";
        ss << process->process_name << "\t";
        ss << "(" << get_current_time() << ")\t";
        ss << "Core: " << process->core_id << "\t\t";
        ss << process->current_instruction+1 << " / " << process->total_instructions() << "\n";
}

void format_finished_entry(std::stringstream& ss, Process* process) {
        ss << process->process_name << "\t";
        ss << process->process_name << "\t";
        ss << "(" << get_current_time() << ")\t";
        ss << "Finished\t";
        ss << process->current_instruction << " / " << process->total_instructions() << "\n";
}

void ProcessViewer::list_processes() {

    auto finished_pids = process_manager.get_finished_pids();
    auto active_pids = process_manager.get_active_pids();

    std::string divider = "------------------------------------------\n"; 
    std::cout << divider << "Running processes:\n";
    if (active_pids.size() == 0) {
        std::cout << "None\n";
    } else {
        for (int pid : active_pids) {
            Process* p = process_manager.get_process(pid);
            std::stringstream entry_string;
            format_active_entry(entry_string, p);
            std::cout << entry_string.str();
        }
    }
    std::cout << "\nFinished processes:\n";
    if (finished_pids.size() == 0) {
        std::cout << "None\n";
    } else {
        for (int pid : finished_pids) {
            Process* p = process_manager.get_process(pid);
            std::stringstream entry_string;
            format_finished_entry(entry_string, p);
            std::cout << entry_string.str();
        }
    }
    std::cout << divider;
}

void ProcessViewer::view_process(std::string process_name) {
    Process* process = process_manager.get_process(process_name);
    std::string input;

    while (true) {
        std::cout << "\nroot:\\screen\\> ";

        if (!std::getline(std::cin, input)) {
            // EOF or input stream closed
            break;
        }

        if (input.empty()) {
            continue;
        }

        if (input == "process-smi") {
            print_log(process->id);
        }
        else if (input == "exit") {
            break;
        }
        else {
            std::cout << "Invalid input! use \"process-smi\" or \"exit\"";
        }
    }
}

void ProcessViewer::print_log(int pid) {
    auto logs = logger.get_logs(pid);
    Process* process = process_manager.get_process(pid);

    std::cout << "\n\nProcess name: " << process->process_name << "\n";
    std::cout << "ID: " << process->id << "\n";
    std::cout << "Logs:\n";
    for (const auto& entry : logs) {
        std::stringstream log_string;
        log_string << "(" << entry.timestamp << ") ";
        log_string << "Core:" << entry.core_id;
        log_string << " \"" << entry.message << "\"\n";
        std::cout << log_string.str();
    }

    std::cout << "\nCurrent instruction line: " << process->current_instruction << "\n";
    std::cout << "Lines of code: " << process->total_instructions() << "\n";
}