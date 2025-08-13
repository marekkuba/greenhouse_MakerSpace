#include "sensors.h"
#include "config.h"

bool readDHT(uint8_t pin, DHT* sensor, float* temp, float* hum) {
  // Check if we have recent cached reading
  if (dhtCache.find(pin) != dhtCache.end()) {
    DHTReading cache = dhtCache[pin];
    if (millis() - cache.lastReadTime < 2000) {
      *temp = cache.temperature;
      *hum = cache.humidity;
      return true;
    }
  }

  // Perform new reading
  float newTemp = sensor->readTemperature();
  float newHum = sensor->readHumidity();

  // If read fails, try to use cache if available
  if (isnan(newTemp)) {
    if (dhtCache.find(pin) != dhtCache.end()) {
      newTemp = dhtCache[pin].temperature;
    }
  }

  if (isnan(newHum)) {
    if (dhtCache.find(pin) != dhtCache.end()) {
      newHum = dhtCache[pin].humidity;
    }
  }

  // Update cache
  DHTReading reading;
  reading.temperature = newTemp;
  reading.humidity = newHum;
  reading.lastReadTime = millis();
  dhtCache[pin] = reading;

  *temp = newTemp;
  *hum = newHum;

  return !(isnan(newTemp)) && !(isnan(newHum));
}


bool readValue(const String& driver, uint8_t pin, const String& paramName, float& out) {
  if (driver == "analog") {
    out = analogRead(pin);
    return true;
  } else if (driver == "digital") {
    out = digitalRead(pin);
    return true;
  } else if (driver == "DHT22") {
    // decide temp vs humidity by paramName
    auto it = dhtCache.find(pin);
    if (it == dhtCache.end() || (millis() - it->second.lastReadTime) > 2000) {
      // trigger a fresh read using an existing DHT instance on that pin
      DHT* dht = nullptr;
      for (auto &d : devices) if (d.driver == "DHT22" && d.pin == pin && d.dhtPtr) { dht = d.dhtPtr; break; }
      if (dht) {
        float t,h; readDHT(pin, dht, &t, &h); // updates cache
      } else {
        // create on the fly if missing
        DHT* sensor = new DHT(pin, DHT22);
        sensor->begin();
        float t,h; readDHT(pin, sensor, &t, &h);
        // not storing pointer globally to avoid leak complexity; optional improvement
      }
    }
    auto r = dhtCache[pin];
    if (paramName.indexOf("temp") >= 0) out = r.temperature;
    else if (paramName.indexOf("hum") >= 0) out = r.humidity;
    else out = r.temperature; // default
    return !isnan(out);
  }
  return false;
}

