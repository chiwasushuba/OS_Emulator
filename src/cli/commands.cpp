// Contains logic for the commands, mostly composed of calls to other components

#include <iostream>
#include "commands.h"
#include "console.h"
#include "kernel.h"

Commands::Commands(Kernel* kernel) : kernel_(kernel) {}

void Commands::setConsole(Console* console) {
    console_ = console;
}

void Commands::initialize() {
    // TODO: Implement initialize
    std::cout << "System initialized.\n";
}

void Commands::exit() {
    std::cout << "Exiting...\n";

    // Stop the console CLI loop
    if (console_) {
        console_->stop();
    }

    // Stop the kernel (ends the clock thread)
    kernel_->shutdown();
}

void Commands::screenCreate(const std::string& processName) {
    // TODO: Implement screen -s
    std::cout << "screen -s " << processName << " (not yet implemented)\n";
}

void Commands::screenResume(const std::string& processName) {
    // TODO: Implement screen -r
    std::cout << "screen -r " << processName << " (not yet implemented)\n";
}

void Commands::screenList() {
    // TODO: Implement screen -ls
    std::cout << "screen -ls (not yet implemented)\n";
}

void Commands::schedulerStart() {
    kernel_->start_scheduler();
}

void Commands::schedulerStop() {
    kernel_->stop_scheduler();
}

void Commands::reportUtil() {
    // TODO: Implement report-util
    //std::cout << "report-util (not yet implemented)\n";
    kernel_->generate_report_file();
}

void Commands::help() {
    std::cout << "Available commands:\n"
              << "  initialize           - Initialize the system\n"
              << "  exit                 - Exit the emulator\n"
              << "  clear                - Clear the console screen\n"
              << "  scheduler-start      - Start generating background processes\n"
              << "  scheduler-stop       - Stop generating background processes\n"
              << "  report-util          - Report current CPU utilization\n"
              << "  screen -ls           - List all active screens (processes)\n"
              << "  screen -s <name>     - Create and attach to a new screen (process)\n"
              << "  screen -r <name>     - Resume and attach to an existing screen\n"
              << "  help                 - Display this help message\n";
}
