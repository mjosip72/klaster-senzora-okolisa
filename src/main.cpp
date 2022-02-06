
#if true

//#define DEBUG_ME
//#define USE_WIFI

/* #region Includes */

#include <Arduino.h>
#include <bsec.h>
#include <WiFi.h>

#include <TFT_eSPI.h>
#include <PCF85063A.h>
#include <SparkFunBQ27441.h>
#include <SD.h>
#include <FS.h>
#include <mbedtls/md.h>

#ifdef DEBUG_ME
#define DEBUG
#endif

#include "pms5003.h"
#include "StringUtils.h"
#include "LedOutput.h"
#include "TouchUtils.h"
#include "Timer.h"
#include "Log.h"

//#include "images.h"

/* #endregion */

/* #region Pins */

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

#define SERIAL2_RX 16
#define SERIAL2_TX 17

/* #endregion */

/* #region Defines */

#define BSEC_STATE_PATH "/sys/bsec_state.bin"

#define DISPLAY_MODE_OFF 0
#define DISPLAY_MODE_AIR_QUALITY 10
#define DISPLAY_MODE_AIR_OTHER 11
#define DISPLAY_MODE_MENU 20

/* #endregion */

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

bsec_virtual_sensor_t sensorList[10] = {
	BSEC_OUTPUT_RAW_TEMPERATURE,
	BSEC_OUTPUT_RAW_PRESSURE,
	BSEC_OUTPUT_RAW_HUMIDITY,
	BSEC_OUTPUT_RAW_GAS,
	BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
	BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
	BSEC_OUTPUT_IAQ,
	BSEC_OUTPUT_STATIC_IAQ,
	BSEC_OUTPUT_CO2_EQUIVALENT,
	BSEC_OUTPUT_BREATH_VOC_EQUIVALENT
};

RTC_DATA_ATTR Bsec bme680;
RTC_DATA_ATTR uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
RTC_DATA_ATTR int64_t bme680_nextCall;

TFT_eSPI tft;
PCF85063A rtc;
BQ27441 battery;

/* #region Deklaracije funkcija */

int64_t getTimestamp();
uint64_t getTime();

void setDisplayMode(uint8_t newDisplayMode);

namespace DeepSleep {

	RTC_DATA_ATTR bool deepSleep = false;
	RTC_DATA_ATTR bool shortNap = false;
	RTC_DATA_ATTR bool lastNap = false;
	int64_t offsetTime;

	void enable();
	void disable();

	void callSetup();
	void callLoop();

	bool isEnabled();

}

void displayBmeData();
void displayPmsData();
void displayTimeClock();
void displayMenu();

void saveData();
void handleUserInput(uint8_t touchEvent);

void loadBsecState();
void saveBsecState();

void turnOnWiFi();
void turnOffWiFi();

/* #endregion */

/* #region setup, loop */

RTC_DATA_ATTR uint8_t displayMode = DISPLAY_MODE_OFF;
char dataBuffer[256];
char dataBufferMini[32];

RTC_DATA_ATTR bool shouldSaveData = false;
bool wifiOn = false;

RTC_DATA_ATTR uint8_t samplePeriodMinutes = 4;
RTC_DATA_ATTR uint8_t lastSampledMinute = 100;

Timer timeClock(displayTimeClock);

