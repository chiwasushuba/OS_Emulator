#pragma once
#include <string>

// Assuming Instruction is a custom type you've defined elsewhere
// struct Instruction { ... };

struct Process {
    const std::string pid;
    const Instruction instruction; // Removed std::
    const int priority_count;      // Removed std::
    const std::string logs;

    // Constructor to initialize the const member variables
    Process(std::string p, Instruction inst, int priority, std::string l)
        : pid(std::move(p)), instruction(inst), priority_count(priority), logs(std::move(l)) {}
};