#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <DHT.h>
#include <Ticker.h>
#include <vector>
#include <map>
#include "actuators.h"
#include "config.h"
#include "control.h"
#include "mappings.h"
#include "model.h"
#include "mqtt.h"
#include "network.h"
#include "persistence.h"
#include "sensors.h"

void loop() {
  unsigned long now = millis();
  
  static unsigned long lastStatus = 0;
  if (now - lastStatus > 5000) {
    lastStatus = now;
    Serial.printf("[STATUS] WiFi: %s, MQTT: %s\n", 
                 WiFi.isConnected() ? "Connected" : "Disconnected",
                 mqttClient.connected() ? "Connected" : "Disconnected");
  }

  if (now - previousMillis < PUBLISH_INTERVAL) {
    delay(10);
    return;
  }
  
  previousMillis = now;

  if (!mqttClient.connected()) {
    Serial.println("[WARN] MQTT not connected, skipping publish");
    if (WiFi.isConnected()) {
      connectToMqtt();
    }
    return;
  }
    controlTick();
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n[BOOT] Starting...");
  initFilesystem();
  loadConfig();
  loadNetworkConfig();
  loadMappings();        // new
  loadTargets();         // optional; will apply after first model load
  registerWifiHandlers();
  registerMqttHandlers();
  Serial.println("[BOOT] Initialization complete");
  setupWifi();
}
