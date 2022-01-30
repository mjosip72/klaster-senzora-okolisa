
#if false

#include "Test.h"

#include <WiFi.h>
#include <time.h>
#include <PCF85063A.h>

const char* ssid       = "WLAN_18E1D0";
const char* password   = "R7gaQ3P25Nf8";

const char* ntpServer = "hr.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

PCF85063A rtc;



void setTime() {

	rtc.readTime();

	tm time = {0};
	time.tm_year = rtc.getYear() - 1900;
  time.tm_mon = rtc.getMonth() - 1;
  time.tm_mday = rtc.getDay();
  time.tm_hour = rtc.getHour();
  time.tm_min = rtc.getMinute();
  time.tm_sec = rtc.getSecond();

	time_t epoch = mktime(&time);
	timeval tv;
	tv.tv_sec = epoch;
	tv.tv_usec = 0;
	settimeofday(&tv, NULL);

	#ifdef DEBUG
	tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    rtc.readTime();
    Serial.printf("%d.%d.%d. %d:%d:%d\n", rtc.getDay(), rtc.getMonth(), rtc.getYear(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());
    Serial.printf("%d.%d.%d. %d:%d:%d\n", timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }else{
		Log::println("Failed to set time");
	}
	#endif
	 
}


void _setup() {

  Wire.begin();


  //setTime();

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  


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

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

}

void _loop() {

}

#endif