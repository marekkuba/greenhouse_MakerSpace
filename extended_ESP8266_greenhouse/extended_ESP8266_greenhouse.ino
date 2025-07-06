#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <DHT.h>
#include <Ticker.h>
#include <vector>    // Dodane dla std::vector
#include <map>       // Dodane dla std::map

// --- Wi‑Fi / MQTT config --------------------------------------------
#define WIFI_SSID      "SSID"
#define WIFI_PASSWORD  "PASSWORD"
#define MQTT_HOST      IPAddress(192,168,1,XXX) //ustawic poprawne IP MQTT brokera
#define MQTT_PORT      1883

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
AsyncMqttClient mqttClient;
Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;

const unsigned long PUBLISH_INTERVAL = 10000;
unsigned long previousMillis = 0;

// Device configuration struct
struct DeviceConfig {
  String   name;
  String   driver;
  String   type;
  uint8_t  pin;
  float    minValue;
  float    maxValue;
  uint8_t  flowerzoneId;
  uint8_t  flowerId;
  DHT*     dhtPtr;
};

std::vector<DeviceConfig> devices;

// DHT cache structure
struct DHTReading {
  float temperature = NAN;
  float humidity = NAN;
  unsigned long lastReadTime = 0;
};

std::map<uint8_t, DHTReading> dhtCache;  // Mapa do cache'owania odczytów

// Function prototypes
void setupWifi();
void connectToMqtt();
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttPublish(uint16_t packetId);
void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties props,
                   size_t len, size_t idx, size_t total);
String makeTopic(const DeviceConfig& dev);
void loadConfig();

String makeTopic(const DeviceConfig& dev) {
  String t = "greenhouse";
  if (dev.flowerzoneId) t += "/zone" + String(dev.flowerzoneId);
  if (dev.flowerId)     t += "/plant" + String(dev.flowerId);
  t += "/" + dev.name;
  return t;
}

void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed");
    return;
  }
  
  if (!LittleFS.exists("/config.json")) {
    Serial.println("[ERROR] config.json not found");
    return;
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println("[ERROR] Failed to open config.json");
    return;
  }
  
  size_t size = f.size();
  // Dynamiczny bufor zamiast unique_ptr
  char* buf = new char[size];
  f.readBytes(buf, size);
  f.close();

  StaticJsonDocument<2048> doc;
  auto err = deserializeJson(doc, buf, size);
  delete[] buf;  // Zwolnienie pamięci
  
  if (err) {
    Serial.printf("[ERROR] Config parse failed: %s\n", err.c_str());
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    DeviceConfig dev;
    dev.name         = obj["name"].as<String>();
    dev.driver       = obj["driver"].as<String>();
    dev.type         = obj["type"].as<String>();
    dev.pin          = obj["pin"].as<uint8_t>();
    dev.minValue     = obj.containsKey("minValue") ? obj["minValue"].as<float>() : NAN;
    dev.maxValue     = obj.containsKey("maxValue") ? obj["maxValue"].as<float>() : NAN;
    dev.flowerzoneId = obj["flowerzoneId"].as<uint8_t>();
    dev.flowerId     = obj["flowerId"].as<uint8_t>();
    dev.dhtPtr       = nullptr;

    if (dev.driver == "DHT22") {
      // Sprawdź czy sensor już istnieje dla tego pinu
      bool sensorExists = false;
      for (auto &d : devices) {
        if (d.driver == "DHT22" && d.pin == dev.pin && d.dhtPtr) {
          dev.dhtPtr = d.dhtPtr;  // Użyj istniejącej instancji
          sensorExists = true;
          Serial.printf("[CONFIG] Reusing existing DHT22 on pin %d\n", dev.pin);
          break;
        }
      }
      
      // Jeśli nie istnieje, utwórz nowy
      if (!sensorExists) {
        DHT* sensor = new DHT(dev.pin, DHT22);
        sensor->begin();
        dev.dhtPtr = sensor;
        Serial.printf("[CONFIG] Initialized DHT22 on pin %d\n", dev.pin);
      }
    }
    
    if (dev.type == "toggle") {
      pinMode(dev.pin, OUTPUT);
      digitalWrite(dev.pin, LOW);
      Serial.printf("[CONFIG] Initialized toggle on pin %d\n", dev.pin);
    }
    
    devices.push_back(dev);
  }
  Serial.printf("[CONFIG] Loaded %d devices\n", (int)devices.size());
}

void setupWifi() {
  Serial.println("[WIFI] Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("[WIFI] Waiting 1s before MQTT connection...");
  delay(1000);
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("[WIFI] Disconnected!");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, setupWifi);
}

void connectToMqtt() {
  Serial.println("[MQTT] Attempting connection...");
  Serial.printf("[MQTT] Broker: %d.%d.%d.%d:%d\n", 
                MQTT_HOST[0], MQTT_HOST[1], MQTT_HOST[2], MQTT_HOST[3],
                MQTT_PORT);
                
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.printf("[MQTT] Connected! Session: %d\n", sessionPresent);
  
  for (auto &dev : devices) {
    if (dev.type == "toggle") {
      String topic = makeTopic(dev);
      if (mqttClient.subscribe(topic.c_str(), 1)) {
        Serial.printf("[MQTT] Subscribed to %s\n", topic.c_str());
      } else {
        Serial.printf("[MQTT] FAILED to subscribe to %s\n", topic.c_str());
      }
    }
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("[MQTT] Disconnected! Reason: %d\n", (int)reason);
  
  if (WiFi.isConnected()) {
    Serial.println("[MQTT] Reconnecting in 5s...");
    mqttReconnectTimer.once(5, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.printf("[MQTT] Publish ACK for PID %d\n", packetId);
}

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties props,
                   size_t len, size_t idx, size_t total) {
  String t = String(topic);
  String msg = String(payload).substring(0, len);
  
  Serial.printf("[MQTT] Message received: [%s] %s\n", t.c_str(), msg.c_str());
  
  for (auto &dev : devices) {
    if (dev.type == "toggle" && makeTopic(dev) == t) {
      bool on = (msg == "ON" || msg == "1");
      digitalWrite(dev.pin, on ? HIGH : LOW);
      Serial.printf("[ACTION] %s set to %s\n", dev.name.c_str(), on?"ON":"OFF");
    }
  }
}

// Read DHT sensor with caching
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
    } else {
      Serial.printf("[SENSOR] FAILED to publish %s\n", topic.c_str());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n[BOOT] Starting...");

  if (!LittleFS.begin()) {
    Serial.println("[ERROR] Failed to mount LittleFS");
  } else {
    Serial.println("[FS] LittleFS mounted");
  }

  loadConfig();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(60);

  Serial.println("[BOOT] Initialization complete");
  setupWifi();
}
