#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "config.h"

void validateConfig(Config& config) {
    Config default_config;
    
    if(config.num_cpu < 1 || config.num_cpu > 128) {
        std::cerr << "ERROR: num_cpu must be [1, 128]; using default instead...\n";
        config.num_cpu = default_config.num_cpu;
    }
    
    if(config.scheduler != "fcfs" && config.scheduler != "rr") {
        std::cerr << "ERROR: scheduler must be \"fcfs\" or \"rr\"; using default instead...\n";
        config.scheduler = default_config.scheduler;
    }
    
    if(config.quantum_cycles < 1 || config.quantum_cycles > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: quantumcycles must be [1, 2^32]; using default instead...\n";

        config.quantum_cycles = default_config.quantum_cycles;
    }


    if(config.batch_process_freq < 1 || config.batch_process_freq > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: batchprocess-freq must be [1, 2^32]; using default instead...\n";

        config.batch_process_freq = default_config.batch_process_freq;
    }


    if(config.min_ins < 1 || config.min_ins > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: min-ins must be [1, 2^32]; using default instead...\n";

        config.min_ins = default_config.min_ins;
    }


    if(config.max_ins < 1 || config.max_ins > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: max-ins must be [1, 2^32]; using default instead...\n";

        config.max_ins = default_config.max_ins;
    }


    if(config.min_ins > config.max_ins) {
        std::cerr
            << "ERROR: min-ins cannot be greater than max-ins; using default instead...\n";

        config.min_ins = default_config.min_ins;
        config.max_ins = default_config.max_ins;
    }


    if(config.delays_per_exec > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: delays-perexec must be [0, 2^32]; using default instead...\n";

        config.delays_per_exec = default_config.delays_per_exec;
    }

    if(config.max_overall_mem < 1 || config.max_overall_mem > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: max-overall-mem must be [1, 2^32]; using default instead...\n";

        config.max_overall_mem = default_config.max_overall_mem;
    }

    if(config.mem_per_frame < 1 || config.mem_per_frame > MAX_CONFIG_VALUE) {
        std::cerr
            << "ERROR: mem-per-frame must be [1, 2^32]; using default instead...\n";

        config.mem_per_frame = default_config.mem_per_frame;
    }

    auto is_power_of_2 = [](uint64_t n) { return n > 0 && (n & (n - 1)) == 0; };
    if(config.min_mem_per_proc < 64 || config.min_mem_per_proc > 65536 || !is_power_of_2(config.min_mem_per_proc) || config.min_mem_per_proc > config.max_overall_mem) {
        std::cerr << "ERROR: min-mem-per-proc must be a power of 2 between 64 and 65536, and <= max_overall_mem; using default...\n";
        config.min_mem_per_proc = default_config.min_mem_per_proc;
    }

    if(config.max_mem_per_proc < 64 || config.max_mem_per_proc > 65536 || !is_power_of_2(config.max_mem_per_proc) || config.max_mem_per_proc > config.max_overall_mem) {
        std::cerr << "ERROR: max-mem-per-proc must be a power of 2 between 64 and 65536, and <= max_overall_mem; using default...\n";
        config.max_mem_per_proc = default_config.max_mem_per_proc;
    }

    if(config.min_mem_per_proc > config.max_mem_per_proc) {
        std::cerr << "ERROR: min-mem-per-proc > max-mem-per-proc; using default...\n";
        config.min_mem_per_proc = default_config.min_mem_per_proc;
        config.max_mem_per_proc = default_config.max_mem_per_proc;
    }
}

namespace {
std::string resolve_config_path(const std::string& filename) {
    const std::filesystem::path input_path(filename);
    std::vector<std::filesystem::path> candidates;

    if (input_path.is_absolute()) {
        candidates.push_back(input_path);
    } else {
        const std::filesystem::path cwd = std::filesystem::current_path();
        candidates.push_back(cwd / input_path);
        candidates.push_back(cwd / ".." / input_path);
        candidates.push_back(cwd / ".." / ".." / input_path);
        candidates.push_back(std::filesystem::path("config.txt"));
        candidates.push_back(std::filesystem::path("../config.txt"));
        candidates.push_back(std::filesystem::path("../../config.txt"));
    }

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    }

    return {};
}
} // namespace

void loadConfig(const std::string& filename, Config& config) {
    const std::string resolved_path = resolve_config_path(filename);
    std::ifstream file(resolved_path.empty() ? filename : resolved_path);

    if (!file.is_open()) {
        std::cerr << "Failed to open config file '" << filename << "', using default...\n";
        return;
    }

    std::string line;

    // TODO: Check if these safeguards are enough.
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        // convert from str to stream
        std::stringstream ss(line);

        std::string key;
        std::string value;

        // Extract whitespace separatated tokens
        ss >> key >> value;

        if (key == "num-cpu") {
            config.num_cpu = std::stoi(value);
        }
        else if (key == "scheduler") {
            config.scheduler = value;
        }
        else if (key == "quantum-cycles") {
            config.quantum_cycles = std::stoull(value);
        }
        else if (key == "batch-process-freq") {
            config.batch_process_freq = std::stoull(value);
        }
        else if (key == "min-ins") {
            config.min_ins = std::stoull(value);
        }
        else if (key == "max-ins") {
            config.max_ins = std::stoull(value);
        }
        else if (key == "delays-per-exec") {
            config.delays_per_exec = std::stoull(value);
        }
        else if (key == "max-overall-mem") {
            config.max_overall_mem = std::stoull(value);
        }
        else if (key == "mem-per-frame") {
            config.mem_per_frame = std::stoull(value);
        }
        else if (key == "min-mem-per-proc") {
            config.min_mem_per_proc = std::stoull(value);
        }
        else if (key == "max-mem-per-proc") {
            config.max_mem_per_proc = std::stoull(value);
        }
        else {
            std::cerr << "Unknown parameter: " << key << '\n';
        }
    }

    validateConfig(config);
}