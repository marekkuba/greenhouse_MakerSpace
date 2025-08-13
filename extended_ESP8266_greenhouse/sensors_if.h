#pragma once
#include <Arduino.h>
#include <map>
#include <DHT.h>
#include "sensor_types.h"


struct SensorKey {
    SensorDriver driver;
    uint8_t pin;
    bool operator<(const SensorKey& other) const {
        if (driver != other.driver) return driver < other.driver;
        return pin < other.pin;
    }
};
// Sensor interface
class ISensor {
public:
    virtual ~ISensor() {}
    virtual bool read(const String& paramName, float& out) = 0;
    virtual unsigned long lastReadMs() const = 0;
};

// Analog sensor implementation
class AnalogSensor : public ISensor {
public:
    explicit AnalogSensor(uint8_t pin) : _pin(pin) {}
    bool read(const String&, float& out) override {
        out = analogRead(_pin);
        _lastRead = millis();
        return true;
    }
    unsigned long lastReadMs() const override { return _lastRead; }
private:
    uint8_t _pin;
    unsigned long _lastRead = 0;
};

// Digital sensor implementation
class DigitalSensor : public ISensor {
public:
    explicit DigitalSensor(uint8_t pin) : _pin(pin) { pinMode(pin, INPUT); }
    bool read(const String&, float& out) override {
        out = digitalRead(_pin);
        _lastRead = millis();
        return true;
    }
    unsigned long lastReadMs() const override { return _lastRead; }
private:
    uint8_t _pin;
    unsigned long _lastRead = 0;
};

// DHT22 sensor with built‑in temp/humidity caching
class DHT22Sensor : public ISensor {
    public:
        explicit DHT22Sensor(uint8_t pin) : _pin(pin), _dht(pin, DHT22) {
            _dht.begin();
        }
        bool read(const String& paramName, float& out) override {
            unsigned long now = millis();
            const unsigned long minInterval = 2000;
            if (now - _lastSample >= minInterval || isnan(_temp) || isnan(_hum)) {
                float t = _dht.readTemperature();
                float h = _dht.readHumidity();
                if (!isnan(t)) _temp = t;
                if (!isnan(h)) _hum = h;
                _lastSample = now;
            }
            if (paramName.indexOf("hum") >= 0)
                out = _hum;
            else
                out = _temp;
            _lastRead = now;
            return !isnan(out);
        }
        unsigned long lastReadMs() const override { return _lastRead; }
    private:
        uint8_t _pin;
        DHT _dht;
        float _temp = NAN;
        float _hum  = NAN;
        unsigned long _lastSample = 0;
        unsigned long _lastRead   = 0;
};

// Sensor manager
class SensorManager {
    public:
        ~SensorManager() {
            for (auto& kv : _sensors) delete kv.second;
        }
        bool read(SensorDriver driver, uint8_t pin, const String& paramName, float& out) {
            ISensor* s = getOrCreate(driver, pin);
            return s && s->read(paramName, out);
        }
    private:
        ISensor* getOrCreate(SensorDriver driver, uint8_t pin) {
            SensorKey key{driver, pin};
            auto it = _sensors.find(key);
            if (it != _sensors.end()) return it->second;

            ISensor* s = nullptr;
            switch (driver) {
                case SensorDriver::Analog:  s = new AnalogSensor(pin); break;
                case SensorDriver::Digital: s = new DigitalSensor(pin); break;
                case SensorDriver::DHT22:   s = new DHT22Sensor(pin); break;
                default:
                    Serial.println(F("[SENSORS] Unknown driver enum"));
                    break;
            }
            if (s) _sensors[key] = s;
            return s;
        }
        std::map<SensorKey, ISensor*> _sensors;
};
