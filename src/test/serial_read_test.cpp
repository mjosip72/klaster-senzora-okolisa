
#if false

#include <Arduino.h>

// TX -> RX
// 1  -> 16

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
}

const int bufferSize = 256;
uint8_t buffer[bufferSize];

void loop() {

  int available = Serial2.available();

  if(available) {

    int len = available;
    if(len > bufferSize) len = bufferSize;

    Serial2.readBytes(buffer, len);
    Serial.write(buffer, len);

  }

}

#endif
