#ifndef MQTTQUEUE_H
#define MQTTQUEUE_H

#include <functional>
#include <string>

extern MQTTQueue<std::function<std::string()>> mqttTaskQueue;

#endif