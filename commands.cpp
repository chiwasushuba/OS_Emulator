#include <iostream>

bool isValidCommand(const std::string& cmd) {
    if (cmd == "initialize" || cmd == "screen" ||
        cmd == "scheduler-start" || cmd == "scheduler-stop" ||
        cmd == "report-util" || cmd == "clear" || cmd == "exit")
        return true;
    else
        return false;
}