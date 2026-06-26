// Contains logic for the commands, mostly composed of calls to other components
// figures out which kernel function mag-eexecute ng command

#pragma once

#include <string>

class Kernel;
class Console;

class Commands {
public:
    explicit Commands(Kernel* kernel);

    void setConsole(Console* console);

    void initialize();
    void exit();

    void screenCreate(const std::string& processName);
    void screenResume(const std::string& processName);
    void screenList();

    void schedulerStart();
    void schedulerStop();

    void reportUtil();
    void help();

private:
    Kernel* kernel_;
    Console* console_ = nullptr;
};