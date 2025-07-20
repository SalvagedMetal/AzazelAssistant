#include "testFunctionCall.h"

void testIsValidCommand() {
    std::string validCommand = R"({"command": "chat", "arguments": ["Hello, how are you?"]})";
    std::string invalidCommand = R"({"command": "chat", "arguments": ["Hello, how are you?", "extra argument"]})";

    assert(FunctionCall::isValidCommand(validCommand, true) == true);
    assert(FunctionCall::isValidCommand(invalidCommand, true) == false);
}

void testCallFunction() {
    Model model("TestModel", "Testing", "models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf", 0, 2048, 
                "This is a test model, you can only respond what is explicitly given to you.", 0.5f, 0.1f, 0.9f, 0.9f, 0.9f, 10, true, true);
    model.init();

    std::string command = R"({"command": "chat", "arguments": ["Respond to this prompt with: This is a test response"]})";
    std::string response = FunctionCall::call(command, model, true);
    
    assert(!response.empty()); // Ensure the response is not empty
    assert(response == "This is a test response"); // Check if the response matches the expected output
}