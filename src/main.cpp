
#if true

/* #region Includes... */
#include <Arduino.h>
#include <bsec.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SparkFunBQ27441.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncDNSServer.h>
#include <ESPmDNS.h>
#include <PCF85063A.h>
extern "C" {
#include <bootloader_random.h>
}
#include "button_event.h"
#include "lcd_charset.h"
#include "dallas_temperature_sensor.h"
#include "soil_moisture_sensor.h"
#include "altimeter.h"
#include "pms5003.h"
#include "string_utils.h"
/* #endregion */

/* #region Pins */
#define GREEN_BUTTON_PIN 25
#define YELLOW_BUTTON_PIN 26
#define RED_BUTTON_PIN 27
#define ONE_WIRE_BUS 33
#define ANALOG_INPUT_PIN 32
#define RX_PIN 16
#define TX_PIN 17
#define LED_PIN 13
/* #endregion */

/* #region Display modes */
#define DISPLAY_MODE_TEMP_PRESS_HUM 1
#define DISPLAY_MODE_IAQ_CO2_VOC 2
#define DISPLAY_MODE_AIR_QUALITY 3
#define DISPLAY_MODE_MENU 4
#define DISPLAY_MODE_DALLAS_TEMP 5
#define DISPLAY_MODE_BATTERY 6
#define DISPLAY_MODE_ALTITUDE 7
#define DISPLAY_MODE_ALTITUDE_CHANGE 8
#define DISPLAY_MODE_SOIL_MOISTURE 9
#define DISPLAY_MODE_TIME 10
#define DISPLAY_MODE_PMS5003 11
#define DISPLAY_MODE_LCD_CUSTOM_TEXT 100
/* #endregion */

#define AP_WIFI_SSID "Klaster senzora okoliša"
#define AP_DOMAIN_NAME "esc"

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

Bsec bme680;
uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};

/*  USB port
    D+ = ONE WIRE BUS
    D- = SOIL MOISTURE
*/
DallasTemperatureSensor dallasTempSensor(ONE_WIRE_BUS);
SoilMoistureSensor soilMoistureSensor(ANALOG_INPUT_PIN);

LiquidCrystal lcd;
uint8_t display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
uint8_t display_menu_selection = 0;
bool lcd_on = true;

PCF85063A rtc;

PMS5003 pms5003(RX_PIN, TX_PIN);
uint8_t display_pms = 0;

Altimeter altimeter;

#define BATTERY_CAPACITY 2100
#define TERMINATE_VOLTAGE 3600
uint8_t lcd_battery_mode = 1;

bool wifi_on = false;

uint64_t led_blink_time = 0;
bool led_on = false;

#define WEB_BUFFER_SIZE 84
uint8_t web_flag = 0;
char web_buffer[WEB_BUFFER_SIZE];
bool lcd_blinking = false;
bool lcd_bl_on = true;

/* #region Web */
AsyncWebServer web_server(80);
AsyncWebSocket web_socket("/esc");
AsyncDNSServer dns_server;
#include "web-site.h"
/* #endregion */

/* #region Deklaracije funkcija */

void bsec_delay(uint32_t period);
uint64_t get_time();

void load_bsec_state();
void save_bsec_state();

void display_data();
void display_pms_data();

void turn_on_wifi();
void turn_off_wifi();

