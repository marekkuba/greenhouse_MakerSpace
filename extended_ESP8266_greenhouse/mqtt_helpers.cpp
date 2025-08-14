#include "mqtt_helpers.h"
#include "network.h"  // for connectToMqtt
#include "globals.h"  // for mqttClient
#include <ESP8266WiFi.h>

bool ensureMqttConnected() {
    if (!mqttClient.connected()) {
        Serial.println("[WARN] MQTT not connected, skipping publish");
        if (WiFi.isConnected()) {
            connectToMqtt();
        }
        return false;
    }
    return true;
}