void setup() {

	bool deepSleepAwakened = false;

	if(DeepSleep::lastNap) {
		DeepSleep::disable();
		deepSleepAwakened = true;
	}

	if(DeepSleep::isEnabled()) {

		esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

		if(wakeupReason == ESP_SLEEP_WAKEUP_EXT0) DeepSleep::lastNap = true;

		DeepSleep::callSetup();
		return;

	}

	Log::begin(115200);
	Log::println("\nKlaster senzora okolisa\nPowered by: Croduino Nova32 (ESP32) - e-radionica\n");

	Wire.begin();
	LED::begin(LED_PIN);

	/* #region TFT, SD */

	Log::println("Init TFT");

	digitalWrite(TFT_CS, HIGH);
	digitalWrite(TOUCH_CS, HIGH);
	digitalWrite(SD_CS, HIGH);

	pinMode(TOUCH_IRQ, INPUT);
	pinMode(LED_PIN, OUTPUT);

	tft.begin();

	if(deepSleepAwakened) {
		tft.writecommand(0x11);
		delay(120);
		//displayMode = DISPLAY_MODE_AIR_QUALITY;
	}

	tft.setRotation(1);

	uint16_t calData[5] = {329, 3577, 258, 3473, 7};
  tft.setTouch(calData);
	tft.fillScreen(TFT_WHITE);

	Log::println("Init SD card");

	bool sdCardInitialized = false;

	while(!sdCardInitialized) {

		if(SD.begin(SD_CS) == false) {
			tft.print("Postavljanje kartice nije uspjelo");
			digitalWrite(LED_PIN, HIGH);
			delay(2000);
			continue;
		}

		if(SD.cardType() != CARD_SDHC) {
			tft.print("Nema prikljucene SD kartice");
			digitalWrite(LED_PIN, HIGH);
			delay(2000);
			continue;
		}

		sdCardInitialized = true;

	}

	/* #endregion */

	/* #region Battery */

	Log::println("Init battery");
	battery.begin();

	if(battery.itporFlag()) {
		Log::println("Writing gauge config");
		battery.enterConfig();
		battery.setCapacity(2100); // 2100 mAh
		battery.setDesignEnergy(2100 * 3.7f);
		battery.setTerminateVoltage(3600); // 3.6V
		battery.setTaperRate(10 * 2100 / 30);
		battery.exitConfig();
	}else{
		Log::println("Using existing gauge config");
	}

	/* #endregion */

	/* #region Bme680 */

	Log::println("Init Bme680");

	bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
	if(deepSleepAwakened) bme680.nextCall = bme680_nextCall;
	bme680.setConfig(bsec_config_iaq);
	bme680.setTemperatureOffset(0.8);

	if(deepSleepAwakened) bme680.setState(bsecState);
	else loadBsecState();

	bme680.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

	/* #endregion */

	TouchUtils::begin(TOUCH_IRQ);
	PMS5003::init(SERIAL2_RX, SERIAL2_TX);
	timeClock.repeat(800);

	setDisplayMode(DISPLAY_MODE_AIR_QUALITY);
	Log::println("Init done");

}

void loop() {

	if(DeepSleep::isEnabled()) {
		DeepSleep::callLoop();
		return;
	}

	LED::loop();
	timeClock.loop();

	uint8_t touchEvent = TouchUtils::update();
	handleUserInput(touchEvent);

	bool pmsNewData = PMS5003::run();
	if(pmsNewData) displayPmsData();

	bool newData = bme680.run(getTimestamp());
	if(newData) {
		displayBmeData();
		if(shouldSaveData) saveData();
	}else{
				
		bsec_library_return_t status = bme680.status; // bsec_datatypes.h, line 291
    int8_t bme680Status = bme680.bme680Status; // bme680_defs.h, line 122
  
    if(status != BSEC_OK) {
      if(status < BSEC_OK) LED::start(LED_OUTPUT_ERROR);
      else LED::start(LED_OUTPUT_WARNING);
    }

    if(bme680Status != BME680_OK) {
      if(bme680Status < BME680_OK) LED::start(LED_OUTPUT_ERROR);
      else LED::start(LED_OUTPUT_WARNING);
    }

	}

	#ifdef DEBUG
	if(true && newData) {
		
		Serial.println(F("_________________________________"));

		Serial.printf("temp=%.1f, press=%.2f, hum=%.0f, gasResistance=%.1f\n", bme680.temperature, bme680.pressure / 100, bme680.humidity, bme680.gasResistance / 1000);
		Serial.printf("iaq=%.0f, siaq=%.0f, bvoc=%.0f, co2=%.0f, [%d]\n", bme680.iaq, bme680.staticIaq, bme680.breathVocEquivalent, bme680.co2Equivalent, bme680.iaqAccuracy);
		
		Serial.printf("soc=%d %c\n", battery.soc(), '%');
		Serial.printf("voltage=%.0f V\n", battery.voltage() / 1000.0);
		Serial.printf("current=%d mA, maxCurrent=%d mA\n", battery.current(AVG), battery.current(MAX));

		Serial.printf("remain=%d, full=%d\n", battery.capacity(REMAIN), battery.capacity(FULL));
		Serial.printf("avail=%d, avail_full=%d\n", battery.capacity(AVAIL), battery.capacity(AVAIL_FULL));
		Serial.printf("hp=%d %c\n", battery.soh(), '%');

	}else{
		
		bsec_library_return_t status = bme680.status; // bsec_datatypes.h, line 291
    int8_t bme680Status = bme680.bme680Status; // bme680_defs.h, line 122
  
    if(status != BSEC_OK) {
      if(status < BSEC_OK) {
        Log::printf("Bsec error code %d\n", status);
      }else {
        Log::printf("Bsec warning code %d\n", status);
      }
    }

    if(bme680Status != BME680_OK) {
      if(bme680Status < BME680_OK) {
        Log::printf("Bme680 error code %d\n", bme680Status);
      }else {
        Log::printf("Bme680 warning code %d\n", bme680Status);
      }
    }
  
	}
	#endif

}

