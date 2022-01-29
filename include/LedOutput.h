
#pragma once
#include <Arduino.h>

#define LED_OUTPUT_SUCCESS 2, 200, 200
#define LED_OUTPUT_WARNING 4, 50, 50
#define LED_OUTPUT_ERROR 12, 50, 50

namespace LED {

  bool run = false;
  int8_t pin = -1;
  uint8_t blinks = 0;
  uint32_t onTime = 0;
  uint32_t offTime = 0;
  uint64_t time = 0;
  bool on = false;

  void begin(uint8_t _pin) {
    pin = _pin;
  }

  void start(uint8_t _blinks, uint32_t _onTime, uint32_t _offTime = 0) {
    if(pin == -1) return;
    if(_offTime == 0) _offTime = _onTime;
    blinks = _blinks;
    onTime = _onTime;
    offTime = _offTime;
    run = true;
    time = millis();
    on = true;
    digitalWrite(pin, HIGH);
  }

  void loop() {

    if(!run) return;

    uint64_t now = millis();

    if(on) {
      if(now - time >= onTime) {
        time = now;
        on = false;
        digitalWrite(pin, LOW);
        blinks--;
        if(blinks == 0) run = false;
      }
    }else{
      if(now - time >= offTime) {
        time = now;
        on = true;
        digitalWrite(pin, HIGH);
      }
    }

  }

}
