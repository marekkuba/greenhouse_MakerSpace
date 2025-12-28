#include "mqtt.h"
#include "config.h"
#include "model.h"
#include "globals.h"

String getBaseTopic(String greenhouseIpAddress) {
  return "greenhouse/"+greenhouseIpAddress;
}

String getSubscriptionTopic(String greenhouseIpAddress){
    return getBaseTopic(greenhouseIpAddress)+"/set/+";
}

String getStatusTopic(String greenhouseIpAddress) {
    return getBaseTopic(greenhouseIpAddress)+ "/status";
}

void connectToMqtt() {
    Serial.println("[MQTT] Attempting connection...");
    Serial.printf("[MQTT] Broker: %s:%d\n", netConfig.mqtt_host.toString().c_str(), netConfig.mqtt_port);
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  mqttClient.subscribe(getSubscriptionTopic(greenhouse.ipAddress).c_str(), 1);
//  Serial.println("[MQTT] Subscriptions set to: " + getSubscriptionTopic(greenhouse.ipAddress).c_str());
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("[MQTT] Disconnected! Reason: %d\n", (int)reason);

  if (WiFi.isConnected()) {
    Serial.println("[MQTT] Reconnecting in 5s...");
    mqttReconnectTimer.once(5, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
//  Serial.printf("[MQTT] Publish ACK for PID %d\n", packetId);
}

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties props,
                   size_t len, size_t idx, size_t total) {

  String topicStr = String(topic);
  String msg;
  msg.reserve(len+1);
  for (size_t i=0;i<len;i++) msg += payload[i];


  if (topicStr.startsWith(getSubscriptionTopic(greenhouse.ipAddress).c_str())) {
    if(topicStr.endsWith("/model")){
       Serial.println("[MQTT] Received New Model File");
        newModelMessage = msg.c_str();
        newModelMessageLen = msg.length();
        newModelMessageArrived = true;
        return;
    }else if(topicStr.endsWith("/config")){
       Serial.println("[MQTT] Received New Config File");
       newConfigMessage = msg.c.str();
       newConfigMessageLen = msg.length();
       newConfigMessageArrived = true;
    }else if(topicStr.endsWith("/mapping")){
       Serial.println("[MQTT] Received New Mapping File");
       newBindingMessage = msg.c.str();
       newBindingMessageLen = msg.length();
       newBindingMessageArrived = true;
    }
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

void publishTelemetryJson(const String& jsonPayload, String greenhouseIpAddress) {
    if (!mqttClient.connected()) {
//        Serial.println("[WARN] MQTT not connected, cannot publish");
        return;
    }

    String topic = getStatusTopic(greenhouseIpAddress);

    // Publish: QoS 0, Retain False (for live status)
    mqttClient.publish(topic.c_str(), 0, false, jsonPayload.c_str());

    // Serial.printf("[MQTT] Published %d bytes to %s\n", jsonPayload.length(), topic.c_str());
}
