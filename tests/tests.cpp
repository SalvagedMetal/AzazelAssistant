#include <iostream>

#include "testDateTime.h"
#include "testModel.h"
#include "testFunctionCall.h"

int main(int argc, char *argv[]) {
    try { 
        std::cout << "Running DateTime tests..." << std::endl;
        testGetCurrentDateTime();
        testGetDateTime();
        testGetCurrentTimestamp();
        testGetTimestamp();
        testFindFormat();
    } catch (const std::exception& e) {
        std::cerr << "DateTime Test failed: " << e.what() << std::endl;
        return 1; // Return a non-zero value to indicate failure
    }
    try {
        std::cout << "Running Model tests..." << std::endl;
        testModelInitialization();
        testModelResponse();
        testModelChatHistory();
        testModelClearHistory();
    } catch (const std::exception& e) {
        std::cerr << "Model Test failed: " << e.what() << std::endl;
        return 1; // Return a non-zero value to indicate failure
    }
    try {
        std::cout << "Running FunctionCall tests..." << std::endl;
        testIsValidCommand();
        testCallFunction();
    } catch (const std::exception& e) {
        std::cerr << "FunctionCall Test failed: " << e.what() << std::endl;
        return 1; // Return a non-zero value to indicate failure
    }
    std::cout << "All tests passed!" << std::endl;
    return 0;
}