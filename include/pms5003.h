
#pragma once

#include <Arduino.h>

// https://e-radionica.com/productdata/PMS5003.pdf

struct pms5003_data {

  uint16_t start_bytes;
  uint16_t frame_length;

  uint16_t pm10_standard;
  uint16_t pm25_standard;
  uint16_t pm100_standard;

  uint16_t pm10_env;
  uint16_t pm25_env;
  uint16_t pm100_env;
  
  uint16_t particles_03um;
  uint16_t particles_05um;
  uint16_t particles_10um;
  uint16_t particles_25um;
  uint16_t particles_50um;
  uint16_t particles_100um;

  uint16_t reserved;
  uint16_t checksum;

} pms5003;

namespace PMS5003 {

  HardwareSerial* serial;

  int8_t rxPin = -1;
  int8_t txPin = -1;
  
  bool running = false;

  void init(uint8_t _rxPin, uint8_t _txPin) {
    rxPin = _rxPin;
    txPin = _txPin;
  }

  void begin(HardwareSerial* _serial) {
    serial = _serial;
    serial->begin(9600, SERIAL_8N1, rxPin, txPin);
    running = true;
  }

  void end() {
    serial->end();
    running = false;
  }

  bool run() {

    if(!running) return false;

    if(serial->available() == 0) return false;

    if(serial->peek() != 0x42) {
        serial->read();
        return false;
    }

    if(serial->available() < 32) return false;

    uint8_t buffer[32];
    uint16_t sum = 0;
    serial->readBytes(buffer, 32);

    for(uint8_t i = 0; i < 30; i++) sum += buffer[i];

    for(uint8_t i = 0; i < 32; i+=2) {
      uint8_t t = buffer[i];
      buffer[i] = buffer[i + 1];
      buffer[i + 1] = t;
    }

    memcpy(&pms5003, buffer, 32);

    if(pms5003.start_bytes != 0x424d) return false;
    if(pms5003.frame_length != 28) return false;
    if(pms5003.checksum != sum) return false;

    return true;

  }

}