/* #endregion */

/* #region Definicije osnovnih funkcija */

int64_t getTimestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

void bsecDelay(uint32_t period) {
  uint64_t start = millis();
	while(millis() - start < period) {
		bme680.run();
		delay(2);
	}
}

uint64_t getTime() {
	return bme680.getTimeMs();
}

void handleTouchDownEvent() {

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {

		if(TouchUtils::touchInside(32, 133, 64, 64)) saveBsecState();

	}else if(displayMode == DISPLAY_MODE_MENU) {

		if(TouchUtils::touchInside(20, 20, 140, 32)) {
			if(wifiOn) turnOffWiFi();
			else turnOnWiFi();
			displayMenu();
		}else if(TouchUtils::touchInside(20, 72, 140, 32)) {
			shouldSaveData = !shouldSaveData;
			displayMenu();
		}else if(TouchUtils::touchInside(20, 124, 140, 32)) {
			//if(wifiOn) syncTime();
		}else if(TouchUtils::touchInside(20, 176, 140, 32)) {
			if(PMS5003::running) PMS5003::end();
			else PMS5003::begin(&Serial2);
			displayMenu();
		}

	}

}

void handleSwipeEvent(uint8_t swipe) {

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {
		if(swipe == SWIPE_DOWN) setDisplayMode(DISPLAY_MODE_MENU);
		else if(swipe == SWIPE_LEFT) setDisplayMode(DISPLAY_MODE_AIR_OTHER);
		else if(swipe == SWIPE_UP) DeepSleep::enable();
	}else if(displayMode == DISPLAY_MODE_MENU) {
		setDisplayMode(DISPLAY_MODE_AIR_QUALITY);
	}else if(displayMode == DISPLAY_MODE_AIR_OTHER) {
		setDisplayMode(DISPLAY_MODE_AIR_QUALITY);
	}

}

void handleUserInput(uint8_t touchEvent) {
	if(touchEvent == TOUCH_EVENT_TOUCH_DOWN) handleTouchDownEvent();
	else if(touchEvent == TOUCH_EVENT_SWIPE) handleSwipeEvent(TouchUtils::swipe);
}

void saveData() {

	uint8_t hour, minute;
	uint8_t day, month;
	uint16_t year;

	rtc.readTime();
	hour = rtc.getHour();
	minute = rtc.getMinute();
	day = rtc.getDay();
	month = rtc.getMonth();
	year = rtc.getYear();

	if(minute == lastSampledMinute) return;
	if(minute % samplePeriodMinutes != 0) return;

	if(DeepSleep::isEnabled()) {
		if(SD.begin(SD_CS) == false || SD.cardType() != CARD_SDHC) return;
	}

	lastSampledMinute = minute;

	sprintf(dataBufferMini, "/data/%02d-%02d-%d.txt", day, month, year);

	int16_t bytes = sprintf(dataBuffer, "%02d:%02d,%.1f,%.2f,%.0f,%.1f,%.0f,%.0f,%.0f,%.0f,%d\n",
		hour, minute,
		bme680.temperature, bme680.pressure / 100, bme680.humidity, bme680.gasResistance / 1000,
		bme680.iaq, bme680.staticIaq, bme680.breathVocEquivalent, bme680.co2Equivalent, bme680.iaqAccuracy
	);

	if(bytes <= 0) return;

	File file = SD.open(dataBufferMini, FILE_APPEND);
	file.write((uint8_t*)dataBuffer, bytes);
	file.close();

	Log::println("Sampled");

}

