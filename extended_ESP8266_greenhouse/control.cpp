#include "control.h"
#include "config.h"
#include "sensors.h"
#include "mappings.h"
#include "actuators.h"
#include "model.h"
#include "mqtt.h"   // for mqttClient

void controlTick() {

  // Pre-read DHT sensors
  for (auto &dev : devices) {
    if (dev.driver == "DHT22" && dev.dhtPtr) {
      // Sprawdź czy ten pin został już odczytany
      if (dhtCache.find(dev.pin) == dhtCache.end() ||
          (millis() - dhtCache[dev.pin].lastReadTime) > 2000) {
        float temp, hum;
        bool success = readDHT(dev.pin, dev.dhtPtr, &temp, &hum);

        if (success) {
          Serial.printf("[DHT] Read pin %d: temp=%.2fC, hum=%.2f%%\n", dev.pin, temp, hum);
        } else {
          Serial.printf("[DHT] Failed to read sensor on pin %d\n", dev.pin);
        }
      }
    }
  }

  for (auto &dev : devices) {
    if (dev.type != "value") continue;

    float value = NAN;
    if (dev.driver == "analog") {
      value = analogRead(dev.pin);
    }
    else if (dev.driver == "digital") {
      value = digitalRead(dev.pin);
    }
    else if (dev.driver == "DHT22" && dev.dhtPtr) {
      if (dhtCache.find(dev.pin) != dhtCache.end()) {
        DHTReading reading = dhtCache[dev.pin];
        if (dev.name == "air_humidity") {
          value = reading.humidity;
        } else if (dev.name == "air_temp") {
          value = reading.temperature;
        }
      }
    }

    if (isnan(value)) {
      Serial.printf("[SENSOR] Failed to read %s\n", dev.name.c_str());
      continue;
    }

    String topic = makeTopic(dev);
    char payload[10];
    dtostrf(value, 4, 2, payload);

    if (mqttClient.publish(topic.c_str(), 1, true, payload)) {
      Serial.printf("[SENSOR] Published %s: %s to %s\n",
                   dev.name.c_str(), payload, topic.c_str());
    }} else {
      Serial.printf("[SENSOR] FAILED to publish %s\n", topic.c_str());
    }
  }

for (auto &b : bindings) {
  Parameter* p = findParameter(b);
  if (!p) continue;

  // Read current sensor value
  float val = NAN;
  if (readValue(b.readDriver, b.readPin, p->name, val)) {
    p->currentValue = val;
    // publish current value
    String tcur = makeParamTopic(b, "current");
    char payload[16]; dtostrf(val, 4, 2, payload);
    mqttClient.publish(tcur.c_str(), 1, true, payload);
  }

  // Control only if mutable and requestedValue is valid
  if (p->mutableFlag && !isnan(p->requestedValue) && !isnan(p->currentValue)) {
    bool turnOn = false;
    float hysteresis = 0.5; // tune as needed

    if (b.control == "heat") {
      // heat if below target - hysteresis
      if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = true;
      if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = false;
    } else if (b.control == "cool") {
      if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = true;
      if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = false;
    } else if (b.control == "humidify") {
      if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = true;
      if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = false;
    } else if (b.control == "dehumidify") {
      if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = true;
      if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = false;
    }

    writeActuator(b.writeDriver, b.writePin, turnOn);
    // Optionally publish actuator state
    String tact = makeParamTopic(b, "actuator");
    mqttClient.publish(tact.c_str(), 1, true, turnOn ? "ON" : "OFF");
  }

  // Also publish requested value so server can verify
  if (!isnan(p->requestedValue)) {
    String treq = makeParamTopic(b, "requested");
    char payload[16]; dtostrf(p->requestedValue, 4, 2, payload);
    mqttClient.publish(treq.c_str(), 1, true, payload);
  }
}