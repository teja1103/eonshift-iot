#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// WiFi credentials
const char *ssid = "Wifi";
const char *password = "password";

// MQTT Broker details
const char *mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char *mqttClientId = "ESP8266Clientviki123453"; // Ensure this is unique if connecting multiple devices
const char *mqttPublishTopic = "eonshift/292720d6-9447-4256-ab3c-d3adbd98c6bc/energy";
const char *mqttSubscribeTopic = "292720d6-9447-4256-ab3c-d3adbd98c6bc/status";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_INA219 ina219;

bool isActive = true; // Global variable to track active/inactive status

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
      digitalWrite(LED_BUILTIN, LOW); // Turn on the light (LED)
      Serial.println("Light turned ON and data publishing resumed");
    } else if (message == "inactive") {
      isActive = false;
      digitalWrite(LED_BUILTIN, HIGH); // Turn off the light (LED)
      Serial.println("Light turned OFF and data publishing stopped");
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
    // Check if the INA219 sensor is detected
    if (!ina219.begin()) {
      Serial.println("Failed to find INA219 chip. Please check wiring and power.");
      return;
    }

    float current_mA = ina219.getCurrent_mA(); // Current in milliamps
    float voltage_V = ina219.getBusVoltage_V(); // Voltage in volts
    float power_W = (current_mA / 1000.0) * voltage_V; // Power in watts

    // Time interval is 5 seconds, or 0.001389 hours (5 / 3600)
    float energy_Wh = power_W * 0.001389; // Energy in watt-hours for the 5-second interval

    // Convert energy to a string
    char energyStr[20];
    dtostrf(energy_Wh, 6, 4, energyStr); // Convert float to string

    // Print the energy data to the Serial Monitor
    Serial.print("Energy (Wh) for 5 seconds: ");
    Serial.println(energyStr);

    client.publish(mqttPublishTopic, energyStr);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Start I2C interface
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  pinMode(LED_BUILTIN, OUTPUT);

  // Check if the INA219 sensor is detected during setup
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip during setup. Please check wiring and power.");
    while (1) { delay(10); }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastPublishTime = millis();
  if (millis() - lastPublishTime > 5000) { // Publish every 5 seconds
    publishEnergyData();
    lastPublishTime = millis();
  }
}