void on_web_socket_event(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void send_data_to_web();

/* #endregion */

/* #region Button events */

ButtonEvent greenButton(GREEN_BUTTON_PIN);
ButtonEvent yellowButton(YELLOW_BUTTON_PIN);
ButtonEvent redButton(RED_BUTTON_PIN);

void onGreenButton(bool longPress) {

    if(!lcd_on) return;

    if(display_mode == DISPLAY_MODE_MENU) {
        display_menu_selection++;
        if(display_menu_selection == 4) display_menu_selection = 0;
        display_data();
        return;
    }

    if(longPress) {
        display_mode = DISPLAY_MODE_MENU;
        display_menu_selection = 0;
        display_data();
        return;
    }

    switch(display_mode) {

    case DISPLAY_MODE_TEMP_PRESS_HUM:
        display_mode = DISPLAY_MODE_AIR_QUALITY;
        break;

    case DISPLAY_MODE_IAQ_CO2_VOC:
    case DISPLAY_MODE_AIR_QUALITY:
        if(display_pms != 0) {display_mode = DISPLAY_MODE_PMS5003; display_pms_data(); return;}
        else if(dallasTempSensor.isConnected()) display_mode = DISPLAY_MODE_DALLAS_TEMP;
        else if(soilMoistureSensor.isConnected()) display_mode = DISPLAY_MODE_SOIL_MOISTURE;
        else display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        break;
    
    case DISPLAY_MODE_PMS5003:
        if(dallasTempSensor.isConnected()) display_mode = DISPLAY_MODE_DALLAS_TEMP;
        else if(soilMoistureSensor.isConnected()) display_mode = DISPLAY_MODE_SOIL_MOISTURE;
        else display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        break;

    case DISPLAY_MODE_DALLAS_TEMP:
        if(soilMoistureSensor.isConnected()) display_mode = DISPLAY_MODE_SOIL_MOISTURE;
        else display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        break;

    case DISPLAY_MODE_SOIL_MOISTURE:
        display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        break;

    case DISPLAY_MODE_LCD_CUSTOM_TEXT:
    case DISPLAY_MODE_ALTITUDE:
    case DISPLAY_MODE_ALTITUDE_CHANGE:
    case DISPLAY_MODE_BATTERY:
    case DISPLAY_MODE_TIME:
        display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        break;
    }

    display_data();

}

void onYellowButton(bool longPress) {

    if(!lcd_on) return;

    if(display_mode == DISPLAY_MODE_MENU) {

        switch(display_menu_selection) {
        
        case 0:
            if(wifi_on) turn_off_wifi();
            else turn_on_wifi();
            break;

        case 1:
            if(soilMoistureSensor.isConnected()) soilMoistureSensor.disconnect();
            else soilMoistureSensor.connect();
            break;

        case 2:
            if(dallasTempSensor.isConnected()) dallasTempSensor.end();
            else dallasTempSensor.begin();
            break;

        case 3:
            if(display_pms == 0) {
                display_pms = 1;
                pms5003.begin();
            }else {
                display_pms = 0;
                pms5003.end();
            }
            break;

        }

        display_data();
        return;
    }

    if(longPress) {

        if(display_mode == DISPLAY_MODE_TEMP_PRESS_HUM) {
            display_mode = DISPLAY_MODE_ALTITUDE;
        }

        if(display_mode == DISPLAY_MODE_SOIL_MOISTURE) {
            soilMoistureSensor.saveMinMax();
            lcd.clear();
            lcd.print("Spremljeno");
            bsec_delay(400);
        }

        if(display_mode == DISPLAY_MODE_IAQ_CO2_VOC or display_mode == DISPLAY_MODE_AIR_QUALITY) {

            if(bme680.iaqAccuracy == 3) {
                lcd.clear();
                lcd.print("Spremanje stanja");
                bsec_delay(300);

                save_bsec_state();
                lcd.setCursor(0, 1);
                lcd.print("Spremljeno");
                bsec_delay(400);
            }else{
                lcd.clear();
                lcd.print("Kalibrira se");
                bsec_delay(400);
            }
        }

        display_data();
        return;
    }

    switch(display_mode) {

    case DISPLAY_MODE_TEMP_PRESS_HUM:
        display_mode = DISPLAY_MODE_TIME;
        break;

    case DISPLAY_MODE_IAQ_CO2_VOC:
        display_mode = DISPLAY_MODE_AIR_QUALITY;
        break;

    case DISPLAY_MODE_AIR_QUALITY:
        display_mode = DISPLAY_MODE_IAQ_CO2_VOC;
        break;

    case DISPLAY_MODE_PMS5003:
        display_pms++;
        if(display_pms == 3) display_pms = 1;
        display_pms_data();
        return;
        break;

    case DISPLAY_MODE_ALTITUDE:
        display_mode = DISPLAY_MODE_ALTITUDE_CHANGE;
        altimeter.set_altitude_change_pressure(bme680.pressure / 100.0);
        break;

    case DISPLAY_MODE_ALTITUDE_CHANGE:
        display_mode = DISPLAY_MODE_ALTITUDE;
        break;

    case DISPLAY_MODE_BATTERY:
        lcd_battery_mode++;
        if(lcd_battery_mode == 4) lcd_battery_mode = 1;
        break;

    case DISPLAY_MODE_SOIL_MOISTURE:
        soilMoistureSensor.calibrateMinMax();
        break;
    
    }

    display_data();

}

void onRedButton(bool longPress) {

    if(display_mode == DISPLAY_MODE_MENU) {
        display_mode = DISPLAY_MODE_TEMP_PRESS_HUM;
        display_data();
        return;
    }

    if(display_mode == DISPLAY_MODE_SOIL_MOISTURE) {
        soilMoistureSensor.resetMinMax();
        display_data();
        return;
    }

    if(lcd_on and longPress) {
        display_mode = DISPLAY_MODE_BATTERY;
        display_data();
        return;
    }

    if(!lcd_on and longPress) {
        lcd.begin(20, 4);
        LCD_Charset::current_charset = 0;
        display_data();
        return;
    }

    if(lcd_on) {
        lcd_on = false;
        lcd.noDisplay();
        lcd.noBacklight();
    }else{
        lcd_on = true;
        lcd.backlight();
        lcd.display();
        display_data();
    }

}

/* #endregion */

/* #region Setup, Loop */

void setup() {

    pinMode(GREEN_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(YELLOW_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(RED_BUTTON_PIN, INPUT_PULLDOWN);

    greenButton.onPress = onGreenButton;
    yellowButton.onPress = onYellowButton;
    redButton.onPress = onRedButton;

    pinMode(LED_PIN, OUTPUT);

    /* #region EEPROM */
    // 140 Bytes - BSEC STATE
    // 4 Bytes - SOIL MOISTURE MIN MAX
    EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1 + 4);
    soilMoistureSensor.loadMinMax();
    /* #endregion */

    Wire.begin();

    /* #region LCD */

    lcd.begin(20, 4);
    LCD_Charset::requestCharset(LCD_CHARSET_BASIC);

    uint8_t smile_char = LCD_CHAR_SMILE2;

    bootloader_random_enable();
    if(esp_random() % 10000 < 2500) smile_char = LCD_CHAR_SMILE1;
    bootloader_random_disable();

    lcd.clear();
    lcd.print("KLASTER SENZORA");
    lcd.setCursor(0, 1);
    lcd.print("OKOLISA ");
    lcd.write(smile_char);
    delay(300);

    /* #endregion */

    /* #region Battery config */

    bool battery_result = Battery.begin();
    if(!battery_result) while(true);
    Battery.enterConfig();
    Battery.setCapacity(BATTERY_CAPACITY);
    Battery.setDesignEnergy(BATTERY_CAPACITY * 3.7f);
    Battery.setTerminateVoltage(TERMINATE_VOLTAGE);
    Battery.setTaperRate(10 * BATTERY_CAPACITY / 12);
    Battery.exitConfig();

    /* #endregion */

    /* #region Bme680 config */

    bme680.begin(BME680_I2C_ADDR_PRIMARY, Wire);
    bme680.setConfig(bsec_config_iaq);
    bme680.setTemperatureOffset(0.8);
    load_bsec_state();

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

    /* #region Web */

    web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, NULL);
    });

    web_server.onNotFound([](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, NULL);
    });

    web_socket.onEvent(on_web_socket_event);
    web_server.addHandler(&web_socket);

    /* #endregion */

}

