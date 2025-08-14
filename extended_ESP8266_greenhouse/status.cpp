#include "status.h"
#include "globals.h"  // for mqttClient
#include <ESP8266WiFi.h>

void logStatusIfNeeded() {
    static unsigned long lastStatus = 0;
    unsigned long now = millis();
    if (now - lastStatus > 5000) {
        lastStatus = now;
        Serial.printf("[STATUS] WiFi: %s, MQTT: %s\n",
                      WiFi.isConnected() ? "Connected" : "Disconnected",
                      mqttClient.connected() ? "Connected" : "Disconnected");
    }
}