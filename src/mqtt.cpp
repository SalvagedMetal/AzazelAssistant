#include "mqtt.h"

std::mutex MQTTClient::mtx;
std::condition_variable MQTTClient::cv;
template class MQTTQueue<std::function<std::string()>>;
bool MQTTClient::isVerbose = false;
std::thread MQTTClient::mqttWorkerThread;

//extern declaration
std::atomic<bool> mqttRunning{true};
MQTTQueue<std::function<std::string()>> mqttTaskQueue;

void MQTTClient::onConnect(struct mosquitto *mosq, void *obj, int reason_code)
{
    if (isVerbose)
        std::cout << "on_connect: " << mosquitto_connack_string(reason_code) << std::endl;
    if(reason_code != 0)
        mosquitto_disconnect(mosq);
}


void MQTTClient::onPublish(struct mosquitto *mosq, void *obj, int mid)
{
    if (isVerbose)
        std::cout << "Message with mid " << mid << " has been published." << std::endl;
}


void MQTTClient::onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    MQTTClient* client = static_cast<MQTTClient*>(userdata); //access object instance
    if (message && message->payload) {
        std::unique_lock<std::mutex> lock(mtx);
        client->subscribedMessage.assign(static_cast<const char*>(message->payload), message->payloadlen);
        if (isVerbose)
            std::cout << "Received retained message: " << client->subscribedMessage << std::endl;
        client->messageFlag = true;
        lock.unlock();
        client->cv.notify_one();
    }
}


std::string MQTTClient::Publish(int *mid, const std::string& payload, int payloadLen, const std::string& topic, int qos, bool retain)
{
    std::lock_guard<std::mutex> lock(mtx);
    rc = mosquitto_publish(mosq, mid, topic.c_str(), payloadLen, payload.c_str(), qos, retain);
    if(rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Error publishing: " + std::string(mosquitto_strerror(rc)));
    }
    return "Published to topic: " + topic;
}


std::string MQTTClient::Subscribe(const std::string& topic, int qos)
{
    // Reset state before subscribing
    {
        std::lock_guard<std::mutex> lock(mtx);
        messageFlag = false;
        subscribedMessage.clear();
    }

    mosquitto_message_callback_set(mosq, onMessage);

    int rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Subscribe failed: " + std::string(mosquitto_strerror(rc)));
    }

    // Wait for message
    std::unique_lock<std::mutex> lock(mtx);
    if (!cv.wait_for(lock, std::chrono::seconds(5), [this]{ return messageFlag; })) {
        throw std::runtime_error("Timeout waiting for message on topic");
    }
    
    return subscribedMessage;
}


void MQTTClient::Disconnect() {
    rc = mosquitto_disconnect(mosq);
    if(rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Error: " + std::string(mosquitto_strerror(rc)));
    }
}


void MQTTClient::Destroy() {
    // ensure worker stopped
    mqttRunning.store(false);
    mqttTaskQueue.push(std::function<std::string()>()); // wake worker if blocked
    if (mqttWorkerThread.joinable()) mqttWorkerThread.join();

    mosquitto_destroy(mosq);
    mosq = nullptr;
    mosquitto_lib_cleanup();
    if (isVerbose)
        std::cout << "Mosquitto client destroyed and library cleaned up." << std::endl;
}


void MQTTClient::Stop(bool force) {
    rc = mosquitto_loop_stop(mosq, force);
    if(rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Error: " + std::string(mosquitto_strerror(rc)));
    }
    // signal worker to stop and wake it
    mqttRunning.store(false);
    mqttTaskQueue.push(std::function<std::string()>()); // push empty task to wake the worker

    if (mqttWorkerThread.joinable()) {
        mqttWorkerThread.join();
        if (isVerbose) {
            std::cout << "MQTT Worker thread stopped." << std::endl;
        }
    }
}


void MQTTClient::Init(std::string usr, std::string pwd, std::string id, bool cleanSession) {

    mosquitto_lib_init();
    if (isVerbose)
        std::cout << "Mosquitto library initialized." << std::endl;

    mosq = mosquitto_new(id.c_str(), cleanSession, this);
    if(mosq == nullptr) {
        std::cerr << "Error: Out of memory." << std::endl;
        throw std::runtime_error("Error: Out of memory.");
    }

    if (!usr.empty() && !pwd.empty())
        rc = mosquitto_username_pw_set(mosq, usr.c_str(), pwd.c_str());
    else 
        rc = mosquitto_username_pw_set(mosq, nullptr, nullptr);

    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        throw std::runtime_error("Error setting username/password: " + std::string(mosquitto_strerror(rc)));
    }
    if (isVerbose && (!usr.empty() && !pwd.empty()))
        std::cout << "Mosquitto password set" << std::endl;


    mosquitto_connect_callback_set(mosq, onConnect);
    mosquitto_publish_callback_set(mosq, onPublish);
    mosquitto_message_callback_set(mosq, onMessage);
    initialized = true;
}


void MQTTClient::Start(std::string ip, int port, int keepalive) {
    bool connectFlag = false;
    while (!connectFlag) {
        if (isVerbose)
            std::cout << "Mosquitto connecting..." << std::endl;
        rc = mosquitto_connect(mosq, ip.c_str(), port, keepalive);
        if(rc != MOSQ_ERR_SUCCESS) {
            throw std::runtime_error("Error: " + std::string(mosquitto_strerror(rc)));
        } else {
            connectFlag = true;
        }
    }

    rc = mosquitto_loop_start(mosq); //for asynchronous loop
    if(rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        throw std::runtime_error("Error: " + std::string(mosquitto_strerror(rc)));
    }
     if (!mqttWorkerThread.joinable()) {
        mqttRunning.store(true);
        mqttWorkerThread = std::thread(
            &MQTTQueue<std::function<std::string()>>::mqttWorker, &mqttTaskQueue, std::ref(*this));
        if (isVerbose) {
            std::cout << "MQTT Worker thread started." << std::endl;
        }
    }
}


// MQTTQueue implementation
    template<typename T>
    void MQTTQueue<T>::push(T item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_one();
    }

    template<typename T>
    T MQTTQueue<T>::pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty() || !mqttRunning.load(); });
        if (queue.empty()) {
            return T(); // return default if stopping
        }
        T item = queue.front();
        queue.pop();

        return item;
    }


    template<typename T>
    void MQTTQueue<T>::mqttWorker(MQTTClient& mqtt) {
        while (mqttRunning.load()) {
            T task = pop();
            try {
                if (!task) {
                    continue; // Skip if the task is empty
                }
                task(); // Run the task safely
            } catch (const std::exception& e) {
                std::cerr << "MQTT Worker Error: " << e.what() << std::endl;
            }
        }
    }
