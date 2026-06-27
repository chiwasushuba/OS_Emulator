#pragma once
#include <queue>
#include <vector>
#include "os_process.h"
#include "cpu.h"

class Scheduler {
protected:
    CPUManager& cpu_manager;
    ProcessManager& process_manager;
public:
    Scheduler(CPUManager& cpu_m, ProcessManager& proc_m) 
        : cpu_manager(cpu_m), process_manager(proc_m) {}
    
    virtual ~Scheduler() = default;

    // Called when a new process is created
    virtual void add_process(Process* process) = 0;

    // Called every CPU tick to perform scheduling (assigning processes to cores)
    virtual void tick() = 0;
};

class RoundRobinScheduler : public Scheduler {
private:
    std::queue<Process*> ready_queue;
    uint64_t quantum;
    std::vector<uint64_t> core_cycles;

public:
    RoundRobinScheduler(CPUManager& cpu_m, ProcessManager& proc_m, uint64_t quantum);
    
    void add_process(Process* process) override;
    void tick() override;
};

class FCFSScheduler : public Scheduler {
private:
    std::queue<Process*> ready_queue;

public:
    FCFSScheduler(CPUManager& cpu_m, ProcessManager& proc_m);
    
    void add_process(Process* process) override;
    void tick() override;
};