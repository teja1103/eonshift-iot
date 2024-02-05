#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
const char *ssid = "rpi";
const char *password = "password123";

// MQTT Broker details
const char *mqttServer = "10.42.0.1";
const int mqttPort = 1883;
const char *mqttClientId = "ESP8266ClientMachine55455445sda3332"; // Ensure this is unique if connecting multiple devices
const char *mqttPublishTopic = "eonshift/8f55bad2-974d-4aa1-b100-2a1ce54f2931/energy";
const char *mqttSubscribeTopic = "eonshift/8f55bad2-974d-4aa1-b100-2a1ce54f2931/status";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_INA219 ina219;

const int relayPin = D5;  // Relay control pin
bool isActive = false;    // Active/inactive status

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == mqttSubscribeTopic) {
    if (message == "active") {
      isActive = true;
      digitalWrite(relayPin, HIGH); // Turn on the relay (motor on)
      Serial.println("Device activated, motor turned ON");
    } else if (message == "inactive") {
      isActive = false;
      digitalWrite(relayPin, LOW); // Turn off the relay (motor off)
      Serial.println("Device deactivated, motor turned OFF");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqttClientId)) {
      Serial.println("connected");
      client.subscribe(mqttSubscribeTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void publishEnergyData() {
  if (isActive) {
    float current_mA = ina219.getCurrent_mA();
    float voltage_V = ina219.getBusVoltage_V();
    float power_W = (current_mA / 1000.0) * voltage_V;
    float energy_Wh = (power_W * 0.001389)* 1000; // Energy for the 5-second interval

    if (energy_Wh < 0) {
      energy_Wh = 0;
    }
    
    char energyStr[20];
    dtostrf(energy_Wh, 6, 4, energyStr);
    Serial.print("Energy (Wh) for 5 seconds: ");
    Serial.println(energyStr);
    client.publish(mqttPublishTopic, energyStr);
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  Wire.begin();
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Relay is normally HIGH (motor off)

//  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastPublishTime = millis();
  if (millis() - lastPublishTime > 5000) {
    publishEnergyData();
    lastPublishTime = millis();
  }
}
