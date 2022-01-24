
#if false

/* #region MAIN */

#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#define DEBUG
#include "Log.h"

// TFT_SCLK 18
// TFT_MOSI 23
// TFT_MISO 19
// TFT_CS 32
// TFT_DC 33
// TFT_RST 25
// TOUCH_CS 26
#define TOUCH_IRQ 27
#define SD_CS 14
#define LED_PIN 13

TFT_eSPI tft;

void _setup();
void _loop();

void setup() {

	Serial.begin(115200);

	digitalWrite(TFT_CS, HIGH);
	digitalWrite(TOUCH_CS, HIGH);
	digitalWrite(SD_CS, HIGH);

	pinMode(TOUCH_IRQ, INPUT);
	pinMode(LED_PIN, OUTPUT);

	tft.begin();
	tft.setRotation(1);
	uint16_t calData[5] = {329, 3577, 258, 3473, 7};
  tft.setTouch(calData);
	tft.fillScreen(TFT_WHITE);

  _setup();

}

void loop() {
  _loop();
}

/* #endregion */

#if false // sd test

#include <SD.h>
#include <FS.h>
#include "SdUtils.h"
#include <PCF85063A.h>
#include <time.h>

PCF85063A rtc;

void data_append(const char* name) {

  File f = SD.open(name, FILE_APPEND);
  f.println(name);
  f.close();


}

void _setup() {

  Serial.println(millis());

  if(!SD.begin(14)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE || cardType != CARD_SDHC){
    Serial.println("No SD card attached");
    return;
  }

  Wire.begin();



  data_append("/test1.txt");

  
  tm set2 = {0};
  rtc.readTime();
  set2.tm_year = rtc.getYear() - 1900;
  set2.tm_mon = rtc.getMonth() - 1;
  set2.tm_mday = rtc.getDay();
  set2.tm_hour = rtc.getHour();
  set2.tm_min = rtc.getMinute();
  set2.tm_sec = rtc.getSecond();
  set2.tm_isdst = 1;
  time_t epoch2 = mktime(&set2);
  timeval tv2;
  tv2.tv_sec = epoch2;
  tv2.tv_usec = 0;
  settimeofday(&tv2, NULL);

  tm t2;
  if(getLocalTime(&t2)) {
    Serial.printf("%d %d %d %d.%d.%d. %d:%d:%d\n", t2.tm_wday, t2.tm_yday, t2.tm_isdst, t2.tm_mday, t2.tm_mon+1, t2.tm_year+1900, t2.tm_hour, t2.tm_min, t2.tm_sec);
  }

  data_append("/test2.txt");
  Serial.println(millis());
}

void _loop() {

}

#endif

#if false // touch test

#include "TouchUtils.h"

void _setup() {
  TouchUtils::init(TOUCH_IRQ);
}

void _loop() {
  uint8_t t = TouchUtils::update();
	if(t == TOUCH_EVENT) {
		Serial.printf("Touch %d, %d\n", TouchUtils::x, TouchUtils::y);
		tft.fillScreen(TFT_BLACK);
		tft.drawFastHLine(0, TouchUtils::y, 320, TFT_RED);
		tft.drawFastVLine(TouchUtils::x, 0, 240, TFT_RED);
	}else if(t == TOUCH_EVENT_SWIPE_LEFT) {
		Serial.println("Swipe left");
	}else if(t == TOUCH_EVENT_SWIPE_RIGHT) {
		Serial.println("Swipe right");
	}else if(t == TOUCH_EVENT_SWIPE_UP) {
		Serial.println("Swipe up");
	}else if(t == TOUCH_EVENT_SWIPE_DOWN) {
		Serial.println("Swipe down");
	}
}

#endif

#if false // tft images pos test

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "images.h"

TFT_eSPI tft;

const char* get_iaq_label(float iaq) {
    if(iaq <= 50) return  "odlicno";
    if(iaq <= 100) return "dobro";
    if(iaq <= 150) return "blago zagadjeno";
    if(iaq <= 200) return "umjereno zagadjeno";
    if(iaq <= 250) return "jako zagadjeno";
    if(iaq <= 350) return "ozbiljno zagadjeno";
    return                "krajnje zagadjeno";
}

const uint16_t* images[] = {tempIcon, humIcon, pressIcon, iaqIcon, co2Icon, pm25Icon};

void _setup() {

  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(ILI9341_WHITE);
  tft.setSwapBytes(true);

  // TL_DATUM = 0 Top left (default)
  // TC_DATUM = 1 Top centre
  // TR_DATUM = 2 Top right
  // ML_DATUM = 3 Middle left
  // MC_DATUM = 4 Middle centre
  // MR_DATUM = 5 Middle right
  // BL_DATUM = 6 Bottom left
  // BC_DATUM = 7 Bottom centre
  // BR_DATUM = 8 Bottom right
  // L_BASELINE = 9 Left character baseline (Line the 'A' character would sit on)
  // C_BASELINE = 10 Centre character baseline
  // R_BASELINE = 11 Right character baseline

}

