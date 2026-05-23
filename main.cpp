#include <iostream>
#include "constants.h"
#include "ui.h"
#include "commands.h"

int main()
{
    std::string command;
    bool validCommand;
    
	// Print title card and instructions
    printIntro();

    // Require initialization first
    while (command != "initialize") {
        std::cout << "Enter a command: ";
        std::cin >> command;

        validCommand = isValidCommand(command);

        if (command == "initialize") {
            std::cout << Colors::LIGHT_BLUE << command << Colors::WHITE << " command recognized. Doing something . . .\n\n";
        }
        else if (validCommand) {
            std::cout << Colors::RED
                << "Initialize first. Please try again.\n"
                << Colors::WHITE
                << "\n";
        }
        else {
			std::cout << Colors::RED
				<< "Invalid command. Please try again.\n"
				<< Colors::WHITE
                << "\n";
        }
    }

	// Main command loop
    while (command != "exit") {
        std::cout << "Enter a command: ";
        std::cin >> command;

        validCommand = isValidCommand(command);

        if (validCommand) {
            std::cout << Colors::LIGHT_BLUE << command << Colors::WHITE << " command recognized. Doing something . . .\n\n";
        }
        else {
            std::cout << Colors::RED
                << "Invalid command. Please try again.\n"
                << Colors::WHITE
				<< "\n";
        }

        if (command == "clear") {
            system("CLS");
            printIntro();
        }
    }

	// Exit message
    std::cout << Colors::LIGHT_YELLOW
        << "Exiting CSOPESY commandline. Goodbye!\n"
        << Colors::WHITE;

    return 0;
}