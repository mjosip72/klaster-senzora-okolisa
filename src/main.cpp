
#if true

#define DEBUG_ME
//#define USE_BATTERY

/* #region Includes */

#include <Arduino.h>
#include <bsec.h>
#include <WiFi.h>

#include <TFT_eSPI.h>
#include <PCF85063A.h>

#ifdef USE_BATTERY
#include <SparkFunBQ27441.h>
#endif

#include <SD.h>
#include <FS.h>
#include "mbedtls/md.h"

#ifdef DEBUG_ME
#define DEBUG
#endif

#include "StringUtils.h"
#include "LedOutput.h"
#include "TouchUtils.h"
#include "Timer.h"
#include "Log.h"

#include "images.h"

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

/* #endregion */

/* #region Defines */

#define BSEC_STATE_PATH "/bsec_state.bin"

#define DISPLAY_MODE_OFF 0
#define DISPLAY_MODE_AIR_QUALITY 10
#define DISPLAY_MODE_MENU 20
#define DISPLAY_MODE_TIME_CLOCK 30
#define DISPLAY_MODE_BATTERY 31

/* #endregion */

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

Bsec bme680;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};

TFT_eSPI tft;
PCF85063A rtc;
#ifdef USE_BATTERY
BQ27441 battery;
#endif

/* #region Deklaracije funkcija */

uint64_t getTime();

void display();
void setTime();

void displayAirQualitiy();
void displayTimeClock();
void saveData();

void loadBsecState();
void saveBsecState();

/* #endregion */

/* #region setup, loop */

uint8_t displayMode = DISPLAY_MODE_AIR_QUALITY;
char dataBuffer[256];
char dataBufferMini[32];
bool shouldSaveData = false;

Timer timeClock(displayTimeClock);

void setup() {

	Log::begin(115200);
	Log::println("\nKlaster senzora okolisa\nPowered by: Croduino Nova32 (ESP32) - e-radionica\n");

	Wire.begin();

	/* #region TFT, SD */

	Log::println("Init TFT");

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

	Log::println("Init SD card");

	if(SD.begin(SD_CS) == false) {
		tft.print("Postavljanje kartice nije uspjelo");
		digitalWrite(LED_PIN, HIGH);
		while(true) delay(100);
	}

	if(SD.cardType() != CARD_SDHC) {
		tft.print("Nema prikljucene SD kartice");
		digitalWrite(LED_PIN, HIGH);
		while(true) delay(100);
	}

	/* #endregion */

	/* #region Battery */

	#ifdef USE_BATTERY
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
	#endif

	/* #endregion */

	/* #region Bme680 */

	Log::println("Init Bme680");

	bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
	bme680.setConfig(bsec_config_iaq);
	bme680.setTemperatureOffset(0.8);

	loadBsecState();

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

	bme680.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

	/* #endregion */

	TouchUtils::begin(TOUCH_IRQ);
	LED::begin(LED_PIN);
	setTime();

	timeClock.repeat(1000);

	Log::println("Init done");

}

