
#if false

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_ILI9341.h>
#include "StringUtils.h"

#define TFT_CS 32
#define TFT_RST 33
#define TFT_DC 25
#define TFT_MOSI 26
#define TFT_CLK 27
#define TFT_MISO 14
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

AsyncWebServer webServer(80);
AsyncWebSocket webSocket("/socket");

uint8_t wflag = 0;
const uint16_t packetSize = 320*1;
uint16_t packetBuffer[packetSize];

void on_web_socket_event(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t *data, size_t len) {

  if(type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len) {
      
      if(info->opcode == WS_TEXT) {
        data[len] = 0;
        Serial.print("Message data=");
        Serial.println((char*)data);
        if(StringUtils::equals((char*)data, "img")) wflag = 1;
        else if(StringUtils::equals((char*)data, "finish")) wflag = 3;
      }else if(info->opcode == WS_BINARY) {
        Serial.printf("Message len=%d\n", len);
        memcpy(packetBuffer, data, len);
        wflag = 2;
      }
      
    }
  }

}

void setup() {

  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);

  WiFi.begin("WLAN_18E1D0", "R7gaQ3P25Nf8");
  while(!WiFi.isConnected());

  Serial.print("Ip: ");
  Serial.println(WiFi.localIP());

  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", "OK", NULL);
  });

  webSocket.onEvent(on_web_socket_event);
  webServer.addHandler(&webSocket);

  webServer.begin();

}

void loop() {

  if(wflag == 1) {
    wflag = 0;
    tft.startWrite();
    tft.setAddrWindow(0, 0, 320, 240);
    webSocket.textAll("next");
  }else if(wflag == 2) {
    wflag = 0;
    tft.writePixels(packetBuffer, packetSize);
    webSocket.textAll("next");
  }else if(wflag == 3) {
    wflag = 0;
    tft.endWrite();
  }

}

#endif
