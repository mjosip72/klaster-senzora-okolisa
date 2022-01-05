
#if false

/* #region Includes */

#include <Arduino.h>
#include <bsec.h>
#include <WiFi.h>

#include <TFT_eSPI.h>
#include <PCF85063A.h>
#include <SparkFunBQ27441.h>

#define DEBUG
#include "StringUtils.h"
#include "SdUtils.h"
#include "Log.h"

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

/* #endregion */

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

Bsec bme680;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};

TFT_eSPI tft;
PCF85063A rtc;
BQ27441 battery;

/* #region Deklaracije funkcija */

void loadBsecState();
void saveBsecState();

/* #endregion */

/* #region setup, loop */

void setup() {

	Log::begin(115200);

	digitalWrite(PIN_TFT_CS, HIGH);
	digitalWrite(PIN_TOUCH_CS, HIGH);
	digitalWrite(PIN_SD_CS, HIGH);

	tft.begin();
	tft.setRotation(1);

	Wire.begin();
	EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);

	/* #region Battery */

	battery.begin();

	if(battery.itporFlag()) {
		Log::println("Writing gauge config");
		battery.enterConfig();
		battery.setCapacity(2100); // 2100 mAh
		battery.setDesignEnergy(2100 * 3.7f);
		battery.setTerminateVoltage(3600); // 3.6V
		battery.setTaperRate(10 * 2100 / 12);
		battery.exitConfig();
	}else{
		Log::println("Using existing gauge config");
	}

	/* #endregion */

	/* #region Bme680 */

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


}

void loop() {

	tft.getTouch(0, 0, 0);

}

/* #endregion */

/* #region Definicije glavnih funkcija */

/* #endregion */

/* #region Definicije funkcija za učitavanje i spremanja stanja bsec algoritma */

void loadBsecState() {

  Log::println("Loading bsec state");

  if(EEPROM.read(0) != BSEC_MAX_STATE_BLOB_SIZE) {
    Log::println("No bsec state");
    return;
  }

  for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
    bsecState[i] = EEPROM.read(i + 1);
  }
  bme680.setState(bsecState);

	Log::println("Loaded");

}

void saveBsecState() {

	Log::println("Saving bsec state");

  if(bme680.iaqAccuracy != 3) return;

  bme680.getState(bsecState);
  EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
  for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
    EEPROM.write(i + 1, bsecState[i]);
  }
  EEPROM.commit();

	Log::println("Saved");

}

/* #endregion */

#endif