void loop() {

    /* #region Web flag */

    if(web_flag != 0) {

        if(web_flag == 1) {

            web_flag = 0;

            display_mode = DISPLAY_MODE_LCD_CUSTOM_TEXT;
            lcd.clear();

            uint8_t row = 0;
            size_t len = strlen(web_buffer);
            for(size_t i = 0; i < len; i++) {
                char c = web_buffer[i];
                if(c == '\n') {
                    row++;
                    lcd.setCursor(0, row);
                    continue;
                }
                lcd.write(c);
            }

        }else if(web_flag == 2) {

            web_flag = 0;
             
            int hour = 0, minute = 0, sec = 0;
            int weekday = 0, day = 0, month = 0, year = 0;

            sscanf(web_buffer, "%d %d %d %d %d %d %d", &hour, &minute, &sec, &weekday, &day, &month, &year);

            rtc.setDate(weekday, day, month, year);
            rtc.setTime(hour, minute, sec);

            lcd.clear();
            lcd.print("Vrijeme uspjesno");
            lcd.setCursor(0, 1);
            lcd.print("sinkronizirano");

            bsec_delay(1200);
            display_data();

        }else if(web_flag == 3) {

            web_flag = 0;
            lcd_blinking = false;

            if(equals(web_buffer, "on")) lcd.backlight();
            else if(equals(web_buffer, "off")) lcd.noBacklight();
            else if(equals(web_buffer, "blink")) lcd_blinking = true;

        }

    }

    /* #endregion */

    /* #region Buttons i LED loop */

    greenButton.loop();
    yellowButton.loop();
    redButton.loop();

    if(!lcd_on) {
        uint64_t now = get_time();
        if(led_on) {
            if(now - led_blink_time >= 100) {
                led_blink_time = now;
                led_on = false;
                digitalWrite(LED_PIN, LOW);
            }
        }else{
            if(now - led_blink_time >= 2000) {
                led_blink_time = now;
                led_on = true;
                digitalWrite(LED_PIN, HIGH);
            }
        }
    }

    if(lcd_blinking && lcd_on) {
        uint64_t now = get_time();
        if(now - led_blink_time >= 80) {
            led_blink_time = now;
            lcd_bl_on = !lcd_bl_on;
            if(lcd_bl_on) lcd.backlight();
            else lcd.noBacklight();
        }
    }

    /* #endregion */

    /* #region PMS5003 loop */
    if(display_pms != 0) {
        int8_t result = pms5003.run();
        if(display_mode == DISPLAY_MODE_PMS5003 and result == PMS5003_DATA_READY) display_pms_data();
    }
    /* #endregion */

    bool new_data = bme680.run();

    bool display_data_ok = true;
    switch(display_mode) {
        case DISPLAY_MODE_LCD_CUSTOM_TEXT:
        case DISPLAY_MODE_PMS5003:
            display_data_ok = false;
            break;
    }

    if(lcd_on && new_data && display_data_ok) {
        if(display_mode == DISPLAY_MODE_DALLAS_TEMP) dallasTempSensor.readTemperature();
        if(display_mode == DISPLAY_MODE_SOIL_MOISTURE) soilMoistureSensor.read();
        display_data();
    }

    if(wifi_on && new_data) send_data_to_web();

}

