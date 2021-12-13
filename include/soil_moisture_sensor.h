
#pragma once

#include <Arduino.h>
#include <EEPROM.h>

#define SOIL_MOISTURE_EEPROM_MIN_VALUE_ADDR 140
#define SOIL_MOISTURE_EEPROM_MAX_VALUE_ADDR 142

class SoilMoistureSensor {

    private:
    uint8_t adc_pin;
    bool connected;
    uint16_t min_value;
    uint16_t max_value;
    float _raw;

    public:

    SoilMoistureSensor(uint8_t _adc_pin) {
        adc_pin = _adc_pin;
        connected = false;
    }

    void read() {
        float value = 0;
        for(uint16_t i = 0; i < 2000; i++) value += analogRead(adc_pin);
        value /= 2000.0;
        _raw = roundf(4095 - value);
    }

    float raw() {
        return _raw;
    }

    uint8_t percentage() {

        float r = _raw;
        if(r < min_value) r = min_value;
        else if(r > max_value) r = max_value;

        r -= min_value;

        float prc = r / (float)(max_value - min_value) * 100.0;

        return (uint8_t)roundf(prc);
    }

    uint16_t getMinValue() {
        return min_value;
    }

    uint16_t getMaxValue() {
        return max_value;
    }

    void loadMinMax() {
        EEPROM.get(SOIL_MOISTURE_EEPROM_MIN_VALUE_ADDR, min_value);
        EEPROM.get(SOIL_MOISTURE_EEPROM_MAX_VALUE_ADDR, max_value);
        if(min_value == 0xFFFF or max_value == 0xFFFF) resetMinMax();
    }

    void resetMinMax() {
        min_value = 0xFFFF;
        max_value = 0;
    }

    void saveMinMax() {
        EEPROM.put(SOIL_MOISTURE_EEPROM_MIN_VALUE_ADDR, min_value);
        EEPROM.put(SOIL_MOISTURE_EEPROM_MAX_VALUE_ADDR, max_value);
        EEPROM.commit();
    }

    void calibrateMinMax() {
        uint16_t r = raw();
        if(r < min_value) min_value = r;
        else if(r > max_value) max_value = r;
    }

    void connect() {
        connected = true;
    }

    void disconnect() {
        connected = false;
    }

    bool isConnected() {
        return connected;
    }

};
