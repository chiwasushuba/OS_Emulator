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
    std::cout << "report-util (not yet implemented)\n";
}