/* #endregion */

/* #region Definicije funkcija */

void bsec_delay(uint32_t period) {
    uint64_t start = millis();
    while(millis() - start < period) bme680.run();
}

uint64_t get_time() {
    return bme680.getTimeMs();
}

const char* get_iaq_label(float iaq) {
    if(iaq <= 50) return "odlicno \x03";
    if(iaq <= 100) return "dobro \x04";
    if(iaq <= 150) return "blago zagadjeno \x04";
    if(iaq <= 200) return "umjereno zagadjeno \x05";
    if(iaq <= 250) return "jako zagadjeno \x06";
    if(iaq <= 350) return "ozbiljno zagadjeno \x07";
    return "krajnje zagadjeno \x07";
}

void display_progress_bar(uint8_t p, uint8_t row) {

    if(p > 40) return;
    lcd.setCursor(0, row);

	if (p == 0) lcd.write(LCD_CHAR_PROG_STR_0);
	else lcd.write(LCD_CHAR_PROG_STR_1);

	int8_t l = p / 2;
	if (l == 20) l = 19;

	int8_t i = 1;
	while (i < l) {
		lcd.write(LCD_CHAR_PROG_MID_2);
		i++;
	}

	if (p != 1 && p != 39 && p % 2) {
		lcd.write(LCD_CHAR_PROG_MID_1);
		i++;
	}

	while (i < 19) {
		lcd.write(LCD_CHAR_PROG_MID_0);
		i++;
	}

	if (p == 40) lcd.write(LCD_CHAR_PROG_END_1);
	else lcd.write(LCD_CHAR_PROG_END_0);

}

