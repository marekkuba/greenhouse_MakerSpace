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
#include "scheduler.h"
#include "mqtt_helpers.h"
#include "globals.h"

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

    // Check if 24 hours have passed since boot and reboot if yes
    if (millis() - bootTime > REBOOT_INTERVAL_MS) {
        Serial.println(F("[SYS] Scheduled Daily Reboot to prevent memory fragmentation."));
        systemRebootNeeded = true; // Uses your existing reboot logic
    }
    if(newModelMessageArrived){
        bool ok = parseGreenhouseJson(newModelMessage, newModelMessageLen, true);
        Serial.printf("[MODEL] Parse %s\n", ok ? "OK" : "FAIL");
        newModelMessageArrived = false;
    }

    readSensors();
    runControlLogic();

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
  bootTime = millis();
  initSerial();
  initFilesystem();
  loadAllConfigs();
  Serial.println("[SERIAL] Initialization complete");

  registerWifiHandlers();
  registerMqttHandlers();

  Serial.println("[BOOT] Initialization complete");
  setupWifi();

  Serial.println("[BOOT] trying to restore model");
  loadModel();
  resolveBindings();

}
