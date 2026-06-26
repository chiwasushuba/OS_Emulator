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

    // tokenizes input string into a vector of strings based on whitespace
    std::vector<std::string> tokenize(const std::string& input);

    // checks if the command is valid based on the tokens
    bool isValidCommand(const std::vector<std::string>& tokens);

    // routes a validated command to the Commands layer
    void executeCommand(const std::vector<std::string>& tokens);

    // prints error for invalid commands
    void printInvalidCommand() const;

    Commands* commands_;
};