#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
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

    auto is_power_of_2 = [](uint64_t n) { return n > 0 && (n & (n - 1)) == 0; };

    if(config.max_overall_mem < 64 || config.max_overall_mem > 65536 || !is_power_of_2(config.max_overall_mem)) {
        std::cerr
            << "ERROR: max-overall-mem must be a power of 2 between 64 and 65536; using default instead...\n";

        config.max_overall_mem = default_config.max_overall_mem;
    }

    if(config.mem_per_frame < 64 || config.mem_per_frame > 65536 || !is_power_of_2(config.mem_per_frame)) {
        std::cerr
            << "ERROR: mem-per-frame must be a power of 2 between 64 and 65536; using default instead...\n";

        config.mem_per_frame = default_config.mem_per_frame;
    }

    if(config.mem_per_frame > config.max_overall_mem || (config.max_overall_mem % config.mem_per_frame) != 0) {
        std::cerr << "ERROR: mem-per-frame must divide max-overall-mem; using default instead...\n";
        config.mem_per_frame = default_config.mem_per_frame;
		config.max_overall_mem = default_config.max_overall_mem;
    }

    if(config.min_mem_per_proc < 64 || config.min_mem_per_proc > 65536 || !is_power_of_2(config.min_mem_per_proc) || config.min_mem_per_proc > config.max_overall_mem) {
        std::cerr << "ERROR: min-mem-per-proc must be a power of 2 between 64 and 65536, and <= max_overall_mem; using default...\n";
        config.min_mem_per_proc = default_config.min_mem_per_proc;
		if (config.min_mem_per_proc > config.max_overall_mem) {
			config.max_overall_mem = default_config.max_overall_mem;
			config.max_mem_per_proc = default_config.max_mem_per_proc;
		}
    }

    if(config.max_mem_per_proc < 64 || config.max_mem_per_proc > 65536 || !is_power_of_2(config.max_mem_per_proc) || config.max_mem_per_proc > config.max_overall_mem) {
        std::cerr << "ERROR: max-mem-per-proc must be a power of 2 between 64 and 65536, and <= max_overall_mem; using default...\n";
        config.max_mem_per_proc = default_config.max_mem_per_proc;
		if (config.max_mem_per_proc > config.max_overall_mem) {
			config.max_overall_mem = default_config.max_overall_mem;
		}
    }

    if(config.min_mem_per_proc > config.max_mem_per_proc) {
        std::cerr << "ERROR: min-mem-per-proc > max-mem-per-proc; using default...\n";
        config.min_mem_per_proc = default_config.min_mem_per_proc;
        config.max_mem_per_proc = default_config.max_mem_per_proc;
    }
}

namespace {
bool file_exists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

std::string resolve_config_path(const std::string& filename) {
    std::vector<std::string> candidates;

    if (!filename.empty() && filename[0] == '/' || filename.size() > 1 && filename[1] == ':') {
        candidates.push_back(filename);
    } else {
        candidates.push_back(filename);
        candidates.push_back("./" + filename);
        candidates.push_back("../" + filename);
        candidates.push_back("../../" + filename);
        candidates.push_back("config.txt");
        candidates.push_back("../config.txt");
        candidates.push_back("../../config.txt");
    }

    for (const auto& candidate : candidates) {
        if (file_exists(candidate)) {
            return candidate;
        }
    }

    return {};
}
} // namespace

void loadConfig(const std::string& filename, Config& config) {
    const std::string resolved_path = resolve_config_path(filename);
    std::ifstream file(resolved_path.empty() ? filename : resolved_path);

    // Anchor every generated file to config.txt's own directory.
    if (!resolved_path.empty()) {
        size_t slash = resolved_path.find_last_of("/\\");
        config.base_dir = (slash == std::string::npos)
                              ? std::string()
                              : resolved_path.substr(0, slash + 1);
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open config file '" << filename << "', using default...\n";
        validateConfig(config);
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

        // The spec's sample config quotes string values, e.g. `scheduler "fcfs"`.
        // Without stripping them the value reads as "\"fcfs\"", fails validation,
        // and silently falls back to the default scheduler - which cannot be
        // corrected at grading time, since the quiz forbids recompiling.
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        try {
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
            // The spec's parameter table writes "delays-per-exec" but the handed-out
            // test-case configs write "delay-per-exec". The quiz forbids recompiling
            // between questions, so both spellings must be accepted - otherwise the
            // value silently falls back to the default and the delay test is unfixable.
            else if (key == "delays-per-exec" || key == "delay-per-exec") {
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
        } catch (...) {
            std::cerr << "ERROR: Invalid numeric value for " << key << ". Using default...\n";
        }
    }

    validateConfig(config);
}