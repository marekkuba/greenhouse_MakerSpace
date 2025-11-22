#pragma once
#include <AsyncMqttClient.h>
#include <functional>

void connectToMqtt();
void registerMqttHandlers();
void publishGreenhouseState();
void publishTelemetryJson(const String& jsonPayload);
extern AsyncMqttClient mqttClient;

