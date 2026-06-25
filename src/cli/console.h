// display text, read input, and pass to command handler

#pragma once

#include <atomic>

class CommandHandler;

class Console {
public:
    explicit Console(CommandHandler* handler);

    // main cli loop (keyboard polling)
    void run();

    // terminate console (running=false)
    void stop();

private:
    CommandHandler* handler_;

    // allows safe access of multiple threads
    std::atomic<bool> running_;
};