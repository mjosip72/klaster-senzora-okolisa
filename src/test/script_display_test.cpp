
#if false

#include <Test.h>
#define N Serial.parseInt()

void drawImageSD(uint16_t x, uint16_t y, const char* name) {

  if(!SD.exists(name)) return;
  File file = SD.open(name, FILE_READ);

  char sizeBuff[4];
  file.readBytes(sizeBuff, 4);

  uint16_t w = (((uint16_t)sizeBuff[0] << 8) & 0xFF00) | ((uint16_t)sizeBuff[1] & 0xFF);
  uint16_t h = (((uint16_t)sizeBuff[2] << 8) & 0xFF00) | ((uint16_t)sizeBuff[3] & 0xFF);

  char* buffer = new char[w*2];
  for(uint16_t i = 0; i < h; i++) {
    file.readBytes(buffer, w*2);
    tft.pushImage(x, y + i, w, 1, (uint16_t*)buffer);
  }

  file.close();

  delete[] buffer;

}

void _setup() {
  SD_begin();
}

void _loop() {

  if(Serial.available()) {

    String cmd = Serial.readStringUntil(' ');

    if(cmd == "fillScreen") {
      tft.fillScreen(N);
    }else if(cmd == "drawLine") {
      tft.drawLine(N, N, N, N, N);
    }else if(cmd == "drawFastHLine") {
      tft.drawFastHLine(N, N, N, N);
    }else if(cmd == "drawFastVLine") {
      tft.drawFastVLine(N, N, N, N);
    }
    
    else if(cmd == "drawRect") {
      tft.drawRect(N, N, N, N, N);
    }else if(cmd == "fillRect") {
      tft.fillRect(N, N, N, N, N);
    }else if(cmd == "drawCircle") {
      tft.drawCircle(N, N, N, N);
    }else if(cmd == "fillCircle") {
      tft.fillCircle(N, N, N, N);
    }
    
    else if(cmd == "drawTriangle") {
      tft.drawTriangle(N, N, N, N, N, N, N);
    }else if(cmd == "fillTriangle") {
      tft.fillTriangle(N, N, N, N, N, N, N);
    }else if(cmd == "drawRoundRect") {
      tft.drawRoundRect(N, N, N, N, N, N);
    }else if(cmd == "fillRoundRect") {
      tft.fillRoundRect(N, N, N, N, N, N);
    }
    
    else if(cmd == "setTextColor") {
      tft.setTextColor(N);
    }else if(cmd == "setTextDatum") {
      tft.setTextDatum(N);
    }else if(cmd == "setTextFont") {
      tft.setTextFont(N);
    }else if(cmd == "setTextPadding") {
      tft.setTextPadding(N);
    }else if(cmd == "setTextSize") {
      tft.setTextSize(N);
    }else if(cmd == "setTextWrap") {
      tft.setTextWrap(N, N);
    }
    
    else if(cmd == "drawString") {
      tft.drawString(Serial.readStringUntil(0), N, N);
    }else if(cmd == "pushImage") {
      String name = Serial.readStringUntil(0);
      drawImageSD(N, N, name.c_str());
    }

    while(Serial.read() != '\n') delay(2);

  }

}

#endif
