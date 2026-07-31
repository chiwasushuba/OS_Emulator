#include "instruction_parser.h"
#include <sstream>
#include <iostream>

Operand parse_operand(const std::string& s) {
    // Try parsing as number first
    try {
        size_t pos = 0;
        unsigned long val = std::stoul(s, &pos);
        if (pos == s.size()) {
            // Entire string was a number
            uint16_t clamped = (val > 65535) ? 65535 : static_cast<uint16_t>(val);
            return Operand{false, "", clamped};
        }
    } catch (...) {}
    // Treat as variable name
    return Operand{true, s, 0};
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::unique_ptr<Instruction>> parse_instructions(const std::string& raw) {
    std::vector<std::unique_ptr<Instruction>> result;

    // Split by semicolons
    std::vector<std::string> parts;
    std::stringstream ss(raw);
    std::string part;
    while (std::getline(ss, part, ';')) {
        std::string trimmed = trim(part);
        if (!trimmed.empty()) {
            parts.push_back(trimmed);
        }
    }

    // Parse each instruction
    for (const auto& inst_str : parts) {
        std::istringstream iss(inst_str);
        std::string opcode;
        iss >> opcode;

        if (opcode == "DECLARE") {
            std::string var;
            std::string val_s;
            iss >> var >> val_s;
            uint16_t val = 0;
            try { val = static_cast<uint16_t>(std::stoul(val_s)); } catch (...) {}
            result.push_back(std::make_unique<DeclareInstruction>(var, val));
        }
        else if (opcode == "ADD") {
            std::string dest, lhs_s, rhs_s;
            iss >> dest >> lhs_s >> rhs_s;
            result.push_back(std::make_unique<AddInstruction>(dest,
                parse_operand(lhs_s), parse_operand(rhs_s)));
        }
        else if (opcode == "SUBTRACT") {
            std::string dest, lhs_s, rhs_s;
            iss >> dest >> lhs_s >> rhs_s;
            result.push_back(std::make_unique<SubtractInstruction>(dest,
                parse_operand(lhs_s), parse_operand(rhs_s)));
        }
        else if (opcode == "PRINT") {
            // Rest of line is the message
            std::string rest;
            std::getline(iss, rest);
            rest = trim(rest);
            // Strip surrounding quotes if present
            if (rest.size() >= 2 && rest.front() == '"' && rest.back() == '"') {
                rest = rest.substr(1, rest.size() - 2);
            }
            // Check if it contains a + for variable concatenation like "Result: " + varC
            size_t plus_pos = rest.find('+');
            if (plus_pos != std::string::npos) {
                std::string msg_part = trim(rest.substr(0, plus_pos));
                std::string var_part = trim(rest.substr(plus_pos + 1));
                // Strip quotes from msg_part
                if (msg_part.size() >= 2 && msg_part.front() == '"' && msg_part.back() == '"') {
                    msg_part = msg_part.substr(1, msg_part.size() - 2);
                }
                // Strip parentheses from PRINT(...) style
                if (var_part.back() == ')') var_part.pop_back();
                result.push_back(std::make_unique<PrintInstruction>(msg_part, var_part));
            } else {
                // Check for PRINT("msg") style with parentheses
                if (rest.front() == '(' && rest.back() == ')') {
                    rest = rest.substr(1, rest.size() - 2);
                }
                if (rest.size() >= 2 && rest.front() == '"' && rest.back() == '"') {
                    rest = rest.substr(1, rest.size() - 2);
                }
                result.push_back(std::make_unique<PrintInstruction>(rest));
            }
        }
        else if (opcode == "READ") {
            std::string var, addr_s;
            iss >> var >> addr_s;
            uint32_t addr = 0;
            try { addr = std::stoul(addr_s, nullptr, 16); } catch (...) {}
            result.push_back(std::make_unique<ReadInstruction>(var, addr));
        }
        else if (opcode == "WRITE") {
            std::string addr_s, val_s;
            iss >> addr_s >> val_s;
            uint32_t addr = 0;
            try { addr = std::stoul(addr_s, nullptr, 16); } catch (...) {}
            result.push_back(std::make_unique<WriteInstruction>(addr, parse_operand(val_s)));
        }
        else if (opcode == "SLEEP") {
            int n = 1;
            iss >> n;
            if (n < 1) n = 1;
            if (n > 255) n = 255;
            result.push_back(std::make_unique<SleepInstruction>(static_cast<uint8_t>(n)));
        }
        else {
            // Unknown instruction — skip
            std::cerr << "Warning: Unknown instruction '" << opcode << "'\n";
        }
    }

    return result;
}
