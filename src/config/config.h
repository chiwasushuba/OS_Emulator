#pragma once
#include <string>
#include <cstdint>
constexpr uint64_t MAX_CONFIG_VALUE = 4294967296ULL;

struct Config {
    int num_cpu = 4;
    std::string scheduler = "rr";
    uint64_t quantum_cycles = 5;
    uint64_t batch_process_freq = 1;
    uint64_t min_ins = 1000;
    uint64_t max_ins = 2000;
    uint64_t delays_per_exec = 0;
};

void validateConfig(Config& config);
void loadConfig(const std::string& filename, Config& config);