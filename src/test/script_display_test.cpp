
#if true

#include <Test.h>

#define N Serial.parseInt()

WiFiServer server(7200);
WiFiClient client;
bool wifiOn = false;

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

void drawImageWiFi(uint16_t sx, uint16_t sy) {

  Serial.write(0);

  if(!wifiOn) return;

  WiFiClient client;
  while(client.connected() == false) client = server.accept();

  uint8_t sizeBuff[4];
  while(client.available() < 4);
  client.readBytes(sizeBuff, 4);

  uint32_t w = (((uint16_t)sizeBuff[0] << 8) & 0xFF00) | ((uint16_t)sizeBuff[1] & 0xFF);
  uint32_t h = (((uint16_t)sizeBuff[2] << 8) & 0xFF00) | ((uint16_t)sizeBuff[3] & 0xFF);

  uint32_t ln = 2 * w;

  uint8_t* buffer = new uint8_t[ln];
  tft.setAddrWindow(sx, sy, w, h);

  client.write((uint8_t)0);

  for(uint16_t y = 0; y < h; y++) {

    while(client.available() < ln);
    client.readBytes(buffer, ln);

    tft.pushColors(buffer, ln);
    client.write((uint8_t)0);

  }

  delete[] buffer;

  client.stop();

}

void _setup() {
  Serial.begin(1000000);
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

    else if(cmd == "setCursor") {
      tft.setCursor(N, N);
    }else if(cmd == "print") {
      tft.print(Serial.readStringUntil(0));
    }else if(cmd == "println") {
      tft.println(Serial.readStringUntil(0));
    }
    
    else if(cmd == "drawString") {
      tft.drawString(Serial.readStringUntil(0), N, N);
    }else if(cmd == "pushImageSD") {
      String name = Serial.readStringUntil(0);
      drawImageSD(N, N, name.c_str());
    }else if(cmd == "pushImage") {

      int x = N;
      int y = N;

      while(Serial.read() != '\n') delay(2);
      Serial.write(0);

      drawImageWiFi(x, y);
      return;
    }

    else if(cmd == "writecommand") {
      tft.writecommand(N);
    }else if(cmd == "writedata") {
      tft.writedata(N);
    }

    else if(cmd == "wifibegin") {
      String ssid = Serial.readStringUntil(0);
      String pass = Serial.readStringUntil(0);
      WiFi.disconnect(true);
      WiFi.begin(ssid.c_str(), pass.c_str());
      digitalWrite(LED_PIN, HIGH);
      while(WiFi.status() != WL_CONNECTED);
      digitalWrite(LED_PIN, LOW);
      wifiOn = true;
      server.begin();
      MDNS.begin("esp32");
      MDNS.addService("tcp", "tcp", 7200);
    }else if(cmd == "wifiend") {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      server.end();
      MDNS.end();
      wifiOn = false;
    }

    while(Serial.read() != '\n') delay(2);
    Serial.write(0);


  }

}

#endif
