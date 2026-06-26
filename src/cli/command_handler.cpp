// Command handler: tokenizes input, validates, and routes to the correct Commands method

#include <iostream>
#include <sstream>
#include "command_handler.h"
#include "commands.h"
#include "kernel.h"

CommandHandler::CommandHandler(Commands* commands)
    : commands_(commands)
{}

std::vector<std::string> CommandHandler::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool CommandHandler::isValidCommand(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return false;

    const std::string& cmd = tokens[0];

    if (cmd == "initialize")       return tokens.size() == 1;
    if (cmd == "exit")             return tokens.size() == 1;
    if (cmd == "clear")            return tokens.size() == 1;
    if (cmd == "scheduler-start")  return tokens.size() == 1;
    if (cmd == "scheduler-stop")   return tokens.size() == 1;
    if (cmd == "report-util")      return tokens.size() == 1;
    if (cmd == "help")             return tokens.size() == 1;

    if (cmd == "screen") {
        if (tokens.size() == 2 && tokens[1] == "-ls") return true;
        if (tokens.size() == 3 && tokens[1] == "-s")  return true;
        if (tokens.size() == 3 && tokens[1] == "-r")  return true;
        return false;
    }

    return false;
}

void CommandHandler::handleCommand(const std::string& input) {
    std::vector<std::string> tokens = tokenize(input);

    if (tokens.empty()) return;

    if (!isValidCommand(tokens)) {
        std::cout << "Invalid command. Please try again.\n";
        return;
    }

    const std::string& cmd = tokens[0];

    if (cmd == "initialize") {
        commands_->initialize();
        return;
    }
    else if (cmd == "exit") {
        commands_->exit();
        return;
    }

    // Checker for initialization status for commands that need "initialize" to be called first 
    if (!commands_->getKernel()->get_initialized_status()) {
        std::cout << "Error: System is uninitialized. Please run 'initialize' first.\n";
        return; 
    }
    else if (cmd == "clear") {
        #ifdef _WIN32
            system("CLS");
        #else
            system("clear");
        #endif
    }
    else if (cmd == "scheduler-start") {
        commands_->schedulerStart();
    }
    else if (cmd == "scheduler-stop") {
        commands_->schedulerStop();
    }
    else if (cmd == "report-util") {
        commands_->reportUtil();
    }
    else if (cmd == "help") {
        commands_->help();
    }
    else if (cmd == "screen") {
        if (tokens[1] == "-ls") {
            commands_->screenList();
        }
        else if (tokens[1] == "-s") {
            commands_->screenCreate(tokens[2]);
        }
        else if (tokens[1] == "-r") {
            commands_->screenResume(tokens[2]);
        }
    }
}

