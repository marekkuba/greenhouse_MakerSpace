// START OF INTRODUCTION
// In Adriuino IDE install in Sketch->Include Library->Manage Librares following libraries "Adafruit Unified Sensor", "DHT sensor library"
// From tutorial https://randomnerdtutorials.com/esp8266-nodemcu-mqtt-publish-dht11-dht22-arduino/ for MQTT we need to also install two libraries:
// https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip 
// https://github.com/marvinroger/async-mqtt-client/archive/master.zip
// after downloading .zip include it in sketch->Include Library->Add .ZIP library
// END OF INTRODUCTION
#include "DHT.h" // biblioteka do obslugi czujnika DHT22
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

// Wiersze konfiguracyjne WiFi - musi być 2.4GHz
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASS"

// Uruchomiłem to na Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, XXX) // XXX zamienić na odpowiednie IP MQTT brokera
// Jeśli będzie w chmurze to trzeba tu dać nazwę domeny, ale na razie pisałem to na LANa
//#define MQTT_HOST "example.com"

// Domyślny port na MQTT
#define MQTT_PORT 1883 

// Topics do publikacji
#define MQTT_PUB_TEMP "esp/sensors/temperature"
#define MQTT_PUB_HUM "esp/sensors/humidity"
#define MQTT_PUB_SOIL "esp/sensors/soil"


// Wiersze konfiguracyjne PINow jakie podpialem
#define DHTPIN D4         // sygnał z czujnika DHT22 - temperatura i wilgotnosc powietrza
#define SOIL_ANALOG A0   // analogowy sygnał wilgotności gleby
#define SOIL_DIGITAL D2  // cyfrowy sygnał z czujnika (D0)

// Typ czujnika DHT jaki używamy
#define DHTTYPE DHT22 
 

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to hold sensor readings
float temp;
float hum;
float soilAnalog;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Zmienna zapisujaca kiedy obyło sie ostatnia MQTT_pub w milisekundach
const long interval = 10000;        // czas w milisekundach po jakim ma publikowac dane na MQTT

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

// Na tę chwilę zakomentowane MQTT_subscribe - potem używane do zmiany parametrów toggle
/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200); // na innych niz 115200 wylatuje mi gibberish, ale nie wiem do końca o co chodzi xd (ale sie dowiem)
  Serial.println();

// ustawienie modu analogwego pinu od wilgotności gleby na INPUT
  pinMode(SOIL_DIGITAL, INPUT);
// inicjalizacja sensora DHT22
  dht.begin();
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // Jeśli remote access musi byc authenticated to odkomentować to z username i password uzyty do authentication
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Update timestamp ostatniego odczytu danych z sensorów w milisekundach
    previousMillis = currentMillis;
    // Update wilgotnosci
    hum = dht.readHumidity();
    // Update temperatury 
    temp = dht.readTemperature();
    // Update wilgotności gleby
    soilAnalog = analogRead(SOIL_ANALOG);
    
    // MQTT_pub na topic esp/sensors/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // MQTT_pub na topic esp/sensors/soil
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_SOIL, 1, true, String(soilAnalog).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_SOIL, packetIdPub1);
    Serial.printf("Message: %.2f \n", soilAnalog);

    // MQTT_pub na topic esp/sensors/humidity
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);
  }
}
