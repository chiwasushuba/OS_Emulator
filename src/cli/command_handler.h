// TODO: Implement command handler
// - validation
// - switch handling

#pragma once

#include <string>
#include <vector>
#include "kernel.h"

class Kernel;

class CommandHandler {
private:
    Kernel* kernel_;
    
    // tokenizes input string into a vector of strings based on whitespace
    std::vector<std::string> tokenize(const std::string& input);

    // checks if the command is valid based on the tokens
    bool isValidCommand(const std::vector<std::string>& tokens);

    // routes a validated command to the Commands layer
    bool executeCommand(const std::vector<std::string>& tokens);

    void help();
public:
    // constructor
    explicit CommandHandler(Kernel* kernel)
        : kernel_(kernel) {}

    // validates and executes input command string
    // screen -ls -> ["screen", "-ls"] -> valid? -> 
    // yes -> reroute command to screenList()
    bool handleCommand(const std::string& input);
};