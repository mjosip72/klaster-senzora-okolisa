
#pragma once

#include <Arduino.h>

#define PMS5003_START_BYTE_MISMATCH -1
#define PMS5003_TRANSFER_ERROR -2
#define PMS5003_CHECKSUM_MISMATCH -3
#define PMS5003_WAIT 0
#define PMS5003_DATA_READY 1

// https://e-radionica.com/productdata/PMS5003.pdf

struct pms5003_data {

  uint16_t start_bytes;
  uint16_t frame_length;

  // ug/m3
  uint16_t pm10_standard; // PM1.0
  uint16_t pm25_standard; // PM2.5
  uint16_t pm100_standard; // PM10

  // ug/m3
  uint16_t pm10_env; // PM1.0
  uint16_t pm25_env; // PM2.5
  uint16_t pm100_env; // PM100
  
  // 0.1L
  uint16_t particles_03um;
  uint16_t particles_05um;
  uint16_t particles_10um;
  uint16_t particles_25um;
  uint16_t particles_50um;
  uint16_t particles_100um;

  uint16_t reserved;
  uint16_t checksum;

};

class PMS5003 {

    private:
    uint8_t rx_pin;
    uint8_t tx_pin;

    public:

    pms5003_data data;

    PMS5003(uint8_t rx, uint8_t tx) {
      rx_pin = rx;
      tx_pin = tx;
    }

    void begin() {
      Serial1.begin(9600, SERIAL_8N1, rx_pin, tx_pin);
    }

    void end() {
      Serial1.end();
    }

    int8_t run();

};

int8_t PMS5003::run() {

    if(Serial1.available() == 0) return PMS5003_WAIT;

    if(Serial1.peek() != 0x42) {
        Serial1.read();
        return PMS5003_START_BYTE_MISMATCH;
    }

    if(Serial1.available() < 32) return PMS5003_WAIT;

    uint8_t buffer[32];
    uint16_t sum = 0;
    Serial1.readBytes(buffer, 32);

    for(uint8_t i = 0; i < 30; i++) sum += buffer[i];

    for(uint8_t i = 0; i < 32; i+=2) {
      uint8_t t = buffer[i];
      buffer[i] = buffer[i + 1];
      buffer[i + 1] = t;
    }

    memcpy(&data, buffer, 32);

    if(data.start_bytes != 0x424d) return PMS5003_TRANSFER_ERROR;
    if(data.frame_length != 28) return PMS5003_TRANSFER_ERROR;
    if(data.checksum != sum) return PMS5003_CHECKSUM_MISMATCH;

    return PMS5003_DATA_READY;

}
