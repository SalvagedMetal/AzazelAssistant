#include "src/dateTime.h"
#include "src/model.h"
#include "src/functionCall.h"
#include "src/configReader.h"
#include "src/mqtt.h"

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
    // Load configuration
    ConfigReader configReader;
    try {
        configReader.readConfig("config.json", isVerbose); 
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }

    // Initialize MQTT client
    ConfigVars::MQTTConfig mqttConfig = configReader.getMQTT().at(0);
    MQTTClient client;
    client.setVerbose(isVerbose);
    if (mqttConfig.enabled) {
        if (isVerbose) {
            std::cout << "Initializing MQTT client..." << std::endl;
        }
        try {
            client.Init(mqttConfig.username, mqttConfig.password, mqttConfig.client_id, mqttConfig.clean_session);
            client.Start(mqttConfig.broker_ip, mqttConfig.broker_port, mqttConfig.keepalive);
        } catch (const std::exception &e) {
            std::runtime_error("Error initializing MQTT client: " + std::string(e.what()));
            return 1;
        }
    }
    
    
    MQTTQueue<std::string> queue;

    // Initialize the models
    Model commandModel;
    Model chatModel;
    for (const auto& modelConfig : configReader.getModels()) {
        if (modelConfig.purpose == "Command") {
            commandModel.setModelName(modelConfig.name);
            commandModel.setModelPurpose(modelConfig.purpose);
            commandModel.setModelPath(modelConfig.path);
            commandModel.setNGL(modelConfig.ngl);
            commandModel.setNCTX(modelConfig.n_ctx);
            commandModel.setInitMessage(modelConfig.init_message);
            commandModel.setTemp(modelConfig.temp);
            commandModel.setMinP(modelConfig.min_p);
            commandModel.setTopP(modelConfig.top_p);
            commandModel.setTypical(modelConfig.typical);
            commandModel.setDist(modelConfig.dist);
            commandModel.setTopK(modelConfig.top_k);
            commandModel.setKeepHistory(modelConfig.keepHistory);
            commandModel.setVerbose(isVerbose);
            if (isVerbose) {
                std::cout << "Command Model initialized: " << commandModel.getModelName() << std::endl;
            }
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
            if (isVerbose) {
                std::cout << "Chat Model initialized: " << chatModel.getModelName() << std::endl;
            }
        }
    }
    try {
        commandModel.init();
        chatModel.init();
    } catch (const std::exception &e) {
        std::cerr << "Error initializing command model: " << e.what() << std::endl;
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

        if (isVerbose)
            std::cout << commandString << std::endl;

        try {
            isValid = FunctionCall::isValidCommand(commandString, isVerbose);
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
