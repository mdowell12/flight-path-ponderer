#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "EPD_3in97.h"
#include "GUI_Paint.h"
#include "fonts.h"

const char* ssid       = SECRET_SSID;
const char* password   = SECRET_PASS;
const char* lambdaUrl  = SECRET_LAMBDA_URL;

struct FlightData {
  String flight;
  String origin;
  String origin_city;
  String destination;
  String destination_city;
  String aircraft_type;
  int    altitude_ft;
  int    speed_kts;
  int    eta_minutes;
  bool   valid;
};

void connectWifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");
}

FlightData fetchFlightData() {
  FlightData data = {};
  data.valid = false;

  if (WiFi.status() != WL_CONNECTED) return data;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, lambdaUrl);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", code);
    http.end();
    return data;
  }

  String payload = http.getString();
  http.end();
  Serial.println(payload);

  JsonDocument body;
  DeserializationError err = deserializeJson(body, payload);
  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return data;
  }

  data.flight           = body["flight"].as<String>();
  data.origin           = body["origin"].as<String>();
  data.origin_city      = body["origin_city"].as<String>();
  data.destination      = body["destination"].as<String>();
  data.destination_city = body["destination_city"].as<String>();
  data.aircraft_type    = body["aircraft_type"].as<String>();
  data.altitude_ft      = body["altitude_ft"].as<int>();
  data.speed_kts        = body["speed_kts"].as<int>();
  data.eta_minutes      = body["eta_minutes"].as<int>();
  data.valid            = true;
  return data;
}

void renderDisplay(const FlightData& data) {
  UWORD imageSize = ((EPD_3IN97_WIDTH % 8 == 0) ? (EPD_3IN97_WIDTH / 8) : (EPD_3IN97_WIDTH / 8 + 1)) * EPD_3IN97_HEIGHT;
  UBYTE *image = (UBYTE *)malloc(imageSize);
  if (!image) {
    Serial.println("malloc failed");
    return;
  }

  Paint_NewImage(image, EPD_3IN97_WIDTH, EPD_3IN97_HEIGHT, 0, WHITE);
  Paint_Clear(WHITE);

  if (!data.valid) {
    Paint_DrawString_EN(20, 220, "No flight data", &Font24, BLACK, WHITE);
  } else {
    char buf[64];

    // Flight number
    Paint_DrawString_EN(20, 20, data.flight.c_str(), &Font24, BLACK, WHITE);

    // Route: KSEA -> KPHX
    snprintf(buf, sizeof(buf), "%s  ->  %s", data.origin.c_str(), data.destination.c_str());
    Paint_DrawString_EN(20, 70, buf, &Font24, BLACK, WHITE);

    // City names
    snprintf(buf, sizeof(buf), "%s  to  %s", data.origin_city.c_str(), data.destination_city.c_str());
    Paint_DrawString_EN(20, 115, buf, &Font16, BLACK, WHITE);

    // Separator line
    Paint_DrawLine(20, 150, 780, 150, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Aircraft, altitude, speed, ETA
    snprintf(buf, sizeof(buf), "Aircraft: %s", data.aircraft_type.c_str());
    Paint_DrawString_EN(20, 170, buf, &Font20, BLACK, WHITE);

    snprintf(buf, sizeof(buf), "Altitude: %d ft", data.altitude_ft);
    Paint_DrawString_EN(20, 210, buf, &Font20, BLACK, WHITE);

    snprintf(buf, sizeof(buf), "Speed:    %d kts", data.speed_kts);
    Paint_DrawString_EN(20, 250, buf, &Font20, BLACK, WHITE);

    snprintf(buf, sizeof(buf), "ETA:      %d min", data.eta_minutes);
    Paint_DrawString_EN(20, 290, buf, &Font20, BLACK, WHITE);
  }

  EPD_3IN97_Init();
  EPD_3IN97_Display(image);
  EPD_3IN97_Sleep();
  free(image);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  DEV_Module_Init();
  connectWifi();
}

void loop() {
  Serial.println("Fetching flight data...");
  FlightData data = fetchFlightData();
  renderDisplay(data);
  Serial.println("Display updated. Sleeping 30s.");
  delay(30000);
}
