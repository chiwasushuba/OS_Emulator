// TODO: Implement command handler
// - validation
// - switch handling

#pragma once

#include <string>
#include <vector>

class Commands;

class CommandHandler {
public:

    // constructor
    explicit CommandHandler(Commands* commands);

    // validates and executes input command string
    // screen -ls -> ["screen", "-ls"] -> valid? -> 
    // yes -> reroute command to screenList()
    void handleCommand(const std::string& input);

private:

    // checks if the command is valid based on the tokens
    bool isValidCommand(const std::vector<std::string>& tokens);

    // tokenizes input string into a vector of strings based on whitespace
    std::vector<std::string> tokenize(const std::string& input);

    Commands* commands_;
};