#include "core.h"
#include "os_process.h"
#include <iostream>
#include <fstream>
#include <sstream>

ReportGenerator::ReportGenerator(ProcessManager& pm) : process_manager(pm) {}

void ReportGenerator::generate_report(const std::string& filename) {
    // Open the text file for writing
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Retrieve active and finished processes from ProcessManager
    auto finished_pids = process_manager.get_finished_pids();
    auto active_pids = process_manager.get_active_pids();

    std::string divider = "------------------------------------------\n";

    // 1. Write Running Processes Section
    outfile << divider << "Running processes:\n";
    if (active_pids.size() == 0) {
        outfile << "None\n";
    }
    else {
        for (int pid : active_pids) {
            Process* p = process_manager.get_process(pid);
            if (p) {
                outfile << p->process_name << "\t";
                outfile << p->process_name << "\t";
                outfile << "(" << get_current_time() << ")\t";
                outfile << "Core: " << p->core_id << "\t\t";
                outfile << p->current_instruction + 1 << " / " << p->total_instructions() << "\n";
            }
        }
    }

    // 2. Write Finished Processes Section
    outfile << "\nFinished processes:\n";
    if (finished_pids.size() == 0) {
        outfile << "None\n";
    }
    else {
        for (int pid : finished_pids) {
            Process* p = process_manager.get_process(pid);
            if (p) {
                outfile << p->process_name << "\t";
                outfile << p->process_name << "\t";
                outfile << "(" << get_current_time() << ")\t";
                outfile << "Finished\t";
                outfile << p->current_instruction << " / " << p->total_instructions() << "\n";
            }
        }
    }

    outfile << divider;

    //brief summary block at the bottom for better "utilization report" vibes
    outfile << "\nSummary Stats:\n";
    outfile << "Total Active Processes: " << active_pids.size() << "\n";
    outfile << "Total Finished Processes: " << finished_pids.size() << "\n";
    outfile << "Total Registered Processes: " << (active_pids.size() + finished_pids.size()) << "\n";

    outfile.close();

    // Print a confirmation message to the CLI console
    std::cout << "Report successfully generated and saved to " << filename << "\n";
}
