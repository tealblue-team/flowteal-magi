#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "lib/ArduinoJson-v6.19.4.h"
#include "config.hpp"

#define USE_SERIAL Serial
#define SPRINKLER_ENDPOINT "rp"
#define SPRINKLER_PROGTYPE "84"
#define SPRINKLER_ZONEINDEX "0"
#define SPRINKLER_DUR_SEC "5"

const char CMD_START_ZONE_1[] = "http://" SPRINKLER_IP "/" SPRINKLER_ENDPOINT "?dkey=" SPRINKLER_KEY "&pid=" SPRINKLER_PROGTYPE "&zid=" SPRINKLER_ZONEINDEX "&dur=" SPRINKLER_DUR_SEC;
const char CMD_LIST_PARAMS[] = "http://" SPRINKLER_IP "/jc";

WiFiMulti wifiMulti;
HTTPClient http;

void setup() {
    USE_SERIAL.begin(115200);
    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();
    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }
    wifiMulti.addAP(SSID_NAME, SSID_PASSWORD);
    USE_SERIAL.print("[HTTP] begin...\n");
    //http.useHTTP10(true);
    http.begin(CMD_START_ZONE_1);
    //http.begin(CMD_LIST_PARAMS);
}

void loop() {
    if((wifiMulti.run() == WL_CONNECTED)) {
        USE_SERIAL.print("[HTTP] GET...\n");
        int httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
            if(httpCode == HTTP_CODE_OK) {
              StaticJsonDocument<64> doc;
              DeserializationError error = deserializeJson(doc, http.getString());
              if (error) {
                USE_SERIAL.print("deserializeJson() failed: ");
                USE_SERIAL.println(error.c_str());
                return;
              }
              int result = doc["result"]; // 1
              const char* item = doc["item"]; // nullptr
              USE_SERIAL.println(result == 1 ? "CMD succeeded" : "CMD failed");
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    delay(10000);
}
