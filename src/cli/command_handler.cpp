// Command handler: tokenizes input, validates, and routes to the correct Commands method

#include <iostream>
#include <sstream>
#include "command_handler.h"
#include "kernel.h"

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

void CommandHandler::help() {
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

void CommandHandler::clearScreen() {
    #ifdef _WIN32
        system("CLS");
    #else
        system("clear");
    #endif
}

bool CommandHandler::handleCommand(const std::string& input) {
    std::vector<std::string> tokens = tokenize(input);

    if (tokens.empty()) return true;

    if (!isValidCommand(tokens)) {
        std::cout << "Invalid command. Please try again.\n";
        return true;
    }
    
    // Special case, only involves the screen not the kernel.
    const std::string& cmd = tokens[0];
    if (cmd == "clear") {
        clearScreen();
    }
    else if (cmd == "help") {
        help();
    }

    CommandPacket packet;
    if (cmd == "initialize") {
        packet.type = CommandType::INITIALIZE;
    } 
    else if (cmd == "scheduler-start") {
        packet.type = CommandType::START_SCHEDULER;
    } 
    else if (cmd == "scheduler-stop") {
        packet.type = CommandType::STOP_SCHEDULER;
    } 
    else if (cmd == "report") {
        packet.type = CommandType::REPORT;
    } 
    else if (cmd == "exit") {
        packet.type = CommandType::EXIT;
    } 
    else if (cmd == "screen") {
        packet.type = CommandType::SCREEN;
        if (tokens.size() >= 2) {
            if (tokens[1] == "-ls") {
                packet.screen_action = ScreenAction::LIST;
            } else if (tokens[1] == "-s" && tokens.size() > 2) {
                packet.screen_action = ScreenAction::CREATE;
                packet.payload = tokens[2]; // screen name
            } else if (tokens[1] == "-r" && tokens.size() > 2) {
                packet.screen_action = ScreenAction::READ;
                packet.payload = tokens[2]; // screen name
            }
        }
        
        if (packet.screen_action == ScreenAction::NONE) {
            std::cout << "Usage: screen [-ls] | [-s name] | [-r name]\n";
            return true; // Reject bad CLI flags early
        }
    }
    if (packet.type == CommandType::EXIT) {
        kernel_->handle_command(packet);
        return false;
    }

    if (packet.type != CommandType::UNKNOWN) {
        kernel_->handle_command(packet);
    }

    return true;
}

