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
#include "status.h"
#include "scheduler.h"
#include "mqtt_helpers.h"

void handleReboot() {
    static unsigned long rebootTimer = 0;
    if (systemRebootNeeded) {
        if (rebootTimer == 0) {
            // Start a countdown to allow MQTT ACK to be sent out
            Serial.println("[SYS] Rebooting in 2 seconds...");
            rebootTimer = millis();
        }

        if (millis() - rebootTimer > 2000) {
            Serial.println("[SYS] Restarting now.");
            ESP.restart();
        }
    }
}

void loop() {
    handleReboot();
    if (systemRebootNeeded) return;
    logStatusIfNeeded();

    // Always run control logic so actuators react ASAP
    controlTick();

//     Only publish the model periodically
    if (isMqttReady() && timeToPublish()) {
        publishModel();
    }
}

void initSerial() {
    Serial.begin(115200);
    delay(1000);
}

void setup() {
  Serial.println(F("\n\n[BOOT] Starting..."));

  initSerial();
  bootMessage();
  initFilesystem();
  loadAllConfigs();
  Serial.println("[SERIAL] Initialization complete");

  registerWifiHandlers();
  registerMqttHandlers();

  Serial.println("[BOOT] Initialization complete");
  setupWifi();

  Serial.println("[BOOT] trying to restore model");
  loadModel();

}
