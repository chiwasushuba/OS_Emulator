#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
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

    if(config.mem_per_proc < 1 || config.mem_per_proc > config.max_overall_mem) {
        std::cerr
            << "ERROR: mem-per-proc must be [1, max-overall-mem]; using default instead...\n";

        config.mem_per_proc = default_config.mem_per_proc;
    }
}

void loadConfig(const std::string& filename, Config& config) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open config file, using default...\n";
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
        else if (key == "mem-per-proc") {
            config.mem_per_proc = std::stoull(value);
        }
        else {
            std::cerr << "Unknown parameter: " << key << '\n';
        }
    }

    validateConfig(config);
}