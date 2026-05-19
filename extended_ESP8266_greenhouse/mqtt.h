#pragma once
#include <AsyncMqttClient.h>
#include <functional>

void connectToMqtt();
void registerMqttHandlers();
void publishTelemetryJson(const String& jsonPayload, String greenhouseIpAddress);
extern AsyncMqttClient mqttClient;

