#include "instruction_parser.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

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
    } catch (const std::out_of_range&) {
        // A literal too big for unsigned long is still a literal - clamp it
        // rather than falling through and treating "99999999999999999999" as a
        // variable name (which would silently resolve to 0).
        if (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0])))
            return Operand{false, "", 65535};
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

static bool is_valid_identifier(const std::string& s) {
    if (s.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_') return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
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
        // The spec writes calls as PRINT("..."), with no space before the paren, so
        // a plain `iss >> opcode` would read `PRINT("Result:` as the opcode. Detach a
        // '(' that sits inside the leading token so both PRINT(x) and PRINT x parse.
        std::string normalized = inst_str;
        size_t paren = normalized.find('(');
        if (paren != std::string::npos && paren > 0) {
            size_t first_ws = normalized.find_first_of(" \t");
            if (first_ws == std::string::npos || paren < first_ws) {
                normalized.insert(paren, " ");
            }
        }

        std::istringstream iss(normalized);
        std::string opcode;
        iss >> opcode;

        if (opcode == "DECLARE") {
            std::string var;
            std::string val_s;
            iss >> var >> val_s;
            if (!is_valid_identifier(var)) {
                return {};
            }
            // "uint16 variables are clamped between (0, max(uint16))" - a plain
            // static_cast would wrap instead (70000 -> 4464), silently storing a
            // value the user never asked for. Out-of-range also clamps rather
            // than rejecting the whole instruction list.
            uint16_t val = 0;
            try {
                unsigned long raw = std::stoul(val_s);
                val = (raw > 65535UL) ? static_cast<uint16_t>(65535) : static_cast<uint16_t>(raw);
            } catch (const std::out_of_range&) {
                val = 65535;
            } catch (...) {
                std::cerr << "Error: Malformed instruction '" << inst_str << "'\n";
                return {};
            }
            result.push_back(std::make_unique<DeclareInstruction>(var, val));
        }
        else if (opcode == "ADD") {
            std::string dest, lhs_s, rhs_s;
            iss >> dest >> lhs_s >> rhs_s;
            if (!is_valid_identifier(dest)) {
                return {};
            }
            result.push_back(std::make_unique<AddInstruction>(dest,
                parse_operand(lhs_s), parse_operand(rhs_s)));
        }
        else if (opcode == "SUBTRACT") {
            std::string dest, lhs_s, rhs_s;
            iss >> dest >> lhs_s >> rhs_s;
            if (!is_valid_identifier(dest)) {
                return {};
            }
            result.push_back(std::make_unique<SubtractInstruction>(dest,
                parse_operand(lhs_s), parse_operand(rhs_s)));
        }
        else if (opcode == "PRINT") {
            // Rest of line is the message
            std::string rest;
            std::getline(iss, rest);
            rest = trim(rest);
            // Drop the wrapping parens of PRINT(...) before anything else, so the
            // concatenation scan below sees `"Result: " + varC`, not `("Result: " + varC)`.
            if (rest.size() >= 2 && rest.front() == '(' && rest.back() == ')') {
                rest = trim(rest.substr(1, rest.size() - 2));
            }
            // Strip surrounding quotes if present
            if (rest.size() >= 2 && rest.front() == '"' && rest.back() == '"') {
                rest = rest.substr(1, rest.size() - 2);
            }
            // Check if it contains a + for variable concatenation like "Result: " + varC
            size_t plus_pos = std::string::npos;
            bool in_quotes = false;
            for (size_t i = 0; i < rest.size(); ++i) {
                if (rest[i] == '"') {
                    in_quotes = !in_quotes;
                } else if (rest[i] == '+' && !in_quotes) {
                    plus_pos = i;
                    break;
                }
            }
            if (plus_pos != std::string::npos) {
                std::string msg_part = trim(rest.substr(0, plus_pos));
                std::string var_part = trim(rest.substr(plus_pos + 1));
                if (msg_part.size() >= 2 && msg_part.front() == '"' && msg_part.back() == '"') {
                    msg_part = msg_part.substr(1, msg_part.size() - 2);
                }
                if (!var_part.empty() && var_part.back() == ')') var_part.pop_back();
                result.push_back(std::make_unique<PrintExpressionInstruction>(msg_part, var_part));
            } else {
                if (!rest.empty() && rest.front() == '(' && rest.back() == ')') {
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
            if (!is_valid_identifier(var)) {
                return {};
            }
            uint32_t addr = 0;
            try { addr = std::stoul(addr_s, nullptr, 16); } catch (...) { std::cerr << "Error: Malformed instruction '" << inst_str << "'\n"; return {}; }
            result.push_back(std::make_unique<ReadInstruction>(var, addr));
        }
        else if (opcode == "WRITE") {
            std::string addr_s, val_s;
            iss >> addr_s >> val_s;
            uint32_t addr = 0;
            try { addr = std::stoul(addr_s, nullptr, 16); } catch (...) { std::cerr << "Error: Malformed instruction '" << inst_str << "'\n"; return {}; }
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
            std::cerr << "Warning: Unknown instruction '" << opcode << "'\n";
            return {};
        }
    }

    return result;
}
