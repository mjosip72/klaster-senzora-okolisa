
#pragma once

#include <math.h>

class Altimeter {

    private:

    float pressure_for_alt_change;
    float altitude_for_alt_change;

    public:

    float pressure_at_sea_level = 1013.25;
    float temperature_at_sea_level = 15;
    float temperature_lapse_rate = 0.0065;

    Altimeter() {
        
    }

    float altitude(float pressure) {

        float k = (temperature_at_sea_level + 273.15) / temperature_lapse_rate;
        float pr = pressure / pressure_at_sea_level;

        return k * ( 1.0 - powf(pr, 1.0/5.255) );

    }

    void set_altitude_change_pressure(float pressure) {
        pressure_for_alt_change = pressure;
        altitude_for_alt_change = altitude(pressure);
    }

    float altitude_change_method1(float pressure) {
        float alt2 = altitude(pressure);
        return alt2 - altitude_for_alt_change;
    }

    float altitude_change_method2(float pressure) {
        float k = (temperature_at_sea_level + 273.15) / temperature_lapse_rate;
        float pr = pressure / pressure_for_alt_change;
        return k * ( 1.0 - powf(pr, 1.0/5.255) );
    }

    float altitude_change_method3(float pressure, float temp) {
        float k = (temp + 273.15) / temperature_lapse_rate;
        float pr = pressure / pressure_for_alt_change;
        return k * ( 1.0 - powf(pr, 1.0/5.255) );
    }

};
