#include <thread>

class Kernel {
    private:
        bool is_running;

        void main_loop() {
            while(this->is_running) {
                // increment tick for cpu and scheduler
            }
        }
    public:
        Kernel() {
            this->is_running = false;
        }

        void start() {
            this->is_running = true;

            // Load config variables

            // Pass config variables to:
            // initialize scheduler
            // initialize cpu manager

            std::thread clock_thread(&Kernel::main_loop, this);
        }
};