/* #endregion */

/* #region Definicije funkcija za prikaz */

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

//bool shouldRenderBmeIcons = true;
//bool shouldRenderPmsIcons = true;

void renderIcons() {

	/*
	if(shouldRenderBmeIcons) {
		tft.pushImage(32, 43, iconWidth, iconHeight, tempIcon);
		tft.pushImage(128, 43, iconWidth, iconHeight, humIcon);
		tft.pushImage(224, 43, iconWidth, iconHeight, pressIcon);
		tft.pushImage(32, 133, iconWidth, iconHeight, gasIcon);
		tft.pushImage(128, 133, iconWidth, iconHeight, co2Icon);
		tft.pushImage(224, 133, iconWidth, iconHeight, vocIcon);
		shouldRenderBmeIcons = false;
	}*/

	drawImageSD(32, 43, "/sys/temp.img");
	drawImageSD(128, 43, "/sys/hum.img");
	drawImageSD(224, 43, "/sys/press.img");
	drawImageSD(32, 133, "/sys/air.img");

	if(PMS5003::running) {
		drawImageSD(128, 133, "/sys/pm10.img");
		drawImageSD(224, 133, "/sys/pm25.img");
	}else{
		drawImageSD(128, 133, "/sys/pm10d.img");
		drawImageSD(224, 133, "/sys/pm25d.img");
	}

}

const char* getIaqLabel() {
	return "";
	/*if(bme680.iaqAccuracy < 2) return "kalibrira se...";
	float iaq = bme680.iaq;
  if(iaq <= 50) return "odlicno";
  if(iaq <= 100) return "dobro";
  if(iaq <= 150) return "blago zagadjeno";
  if(iaq <= 200) return "umjereno zagadjeno";
  if(iaq <= 250) return "jako zagadjeno";
  if(iaq <= 350) return "ozbiljno zagadjeno";
  return "krajnje zagadjeno";*/
}

void displayBmeData() {

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {

		//renderIcons();

		tft.fillRect(16, 109, 288, 16, TFT_WHITE);
		tft.fillRect(16, 199, 96, 37, TFT_WHITE);

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TC_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		sprintf(dataBuffer, "%.1f C", bme680.temperature);
		tft.drawString(dataBuffer, 64, 109);

		sprintf(dataBuffer, "%.0f %c", bme680.humidity, '%');
		tft.drawString(dataBuffer, 160, 109);
		
		sprintf(dataBuffer, "%.1f hPa", bme680.pressure / 100);
		tft.drawString(dataBuffer, 256, 109);
		
		if(bme680.iaqAccuracy != 3) sprintf(dataBuffer, "%.0f [%d]", bme680.iaq, bme680.iaqAccuracy);
		else sprintf(dataBuffer, "%.0f", bme680.iaq);
		tft.drawString(dataBuffer, 64, 199);

		tft.setTextDatum(TC_DATUM);
		tft.setTextFont(1);

		tft.drawString("umjereno", 64, 215);
		tft.drawString("zagadjeno", 64, 223);

	}else if(displayMode == DISPLAY_MODE_AIR_OTHER) {

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TL_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		tft.fillRect(40, 2, 100, 54, TFT_WHITE);
		
		sprintf(dataBuffer, "= %.1f kOhm", bme680.gasResistance / 1000.0);
		tft.drawString(dataBuffer, 44, 4);

		sprintf(dataBuffer, "= %.0f ppm", bme680.breathVocEquivalent);
		tft.drawString(dataBuffer, 44, 20);

		sprintf(dataBuffer, "= %.0f ppm", bme680.co2Equivalent);
		tft.drawString(dataBuffer, 44, 36);

	}

}

