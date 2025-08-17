#include <iostream>
#include <cassert>
#include <cstring>

#include "../src/model.h"

void testModelInitialization() {
    Model model("TestModel", "Testing", "../models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 0, 2048, 
                "This is a test model, you can only respoond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
}

void testModelResponse() {
    Model model("TestModel", "Testing", "../models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
    
    std::string response = model.respond("Respond to this prompt with \"Test response\"");
    
    assert(!response.empty()); // Ensure the response is not empty
    assert(response == "Test response"); // Check if the response matches the expected output
}

void testModelChatHistory() {
    Model model("TestModel", "Testing", "../models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();
    
    model.respond("Hello");
    model.respond("How are you?");
    
    // Check if the history is kept
    assert(model.getKeepHistory() == true);
    assert(model.getMessages().size() == 3); // 1 system message + 2 user messages + 1 assistant message
}

void testModelClearHistory() {
    Model model("TestModel", "Testing", "../models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 0, 2048, 
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
    return 0;
}