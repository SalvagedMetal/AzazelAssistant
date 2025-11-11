#include <iostream>
#include <cassert>

#include "../src/mqtt.h"
#include "../src/configReader.h"

ConfigReader configReader;


void testMQTTInitialization(std::string user, std::string password, std::string clientId) {
    MQTTClient mqttClient;
    mqttClient.Init(user, password, clientId, true);
    assert(mqttClient.isInitialized() == true);
}

void testMQTTStartStop(std::string ip, int port, int keepalive, std::string user, std::string password, std::string clientId) {
    MQTTClient mqttClient;
    mqttClient.Init(user, password, clientId, true);
    mqttClient.Start(ip, port, keepalive);
    mqttClient.Stop(false);

    assert(true); // If no exceptions were thrown, the test passes
}

void testConnectionFailure(std::string ip, int port, int keepalive, std::string user, std::string password, std::string clientId) {
    MQTTClient mqttClient;
    mqttClient.Init(user, password, clientId, true);
    try {
        mqttClient.Start(ip, port, keepalive);
    } catch (const std::exception &e) {
        std::cerr << "Expected connection failure: " << e.what() << std::endl;
        return;
    }
    assert(false); // Should not reach here
}

void testMQTTPublishSubscribe(std::string ip, int port, int keepalive, std::string user, std::string password, std::string clientId) {
    MQTTClient mqttClient;
    mqttClient.Init(user, password, clientId, true);
    mqttClient.Start(ip, port, keepalive);

    const std::string testTopic = "test/topic";
    const std::string testPayload = "Hello MQTT";

    int mid = 0;
    mqttClient.Publish(&mid, testPayload, testPayload.size(), testTopic, 1, false);
    std::string receivedMessage = mqttClient.Subscribe(testTopic, 1);

    assert(receivedMessage == testPayload);

    mqttClient.Stop(false);
}

int main() {
    try {
        configReader.readConfig("../config.json", true);
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    ConfigVars::MQTTConfig mqttConfig = configReader.getMQTTConfig();
    if (!mqttConfig.enabled) {
        std::cout << "MQTT is not enabled in the configuration." << std::endl;
        return 0;
    }

    try {
        std::cout << "Running MQTT tests..." << std::endl;
        testMQTTInitialization(mqttConfig.username, mqttConfig.password, mqttConfig.client_id);
        testMQTTStartStop(mqttConfig.broker_ip, mqttConfig.broker_port, mqttConfig.keepalive,
                          mqttConfig.username, mqttConfig.password, mqttConfig.client_id);
        testConnectionFailure("invalid.broker.ip", 1883, 60,
                              mqttConfig.username, mqttConfig.password, mqttConfig.client_id);
        testMQTTPublishSubscribe(mqttConfig.broker_ip, mqttConfig.broker_port, mqttConfig.keepalive,
                                 mqttConfig.username, mqttConfig.password, mqttConfig.client_id);
    } catch (const std::exception& e) {
        std::cerr << "MQTT Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}