void displayPmsData() {

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {

		//renderIcons();

		tft.fillRect(112, 199, 193, 37, TFT_WHITE);

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TC_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		sprintf(dataBuffer, "%d ug\\m3", pms5003.pm10_env);
		tft.drawString(dataBuffer, 160, 199);
		
		sprintf(dataBuffer, "%d ug\\m3", pms5003.pm25_env);
		tft.drawString(dataBuffer, 256, 199);

		tft.setTextDatum(TC_DATUM);
		tft.setTextFont(1);

		tft.drawString("umjereno", 160, 215);
		tft.drawString("zagadjeno", 160, 223);

		tft.drawString("umjereno", 256, 215);
		tft.drawString("zagadjeno", 256, 223);

	}else if(displayMode == DISPLAY_MODE_AIR_OTHER) {

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TL_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		tft.fillRect(99, 58, 150, 54, TFT_WHITE);
		tft.fillRect(124, 114, 112, 108, TFT_WHITE);

		sprintf(dataBuffer, "= %d/%d ug\\m3", pms5003.pm10_standard, pms5003.pm10_env);
		tft.drawString(dataBuffer, 103, 60);

		sprintf(dataBuffer, "= %d/%d ug\\m3", pms5003.pm25_standard, pms5003.pm25_env);
		tft.drawString(dataBuffer, 103, 76);

		sprintf(dataBuffer, "= %d/%d ug\\m3", pms5003.pm100_standard, pms5003.pm100_env);
		tft.drawString(dataBuffer, 103, 92);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_03um);
		tft.drawString(dataBuffer, 127, 116);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_05um);
		tft.drawString(dataBuffer, 127, 132);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_10um);
		tft.drawString(dataBuffer, 127, 148);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_25um);
		tft.drawString(dataBuffer, 127, 172);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_50um);
		tft.drawString(dataBuffer, 127, 188);

		sprintf(dataBuffer, "= %d ppm", pms5003.particles_100um);
		tft.drawString(dataBuffer, 127, 204);

	}

}

void displayTimeClock() {

	if(displayMode != DISPLAY_MODE_AIR_QUALITY) return;

	uint8_t hour, minute, second;
	//uint8_t day, month;
	//uint16_t year;

	rtc.readTime();
	hour = rtc.getHour();
	minute = rtc.getMinute();
	second = rtc.getSecond();

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {

		//tft.fillRect(0, 0, 68, 22, TFT_WHITE);
		//tft.fillRect(266, 0, 54, 22, TFT_WHITE);
		tft.fillRect(0, 0, 320, 22, TFT_WHITE);

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TL_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		uint16_t soc = battery.soc();
		uint16_t voltage = battery.voltage();
		int16_t current = -battery.current();
		uint16_t remain = battery.capacity(REMAIN);
		uint16_t avail = battery.capacity(AVAIL);

		sprintf(dataBuffer, "%02d:%02d:%02d, %d%c, %.1fV, %dmA, %d/%dmAh", hour, minute, second, soc, '%', voltage / 1000.0, current, remain, avail);
		tft.drawString(dataBuffer, 6, 4);

	}

}

void displayMenu() {

	tft.setTextColor(TFT_BLACK);
	tft.setTextFont(2);
	tft.setTextSize(1);
	tft.setTextDatum(CC_DATUM);

	tft.drawRoundRect(20, 20, 140, 32, 6, wifiOn ? TFT_GREEN : TFT_RED);
	tft.drawString("WiFi", 90, 36);

	tft.drawRoundRect(20, 72, 140, 32, 6, shouldSaveData ? TFT_GREEN : TFT_RED);
	tft.drawString("Spremanje", 90, 88);

	tft.drawRoundRect(20, 124, 140, 32, 6, wifiOn ? TFT_GREEN : TFT_RED);
	tft.drawString("Sinkroniziraj sat", 90, 140);
	
	tft.drawRoundRect(20, 176, 140, 32, 6, PMS5003::running ? TFT_GREEN : TFT_RED);
	tft.drawString("PMS5003", 90, 192);

}

