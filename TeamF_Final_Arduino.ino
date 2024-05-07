#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"  // Include for NTP functions

// Network Configuration
const char* ssid = "Tufts_Wireless";
const char* password = "";  // Empty password for demonstration

// MQTT Broker Configuration
const char* mqtt_server = "en1-pi.eecs.tufts.edu";
const char* mqtt_publish_topic = "teamF/node3/tempupdate";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Sensor Configuration
const int I2C_SDA = 7;  // Adjusted SDA pin
const int I2C_SCL = 8;  // Adjusted SCL pin
const int sensorAddress = 0x37;  // I2C address of PCT2075
const int enablePin = 4;         // GPIO4 used to enable temperature reading

// RTC Memory to store the battery level across deep sleeps
RTC_DATA_ATTR float batteryLevel = 100.0;

void setup_wifi() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initiate NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync: ");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 24 * 3600) {
    delay(100);
    Serial.print(".");
    nowSecs = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_gpio() {
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);  // Set GPIO4 high to enable the sensor
}

float readTemperature() {
  Wire.beginTransmission(sensorAddress);
  Wire.write(0x00); // Temperature register
  Wire.endTransmission();
  Wire.requestFrom(sensorAddress, 2);
  if (Wire.available() == 2) {
    int tempRaw = (Wire.read() << 8) | Wire.read();
    return (tempRaw >> 5) * 0.125; // Convert to Celsius
  }
  return 0.0;
}

void setup() {
  setup_wifi();
  setup_gpio();  // Setup GPIO pin
  client.setServer(mqtt_server, 1883);
  Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C communication
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  time_t now;
  time(&now);  // Get the current epoch time

  // Convert epoch time to string
  char ascii_epoch_time[20];
  sprintf(ascii_epoch_time, "%lu", now);

  delay(5000);
  float temperature = readTemperature();
//  float temperature = readTemperature();
  char message[100];
  snprintf(message, sizeof(message), "%s,%.2f,%.2f", ascii_epoch_time, temperature, batteryLevel);

  client.publish(mqtt_publish_topic, message);
  Serial.println("Published message: ");
  Serial.println(message);

  // Decrement battery level randomly between 0.01 and 0.05
  batteryLevel -= (random(1, 6) / 100.0);

  Serial.println("Going into deep sleep for 1 hour...");
  esp_sleep_enable_timer_wakeup(3600 * 1000000UL); // 1 hour in microseconds
  //esp_sleep_enable_timer_wakeup(60 * 1000000); // 1 min in microseconds
  //esp_sleep_enable_timer_wakeup(300 * 1000000UL); // 5 minutes in microseconds
  esp_deep_sleep_start();
}


//code receiving ascii-epoch-time from his server
/*#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_sleep.h>  // Include to manage deep sleep

// Network Configuration
const char* ssid = "Tufts_Wireless";
const char* password = "";  // Empty password for demonstration

// MQTT Broker Configuration
const char* mqtt_server = "en1-pi.eecs.tufts.edu";
const char* mqtt_subscribe_topic = "time";
const char* mqtt_publish_topic = "teamF/node0/tempupdate";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Sensor Configuration
const int I2C_SDA = 7;
const int I2C_SCL = 8;
const int sensorAddress = 0x37;  // I2C address of PCT2075
const int enablePin = 4;         // GPIO4 used to enable temperature reading

// Global variable to store the epoch time received via MQTT
char ascii_epoch_time[20] = "0";

// RTC Memory to store the battery level across deep sleeps
RTC_DATA_ATTR float batteryLevel = 100.0;

void setup_wifi() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null-terminate the payload
  // No need to copy as time is not processed in this example
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(mqtt_subscribe_topic); // Subscribe to the time topic
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_gpio() {
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);  // Set GPIO4 high to enable the sensor
}

float readTemperature() {
  Wire.beginTransmission(sensorAddress);
  Wire.write(0x00); // Temperature register
  Wire.endTransmission();
  Wire.requestFrom(sensorAddress, 2);
  if (Wire.available() == 2) {
    int tempRaw = (Wire.read() << 8) | Wire.read();
    return (tempRaw >> 5) * 0.125; // Convert to Celsius
  }
  return 0.0;
}

void setup() {
  setup_wifi();
  setup_gpio();  // Setup GPIO pin
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C communication
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temperature = readTemperature();
  char message[100];
  snprintf(message, sizeof(message), "%s,%.2f,%.2f", ascii_epoch_time, temperature, batteryLevel);

  client.publish(mqtt_publish_topic, message);
  Serial.println("Published message: ");
  Serial.println(message);

  // Decrement battery level randomly between 0.01 and 0.05
  batteryLevel -= (random(1, 6) / 100.0);

  Serial.println("Going into deep sleep for 1 hour...");
  esp_sleep_enable_timer_wakeup(60 * 1000000); // 30 seconds in microseconds
  esp_deep_sleep_start();
}
*/
