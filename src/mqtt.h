#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>
#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

extern std::atomic<bool> mqttRunning;

class MQTTClient {
private:
    struct mosquitto *mosq = nullptr;
    int rc = 0;
    std::string subscribedMessage = "";
    static std::mutex mtx;
    static std::condition_variable cv;
    bool messageFlag = false;
    static bool isVerbose;
    static std::thread mqttWorkerThread;
    bool initialized = false;

    // Callback called when the client receives a CONNACK message from the broker.
    static void onConnect(struct mosquitto *mosq, void *obj, int reason_code);

    // Callback called when the client knows to the best of its abilities that a PUBLISH has been successfully sent.
    static void onPublish(struct mosquitto *mosq, void *obj, int mid);

    // Callback called when a message is received from the broker.
    static void onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

public:
    MQTTClient() = default;
    ~MQTTClient() { Destroy(); };
    // This function publishes a message to a topic.
    /*
        int *mid                        || Message ID pointer
        const std::string& payload      || Message payload
        int payloadLen                  || Length of the payload
        const std::string& topic        || Topic to publish to
        int qos                         || Quality of Service level
        bool retain                     || Whether to retain the message
        returns                         || Confirmation string
    */
    std::string Publish(int *mid, const std::string& payload, int payloadLen, const std::string& topic, int qos, bool retain);

    // This function subscribes to a topic and waits for a message.
    /*
        const std::string& topic        || Topic to subscribe to
        int qos                         || Quality of Service level
        returns                         || Received message payload
    */
    std::string Subscribe(const std::string& topic, int qos);

    // starting
    /*
        Initializes the MQTT client with the given parameters.
        std::string usr          || Username for MQTT broker
        std::string pwd          || Password for MQTT broker
        std::string id           || Client ID for MQTT broker
        bool cleanSession        || Whether to use a clean session
    */
    void Init(std::string usr, std::string pwd, std::string id, bool cleanSession);
    /*
        Starts the MQTT client and connects to the broker.
        std::string ip           || Broker IP address
        int port                 || Broker port
        int keepalive            || Keepalive interval in seconds
    */
    void Start(std::string ip, int port, int keepalive);

    /*
        Stops the MQTT client.
        bool force               || Whether to force stop the client
    */
    void Stop(bool force);
    /*
        Disconnects the MQTT client from the broker.
    */
    void Disconnect();
    /*
        Destroys the MQTT client and cleans up resources.
    */
    void Destroy();

    //setter
    void setVerbose(bool verbose) { isVerbose = verbose; }

    //getter
    bool getVerbose() const { return isVerbose; }
    bool isInitialized() const { return initialized; }
};

template <typename T>
class MQTTQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
public:
    void push(T item);
    T pop();
    void mqttWorker(MQTTClient& client);
};

extern MQTTQueue<std::function<std::string()>> mqttTaskQueue;


#endif
