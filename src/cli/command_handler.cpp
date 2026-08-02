// Command handler: tokenizes input, validates, and routes to the correct Commands method

#include <iostream>
#include <sstream>
#include "command_handler.h"
#include "kernel.h"

static bool is_power_of_2(uint64_t n) { return n > 0 && (n & (n - 1)) == 0; }


std::vector<std::string> CommandHandler::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"') {
            in_quotes = !in_quotes;
            // Don't include the quote character itself
        } else if (c == ' ' && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back(current);
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
    if (cmd == "process-smi")      return tokens.size() == 1;
    if (cmd == "vmstat")           return tokens.size() == 1;
    if (cmd == "help")             return tokens.size() == 1;

    if (cmd == "screen") {
        if (tokens.size() == 2 && tokens[1] == "-ls") return true;
        if (tokens.size() >= 3 && tokens[1] == "-s")  return true;  // 3 or 4 tokens
        if (tokens.size() == 3 && tokens[1] == "-r")   return true;
        if (tokens.size() == 5 && tokens[1] == "-c")   return true;  // name + size + instructions
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
              << "  process-smi          - Report CPU and memory usage\n"
              << "  vmstat               - Report memory and paging statistics\n"
              << "  screen -ls           - List all active screens (processes)\n"
              << "  screen -s <name> [mem_size] - Create and attach to a new screen\n"
              << "  screen -c <name> <mem_size> \"instructions\" - Create a custom process\n"
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
    else if (cmd == "report-util") {
        packet.type = CommandType::REPORT;
    } 
    else if (cmd == "process-smi") {
        packet.type = CommandType::PROCESS_SMI;
    }
    else if (cmd == "vmstat") {
        packet.type = CommandType::VMSTAT;
    }
    else if (cmd == "exit") {
        packet.type = CommandType::EXIT;
    } 
    else if (cmd == "screen") {
        packet.type = CommandType::SCREEN;
        if (tokens.size() >= 2) {
            if (tokens[1] == "-ls") {
                packet.screen_action = ScreenAction::LIST;
            } else if (tokens[1] == "-s" && tokens.size() >= 3) {
                packet.screen_action = ScreenAction::CREATE;
                packet.payload = tokens[2]; // screen name

                if (tokens.size() >= 4) {
                    try {
                        packet.mem_size = std::stoull(tokens[3]);
                    } catch (...) {
                        std::cout << "invalid memory allocation\n";
                        return true;
                    }
                    if (packet.mem_size < 64 || packet.mem_size > 65536
                        || !is_power_of_2(packet.mem_size)) {
                        std::cout << "invalid memory allocation\n";
                        return true;
                    }
                }
            } else if (tokens[1] == "-r" && tokens.size() > 2) {
                packet.screen_action = ScreenAction::READ;
                packet.payload = tokens[2]; // screen name
            } else if (tokens[1] == "-c" && tokens.size() == 5) {
                packet.screen_action = ScreenAction::CREATE_CUSTOM;
                packet.payload = tokens[2]; // screen name

                // Parse mem_size
                try {
                    packet.mem_size = std::stoull(tokens[3]);
                } catch (...) {
                    std::cout << "invalid memory allocation\n";
                    return true;
                }
                if (packet.mem_size < 64 || packet.mem_size > 65536
                    || !is_power_of_2(packet.mem_size)) {
                    std::cout << "invalid memory allocation\n";
                    return true;
                }

                // Instructions string (already unquoted by tokenizer)
                if (tokens.size() >= 5) {
                    packet.raw_instructions = tokens[4];
                }
            }
        }

        if (packet.screen_action == ScreenAction::NONE) {
            std::cout << "Usage: screen [-ls] | [-s name [mem_size]] | [-r name] | [-c name mem_size \"instructions\"]\n";
            return true;
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

