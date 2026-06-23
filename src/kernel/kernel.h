#pragma once

class Kernel {
    private:
        bool is_running;
        void main_loop();
    public:
        Kernel();
        void start();
};