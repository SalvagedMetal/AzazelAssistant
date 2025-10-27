#include "mqtt.h"

std::mutex MQTTClient::mtx;
std::condition_variable MQTTClient::cv;
template class MQTTQueue<std::string>;
bool MQTTClient::isVerbose = false;

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


std::string MQTTClient::Publish(int *mid, const uint8_t* payload, int payloadLen, const std::string& topic, int qos, bool retain)
{
    rc = mosquitto_publish(mosq, mid, topic.c_str(), payloadLen, payload, qos, retain);
    if(rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Error publishing: " + std::string(mosquitto_strerror(rc)));
    }
    return "Published to topic: " + topic;
}


std::string MQTTClient::Subscribe(const char* topic, int qos)
{
    // Reset state before subscribing
    {
        std::lock_guard<std::mutex> lock(mtx);
        messageFlag = false;
        subscribedMessage.clear();
    }

    mosquitto_message_callback_set(mosq, onMessage);

    int rc = mosquitto_subscribe(mosq, nullptr, topic, qos);
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
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    if (isVerbose)
        std::cout << "Mosquitto client destroyed and library cleaned up." << std::endl;
}


void MQTTClient::Stop(bool force) {
    rc = mosquitto_loop_stop(mosq, force);
    if(rc != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("Error: " + std::string(mosquitto_strerror(rc)));
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

    rc = mosquitto_username_pw_set(mosq, usr.c_str(), pwd.c_str());
    if(rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        throw std::runtime_error("Error setting username/password: " + std::string(mosquitto_strerror(rc)));
    }
    if (isVerbose)
        std::cout << "Mosquitto password set" << std::endl;


    mosquitto_connect_callback_set(mosq, onConnect);
    mosquitto_publish_callback_set(mosq, onPublish);
    mosquitto_message_callback_set(mosq, onMessage);
}


void MQTTClient::Start(std::string ip, int port, int keepalive) {
    bool connectFlag = false;
    while (connectFlag == false) {
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
        cv.wait(lock, [this]() { return !queue.empty(); });
        T item = queue.front();
        queue.pop();

        return item;
    }