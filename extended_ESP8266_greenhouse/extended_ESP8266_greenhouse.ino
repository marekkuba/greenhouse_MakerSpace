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
#include "status.h"
#include "scheduler.h"
#include "mqtt_helpers.h"

void loop() {
    logStatusIfNeeded();
    if (!timeToPublish()) return;
    if (!ensureMqttConnected()) return;
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
