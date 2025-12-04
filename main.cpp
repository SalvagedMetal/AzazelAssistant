#include "src/model.h"
#include "src/functionCall.h"
#include "src/mqtt.h"
#include "src/configReader.h"

int main(int argc, char *argv[]) {

    std::string response;
    std::string commandString;
    std::string userInput, input;
    std::string commandInitMessage;
    bool isVerbose = false;
    bool retry = false;

    ConfigVars::config config;
    ConfigVars::MQTTConfig mqttConfig;
    MQTTClient client;
    Model commandModel;
    Model chatModel;
    std::unique_ptr<FunctionCall::ParsedPhrase> parsedPhrasePtr = nullptr;
    ConfigReader configReader;

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
    // Load configuration
    try {
        configReader.readConfig("config.json", isVerbose); 
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    config = configReader.getConfig();

    // Initialize MQTT client
    mqttConfig = configReader.getMQTTConfig();

    if (mqttConfig.enabled) {
        client.setVerbose(isVerbose);
        if (mqttConfig.enabled) {
            if (isVerbose) std::cout << "Initializing MQTT client..." << std::endl;
            try {
                client.Init(mqttConfig.username, mqttConfig.password, mqttConfig.client_id, mqttConfig.clean_session);
                if (client.isInitialized()) {
                    client.Start(mqttConfig.broker_ip, mqttConfig.broker_port, mqttConfig.keepalive);
                } else {
                    throw std::runtime_error("MQTT client initialization failed.");
                }
            } catch (const std::exception &e) {
                std::cerr << "Error initializing MQTT client: " << e.what() << std::endl;
                return 1;
            }
        }
    }

    // Initialize the models
    // Only initialize models if enabled in config
    if (config.ModelEnable) {
        // Phrases in a single string to add to the model init message
        std::string phrasesString = "";
        for (const auto& command : configReader.getCommandCalls()) {
            for (const auto& phrase : command.phrases) {
                phrasesString += phrase + "\n";
            }
        }
        std::vector<ConfigVars::Model> commandModelConfig = configReader.getModels();
        if (commandModelConfig.empty()) {
            std::cerr << "No models defined in configuration." << std::endl;
            return 1;
        }
        for (const auto& modelConfig : commandModelConfig) {
            if (modelConfig.purpose == "Command") {
                // Prepend phrases to init message
                commandInitMessage = modelConfig.init_message + "\n" + phrasesString;
            }
        }

        for (const auto& modelConfig : configReader.getModels()) {
            if (modelConfig.purpose == "Command") {
                commandModel.setModelName(modelConfig.name);
                commandModel.setModelPurpose(modelConfig.purpose);
                commandModel.setModelPath(modelConfig.path);
                commandModel.setNGL(modelConfig.ngl);
                commandModel.setNCTX(modelConfig.n_ctx);
                commandModel.setInitMessage(commandInitMessage);
                commandModel.setTemp(modelConfig.temp);
                commandModel.setMinP(modelConfig.min_p);
                commandModel.setTopP(modelConfig.top_p);
                commandModel.setTypical(modelConfig.typical);
                commandModel.setDist(modelConfig.dist);
                commandModel.setTopK(modelConfig.top_k);
                commandModel.setKeepHistory(modelConfig.keepHistory);
                commandModel.setVerbose(isVerbose);
                if (isVerbose) std::cout << "Command Model initialized: " << commandModel.getModelName() << std::endl;
            } else if (modelConfig.purpose == "Chat") {
                chatModel.setModelName(modelConfig.name);
                chatModel.setModelPurpose(modelConfig.purpose);
                chatModel.setModelPath(modelConfig.path);
                chatModel.setNGL(modelConfig.ngl);
                chatModel.setNCTX(modelConfig.n_ctx);
                chatModel.setInitMessage(modelConfig.init_message);
                chatModel.setTemp(modelConfig.temp);
                chatModel.setMinP(modelConfig.min_p);
                chatModel.setTopP(modelConfig.top_p);
                chatModel.setTypical(modelConfig.typical);
                chatModel.setDist(modelConfig.dist);
                chatModel.setTopK(modelConfig.top_k);
                chatModel.setKeepHistory(modelConfig.keepHistory);
                chatModel.setVerbose(isVerbose);
                if (isVerbose) std::cout << "Chat Model initialized: " << chatModel.getModelName() << std::endl;
            }
        }
        try {
            commandModel.init();
            chatModel.init();
        } catch (const std::exception &e) {
            std::cerr << "Error initializing command model: " << e.what() << std::endl;
            return 1;
        }
    }

    // Initialize function calls
    try {
        if (isVerbose) std::cout << "Initializing function calls..." << std::endl;
            FunctionCall::initCommands(config, &client, &chatModel, isVerbose);
    } catch (const std::exception &e) {
        std::cerr << "Error initializing function calls: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Azazel Assistant is running...\n";
    // Main loop
    while (true) {
        if (!retry || !config.ModelEnable) {
            std::cout << "> ";
            getline(std::cin, userInput);
            input = userInput;
            if (userInput == "quit" || userInput == "q") break;
        } else {
            if (isVerbose) std::cout << "Retrying with AI parsed command..." << std::endl;
            try {
                    input = commandModel.respond(userInput);
            } catch (const std::exception &e) {
                std::cerr << "Error generating command from AI: " << e.what() << std::endl;
                return 1;
            }
            if (isVerbose) std::cout << "AI parsed command: " << input << std::endl;
        }
        if (FunctionCall::parsePhrase(input, parsedPhrasePtr, configReader.getCommandCalls(), isVerbose)) {
            if (isVerbose) {
                std::cout << "Command: " << parsedPhrasePtr->command << std::endl;
                for (const auto& arg : parsedPhrasePtr->arguments) {
                    std::cout << "Argument: " << arg << std::endl;
                }
            }
            std::cout << FunctionCall::call(parsedPhrasePtr, chatModel, client, config, isVerbose) << std::endl;
            retry = false;
        } else if (config.ModelEnable && !retry) {
            retry = true;
        } else {
            std::cout << "Could not parse command." << std::endl;
            retry = false;
        }
        input = "";
        parsedPhrasePtr = nullptr;
    }
    return 0;
}
