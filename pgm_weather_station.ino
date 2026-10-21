#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";

#define SDA_PIN 5
#define SCL_PIN 6
#define DHT_PIN 10

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
DHTesp dht;

float tempMarseille = 0.0;
float humMarseille = 0.0;
bool apiSuccess = false;

unsigned long lastApiUpdate = 0;
const unsigned long apiInterval = 600000; 
unsigned long lastDisplaySwitch = 0;
const unsigned long displayInterval = 10000; 
bool showLocal = true;

void fetchMarseilleWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=43.2965&longitude=5.3698&current=temperature_2m,relative_humidity_2m";
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        tempMarseille = doc["current"]["temperature_2m"];
        humMarseille = doc["current"]["relative_humidity_2m"];
        apiSuccess = true;
      }
    }
    http.end();
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  dht.setup(DHT_PIN, DHTesp::DHT22);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(2, 18, "Connexion");
  u8g2.drawStr(2, 35, "WiFi...");
  u8g2.sendBuffer();

  WiFi.begin(ssid, password);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    fetchMarseilleWeather();
  }
  
  lastApiUpdate = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastApiUpdate >= apiInterval) {
    lastApiUpdate = currentMillis;
    fetchMarseilleWeather();
  }

  if (currentMillis - lastDisplaySwitch >= displayInterval) {
    lastDisplaySwitch = currentMillis;
    showLocal = !showLocal;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  if (showLocal) {
    TempAndHumidity values = dht.getTempAndHumidity();
    if (dht.getStatus() == 0) {
      char tempStr[15], humStr[15];
      snprintf(tempStr, sizeof(tempStr), "H T:%.1fC", values.temperature);
      snprintf(humStr, sizeof(humStr), "H H:%.1f%%", values.humidity);
      u8g2.drawStr(2, 18, tempStr);
      u8g2.drawStr(2, 35, humStr);
    } else {
      u8g2.drawStr(2, 18, "Err Maison");
    }
  } else {
    if (apiSuccess) {
      char tempStr[15], humStr[15];
      snprintf(tempStr, sizeof(tempStr), "M T:%.1fC", tempMarseille);
      snprintf(humStr, sizeof(humStr), "M H:%.0f%%", humMarseille);
      u8g2.drawStr(2, 18, tempStr);
      u8g2.drawStr(2, 35, humStr);
    } else {
      u8g2.drawStr(2, 18, "Err API");
    }
  }

  u8g2.sendBuffer();
  delay(1000);
}