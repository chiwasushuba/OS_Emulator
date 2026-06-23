#pragma once
#include <string>
#include <cstdint>
constexpr uint64_t MAX_CONFIG_VALUE = 4294967296ULL;

struct Config {
    int num_cpu = 1;
    std::string scheduler = "fcfs";
    uint64_t quantum_cycles = 4;
    uint64_t batch_process_freq = 8;
    uint64_t min_ins = 1;
    uint64_t max_ins = 16;
    uint64_t delays_per_exec = 0;
};

void validateConfig(Config& config);
void loadConfig(const std::string& filename, Config& config);