void display_battery_data() {

    switch(lcd_battery_mode) {

    case 1: {

        lcd.print("Napunjenost: ");
        uint16_t soc = Battery.soc();
        lcd.print(soc);
        lcd.print(" %");

        lcd.setCursor(0, 1);
        lcd.print("Preostalo vrijeme");

        lcd.setCursor(0, 2);
        float remain_capacity = Battery.capacity(AVAIL);
        float current = abs(Battery.current(AVG));
        float remain_time_h = remain_capacity / current;

        float time_h;
        float time_min = modff(remain_time_h, &time_h) * 60.0;
        if(time_h > 0) {
            lcd.print(time_h, 0);
            lcd.print("h ");
        }
        lcd.print(time_min, 0);
        lcd.print("min");

        float p = (float)soc * 40.0 / 100.0;
        uint8_t p2 = (uint8_t)roundf(p);
        display_progress_bar(p2, 3);

        break;
    }

    case 2: {

        lcd.print("Napon:  ");
        lcd.print((float)Battery.voltage() / 1000.0, 2);
        lcd.print(" V");

        lcd.setCursor(0, 1);
        lcd.print("Struja: ");
        int16_t current = Battery.current(AVG);
        if(current > 0) lcd.print("+");
        lcd.print(abs(current), 0);
        lcd.print(" mA");

        lcd.setCursor(0, 2);
        lcd.print("Snaga:  ");
        lcd.print(abs(Battery.power()), 0);
        lcd.print(" mW");

        /*lcd.setCursor(0, 3);
        lcd.print("Temp:   ");
        lcd.print(Battery.temperature() / 100.0, 1);
        lcd.print(" ");
        lcd.write(LCD_CHAR_DEGREE);
        lcd.print("C");*/

        break;
    }

    case 3: {

        lcd.print("Kapacitet  [mAh]");

        lcd.setCursor(0, 1);
        lcd.print("Preostali: ");
        lcd.print(Battery.capacity(AVAIL));

        lcd.setCursor(0, 2);
        lcd.print("Ukupni:    ");
        lcd.print(Battery.capacity(AVAIL_FULL));

        /*lcd.setCursor(0, 3);
        lcd.print("Zdravlje:  ");
        lcd.print(Battery.soh());
        lcd.print(" %");*/

        break;
    }

    }

}

/* #endregion */

/* #region Definicija funkcije display_data */

