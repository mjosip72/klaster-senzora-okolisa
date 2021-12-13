
#pragma once

#include <DallasTemperature.h>

class DallasTemperatureSensor {

    private:

    OneWire oneWire;
    DallasTemperature sensor;
    DeviceAddress deviceAddress;
    bool connected;
    float _temperature;

    public:

    DallasTemperatureSensor(uint8_t oneWireBus) : oneWire(oneWireBus), sensor(&oneWire) {
        connected = false;
    }

    void begin() {
        sensor.begin();
        if(sensor.getDS18Count() != 1) return;
        connected = true;
        sensor.getAddress(deviceAddress, 0);
        sensor.setResolution(12);
    }

    void end() {
        connected = false;
    }

    bool isConnected() {
        return connected;
    }

    void readTemperature() {
        sensor.requestTemperaturesByAddress(deviceAddress);
        _temperature = sensor.getTempC(deviceAddress);
    }

    float temperature() {
        return _temperature;
    }

};
