
#if true

/* #region Includes... */
#include <Arduino.h>
#include <bsec.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <PCF85063A.h>
#include <SparkFunBQ27441.h>

#define DEBUG
#include "StringUtils.h"
#include "Log.h"
/* #endregion */

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

Bsec bme680;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};



PCF85063A rtc;

/* #region Deklaracije funkcija */

void loadBsecState();
void saveBsecState();

/* #endregion */

/* #region setup, loop */

void setup() {

	Log::begin(115200);

	Wire.begin();
	EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);

	/* #region */

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

}

/* #endregion */

#endif