void display_data() {

    uint8_t charset = LCD_CHARSET_BASIC;
    if(display_mode == DISPLAY_MODE_BATTERY) charset = LCD_CHARSET_PROG_BAR;
    LCD_Charset::requestCharset(charset);

    bool display_wifi_indicator = wifi_on;

    lcd.clear();

    switch(display_mode) {

    /* #region DISPLAY_MODE_TEMP_PRESS_HUM */
    case DISPLAY_MODE_TEMP_PRESS_HUM:

        lcd.print("Temp:  ");
        lcd.print(bme680.temperature, 1);
        lcd.print(" ");
        lcd.write(LCD_CHAR_DEGREE);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("Tlak:  ");
        lcd.print(bme680.pressure / 100.0, 2);
        lcd.print(" hPa");

        lcd.setCursor(0, 2);
        lcd.print("Vlaga: ");
        lcd.print(bme680.humidity, 0);
        lcd.print(" %");
        
        break;
    /* #endregion */

    /* #region DISPLAY_MODE_IAQ_CO2_VOC */
    case DISPLAY_MODE_IAQ_CO2_VOC:

        if(bme680.iaqAccuracy != 3) {
            lcd.setCursor(wifi_on ? 16 : 17, 0);
            lcd.print("[");
            lcd.print(bme680.iaqAccuracy);
            lcd.print("]");
            lcd.setCursor(0, 0);
        }

        lcd.print("IAQ:   ");
        lcd.print(bme680.iaq, 0);

        lcd.setCursor(0, 1);
        lcd.print("CO2:   ");
        lcd.print(bme680.co2Equivalent, 0);
        lcd.print(" ppm");

        lcd.setCursor(0, 2);
        lcd.print("bVOC:  ");
        lcd.print(bme680.breathVocEquivalent, 0);
        lcd.print(" ppm");

        
        lcd.setCursor(0, 3);
        lcd.print("Otpor: ");
        lcd.print(bme680.gasResistance / 1000.0, 1);
        lcd.print(" k");
        lcd.write(LCD_CHAR_OMEGA);

        break;
    /* #endregion */

    /* #region DISPLAY_MODE_AIR_QUALITY */
    case DISPLAY_MODE_AIR_QUALITY:

        lcd.print("Kvaliteta zraka");
        
        lcd.setCursor(0, 1);
        lcd.print("IAQ: ");
        lcd.print(bme680.iaq, 0);

        if(bme680.iaqAccuracy != 3) {
            lcd.print(" [");
            lcd.print(bme680.iaqAccuracy);
            lcd.print("]");
        }

        lcd.setCursor(0, 2);
        if(bme680.iaqAccuracy == 0) lcd.print("kalibrira se");
        else lcd.print(get_iaq_label(bme680.iaq));
        
        break;
    /* #endregion */

    /* #region DISPLAY_MODE_MENU */
    case DISPLAY_MODE_MENU: {

        display_wifi_indicator = false;

        lcd.setCursor(0, display_menu_selection);
        lcd.write(LCD_CHAR_SELECTION);

        bool en = wifi_on;

        lcd.setCursor(1, 0);

        if(en) lcd.print("[");
        else lcd.print(" ");
        lcd.print("WiFi");
        if(en) {
            lcd.print("]");
            uint8_t n = WiFi.softAPgetStationNum();
            lcd.setCursor(n < 10 ? 17 : 16, 0);
            lcd.print("[");
            lcd.print(n);
            lcd.print("]");
        }

        lcd.setCursor(1, 1);
        en = soilMoistureSensor.isConnected();

        if(en) lcd.print("[");
        else lcd.print(" ");
        lcd.print("Senzor vlage tla");
        if(en) lcd.print("]");

        lcd.setCursor(1, 2);
        en = dallasTempSensor.isConnected();

        if(en) lcd.print("[");
        else lcd.print(" ");
        lcd.print("Dallas senzor");
        if(en) lcd.print("]");

        lcd.setCursor(1, 3);
        en = display_pms != 0;

        if(en) lcd.print("[");
        else lcd.print(" ");
        lcd.print("PMS5003");
        if(en) lcd.print("]");

        break;
    }
    /* #endregion */

    /* #region DISPLAY_MODE_DALLAS_TEMP */
    case DISPLAY_MODE_DALLAS_TEMP:

        lcd.print("Temperatura (Dallas)");
        lcd.setCursor(0, 1);
        lcd.print(dallasTempSensor.temperature());
        lcd.print(" ");
        lcd.write(LCD_CHAR_DEGREE);
        lcd.print("C");

        lcd.setCursor(0, 2);
        lcd.print("Temperatura (BME680)");
        lcd.setCursor(0, 3);
        lcd.print(bme680.temperature);
        lcd.print(" ");
        lcd.write(LCD_CHAR_DEGREE);
        lcd.print("C");

        break;
    /* #endregion */

    /* #region DISPLAY_MODE_BATTERY */
    case DISPLAY_MODE_BATTERY:
        display_wifi_indicator = false;
        display_battery_data();
        break;
    /* #endregion */

    /* #region DISPLAY_MODE_ALTITUDE */
    case DISPLAY_MODE_ALTITUDE:
        
        lcd.print("Nadmorska visina");
        lcd.setCursor(0, 1);
        lcd.print(altimeter.altitude(bme680.pressure / 100.0), 2);
        lcd.print(" m");

        break;
    /* #endregion */

    /* #region DISPLAY_MODE_ALTITUDE_CHANGE */
    case DISPLAY_MODE_ALTITUDE_CHANGE:

        lcd.print("Promjena visine");
        lcd.setCursor(0, 1);
        lcd.print(altimeter.altitude_change_method1(bme680.pressure / 100.0), 2);
        lcd.print(" m");
        lcd.setCursor(0, 2);
        lcd.print(altimeter.altitude_change_method2(bme680.pressure / 100.0), 2);
        lcd.print(" m");
        lcd.setCursor(0, 3);
        lcd.print(altimeter.altitude_change_method3(bme680.pressure / 100.0, bme680.temperature), 2);
        lcd.print(" m");

        break;
    /* #endregion */

    /* #region DISPLAY_MODE_SOIL_MOISTURE */
    case DISPLAY_MODE_SOIL_MOISTURE:

        lcd.print("Vlaga tla");
        lcd.setCursor(0, 1);
        lcd.print(soilMoistureSensor.percentage());
        lcd.print(" %");
        lcd.setCursor(0, 2);
        lcd.print(soilMoistureSensor.raw());
        lcd.setCursor(0, 3);
        lcd.print(soilMoistureSensor.getMinValue());
        lcd.print(" - ");
        lcd.print(soilMoistureSensor.getMaxValue());

        break;

    /* #endregion */

    /* #region DISPLAY_MODE_TIME */
    case DISPLAY_MODE_TIME: {

        rtc.readTime();
        uint8_t hour = rtc.getHour();
        uint8_t minute = rtc.getMinute();
        uint8_t day = rtc.getDay();
        uint8_t month = rtc.getMonth();
        uint16_t year = rtc.getYear();

        uint8_t time_pos = hour < 10 ? 8 : 7;

        lcd.setCursor(time_pos, 1);
        lcd.print(hour);
        lcd.print(":");
        if(minute < 10) lcd.print("0");
        lcd.print(minute);

        uint8_t date_pos = 6;
     
        if(day >= 10 and month >= 10) date_pos--;
        if(hour >= 10) date_pos--;

        lcd.setCursor(date_pos, 2);
        lcd.print(day);
        lcd.print(".");
        lcd.print(month);
        lcd.print(".");
        lcd.print(year);
        lcd.print(".");

        break;
    }
    /* #endregion */

    }

    if(display_wifi_indicator) {
        lcd.setCursor(19, 0);
        lcd.write(LCD_CHAR_WIFI);
    }

}

