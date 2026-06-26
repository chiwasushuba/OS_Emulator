// TODO: Implement console

#include <iostream>
#include "console.h"
#include "command_handler.h"
#include "commands.h"
#include "ui.h"

void Console::run() {
    running_ = true;
    std::string input;

    while (running_) {
        printIntro();

        std::cout << "Enter a command: ";

        // dis keyboard polling 
        std::getline(std::cin, input);

        if (input.empty()) {
            continue; // Skip empty input
        }

        handler_->handleCommand(input);
    }
}