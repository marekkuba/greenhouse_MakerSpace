#pragma once
#include <AsyncMqttClient.h>
#include <functional>

void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttPublish(uint16_t packetId);
void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties props,
                   size_t len, size_t idx, size_t total);
String makeTopic(String greenhouseIpAddress);
void registerMqttHandlers();

extern AsyncMqttClient mqttClient;

