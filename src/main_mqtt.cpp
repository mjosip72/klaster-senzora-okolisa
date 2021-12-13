
#if false

#include <Arduino.h>
#include <bsec.h>
#include <LiquidCrystal.h>
#include <SparkFunBQ27441.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <PCF85063A.h>
#include <AsyncMqttClient.h>
extern "C" {
#include <bootloader_random.h>
}
#include "button_event.h"
#include "lcd_charset.h"
#include "dallas_temperature_sensor.h"
#include "soil_moisture_sensor.h"
#include "altimeter.h"
#include "pms5003.h"

/* #region Pins */
#define GREEN_BUTTON_PIN 25
#define YELLOW_BUTTON_PIN 26
#define RED_BUTTON_PIN 27
#define ONE_WIRE_BUS 33
#define ANALOG_INPUT_PIN 32
#define RX_PIN 16
#define TX_PIN 17
#define WIRE1_SDA 18
#define WIRE1_SCL 19
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
/* #endregion */

/* #region WiFi, Mqtt postavke */

#define AP_WIFI_SSID "Klaster senzora okoliša"
#define WIFI_SSID "WLAN_18E1D0"
#define WIFI_PASS "R7gaQ3P25Nf8"

#define MQTT_HOST "192.168.0.120"
#define MQTT_PORT 1883

/* #endregion */

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

Bsec bme680;
uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};

/*  USB microB
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
bool display_time = false;

PMS5003 pms5003(RX_PIN, TX_PIN);
uint8_t display_pms = 0;

Altimeter altimeter;

#define BATTERY_CAPACITY 2100
#define TERMINATE_VOLTAGE 3600
uint8_t lcd_battery_mode = 1;

AsyncMqttClient mqtt_client;
bool wifi_on = false;
/*
AsyncWebServer web_server(80);
AsyncEventSource web_events("/events");
const char index_html[] = {
#include "index-html.txt"
};
*/

/* #region Deklaracije funkcija */

void bsec_delay(uint32_t period);
uint64_t get_time();

void load_state();
void save_state();

void display_data();
void display_pms_data();

void turn_on_wifi();
void turn_off_wifi();

void wifi_on_sta_connected(WiFiEvent_t event, WiFiEventInfo_t info);
void wifi_on_sta_got_ip(WiFiEvent_t event, WiFiEventInfo_t info);
void wifi_on_sta_disconnected(WiFiEvent_t event, WiFiEventInfo_t info);

void mqtt_on_connect(bool sessionPresent);
void mqtt_on_disconnect(AsyncMqttClientDisconnectReason reason);
void mqtt_on_message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

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

                save_state();
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
    delay(400);

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
    load_state();

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

    //web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //    request->send_P(200, "text/html", index_html, NULL);
    //});
    //web_server.addHandler(&web_events);

    WiFi.onEvent(wifi_on_sta_connected, SYSTEM_EVENT_STA_CONNECTED);
    WiFi.onEvent(wifi_on_sta_got_ip, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent(wifi_on_sta_disconnected, SYSTEM_EVENT_STA_DISCONNECTED);

    mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_client.onConnect(mqtt_on_connect);
    mqtt_client.onDisconnect(mqtt_on_disconnect);
    mqtt_client.onMessage(mqtt_on_message);

}

