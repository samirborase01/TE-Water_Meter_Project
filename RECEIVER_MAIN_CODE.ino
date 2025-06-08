#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// LoRa Pin Configuration
#define LORA_NSS 5
#define LORA_RST 2
#define LORA_DIO0 4
#define LORA_FREQUENCY 433E6

// WiFi Credentials
const char* ssid = "jiomoto";
const char* password = "pramod2031";

// Google Apps Script Web App URL
const char* scriptURL = "https://script.google.com/macros/s/AKfycbzAYjv89FLLcNoda4sjkLAOaCJ1bUm1BVp-3BSALvbj4RgLGCNOmVzDWDsvHzKz7sGJ/exec";

// Reliable WiFi Connection Function
void connectToWiFi() {
  Serial.print("🔌 Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 20000;  // 20 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("📶 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Connection Failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();

  // Initialize LoRa
  Serial.println("📡 Initializing LoRa...");
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("❌ LoRa Initialization Failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(10);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setTxPower(20);
  LoRa.enableCrc();

  Serial.println("✅ LoRa Receiver Ready!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("✅ LoRa Packet Received!");

    String receivedMessage = "";
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      Serial.print(c);
      receivedMessage += c;
    }
    Serial.println();  
    Serial.println("📩 Full Data: " + receivedMessage);

    // Send to Google Sheets
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(scriptURL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Send meter and data
      String postData = "meter=1&data=" + receivedMessage;
      int httpResponseCode = http.POST(postData);

      if (httpResponseCode > 0) {
        Serial.print("✅ Data sent to Google Sheets. Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("❌ Failed to send data. Error: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
      }

      http.end();
    } else {
      Serial.println("⚠ WiFi Disconnected. Reconnecting...");
      connectToWiFi();
    }
  }
}