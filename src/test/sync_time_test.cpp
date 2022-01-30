
#if false

#include "Test.h"

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
	 
}

int64_t getTimestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}


void _setup() {

  Wire.begin();

  delay(2000);

  rtc.readTime();
  Serial.printf("rtc %d.%d.%d. %d:%d:%d\n", rtc.getDay(), rtc.getMonth(), rtc.getYear(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());

  Serial.print("timestamp = "); Serial.print(getTimestamp()); Serial.print(", millis = "); Serial.println(millis());

  delay(1000);

  //setTime();

 // Serial.println("TIME AFTER SET");

  tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    Serial.printf("esp %d %d.%d.%d. %d:%d:%d\n", timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }

  time_t tt;
  time(&tt);
  Serial.print("time = "); Serial.println(tt);

  Serial.print("timestamp = "); Serial.print(getTimestamp()); Serial.print(", millis = "); Serial.println(millis());

  ESP.deepSleep(4000000);

}

void _loop() {

}

#endif