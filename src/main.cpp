#include <iostream>
#include <kernel.h>

int main() {
    std::cout << "Initializing Kernel...\n";
    Kernel kernel;
    kernel.start();
    return 0;
}