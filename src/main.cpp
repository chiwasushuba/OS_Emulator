#include <iostream>
#include <kernel.h>

int main() {
    // Bootstrap
    Kernel kernel;
    kernel.start();
    return 0;
}