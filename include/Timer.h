
#pragma once

#include <Arduino.h>

#define TIMER_STATE_OFF 0
#define TIMER_STATE_ONCE 1
#define TIMER_STATE_REPEAT 2

extern uint64_t getTime();

class Timer {

  private:

  uint64_t time1;
  uint64_t time2;
  void (*func)();

  uint8_t state;

  public:

  uint64_t period;

  Timer(void (*_func)()) {
    func = _func;
    state = TIMER_STATE_OFF;
  }

  void loop() {

    if(state == TIMER_STATE_OFF) return;

    time2 = getTime();
    if(time2 - time1 >= period) {

      if(state == TIMER_STATE_REPEAT) {
        time1 = time2;
        func();
      }else if(state == TIMER_STATE_ONCE) {
        state = TIMER_STATE_OFF;
        func();
      }

    }
  }

  void once(uint64_t _period) {
    period = _period;
    state = TIMER_STATE_ONCE;
    time1 = millis();
  }

  void repeat(uint64_t _period) {
    period = _period;
    state = TIMER_STATE_REPEAT;
    time1 = millis();
  }

};