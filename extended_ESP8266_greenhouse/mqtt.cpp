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
  mqttClient.subscribe(getSubscriptionTopic(netConfig.device_ip).c_str(), 1);
  Serial.println("[MQTT] Connected!");
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

  // 1. If this is the start of a new message (idx == 0), reset the buffer
  static bool rejecting = false;
  if (idx == 0) {
      if (total > 8192) {
          Serial.printf("[MQTT] Oversized message (%u bytes) rejected\n", (unsigned)total);
          rejecting = true;
          return;
      }
      rejecting = false;
      incomingPayloadBuffer = "";
      incomingPayloadBuffer.reserve(total);
  }

  if (rejecting) return;

  // 2. Append the current chunk — bulk concat avoids per-character reallocation
  incomingPayloadBuffer.concat(payload, (unsigned int)len);

  // 3. Only process if we have received the TOTAL message length
  if (idx + len == total) {
      String topicStr = String(topic);

      // Use the global 'incomingPayloadBuffer' instead of the partial 'payload'
      if (topicStr.startsWith((getBaseTopic(netConfig.device_ip)+"/set/").c_str())) {

        if(topicStr.endsWith("/model")){
           Serial.println("[MQTT] Received New Model File (Full)");
            newModelMessage = std::move(incomingPayloadBuffer);
            newModelMessageArrived = true;
        }
        else if(topicStr.endsWith("/config")){
           Serial.println("[MQTT] Received New Config File (Full)");
           newConfigMessage = std::move(incomingPayloadBuffer);
           newConfigMessageArrived = true;
        }
        else if(topicStr.endsWith("/mapping")){
           Serial.println("[MQTT] Received New Mapping File (Full)");
           newBindingMessage = std::move(incomingPayloadBuffer);
           newBindingMessageArrived = true;
        }
      }

      incomingPayloadBuffer = "";
  }
}

void registerMqttHandlers() {
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(netConfig.mqtt_host, netConfig.mqtt_port);
    mqttClient.setKeepAlive(60);
    if (!netConfig.mqtt_username.isEmpty()) {
        mqttClient.setCredentials(netConfig.mqtt_username.c_str(), netConfig.mqtt_password.c_str());
    }
}

void publishTelemetryJson(const String& jsonPayload, String greenhouseIpAddress) {
    if (!mqttClient.connected()) {
        Serial.println("[WARN] MQTT not connected, cannot publish");
        return;
    }

    String topic = getStatusTopic(greenhouseIpAddress);

    // Publish: QoS 0, Retain False (for live status)
    mqttClient.publish(topic.c_str(), 0, false, jsonPayload.c_str());

     Serial.printf("[MQTT] Published %d bytes to %s\n", jsonPayload.length(), topic.c_str());
}