void display_pms_data() {

    pms5003_data &data = pms5003.data;
    lcd.clear();

    switch(display_pms) {
    
    /* #region PM */
    case 1:

        lcd.print("PM std, env");

        lcd.setCursor(0, 1);
        lcd.print("PM1.0: ");
        lcd.print(data.pm10_standard);
        lcd.print(", ");
        lcd.print(data.pm10_env);

        lcd.setCursor(0, 2);
        lcd.print("PM2.5: ");
        lcd.print(data.pm25_standard);
        lcd.print(", ");
        lcd.print(data.pm25_env);

        lcd.setCursor(0, 3);
        lcd.print("PM10:  ");
        lcd.print(data.pm100_standard);
        lcd.print(", ");
        lcd.print(data.pm100_env);

        break;
    /* #endregion */

    /* #region particles */
    case 2:
        
        lcd.print("Broj cestica");

        lcd.setCursor(0, 1);
        lcd.print("p0.3=");
        lcd.print(data.particles_03um);

        lcd.setCursor(0, 2);
        lcd.print("p0.5=");
        lcd.print(data.particles_05um);

        lcd.setCursor(0, 3);
        lcd.print("p1.0=");
        lcd.print(data.particles_10um);

        
        lcd.setCursor(12, 1);
        lcd.print("p2.5=");
        lcd.print(data.particles_25um);

        lcd.setCursor(12, 2);
        lcd.print("p5.0=");
        lcd.print(data.particles_50um);

        lcd.setCursor(12, 3);
        lcd.print("p10= ");
        lcd.print(data.particles_100um);

        break;
    /* #endregion */

    /*
    // PM1.0, PM2.5, PM10 concentration unit - standard particle, ug/m3 
    case 1:

        lcd.print("PM std");

        lcd.setCursor(0, 1);
        lcd.print("PM1.0: ");
        lcd.print(data.pm10_standard);

        lcd.setCursor(0, 2);
        lcd.print("PM2.5: ");
        lcd.print(data.pm25_standard);

        lcd.setCursor(0, 3);
        lcd.print("PM10:  ");
        lcd.print(data.pm100_standard);

        break;

    // PM1.0, PM2.5, PM10 concentration unit - under atmospheric environment, ug/m3
    case 2:

        lcd.print("PM env");

        lcd.setCursor(0, 1);
        lcd.print("PM1.0: ");
        lcd.print(data.pm10_env);

        lcd.setCursor(0, 2);
        lcd.print("PM2.5: ");
        lcd.print(data.pm25_env);

        lcd.setCursor(0, 3);
        lcd.print("PM10:  ");
        lcd.print(data.pm100_env);

        break;

    // d > 0.3, 0.5, 1.0 um
    case 3:

        lcd.print("Broj cestica");

        lcd.setCursor(0, 1);
        lcd.print("p0.3: ");
        lcd.print(data.particles_03um);

        lcd.setCursor(0, 2);
        lcd.print("p0.5: ");
        lcd.print(data.particles_05um);

        lcd.setCursor(0, 3);
        lcd.print("p1.0: ");
        lcd.print(data.particles_10um);

        break;

    // d > 2.5, 5.0, 10.0 um
    case 4:

        lcd.print("Broj cestica");

        lcd.setCursor(0, 1);
        lcd.print("p2.5: ");
        lcd.print(data.particles_25um);

        lcd.setCursor(0, 2);
        lcd.print("p5.0: ");
        lcd.print(data.particles_50um);

        lcd.setCursor(0, 3);
        lcd.print("p10:  ");
        lcd.print(data.particles_100um);

        break;
    */
    }

}

