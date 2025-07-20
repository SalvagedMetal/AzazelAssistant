#include <iostream>
#include <chrono>
#include <thread>

#include "src/dateTime.h"
#include "src/model.h"
#include "src/functionCall.h"

//using namespace std::chrono_literals;

int main(int argc, char *argv[]) {

    std::string response;
    std::string commandString;
    std::string userInput;
    bool isValid = false;
    bool isVerbose = false;

    //Processing command line arguments
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            std::cout << "Usage: AzazelAssistant [options]\n"
                      << "Options:\n"
                      << "  --help, -h       Show this help message\n"
                      << "  --versbose, -v    Enable verbose output\n";
            return 0;
        } else if (std::string(argv[i]) == "--verbose" || std::string(argv[i]) == "-v") {
            std::cout << "Verbose mode enabled\n";
            isVerbose = true;
        }
    }

    // Initialize the models
    Model commandModel;
    commandModel.setVerbose(isVerbose);
    try {
        commandModel.init();
    } catch (const std::exception &e) {
        std::cerr << "Error initializing command model: " << e.what() << std::endl;
        return 1;
    }

    Model chatModel("Phi-4-mini-instruct-Q6_K_L", 
                    "Chatbot", 
                    "models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 
                    0, 2048, 
                    "You are Azazel, a helpful assistant. Respond in a friendly manner and provide useful information.", 
                    0.60f, 0.05f, 0.95f, 0.95f, 0.95f, 40, true, isVerbose);
    try {
        chatModel.init();
    } catch (const std::exception &e) {
        std::cerr << "Error initializing chat model: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Azazel Assistant is running...\n";

    // Main loop to process user input
    while (true) {    
        std::cout << "> ";
        getline(std::cin, userInput);
        try {
            commandString = commandModel.respond(userInput);
        } catch (const std::exception &e) {
            std::cerr << "Error processing input: " << e.what() << std::endl;
        }

        if (isVerbose) {
            std::cout << commandString << std::endl;
        }

        try {
            isValid = FunctionCall::isValidCommand(commandString, isVerbose);
        } catch (const std::exception &e) {
            std::cerr << "Error validating command: " << e.what() << std::endl;
            return 1;
        }
        try {
            if (isValid) {
                try {
                    response = FunctionCall::call(commandString, chatModel, isVerbose);
                } catch (const std::exception &e) {
                    std::cerr << "Error executing command: " << e.what() << std::endl;
                    return 1;
                }
                std::cout << response << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error executing command: " << e.what() << std::endl;
            return 1;
        }
    }
    
    return 0;
}
