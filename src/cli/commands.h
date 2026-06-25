// Contains logic for the commands, mostly composed of calls to other components
// figures out which kernel function mag-eexecute ng command

#pragma once

#include <string>

class Kernel;

class Commands {
public:
    explicit Commands(Kernel* kernel);

    void initialize();
    void exit();

    void screenCreate(const std::string& processName);
    void screenResume(const std::string& processName);
    void screenList();

    void schedulerStart();
    void schedulerStop();

    void reportUtil();

private:
    Kernel* kernel_;
};