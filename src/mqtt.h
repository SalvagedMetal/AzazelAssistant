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

    static void onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

public:
    MQTTClient() = default;
    ~MQTTClient() { Destroy(); };
    // This function publishes a message to a topic.
    std::string Publish(int *mid, const std::string& payload, int payloadLen, const std::string& topic, int qos, bool retain);

    // This function subscribes to a topic and waits for a message.
    std::string Subscribe(const std::string& topic, int qos);

    // starting
    void Init(std::string usr, std::string pwd, std::string id, bool cleanSession);
    void Start(std::string ip, int port, int keepalive);

    void Stop(bool force);
    void Disconnect();
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