/* #endregion */

/* #region Definicije funkcija za učitavanje i spremanja stanja bsec algoritma */

void load_bsec_state() {

    if(EEPROM.read(0) != BSEC_MAX_STATE_BLOB_SIZE) return;

    for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
        bsec_state[i] = EEPROM.read(i + 1);
    }
    bme680.setState(bsec_state);

}

void save_bsec_state() {

    bme680.getState(bsec_state);
    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
        EEPROM.write(i + 1, bsec_state[i]);
    }
    EEPROM.commit();

}

/* #endregion */

/* #region Definicije funkcija za WiFi */

void turn_on_wifi() {

    lcd.clear();
    lcd.print("Ukljucivanje WiFi-a");

    WiFi.softAP(AP_WIFI_SSID, NULL);
    web_server.begin();

    IPAddress local_ip(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(local_ip, local_ip, subnet);
    
    dns_server.start(53, "*", local_ip);
    MDNS.begin(AP_DOMAIN_NAME);
    MDNS.addService("http", "tcp", 80);

    wifi_on = true;
    bsec_delay(400);

}

void turn_off_wifi() {

    lcd.clear();
    lcd.print("Iskljucivanje WiFi-a");

    WiFi.mode(WIFI_OFF);
    web_server.end();

    dns_server.stop();
    MDNS.end();

    wifi_on = false;
    bsec_delay(400);

}

/* #endregion */

/* #region Definicije funkcija za Web */

void send_data_to_web() {
    
    float iaq_value = bme680.iaq;
    if(bme680.iaqAccuracy <= 1) iaq_value = -iaq_value;

    char buffer[32];
    sprintf(buffer, "%.1f,%.2f,%.0f,%.0f", bme680.temperature, bme680.pressure / 100, bme680.humidity, iaq_value);
    web_socket.textAll(buffer);

}

void on_web_socket_lcd(const char* data) {

    size_t len = strlen(data);
    if(len > WEB_BUFFER_SIZE) return;
    strcpy(web_buffer, data);

    web_flag = 1;

}

void on_web_socket_lcd_bl(const char* data) {
    strcpy(web_buffer, data);
    web_flag = 3;
}

void on_web_socket_rtc(const char* data) {
    strcpy(web_buffer, data);
    web_flag = 2;
}

void on_web_socket_data(const char* data) {

    const char* m = extract_value(data);
    if(m == NULL) return;

    if(starts_with(data, "lcd")) on_web_socket_lcd(m);
    else if(starts_with(data, "rtc")) on_web_socket_rtc(m);
    else if(starts_with(data, "bl")) on_web_socket_lcd_bl(m);

}


void on_web_socket_event(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t *data, size_t len) {
    if(type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            on_web_socket_data((const char*)data);
        }
    }else if(type == WS_EVT_CONNECT) {
        send_data_to_web();
    }
}

/* #endregion */

#endif
