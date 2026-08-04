#pragma once
#include <vector>
#include <memory>
#include <string>
#include "os_process.h"

// Parses a semicolon-separated instruction string into Instruction objects.
// Returns empty vector on parse error.
std::vector<std::unique_ptr<Instruction>> parse_instructions(const std::string& raw);

// Helper to parse a token as either a variable name or immediate uint16
Operand parse_operand(const std::string& s);
