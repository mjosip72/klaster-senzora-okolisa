
#pragma once

#include <Arduino.h>

namespace Log {

  void begin(uint32_t baudRate) {
    #ifdef DEBUG
    Serial.begin(baudRate);
    #endif
  }

  void printf(const char* f, ...) {
    #ifdef DEBUG

    va_list args;
    va_start(args, f);
    char buffer[200];

    vsprintf(buffer, f, args);
    Serial.print(buffer);

    va_end(args);
    
    #endif
  }

  void print(const char* p) {
    #ifdef DEBUG
    Serial.print(p);
    #endif
  }

  void println(const char* p) {
    #ifdef DEBUG
    Serial.println(p);
    #endif
  }

  void println() {
    #ifdef DEBUG
    Serial.println();
    #endif
  }

}
