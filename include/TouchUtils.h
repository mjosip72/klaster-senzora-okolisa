
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#define TOUCH_EVENT_NONE 0
#define TOUCH_EVENT_TOUCH_DOWN 10
#define TOUCH_EVENT_SWIPE 20

#define SWIPE_UP 21
#define SWIPE_DOWN 22
#define SWIPE_LEFT 23
#define SWIPE_RIGHT 24

#define TOUCH_STABLE_THRESHOLD_MICROS 20000
#define TOUCH_EVENT_SWIPE_DISTANCE_THRESHOLD 60
#define TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD 15

extern TFT_eSPI tft;

namespace TouchUtils {

  uint8_t irqPin;

  bool touchDown = false;
  bool stable = true;
  uint64_t lastStableTime;
  bool touchState;

  uint16_t x, y, sx, sy;
  uint8_t swipe;
  bool sFlag;

  void begin(uint8_t _irqPin) {
    irqPin = _irqPin;
  }

  uint8_t update() {

    uint8_t e = TOUCH_EVENT_NONE;

    bool touchState = !digitalRead(irqPin);

    if(touchState) {
      if(!touchDown) {
        touchDown = true;
        sFlag = false;
      }
      stable = true;
    }else{
      if(touchDown && stable) {
        stable = false;
        lastStableTime = micros();
      }
    }

    if(!stable) {
      if(micros() - lastStableTime >= TOUCH_STABLE_THRESHOLD_MICROS) {

        stable = true;
        touchDown = false;
        
        if(sFlag) {

          float dis = sqrtf(powf(x - sx, 2) + powf(y - sy, 2));

          if(dis < TOUCH_EVENT_SWIPE_DISTANCE_THRESHOLD) {

            e = TOUCH_EVENT_TOUCH_DOWN;
            x = sx;
            y = sy;

          }else{

            float dy = y - sy;
            float dx = x - sx;
            if(dx == 0) dx = 0.1;

            float a = atan2f(dy, dx) * RAD_TO_DEG;
            if(a < 0) a += 360;

            swipe = 0;

            if(a >= -TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD && a <= TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD)
              swipe = SWIPE_RIGHT;
            else if(a >= 90 - TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD && a <= 90 + TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD)
              swipe = SWIPE_DOWN;
            else if(a >= 180 - TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD && a <= 180 + TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD)
              swipe = SWIPE_LEFT;
            else if(a >= 270 - TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD && a <= 270 + TOUCH_EVENT_SWIPE_ANGLE_THRESHOLD)
              swipe = SWIPE_UP;
            
            if(swipe != 0) e = TOUCH_EVENT_SWIPE;

          }
        }

      }
    }

    if(touchState) {
      if(tft.getTouch(&x, &y)) {
        if(!sFlag) {
          sFlag = true;
          sx = x;
          sy = y;
        }
      }
    }

    return e;
  }

  bool touchInside(int16_t bx, int16_t by, int16_t w, int16_t h) {
    return x >= bx && x <= bx + w && y >= by && y <= by + h;
  }

}