bool render = true;
uint16_t hsp = 32, vsp = 12, th = 14, txoff = 32, tyoff = 8, textSize = 1, textFont = 2, textDatum = 4;

void _loop() {
  
  if(Serial.available()) {
    hsp = Serial.parseInt();
    vsp = Serial.parseInt();
    th = Serial.parseInt();
    txoff = Serial.parseInt();
    tyoff = Serial.parseInt();
    textSize = Serial.parseInt();
    textFont = Serial.parseInt();
    textDatum = Serial.parseInt();
    Serial.printf("hsp=%d, vsp=%d, th=%d, txoff=%d, tyoff=%d, textSize=%d, textFont=%d, textDatum=%d\n", hsp, vsp, th, txoff, tyoff, textSize, textFont, textDatum);
    render = true;
  }

  if(!render) return;

  tft.fillScreen(TFT_WHITE);

  uint16_t cw = 3 * iconWidth + 2 * hsp;
  uint16_t ch = 2 * (iconHeight + th) + vsp;

  uint16_t lyy1 = (240 - ch) / 2;
  uint16_t lyy2 = lyy1 + iconHeight + th + vsp;

  uint16_t lyx1 = (320 - cw) / 2;
  uint16_t lyx2 = lyx1 + iconWidth + hsp;
  uint16_t lyx3 = lyx2 + iconWidth + hsp;

  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(textSize);
  tft.setTextFont(textFont);
  tft.setTextDatum(textDatum);

  tft.pushImage(lyx1, lyy1, iconWidth, iconHeight, images[0]);
  tft.drawRect(lyx1, lyy1 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("28.4 C", lyx1 + txoff, lyy1 + iconHeight + tyoff);

  tft.pushImage(lyx2, lyy1, iconWidth, iconHeight, images[1]);
  tft.drawRect(lyx2, lyy1 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("82 %", lyx2 + txoff, lyy1 + iconHeight + tyoff);

  tft.pushImage(lyx3, lyy1, iconWidth, iconHeight, images[2]);
  tft.drawRect(lyx3, lyy1 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("1020.61 hPa", lyx3 + txoff, lyy1 + iconHeight + tyoff);
  
  tft.pushImage(lyx1, lyy2, iconWidth, iconHeight, images[3]);
  tft.drawRect(lyx1, lyy2 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("370", lyx1 + txoff, lyy2 + iconHeight + tyoff);
  
  tft.pushImage(lyx2, lyy2, iconWidth, iconHeight, images[4]);
  tft.drawRect(lyx2, lyy2 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("3600 ppm", lyx2 + txoff, lyy2 + iconHeight + tyoff);

  tft.pushImage(lyx3, lyy2, iconWidth, iconHeight, images[5]);
  tft.drawRect(lyx3, lyy2 + iconHeight, iconWidth, th, TFT_RED);
  tft.drawString("47 ug/m3", lyx3 + txoff, lyy2 + iconHeight + tyoff);
  
  render = false;

}

#endif

#if false // time test

#include <WiFi.h>
#include <time.h>
#include <PCF85063A.h>

const char* ssid       = "WLAN_18E1D0";
const char* password   = "R7gaQ3P25Nf8";

const char* ntpServer = "hr.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

PCF85063A rtc;

/*
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  Serial.printf("%d %d.%d.%d. %d:%d:%d year=", timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.println(&timeinfo, "%Y");
}


int getMinutes() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 0;
  }
  return timeinfo.tm_min;
}



uint8_t samplePeriodMinutes = 4;
uint8_t lastSampledMinute = 100;

void saveDataSD() {

  tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Error");
    return;
  }

  uint8_t minutes = timeinfo.tm_min;

  if(minutes == lastSampledMinute) return;

  if(minutes % samplePeriodMinutes == 0) {
    lastSampledMinute = minutes;
    Serial.print("SAMPLED ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }

}
*/

void _setup()
{

  Wire.begin();

  Serial.print("MILLIS: ");
  Serial.println(millis());

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
    Serial.print("MILLIS: ");
  Serial.println(millis());

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    
    Serial.printf("%d %d.%d.%d. %d:%d:%d\n", timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.printf("%d %d\n", timeinfo.tm_isdst, timeinfo.tm_yday);

    rtc.setDate(timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    rtc.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    rtc.readTime();
    Serial.printf("%d %d.%d.%d. %d:%d:%d\n", rtc.getWeekday(), rtc.getDay(), rtc.getMonth(), rtc.getYear(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());

  }


  Serial.print("MILLIS: ");
  Serial.println(millis());

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

    Serial.print("MILLIS: ");
  Serial.println(millis());

}

void _loop()
{

}

#endif

#if false // wifi img test

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

void _setup() {

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

void _loop() {

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

#endif
