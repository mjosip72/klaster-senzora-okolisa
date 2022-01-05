
#if false

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;

void setup() {

  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.print("Pozdrav svijete");

}

void loop() {
  
}

#endif

#if true

#include <Arduino.h>
#include "examples.h"

#endif
