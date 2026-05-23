#include <iostream>
#include "ui.h"
#include "constants.h"

void printIntro() {

    // Title Card
    std::cout << R"(
     ______   ______   ______   ______   ______   ______   __  __    
    /_____/\ /_____/\ /_____/\ /_____/\ /_____/\ /_____/\ /_/\/_/\   
    \:::__\/ \::::_\/_\:::_ \ \\:::_ \ \\::::_\/_\::::_\/_\ \ \ \ \  
     \:\ \  __\:\/___/\\:\ \ \ \\:(_) \ \\:\/___/\\:\/___/\\:\_\ \ \ 
      \:\ \/_/\\_::._\:\\:\ \ \ \\: ___\/ \::___\/_\_::._\:\\::::_\/ 
       \:\_\ \ \ /____\:\\:\_\ \ \\ \ \    \:\____/\ /____\:\ \::\ \ 
        \_____\/ \_____\/ \_____\/ \_\/     \_____\/ \_____\/  \__\/ 

    )" << std::endl;

    // Instructions
    std::cout << Colors::GREEN
        << "Hello, welcome to CSOPESY commandline!\n";

    std::cout << Colors::LIGHT_YELLOW
        << "Type 'exit' to quit or 'clear' to clear the screen.\n\n"
        << "** IMPORTANT: Type 'initialize' to load config and start system **\n\n";

    std::cout << Colors::WHITE << "\n";
}