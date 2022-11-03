#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "lib/ArduinoJson-v6.19.4.h"
#include "config.hpp"

//#define USE_SERIAL Serial

// Sprinkler connection (WiFi)
#define SPRINKLER_ENDPOINT "rp"
#define SPRINKLER_PROGTYPE "84"
#define SPRINKLER_ZONEINDEX "0"
#define SPRINKLER_DUR_SEC "5"
const char CMD_START_ZONE_1[] = "http://" SPRINKLER_IP "/" SPRINKLER_ENDPOINT "?dkey=" SPRINKLER_KEY "&pid=" SPRINKLER_PROGTYPE "&zid=" SPRINKLER_ZONEINDEX "&dur=" SPRINKLER_DUR_SEC;
const char CMD_LIST_PARAMS[] = "http://" SPRINKLER_IP "/jc";
WiFiMulti wifiMulti;
HTTPClient http;

// Soil moisture sensor connection (LoRa)
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define RF95_FREQ 868.0
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
// Blinky on receipt
#define LED 13

void setup_wifi() {
    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
    wifiMulti.addAP(SSID_NAME, SSID_PASSWORD);
    Serial.print("[HTTP] begin...\n");
    http.begin(CMD_START_ZONE_1);
    //http.begin(CMD_LIST_PARAMS);
}

void setup_lora() {
  Serial.println();
  Serial.println();
  Serial.println();
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial1.println("Arduino LoRa RX Test!");
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  rf95.setSpreadingFactor(9);
  rf95.setSignalBandwidth(125000);
  rf95.setCodingRate4(8);
  rf95.setPreambleLength(8);
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(13, true);
}

void loop_wifi() {
if((wifiMulti.run() == WL_CONNECTED)) {
        Serial.print("[HTTP] GET...\n");
        int httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            if(httpCode == HTTP_CODE_OK) {
              StaticJsonDocument<64> doc;
              DeserializationError error = deserializeJson(doc, http.getString());
              if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
              }
              int result = doc["result"]; // 1
              const char* item = doc["item"]; // nullptr
              Serial.println(result == 1 ? "CMD succeeded" : "CMD failed");
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
}

void loop_lora() {
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
       Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      
      // Send a reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("Ok?");
  setup_lora();
  //setup_wifi();
}

void loop() {
  loop_lora();
}
