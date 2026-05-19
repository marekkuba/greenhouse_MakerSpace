#pragma once
#include <AsyncMqttClient.h>
#include <functional>

String getBaseTopic(String greenhouseIpAddress);
void connectToMqtt();
void registerMqttHandlers();
void publishTelemetryJson(const String& jsonPayload, String greenhouseIpAddress);
extern AsyncMqttClient mqttClient;

