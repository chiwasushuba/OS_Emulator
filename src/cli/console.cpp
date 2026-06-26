// display text, read input, and pass to command handler

#include <iostream>
#include <string>
#include "console.h"
#include "command_handler.h"
#include "ui.h"

Console::Console(CommandHandler* handler)
    : handler_(handler), running_(false)
{}

void Console::run() {
    running_.store(true);
    std::string input;
    printIntro();

    while (running_.load()) {
        std::cout << "\nroot:\\> ";

        if (!std::getline(std::cin, input)) {
            // EOF or input stream closed
            break;
        }

        if (input.empty()) {
            continue;
        }

        bool should_continue = handler_->handleCommand(input);
        if (!should_continue) {
            break;
        }
    }
}

void Console::stop() {
    running_.store(false);
}