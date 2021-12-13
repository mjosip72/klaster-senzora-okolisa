
#pragma once

#include <Arduino.h>

#define BUTTON_EVENT_RELEASE_THRESHOLD_MICROS 10000
#define BUTTON_EVENT_LONG_PRESS_THRESHOLD_MILLIS 600

class ButtonEvent {

    private:
    uint8_t pin;
    uint64_t pressed_time;
    uint64_t released_time;
    bool button_pressed;
    bool button_released;

    public:
    void (*onPress)(bool longPress);
    ButtonEvent(uint8_t _pin);
    void loop();

};

extern uint64_t get_time();

ButtonEvent::ButtonEvent(uint8_t _pin) {
    button_pressed = false;
    button_released = false;
    pin = _pin;
}

void ButtonEvent::loop() {

    bool button_state = digitalRead(pin);

    if(button_state == HIGH) {
        
        if(button_released and button_pressed) {
            button_released = false;
        }

        if(!button_pressed) {
            button_pressed = true;
            pressed_time = millis();
        }

    }else{
        
        if(button_pressed) {

            if(!button_released) {
                button_released = true;
                released_time = micros();
            }else{
                if(micros() - released_time >= BUTTON_EVENT_RELEASE_THRESHOLD_MICROS) {
                    button_pressed = false;

                    uint64_t duration = millis() - pressed_time;
                    bool longPress = duration >= BUTTON_EVENT_LONG_PRESS_THRESHOLD_MILLIS;
                    onPress(longPress);

                }
            }

        }

    }

}
