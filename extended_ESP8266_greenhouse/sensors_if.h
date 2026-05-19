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

// --- Extended key for mux sensors ---
struct MuxSensorKey {
    SensorDriver driver;
    uint8_t mainPin;
    uint8_t muxChannel;
    std::vector<uint8_t> selPins; // S0..Sn
    bool operator<(const MuxSensorKey& other) const {
        if (driver != other.driver) return driver < other.driver;
        if (mainPin != other.mainPin) return mainPin < other.mainPin;
        if (muxChannel != other.muxChannel) return muxChannel < other.muxChannel;
        return selPins < other.selPins; // Lexicographic vector compare
    }
};

// --- Sensor interface ---
class ISensor {
public:
    virtual ~ISensor() {}
    virtual bool read(const String& paramName, float& out) = 0;
    virtual unsigned long lastReadMs() const = 0;
};

// Analog sensor (direct pin)
class AnalogSensor : public ISensor {
public:
    explicit AnalogSensor(uint8_t pin) : _pin(pin) {}
    bool read(const String&, float& out) override {
        out = analogRead(_pin);
        _lastRead = millis();
        Serial.printf("[READ] analog -> Value: %d Pin: %d\n", (int)out, _pin);
        return true;
    }
    unsigned long lastReadMs() const override { return _lastRead; }
private:
    uint8_t _pin;
    unsigned long _lastRead = 0;
};

// Digital sensor (direct pin)
class DigitalSensor : public ISensor {
public:
    explicit DigitalSensor(uint8_t pin) : _pin(pin) { pinMode(pin, INPUT); }
    bool read(const String&, float& out) override {
//    Serial.printf("[READ] read from digital sensor\n");
        out = digitalRead(_pin);
        Serial.printf("[READ] digital -> Value: %d Pin: %d\n", (int)out, _pin);

        _lastRead = millis();
        return true;
    }
    unsigned long lastReadMs() const override { return _lastRead; }
private:
    uint8_t _pin;
    unsigned long _lastRead = 0;
};

// DHT22 sensor
class DHT22Sensor : public ISensor {
       public:
           explicit DHT22Sensor(uint8_t pin) : _pin(pin), _dht(pin, DHT22) {
               _dht.begin();
           }

           bool read(const String& paramName, float& out) override {
               // Debug log is fine, but be aware it prints on every check
               // Serial.printf("[READ] checking DHT22 sensor logic...\n");

               unsigned long now = millis();
               const unsigned long minInterval = 2000;

               // FIX: Strictly enforce the time interval.
               // Do NOT bypass this check even if values are currently NAN.
               if (now - _lastSample >= minInterval) {

                   float t = _dht.readTemperature();
                   float h = _dht.readHumidity();

                   // Only update our cached values if the sensor returned valid numbers
                   if (!isnan(t)) _temp = t;
                   if (!isnan(h)) _hum = h;

                   // Log only when we actually attempt a hardware read
                   Serial.printf("[READ] DHT22 -> Temp: %.2f, Hum: %.2f\n", _temp, _hum);

                   _lastSample = now;
               }

               // Output whatever valid data we have cached (or NAN if we haven't got a good read yet)
               if (paramName.equalsIgnoreCase("humidity") ||
                   paramName.equalsIgnoreCase("air_humidity")) {

                   out = _hum;

               } else {
                   out = _temp;
               }

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
// Mux analog sensor
class MuxAnalogSensor : public ISensor {
public:
    MuxAnalogSensor(uint8_t mainPin, const std::vector<uint8_t>& selPins, uint8_t muxChannel)
        : _mainPin(mainPin), _selPins(selPins), _muxChannel(muxChannel) {
        for (uint8_t pin : _selPins) {
                    pinMode(pin, OUTPUT);
                }
        }

    bool read(const String&, float& out) override {
        // Set selector lines
        for (size_t i = 0; i < _selPins.size(); ++i) {
            digitalWrite(_selPins[i], (_muxChannel >> i) & 0x01);
        }
        // 500 µs: MUX switch + high-impedance soil-sensor RC settle time.
        // Channels 4-7 flip S2 (a full-swing transition) — 5 µs was far too short.
        delayMicroseconds(500);
        analogRead(_mainPin);      // dummy read: flush ADC sampling-cap charge from previous channel
        delayMicroseconds(200);    // extra settle after dummy read
        out = analogRead(_mainPin);
        Serial.printf("[READ] Multiplexer -> Value: %.2f Pin: %d, Channel: %d\n", out, _mainPin, _muxChannel);
        _lastRead = millis();
        return true;
    }
    unsigned long lastReadMs() const override { return _lastRead; }
private:
    uint8_t _mainPin;
    std::vector<uint8_t> _selPins;
    uint8_t _muxChannel;
    unsigned long _lastRead = 0;
};
class SensorManager {
public:
    ~SensorManager() {
        for (auto& kv : _sensors) delete kv.second;
        for (auto& kv : _muxSensors) delete kv.second;
    }

    // Single unified read interface
    bool read(SensorDriver driver,
              uint8_t mainPin,
              const String& paramName,
              float& out,
              uint8_t muxChannel = 0,
              const std::vector<uint8_t>& muxSelPins = std::vector<uint8_t>()) {
        if (driver == SensorDriver::MuxAnalog) {
            MuxSensorKey key{driver, mainPin, muxChannel, muxSelPins};
            auto it = _muxSensors.find(key);
            if (it == _muxSensors.end()) {
                _muxSensors[key] = new MuxAnalogSensor(mainPin, muxSelPins, muxChannel);
                it = _muxSensors.find(key);
            }
            return it->second->read(paramName, out);
        }
        // For all other drivers, ignore mux fields:
        ISensor* s = getOrCreate(driver, mainPin);
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
//                Serial.println(F("[SENSORS] Unknown or unsupported driver in getOrCreate"));
                break;
        }
        if (s) _sensors[key] = s;
        return s;
    }
    std::map<SensorKey, ISensor*> _sensors;
    std::map<MuxSensorKey, ISensor*> _muxSensors; // Use MuxSensorKey as defined earlier
};
extern SensorManager Sensors;