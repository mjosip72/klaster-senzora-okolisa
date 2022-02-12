
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <SD.h>
#include <FS.h>
#include "TouchUtils.h"
#include <ESPmDNS.h>
#define DEBUG
#include "Log.h"

// TFT_SCLK 18
// TFT_MOSI 23
// TFT_MISO 19
// TFT_CS 32
// TFT_DC 33
// TFT_RST 25
// TOUCH_CS 26
#define TOUCH_IRQ 27
#define SD_CS 14
#define LED_PIN 13

TFT_eSPI tft;

void _setup();
void _loop();

void setup() {

	Log::begin(115200);

	digitalWrite(TFT_CS, HIGH);
	digitalWrite(TOUCH_CS, HIGH);
	digitalWrite(SD_CS, HIGH);

	pinMode(TOUCH_IRQ, INPUT);
	pinMode(LED_PIN, OUTPUT);

	tft.begin();
	tft.setRotation(1);
	uint16_t calData[5] = {329, 3577, 258, 3473, 7};
  tft.setTouch(calData);
	tft.fillScreen(TFT_WHITE);

  _setup();

}

void loop() {
  _loop();
}

void SD_begin() {

	if(!SD.begin(14)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE || cardType != CARD_SDHC){
    Serial.println("No SD card attached");
    return;
  }

}
