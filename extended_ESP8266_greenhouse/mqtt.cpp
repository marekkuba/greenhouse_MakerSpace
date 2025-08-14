#include "mqtt.h"
#include "config.h"
#include "model.h"

String makeTopic(String greenhouseIpAddress) {
  return "greenhouse/"+greenhouseIpAddress;
}

void connectToMqtt() {
    Serial.println("[MQTT] Attempting connection...");
    Serial.printf("[MQTT] Broker: %d.%d.%d.%d:%d\n", netConfig.mqtt_host, netConfig.mqtt_host,netConfig.mqtt_host, netConfig.mqtt_host,netConfig.mqtt_port);
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.printf("[MQTT] Connected! Session: %d\n", sessionPresent);
  mqttClient.subscribe(makeTopic(greenhouse.ipAddress), 1);
  Serial.println("[MQTT] Subscriptions set");
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
  String msg;
  msg.reserve(len+1);
  for (size_t i=0;i<len;i++) msg += payload[i];

  Serial.printf("[MQTT] Message received: [%s] %s\n", t.c_str(), msg.c_str());

  if (t == makeTopic(greenhouse.ipAddress)) {
    bool ok = parseGreenhouseJson(msg.c_str(), msg.length());
    Serial.printf("[MODEL] Parse %s\n", ok ? "OK" : "FAIL");
    return;
  }
}
void registerMqttHandlers() {
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(netConfig.mqtt_host, netConfig.mqtt_port);
    mqttClient.setKeepAlive(60);
}