void loop() {

    greenButton.loop();
    yellowButton.loop();
    redButton.loop();

    /* #region PMS5003 loop */
    if(display_pms != 0) {
        int8_t result = pms5003.run();
        if(display_mode == DISPLAY_MODE_PMS5003 and result == PMS5003_DATA_READY) display_pms_data();
    }
    /* #endregion */

    bool new_data = bme680.run();

    if(lcd_on and new_data and display_mode != DISPLAY_MODE_MENU and display_mode != DISPLAY_MODE_PMS5003) {
        if(display_mode == DISPLAY_MODE_DALLAS_TEMP) dallasTempSensor.readTemperature();
        if(display_mode == DISPLAY_MODE_SOIL_MOISTURE) soilMoistureSensor.read();
        display_data();
    }

    /* #region Send data to web */

    /*
    if(new_data and wifi_on) {
        //const char* sample = "temp=-32.8,press=1024.92,hum=42,res=100.2,acc=3,iaq=800,siaq=800,co2=10000,voc=1000";
        char buffer[100];
        sprintf(buffer, "temp=%.1f,press=%.2f,hum=%.0f,res=%.1f,acc=%d,iaq=%.0f,siaq=%.0f,co2=%.0f,voc=%.0f", bme680.temperature, bme680.pressure / 100.0, bme680.humidity, bme680.gasResistance / 1000.0, (int)bme680.iaqAccuracy, bme680.iaq, bme680.staticIaq, bme680.co2Equivalent, bme680.breathVocEquivalent);
        web_events.send(buffer);
    }
    */

    /* #endregion */

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
        float remain_capacity = Battery.capacity(REMAIN);
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

        lcd.setCursor(0, 3);
        lcd.print("Temp:   ");
        lcd.print(Battery.temperature() / 100.0, 1);
        lcd.print(" ");
        lcd.write(LCD_CHAR_DEGREE);
        lcd.print("C");

        break;
    }

    case 3: {

        lcd.print("Kapacitet  [mAh]");

        lcd.setCursor(0, 1);
        lcd.print("Preostali: ");
        lcd.print(Battery.capacity(REMAIN));
        lcd.print(" ");
        lcd.print(Battery.capacity(AVAIL));

        lcd.setCursor(0, 2);
        lcd.print("Ukupni:    ");
        lcd.print(Battery.capacity(FULL));
        lcd.print(" ");
        lcd.print(Battery.capacity(AVAIL_FULL));

        lcd.setCursor(0, 3);
        lcd.print("Zdravlje:  ");
        lcd.print(Battery.soh());
        lcd.print(" %");

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
        if(en) lcd.print("]");

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

void load_state() {

    if(EEPROM.read(0) != BSEC_MAX_STATE_BLOB_SIZE) return;

    for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
        bsec_state[i] = EEPROM.read(i + 1);
    }
    bme680.setState(bsec_state);

}

void save_state() {

  bme680.getState(bsec_state);
  EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
  for(uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
    EEPROM.write(i + 1, bsec_state[i]);
  }
  EEPROM.commit();

}

/* #endregion */

/* #region Definicije funkcija za WiFi */

void wifi_on_sta_connected(WiFiEvent_t event, WiFiEventInfo_t info) {}

void wifi_on_sta_got_ip(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifi_on = true;
    mqtt_client.disconnect(true);
    mqtt_client.connect();
}

void wifi_on_sta_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if(wifi_on) WiFi.reconnect();
}

void turn_on_wifi() {

    lcd.clear();
    lcd.print("Ukljucivanje WiFi-a");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    bsec_delay(400);

    /*WiFi.softAP(wifi_ssid, NULL);

    IPAddress local_ip(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(local_ip, local_ip, subnet);

    web_server.begin();
    */
}

void turn_off_wifi() {

    lcd.clear();
    lcd.print("Iskljucivanje WiFi-a");
    wifi_on = false;

    bsec_delay(400);

    mqtt_client.disconnect();
    WiFi.mode(WIFI_OFF);

    //web_server.end();

}

/* #endregion */

/* #region Definicije funkcija za Mqtt */

void mqtt_on_connect(bool sessionPresent) {
    mqtt_client.subscribe("com", 2);
}

void mqtt_on_disconnect(AsyncMqttClientDisconnectReason reason) {
    if(wifi_on) mqtt_client.connect();
}

char mqtt_data_buffer[256];

void mqtt_request_data() {

    char* addr = mqtt_data_buffer;
    
    addr += sprintf(addr, "%.1f,%.2f,%.0f,%.1f,",
        bme680.temperature,
        bme680.pressure / 100.0,
        bme680.humidity,
        bme680.gasResistance / 1000.0
    );

    addr += sprintf(addr, "%.0f,%.0f,%d,%.0f,",
        bme680.iaq,
        bme680.staticIaq,
        bme680.iaqAccuracy,
        bme680.breathVocEquivalent
    );

    addr += sprintf(addr, "%d,%d,%d,",
        pms5003.data.pm10_env,
        pms5003.data.pm25_env,
        pms5003.data.pm100_env
    );

    addr += sprintf(addr, "%d,%d,%d,%d,%d,%d",
        pms5003.data.particles_03um,
        pms5003.data.particles_05um,
        pms5003.data.particles_10um,
        pms5003.data.particles_25um,
        pms5003.data.particles_50um,
        pms5003.data.particles_100um
    );

    mqtt_client.publish("data", 2, true, mqtt_data_buffer);

}

void mqtt_on_message(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

    if(strcmp(topic, "com") == 0) {
        if(strcmp(payload, "respond") == 0) mqtt_client.publish("response", 2, true, "OK");
        if(strcmp(payload, "data") == 0) mqtt_request_data();
    }

}

/* #endregion */

#endif