void loop() {

	LED::loop();
	timeClock.loop();

	uint8_t touchEvent = TouchUtils::update();
	if(touchEvent == TOUCH_EVENT_TOUCH_DOWN) {
		if(TouchUtils::touchInside(32, 133, iconWidth, iconHeight)) saveBsecState();
	}

	bool newData = bme680.run();
	if(newData) {
		if(displayMode == DISPLAY_MODE_AIR_QUALITY) displayAirQualitiy();
		if(shouldSaveData) saveData();
	}

	#ifdef DEBUG
	if(newData) {
		
		Serial.println(F("_________________________________"));

		Serial.printf("temp=%.1f, press=%.2f, hum=%.0f, gasResistance=%.1f\n", bme680.temperature, bme680.pressure / 100, bme680.humidity, bme680.gasResistance / 1000);
		Serial.printf("iaq=%.0f, siaq=%.0f, bvoc=%.0f, co2=%.0f, [%d]\n", bme680.iaq, bme680.staticIaq, bme680.breathVocEquivalent, bme680.co2Equivalent, bme680.iaqAccuracy);
		
		#ifdef USE_BATTERY

		Serial.printf("soc=%d %c\n", battery.soc(), '%');
		Serial.printf("voltage=%d mV\n", battery.voltage());
		Serial.printf("current=%d mA, maxCurrent=%d mA\n", battery.current(AVG), battery.current(MAX));
		Serial.printf("power=%d mW\n", battery.power());

		Serial.printf("capacity design=%d\n", battery.capacity(DESIGN));
		Serial.printf("remain=%d, full=%d\n", battery.capacity(REMAIN), battery.capacity(FULL));
		Serial.printf("avail=%d, avail_full=%d\n", battery.capacity(AVAIL), battery.capacity(AVAIL_FULL));
		Serial.printf("remain_f=%d, remain_uf=%d\n", battery.capacity(REMAIN_F), battery.capacity(REMAIN_UF));
		Serial.printf("full_f=%d, full_uf=%d\n", battery.capacity(FULL_F), battery.capacity(FULL_UF));

		Serial.printf("temp=%d, internalTemp=%d\n", battery.temperature(BATTERY), battery.temperature(INTERNAL_TEMP));
		Serial.printf("hp=%d %c\n", battery.soh(), '%');

		#endif

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

/* #region Definicije glavnih funkcija */

uint64_t getTime() {
	return bme680.getTimeMs();
}

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

bool shouldRenderIcons = true;

const char* getIaqLabel() {
	if(bme680.iaqAccuracy < 2) return "kalibrira se...";
	float iaq = bme680.iaq;
  if(iaq <= 50) return "odlicno";
  if(iaq <= 100) return "dobro";
  if(iaq <= 150) return "blago zagadjeno";
  if(iaq <= 200) return "umjereno zagadjeno";
  if(iaq <= 250) return "jako zagadjeno";
  if(iaq <= 350) return "ozbiljno zagadjeno";
  return "krajnje zagadjeno";
}

void displayAirQualitiy() {

	if(shouldRenderIcons) {
		tft.pushImage(32, 43, iconWidth, iconHeight, tempIcon);
		tft.pushImage(128, 43, iconWidth, iconHeight, humIcon);
		tft.pushImage(224, 43, iconWidth, iconHeight, pressIcon);
		tft.pushImage(32, 133, iconWidth, iconHeight, gasIcon);
		tft.pushImage(128, 133, iconWidth, iconHeight, co2Icon);
		tft.pushImage(224, 133, iconWidth, iconHeight, vocIcon);
		shouldRenderIcons = false;
	}

	tft.fillRect(0, 107, 320, 26, TFT_WHITE);
	tft.fillRect(0, 197, 320, 43, TFT_WHITE);

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
	
	sprintf(dataBuffer, "%.0f ppm", bme680.co2Equivalent);
	tft.drawString(dataBuffer, 160, 199);
	
	//sprintf(dataBuffer, "%.0f ppm", bme680.breathVocEquivalent);
	sprintf(dataBuffer, "%.1f Ohm", bme680.gasResistance / 1000);
	tft.drawString(dataBuffer, 256, 199);

	tft.setTextDatum(TL_DATUM);
	tft.setTextFont(1);
	tft.drawString(getIaqLabel(), 32, 215);

}

void displayTimeClock() {

	uint8_t hour, minute, second;
	uint8_t day, month;
	uint16_t year;

	tm time;
	if(getLocalTime(&time)) {
		hour = time.tm_hour;
		minute = time.tm_min;
		second = time.tm_sec;
		day = time.tm_mday;
		month = time.tm_mon + 1;
		year = time.tm_year + 1900;
	}

	if(displayMode == DISPLAY_MODE_AIR_QUALITY) {

		tft.fillRect(0, 0, 68, 22, TFT_WHITE);
		tft.fillRect(266, 0, 54, 22, TFT_WHITE);

		tft.setTextColor(TFT_BLACK);
		tft.setTextDatum(TL_DATUM);
		tft.setTextFont(2);
		tft.setTextSize(1);

		sprintf(dataBuffer, "%02d:%02d:%02d", hour, minute, second);
		tft.drawString(dataBuffer, 6, 4);

		#ifdef USE_BATTERY
		tft.setTextDatum(TR_DATUM);
		sprintf(dataBuffer, "%d %c", battery.soc(), '%');
		tft.drawString(dataBuffer, 314, 4);
		#endif


	}

}

uint8_t samplePeriodMinutes = 4;
uint8_t lastSampledMinute = 100;

void saveData() {

	uint8_t hour, minute;
	uint8_t day, month;
	uint16_t year;

	/* #region get time */
	tm time;
	if(getLocalTime(&time)) {
		hour = time.tm_hour;
		minute = time.tm_min;
		day = time.tm_mday;
		month = time.tm_mon + 1;
		year = time.tm_year + 1900;
	}else{
		LED::start(LED_OUTPUT_ERROR);
		setTime();
		hour = rtc.getHour();
		minute = rtc.getMinute();
		day = rtc.getDay();
		month = rtc.getMonth();
		year = rtc.getYear();
	}
	/* #endregion */

	if(minute == lastSampledMinute) return;
	if(minute % samplePeriodMinutes != 0) return;
	lastSampledMinute = minute;

	Log::println("Sampled");

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

#endif
