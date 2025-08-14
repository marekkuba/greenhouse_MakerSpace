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

    // Always try to keep MQTT alive
    if (!ensureMqttConnected()) return;

    // Always run control logic so actuators react ASAP
    controlTick();

    // Only publish the model periodically
    if (timeToPublish()) {
        publishModel();
    }
}

void initSerial() {
    Serial.begin(115200);
    delay(1000);
}

void bootMessage() {
    Serial.println(F("\n\n[BOOT] Starting..."));
}

void setup() {
  initSerial();
  bootMessage();
  initFilesystem();
  loadAllConfigs();
  tryOfflineModelRestore();

  registerWifiHandlers();
  registerMqttHandlers();

  Serial.println("[BOOT] Initialization complete");
  setupWifi();
}
