
#if false

#include <Test.h>

void _setup() {
  TouchUtils::begin(TOUCH_IRQ);
}

void _loop() {

  uint8_t e = TouchUtils::update();
  if(e == TOUCH_EVENT_TOUCH_DOWN) {
    Log::printf("touch down %d, %d\n", TouchUtils::x, TouchUtils::y);
  }else if(e == TOUCH_EVENT_SWIPE) {
    Log::printf("swipe %d\n", TouchUtils::swipe);
  }

}

#endif