void setDisplayMode(uint8_t newDisplayMode) {

	displayMode = newDisplayMode;
	tft.fillScreen(TFT_WHITE);

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {
		//shouldRenderBmeIcons = true;
		//shouldRenderPmsIcons = true;
		renderIcons();
		displayBmeData();
		displayPmsData();
	}else if(displayMode == DISPLAY_MODE_MENU) {
		displayMenu();
	}else if(displayMode == DISPLAY_MODE_AIR_OTHER) {

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TL_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		tft.drawString("res", 8, 4);
		tft.drawString("bVoc", 8, 20);
		tft.drawString("co2", 8, 36);
		tft.drawString("pm1.0 std/env", 8, 60);
		tft.drawString("pm2.5 std/env", 8, 76);
		tft.drawString("pm10 std/env", 8, 92);
		tft.drawString("particles > 0.3 um", 8, 116);
		tft.drawString("particles > 0.5 um", 8, 132);
		tft.drawString("particles > 1.0 um", 8, 148);
		tft.drawString("particles > 2.5 um", 8, 172);
		tft.drawString("particles > 5.0 um", 8, 188);
		tft.drawString("particles > 10 um", 8, 204);

		displayBmeData();
		displayPmsData();
	}

}

/* #endregion */

/* #region Definicije funkcija za učitavanje i spremanja stanja bsec algoritma */

namespace HmacUtils {

	const uint16_t keyLen = 8;
	const uint8_t key[keyLen] = {0x12, 0xAB, 0xBB, 0xE1, 0x94, 0x3A, 0xDA, 0x7F};
	#define HMAC_SIZE 20

	void hmac_sha1(uint8_t* payload, uint16_t len, uint8_t* hmacResult) {

		mbedtls_md_context_t ctx;
		mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;
		
		mbedtls_md_init(&ctx);
		mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
		mbedtls_md_hmac_starts(&ctx, key, keyLen);
		mbedtls_md_hmac_update(&ctx, payload, len);
		mbedtls_md_hmac_finish(&ctx, hmacResult);
		mbedtls_md_free(&ctx);

	}

}

namespace BsecStateUtils {

	bool readBsecState(const char* path, uint8_t* dest, uint16_t len) {

		if(!SD.exists(path)) return false;

		File file = SD.open(path, FILE_READ);
		if(!file) return false;

		uint16_t fileSize = file.size();
		if(fileSize != len + HMAC_SIZE) return false;

		uint8_t* payload = new uint8_t[len];
		uint8_t hmacExpected[HMAC_SIZE];
		uint8_t hmacActual[HMAC_SIZE];

		uint16_t result = 0;
		result += file.readBytes((char*)payload, len);
		result += file.readBytes((char*)hmacExpected, HMAC_SIZE);
		file.close();

		bool ok = result == fileSize;
		if(ok) {
			HmacUtils::hmac_sha1(payload, len, hmacActual);
			ok = memcmp(hmacExpected, hmacActual, HMAC_SIZE) == 0;

			if(ok) memcpy(dest, payload, len);

		}

		delete[] payload;
		return ok;

	}

	bool writeBsecState(const char* path, uint8_t* payload, uint16_t len) {

		uint8_t hmac[HMAC_SIZE];
		HmacUtils::hmac_sha1(payload, len, hmac);

		File file = SD.open(path, FILE_WRITE);
		if(!file) return false;

		uint16_t result = 0;
		result += file.write(payload, len);
		result += file.write(hmac, HMAC_SIZE);
		file.close();

		if(result != len + HMAC_SIZE) return false;

		///////////////////////////////////////////////////////////

		file = SD.open(path, FILE_READ);
		if(!file) return false;

		uint16_t fileSize = file.size();
		if(fileSize != len + HMAC_SIZE) return false;

		char* payloadActual = new char[len];
		char hmacActual[HMAC_SIZE];

		result = 0;
		result += file.readBytes(payloadActual, len);
		result += file.readBytes(hmacActual, HMAC_SIZE);
		file.close();

		bool ok = result == fileSize;
		if(ok) {
			ok = memcmp(payload, payloadActual, len) == 0;
			if(ok) ok = memcmp(hmac, hmacActual, HMAC_SIZE) == 0;
		}

		delete[] payloadActual;
		return ok;

	}

}

