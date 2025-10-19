#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <string>
#include <mutex>
#include <condition_variable>

class MQTTClient {
private:
    struct mosquitto *mosq = nullptr;
    int rc = 0;
    std::string subscribedMessage = "";
    static std::mutex mtx;
    static std::condition_variable cv;
    bool messageFlag = false;

    // Callback called when the client receives a CONNACK message from the broker.
    static void onConnect(struct mosquitto *mosq, void *obj, int reason_code);

    // Callback called when the client knows to the best of its abilities that a PUBLISH has been successfully sent.
    static void onPublish(struct mosquitto *mosq, void *obj, int mid);

    static void onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

public:
    MQTTClient() = default;
    ~MQTTClient() { Destroy(); };
    // This function pretends to read some data from a sensor and publish it.
    void Publish(int *mid, const uint8_t* payload, int payloadLen, const std::string& topic, int qos, bool retain);

    // This function subscribes to a topic and waits for a message.
    std::string Subscribe(const char* topic, int qos);

    // starting
    void Init(std::string usr, std::string pwd, std::string id, bool cleanSession);
    void Start(std::string ip, int port, int keepalive);

    void Stop(bool force);
    void Disconnect();
    void Destroy();

};

namespace MQTTTask {
    std::string setLight(bool state);
    std::string getTest();
};

#endif