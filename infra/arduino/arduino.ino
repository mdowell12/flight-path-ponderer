#include <WiFi.h>
#include "arduino_secrets.h"  // Not version controlled because contains secrets

// --- Credentials ---
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

void setup() {
  // Use 115200 for ESP32-S3
  Serial.begin(115200);
  
  // Wait for Serial to initialize (crucial for ESP32-S3 USB CDC)
  while (!Serial) {
    delay(10);
  }

  Serial.println("\n--- WiFi Connection Test ---");
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  // Start the connection
  WiFi.begin(ssid, password);

  // Monitor the status
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // We'll add your AWS Lambda request here next!
}