void loadBsecState() {

	bool success = BsecStateUtils::readBsecState(BSEC_STATE_PATH, bsecState, BSEC_MAX_STATE_BLOB_SIZE);

	if(success) {
		bme680.setState(bsecState);
		LED::start(LED_OUTPUT_SUCCESS);
	}else{
		LED::start(LED_OUTPUT_ERROR);
	}

}

void saveBsecState() {

	if(bme680.iaqAccuracy != 3) {
		LED::start(LED_OUTPUT_WARNING);
		return;
	}

	bme680.getState(bsecState);
	bool success = BsecStateUtils::writeBsecState(BSEC_STATE_PATH, bsecState, BSEC_MAX_STATE_BLOB_SIZE);

	if(success) {
		LED::start(LED_OUTPUT_SUCCESS);
	}else {
		LED::start(LED_OUTPUT_ERROR);
	}

}

/* #endregion */

/* #region Definicije funkcija za WiFi */

#ifdef USE_WIFI

void turnOnWiFi() {
	wifiOn = true;
	WiFi.disconnect(true);
	WiFi.begin("WLAN_18E1D0", "R7gaQ3P25Nf8");
}

void turnOffWiFi() {
	wifiOn = false;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

#else
void turnOnWiFi() {}
void turnOffWiFi() {}
#endif

/* #endregion */

/* #region Definicije funkcija za deep sleep */

void DeepSleep::callSetup() {

	Log::begin(115200);
	Log::print("deep setup "); Log::println(getTimestamp());

	Wire.begin();

	digitalWrite(TFT_CS, HIGH);
	digitalWrite(TOUCH_CS, HIGH);
	digitalWrite(SD_CS, HIGH);

	pinMode(TOUCH_IRQ, INPUT);
	pinMode(LED_PIN, OUTPUT);

	bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
	//bme680.nextCall = bme680_nextCall;
	bme680.setConfig(bsec_config_iaq);
	bme680.setTemperatureOffset(0.8);
	bme680.setState(bsecState);
	bme680.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

	Log::print("deep setup done "); Log::println(getTimestamp());
	offsetTime = esp_timer_get_time();

}

void DeepSleep::callLoop() {

	int64_t ts = getTimestamp();
	int64_t __nc = bme680.nextCall;

	int64_t t1 = esp_timer_get_time();

	if(bme680.run(ts)) {
		Log::print("new data "); Log::println(ts);
		Log::print("called at "); Log::println(__nc);

		if(shouldSaveData) saveData();

		bme680.getState(bsecState);
		//bme680_nextCall = bme680.nextCall;
		offsetTime += esp_timer_get_time() - t1;

		Log::print("next call "); Log::println(bme680.nextCall);
		Log::print("now "); Log::println(getTimestamp());
		Log::print("offset time "); Log::println(offsetTime);

		if(lastNap) {
			Log::println("last nap");
			esp_sleep_enable_timer_wakeup(100);
    	esp_deep_sleep_start();
		}

		if(shortNap) {
			shortNap = false;
			Log::println("short nap");
			esp_sleep_enable_timer_wakeup(100);
    	esp_deep_sleep_start();
		}

    uint64_t time_us = ((bme680.nextCall - getTimestamp()) * 1000) - offsetTime - 10000;
		Log::print("deep sleep for "); Log::println(time_us);

		esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, LOW);
    esp_sleep_enable_timer_wakeup(time_us);
    esp_deep_sleep_start();

	}

}

void DeepSleep::enable() {

	if(wifiOn) return;

	deepSleep = true;
	shortNap = true;

	displayMode = DISPLAY_MODE_OFF;
	tft.writecommand(0x10);
	delay(5);

}

void DeepSleep::disable() {
	deepSleep = false;
	lastNap = false;
}

bool DeepSleep::isEnabled() {
	return deepSleep;
}

/* #endregion */

#endif
