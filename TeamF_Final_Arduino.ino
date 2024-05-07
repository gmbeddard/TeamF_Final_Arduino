#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Network Configuration
const char* ssid = "Tufts_Wireless";
const char* password = "";

// MQTT Broker Configuration
const char* mqtt_server = "en1-pi.eecs.tufts.edu";
const char* mqtt_subscribe_topic = "time";
const char* mqtt_publish_topic = "teamF/node1/tempupdate";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Sensor Configuration
const int I2C_SDA = 8;
const int I2C_SCL = 7;
const int sensorAddress = 0x37;  // I2C address of PCT2075

// Global variable to store the epoch time received via MQTT
char ascii_epoch_time[20] = "0";

void setup_wifi() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress()); // Print the MAC address
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, mqtt_subscribe_topic) == 0) {
    // Ensure the payload is null-terminated
    payload[length] = '\0';
    // Copy the payload to the global epoch time variable
    strncpy(ascii_epoch_time, (char*)payload, sizeof(ascii_epoch_time) - 1);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      // Subscribe to the time topic
      client.subscribe(mqtt_subscribe_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
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
  snprintf(message, sizeof(message), "%s,%.2f,3.7V", ascii_epoch_time, temperature);

  client.publish(mqtt_publish_topic, message);
  Serial.println("Published message: ");
  Serial.println(message);

  Serial.println("Going into deep sleep for 30 seconds...");
  esp_sleep_enable_timer_wakeup(30 * 1000000); // 30 seconds in microseconds
  esp_deep_sleep_start();
}
