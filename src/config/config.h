#pragma once
#include <string>
#include <cstdint>
constexpr uint64_t MAX_CONFIG_VALUE = 4294967296ULL;

struct Config {
    int num_cpu = 2;
    std::string scheduler = "rr";
    uint64_t quantum_cycles = 4;
    uint64_t batch_process_freq = 1;
    uint64_t min_ins = 100;
    uint64_t max_ins = 100;
    uint64_t delays_per_exec = 0;

    // Memory manager configuration
    uint64_t max_overall_mem = 16384;  // total bytes of main memory
    uint64_t mem_per_frame = 16;       // bytes per frame (reserved for paging allocators)
    uint64_t mem_per_proc = 4096;      // fixed bytes required by every process
};

void validateConfig(Config& config);
void loadConfig(const std::string& filename, Config& config);