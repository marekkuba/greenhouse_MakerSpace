#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <DHT.h>
#include <Ticker.h>

// Wi‑Fi / MQTT
#define WIFI_SSID      "YOUR_SSID"
#define WIFI_PASSWORD  "YOUR_PASS"
#define MQTT_HOST      IPAddress(192,168,1,XXX)
#define MQTT_PORT      1883

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer, wifiReconnectTimer;

const unsigned long PUBLISH_INTERVAL = 10000;
unsigned long previousMillis = 0;

struct DeviceConfig {
  String   name;
  String   driver;      // "DHT22", "analog", "digital"
  String   type;        // "value", "toggle"
  uint8_t  pin;
  float    minValue;
  float    maxValue;
  uint8_t  flowerzoneId;
  uint8_t  flowerId;
};

std::vector<DeviceConfig> devices;
std::vector<DHT*>        dhtDevices;  // keep DHT objects alive

void setupWifi();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char* topic, char* payload, 
                   AsyncMqttClientMessageProperties props, 
                   size_t len, size_t idx, size_t total);

// Build topic dynamically
String makeTopic(const DeviceConfig& dev) {
  String t = "greenhouse";
  if (dev.flowerzoneId != 0) {
    t += "/zone" + String(dev.flowerzoneId);
  }
  if (dev.flowerId != 0) {
    t += "/plant" + String(dev.flowerId);
  }
  t += "/" + dev.name;
  return t;
}

void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println("Failed to open config.json");
    return;
  }
  StaticJsonDocument<2048> doc;
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("Config parse failed: %s\n", err.c_str());
    return;
  }
  for (JsonObject obj : doc.as<JsonArray>()) {
    DeviceConfig dev;
    dev.name          = obj["name"].as<String>();
    dev.driver        = obj["driver"].as<String>();
    dev.type          = obj["type"].as<String>();
    dev.pin           = obj["pin"].as<uint8_t>();
    dev.minValue      = obj.containsKey("minValue") ? obj["minValue"].as<float>() : 0;
    dev.maxValue      = obj.containsKey("maxValue") ? obj["maxValue"].as<float>() : 0;
    dev.flowerzoneId  = obj["flowerzoneId"].as<uint8_t>();
    dev.flowerId      = obj["flowerId"].as<uint8_t>();

    if (dev.driver == "DHT22") {
      DHT* dht = new DHT(dev.pin, DHT22);
      dht->begin();
      dhtDevices.push_back(dht);
    }
    if (dev.type == "toggle") {
      pinMode(dev.pin, OUTPUT);
    }
    devices.push_back(dev);
  }
}

void setup() {
  Serial.begin(115200);
  loadConfig();

  WiFi.onStationModeGotIP([](auto&){ Serial.println("WiFi connected"); connectToMqtt(); });
  WiFi.onStationModeDisconnected([](auto&){
    Serial.println("WiFi disconnected");
    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, setupWifi);
  });

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  setupWifi();
}

void setupWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.printf("MQTT Connected, session: %d\n", sessionPresent);
  for (auto &dev : devices) {
    if (dev.type == "toggle") {
      String topic = makeTopic(dev);
      mqttClient.subscribe(topic.c_str(), 1);
      Serial.printf("Subscribed to %s\n", topic.c_str());
    }
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("MQTT Disconnected");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload, 
                   AsyncMqttClientMessageProperties, 
                   size_t len, size_t, size_t) {
  String t = String(topic);
  String msg = String(payload).substring(0, len);
  for (auto &dev : devices) {
    if (dev.type == "toggle" && makeTopic(dev) == t) {
      digitalWrite(dev.pin, (msg == "ON" || msg == "1") ? HIGH : LOW);
      Serial.printf("Set %s to %s\n", dev.name.c_str(), msg.c_str());
    }
  }
}

void loop() {
  unsigned long now = millis();
  if (now - previousMillis < PUBLISH_INTERVAL) return;
  previousMillis = now;

  for (size_t i = 0; i < devices.size(); i++) {
    auto &dev = devices[i];
    if (dev.type != "value") continue;

    float value = NAN;
    if (dev.driver == "analog") {
      value = analogRead(dev.pin);
    }
    else if (dev.driver == "digital") {
      value = digitalRead(dev.pin);
    }
    else if (dev.driver == "DHT22") {
      DHT* dht = dhtDevices.front();
      if (dev.name.indexOf("humidity") >= 0)        value = dht->readHumidity();
      else if (dev.name.indexOf("temperature") >= 0) value = dht->readTemperature();
    }

    if (!isnan(value)) {
      String topic = makeTopic(dev);
      mqttClient.publish(topic.c_str(), 1, true, String(value).c_str());
      Serial.printf("Published %s = %.2f to %s\n",
                    dev.name.c_str(), value, topic.c_str());
    }
  }
}

