#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "arduino_secrets.h"  // Not version controlled because contains secrets

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* lambdaUrl = SECRET_LAMBDA_URL;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("Start loop");
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Skip cert validation; Lambda URL uses AWS-managed cert

    HTTPClient http;
    http.begin(client, lambdaUrl);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Lambda response:");
      Serial.println(payload);
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected, skipping request");
  }

  delay(30000);  // Poll every 30 seconds
}