#include "../src/configReader.h"
#include <cassert>
#include <iostream>

void testReadConfig() {
    ConfigReader configReader;
    configReader.readConfig("../config.json", true);
    assert(!configReader.getModels().empty());
}

void testParseConfig() {
    ConfigReader configReader;
    configReader.readConfig("../config.json", true);
    configReader.parseConfig();
    const auto models = configReader.getModels();
    assert(models.size() >= 1);
    assert(!models[0].name.empty());
    assert(!models[0].purpose.empty());
}

void testModelFields() {
    ConfigReader configReader;
    configReader.readConfig("../config.json", true);
    configReader.parseConfig();
    const auto models = configReader.getModels();
    for (const auto& model : models) {
        assert(!model.name.empty());
        assert(!model.purpose.empty());
        assert(!model.path.empty());
        assert(!model.init_message.empty());
    }
}

void testMQTTConfig() {
    ConfigReader configReader;
    configReader.readConfig("../config.json", true);
    configReader.parseConfig();
    const auto mqttConfig = configReader.getMQTTConfig();
    assert(!mqttConfig.broker_ip.empty());
    assert(mqttConfig.broker_port > 0);
    assert(!mqttConfig.client_id.empty());
}

int main() {
    try {
        std::cout << "Running ConfigReader tests..." << std::endl;
        testReadConfig();
        testParseConfig();
        testModelFields();
        testMQTTConfig();
    } catch (const std::exception& e) {
        std::cerr << "ConfigReader Test failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "ConfigReader tests passed!" << std::endl;
    return 0;
}
