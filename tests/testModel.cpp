#include <iostream>
#include <cassert>
#include <cstring>

#include "../src/model.h"
#include "../src/configReader.h"

void testModelInitialization(std::string modelPath) {
    Model model("TestModel", "Testing", "../" + modelPath, 0, 2048, 
                "This is a test model, you can only respoond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
}

void testModelResponse(std::string modelPath) {
    Model model("TestModel", "Testing", "../" + modelPath, 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
    
    std::string response = model.respond("Respond to this prompt with \"Test response\"");
    
    assert(!response.empty()); // Ensure the response is not empty
    assert(response == "Test response"); // Check if the response matches the expected output
}

void testModelChatHistory(std::string modelPath) {
    Model model("TestModel", "Testing", "../" + modelPath, 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
    
    model.respond("Hello");
    model.respond("How are you?");
    
    // Check if the history is kept
    assert(model.getKeepHistory() == true);
    assert(model.getMessages().size() == 3); // 1 system message + 2 user messages + 1 assistant message
}

void testModelClearHistory(std::string modelPath) {
    Model model("TestModel", "Testing", "../" + modelPath, 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
    
    model.respond("Hello");
    model.respond("How are you?");
    
    // Clear the history
    model.clearHistory();
    
    // Check if the history is cleared
    assert(model.getKeepHistory() == true);
    assert(model.getMessages().size() == 1); // Only the system message should remain
}

int main(int argc, char *argv[]) {
    ConfigReader configReader;
    try {
        configReader.readConfig("../config.json");
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    try {
        std::cout << "Running Model tests..." << std::endl;
        testModelInitialization(configReader.getModels()[0].path);
        testModelResponse(configReader.getModels()[0].path);
        testModelChatHistory(configReader.getModels()[0].path);
        testModelClearHistory(configReader.getModels()[0].path);
    } catch (const std::exception& e) {
        std::cerr << "Model Test failed: " << e.what() << std::endl;
        return 1; // Return a non-zero value to indicate failure
    }
    return 0;
}