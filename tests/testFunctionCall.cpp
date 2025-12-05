#include <iostream>
#include <cassert>
#include <cstring>

#include "../src/functionCall.h"
#include "../src/mqtt.h"
#include "../src/model.h"
#include "../src/configVars.h"
#include "../src/dateTime.h"
#include "../src/configReader.h"

void testInitCommands(ConfigVars::config config) {
    MQTTClient mqttClient;
    Model model;

    FunctionCall::initCommands(config, &mqttClient, &model, true);
    assert(!FunctionCall::commandList.empty());
}

void testParsedPhraseCreation(ConfigVars::config config) {
    std::unique_ptr<FunctionCall::ParsedPhrase> parsedPhrase = nullptr;
    std::string testPhrase = "what time is it";

    FunctionCall::parsePhrase(testPhrase, parsedPhrase, config.commandCalls, true);
    assert(parsedPhrase != nullptr);
    assert(parsedPhrase->command == "getCurrentDateTime");
    assert(parsedPhrase->arguments.at(0) == "time");
}

void testCheckTypo() {
    std::string str1 = "hello";
    std::string str2 = "hallo";
    bool result = FunctionCall::checkTypo(str1, str2, FunctionCall::ratio, true);
    assert(result == true);
}

void testCallFunction(ConfigVars::config config, Model& model, MQTTClient& mqttClient) {
    FunctionCall::initCommands(config, &mqttClient, &model, true);
    std::unique_ptr<FunctionCall::ParsedPhrase> parsedPhrase = std::make_unique<FunctionCall::ParsedPhrase>();
    std::vector<FunctionCall::ParsedPhrase> testPhrases;
    std::string expectedString;
    std::vector<std::string> testResponses = {
        "It is ",
        "Today is ",
        "Today is ",
        "It is ",
        "Publish successful",
        "Hello MQTT",
        "Test response"
    };

    testPhrases.push_back({"getCurrentDateTime", {"time"}});
    testPhrases.push_back({"getCurrentDateTime", {"date"}});
    testPhrases.push_back({"getCurrentDateTime", {"day"}});
    testPhrases.push_back({"getDateTime", {"time", "3", "hours"}});
    if (config.mqtt.enabled) {
        testPhrases.push_back({"testPublish", {}});
        testPhrases.push_back({"testSubscribe", {}});
    }
    if (config.ModelEnable) {
        testPhrases.push_back({"chat", {"Respond only with 'Test response'"}});
    }

    for (auto testPhrase : testPhrases) {
        parsedPhrase->command = testPhrase.command;
        parsedPhrase->arguments = testPhrase.arguments;

        std::string result = FunctionCall::call(parsedPhrase, model, mqttClient, config, true);
        if (parsedPhrase->command == "getCurrentDateTime") {
            if (parsedPhrase->arguments[0] == "time") {
                expectedString = "It is " + DateTime::getDateTime(DateTime::getCurrentTimestamp(), DTFormat::HHMMSS24);
            } else if (parsedPhrase->arguments[0] == "date") {
                expectedString = "Today is " + DateTime::getDateTime(DateTime::getCurrentTimestamp(), DTFormat::DDMMYYYY);
            } else if (parsedPhrase->arguments[0] == "day") {
                expectedString = "Today is " + DateTime::getDateTime(DateTime::getCurrentTimestamp(), DTFormat::Day);
            }
        } else if (parsedPhrase->command == "getDateTime") {
            int offset = std::stoi(parsedPhrase->arguments[1]);
            time_t timeStamp = DateTime::getCurrentTimestamp() + offset * 3600;
            expectedString = "It is " + DateTime::getDateTime(timeStamp, DTFormat::HHMMSS24);
        }

        assert(!result.empty());
        assert(result == expectedString);
    }
}

int main(int argc, char *argv[]) {
    ConfigVars::config config;
    ConfigReader configReader;
    Model model;
    MQTTClient mqttClient;
    std::string modelPath = "../models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf";
    try {
        configReader.readConfig("../config.json", true); 
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    config = configReader.getConfig();
    try {
        for (const auto& modelVar : config.models) {
            if (modelVar.purpose == "Chat") {
                model = Model(modelVar.name, modelVar.purpose, modelPath,
                              modelVar.ngl, modelVar.n_ctx,
                              modelVar.init_message,
                              modelVar.temp, modelVar.min_p,
                              modelVar.top_p, modelVar.typical,
                              modelVar.dist, modelVar.top_k,
                              modelVar.keepHistory, true);
                break;
            }
        }
        model.init();
    } catch (const std::exception &e) {
        std::cerr << "Error initializing model: " << e.what() << std::endl;
        return 1;
    }
    try {
        mqttClient.Init("testUser", "testPassword", "testClientId", true);
    } catch (const std::exception &e) {
        std::cerr << "Error initializing MQTT client: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Running FunctionCall tests..." << std::endl;
    try {
        testInitCommands(config);
        testParsedPhraseCreation(config);
        testCheckTypo();
        testCallFunction(config, model, mqttClient);
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
   
    return 0;
}