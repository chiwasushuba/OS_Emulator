// display text, read input, and pass to command handler

#include <iostream>
#include <string>
#include "console.h"
#include "command_handler.h"

Console::Console(CommandHandler* handler)
    : handler_(handler), running_(false)
{}

void Console::run() {
    running_.store(true);
    std::string input;

    while (running_.load()) {
        std::cout << "Enter a command: ";

        if (!std::getline(std::cin, input)) {
            // EOF or input stream closed
            break;
        }

        if (input.empty()) {
            continue;
        }

        handler_->handleCommand(input);
    }
}

void Console::stop() {
    running_.store(false);
}

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