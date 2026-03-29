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

const long SENSOR_INTERVAL = 10000;  // Read sensors every 2 seconds
const long CONTROL_INTERVAL = 10000; // Update control logic every 1 second

unsigned long lastSensorRun = 0;
unsigned long lastControlRun = 0;

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
void handleMQTTMessages(){
    if(newModelMessageArrived){
        bool ok = parseGreenhouseJson(newModelMessage.c_str(), newModelMessage.length(), true);
        Serial.printf("[MODEL] Parse %s\n", ok ? "OK" : "FAIL");
        newModelMessage="";
        newModelMessageArrived = false;
    }
    if(newConfigMessageArrived){
         if (saveConfigRaw(newConfigMessage.c_str(), newConfigMessage.length())) {
             //mqttClient.publish((getBaseTopic()+"/status/ack").c_str(), 0, false, "config_saved_rebooting");
             Serial.println("[SYS] Config saved. Reboot flagged.");
             systemRebootNeeded = true;
         }
         else {
             //mqttClient.publish((getBaseTopic()+"/status/error").c_str(), 0, false, "config_invalid_json");
         }
         newConfigMessage = "";
         newConfigMessageArrived = false;
     }
    if(newBindingMessageArrived){
        if (saveMappingRaw(newBindingMessage.c_str(), newBindingMessage.length())) {
                //mqttClient.publish((getBaseTopic()+"/status/ack").c_str(), 0, false, "mapping_saved_rebooting");
                Serial.println("[SYS] Mapping saved. Reboot flagged.");
                systemRebootNeeded = true;
        } else {
          //mqttClient.publish((getBaseTopic()+"/status/error").c_str(), 0, false, "mapping_invalid_json");
        }

        newBindingMessage = "";
        newBindingMessageArrived = false;
    }
}

void loop() {
    handleReboot();
    if (systemRebootNeeded) return;
    unsigned long currentMillis = millis();
    if (currentMillis - bootTime > REBOOT_INTERVAL_MS) {
        Serial.println("[SYS] Scheduled Daily Reboot to prevent memory fragmentation.");
        systemRebootNeeded = true;
    }

    handleMQTTMessages();

    if (currentMillis - lastSensorRun >= SENSOR_INTERVAL) {
        lastSensorRun = currentMillis;
        readSensors();
        // Serial.println("[SYS] Sensors read"); // debug
    }

    // 3. Run Control Logic (Only every 1 second)
    if (currentMillis - lastControlRun >= CONTROL_INTERVAL) {
        lastControlRun = currentMillis;
        runControlLogic();
    }
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
