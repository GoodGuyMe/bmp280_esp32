#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include "esp_wifi.h"

WiFiClient espClient;
PubSubClient client(espClient);

#include "secrets.h"
//File that contains the following:
//const char* ssid = **; // Enter your WiFi name
//const char* password = **; // Enter WiFi password
//const char* mqtt_server = **;
//const char* publish_path = **;
//#define mqtt_port 1883
//#define MQTT_USER **
//#define MQTT_PASSWORD **

#define MSG_BUFFER_SIZE  (100)
char msg[MSG_BUFFER_SIZE];


#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

float tempOffset = 0;
int SleepSecs = 30;

Adafruit_BMP280 bmp;

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    int count = 0;
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
      count++;
      if (count  > 100) {
        Serial.println("");
        Serial.println("Restarting ESP as it can't connect to wifi");
        Serial.flush();
        ESP.restart();
      }
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      // ... and resubscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  esp_wifi_start();
  Serial.println(F("BMP280 Temperature Sensor"));
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  reconnect();

  while (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    delay(1000);
  }
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_4000); /* Standby time. */
    if (!client.connected()) {
      reconnect();
    }
    Serial.println(ESP.getFreeHeap());
    bmp.wakeup();
    float temp = bmp.readTemperature() + tempOffset;
    Serial.print("Temp: ");
    Serial.println(temp);

    snprintf (msg, MSG_BUFFER_SIZE, "{\"temperature\": %.2f}", temp);
    client.publish(publish_path, msg);
    client.disconnect();
    Serial.println("going into deep sleep:");
    esp_sleep_enable_timer_wakeup(SleepSecs * 1000000);
    bmp.sleep();
    Serial.flush();
    WiFi.disconnect();
    esp_wifi_stop();
    delay(500);
    esp_deep_sleep_start();
}

void loop() {
}
