
#pragma once

#include <LiquidCrystal.h>

extern LiquidCrystal lcd;

namespace LCD_Charset {

/* #region Definicije znakova */

#define LCD_CHAR_DEGREE 0xDF

#define LCD_CHARSET_BASIC 1

#define LCD_CHAR_WIFI (uint8_t)0
#define LCD_CHAR_SELECTION 1
#define LCD_CHAR_OMEGA 2

#define LCD_CHAR_SMILE1 3
#define LCD_CHAR_SMILE2 4
#define LCD_CHAR_SMILE3 5
#define LCD_CHAR_SMILE4 6
#define LCD_CHAR_SMILE5 7

uint8_t char_wifi[] = {0x00,0x00,0x1F,0x00,0x0E,0x00,0x04,0x00};
uint8_t char_selection[] = {0x08,0x0C,0x0E,0x0F,0x0E,0x0C,0x08,0x00};
uint8_t char_omega[] = {0x00,0x0E,0x11,0x11,0x11,0x0A,0x1B,0x00};
uint8_t char_smile1[] = {0x00,0x0A,0x00,0x00,0x1F,0x11,0x0E,0x00}; // :D
uint8_t char_smile2[] = {0x00,0x0A,0x00,0x00,0x11,0x0E,0x00,0x00}; // :)
uint8_t char_smile3[] = {0x00,0x0A,0x00,0x00,0x00,0x1F,0x00,0x00}; // :|
uint8_t char_smile4[] = {0x00,0x0A,0x00,0x00,0x0E,0x11,0x00,0x00}; // :(
uint8_t char_smile5[] = {0x00,0x00,0x0A,0x00,0x00,0x0E,0x11,0x1F}; // :C

#define LCD_CHARSET_PROG_BAR 2

#define LCD_CHAR_PROG_STR_0 (uint8_t)0
#define LCD_CHAR_PROG_STR_1 1
#define LCD_CHAR_PROG_MID_0 2
#define LCD_CHAR_PROG_MID_1 3
#define LCD_CHAR_PROG_MID_2 4
#define LCD_CHAR_PROG_END_0 5
#define LCD_CHAR_PROG_END_1 6

uint8_t char_prog_str_0[] = {0x0F,0x18,0x10,0x10,0x10,0x10,0x18,0x0F};
uint8_t char_prog_str_1[] = {0x0F,0x18,0x13,0x17,0x17,0x13,0x18,0x0F};
uint8_t char_prog_mid_0[] = {0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x1F};
uint8_t char_prog_mid_1[] = {0x1F,0x00,0x18,0x18,0x18,0x18,0x00,0x1F};
uint8_t char_prog_mid_2[] = {0x1F,0x00,0x1B,0x1B,0x1B,0x1B,0x00,0x1F};
uint8_t char_prog_end_0[] = {0x1E,0x03,0x01,0x01,0x01,0x01,0x03,0x1E};
uint8_t char_prog_end_1[] = {0x1E,0x03,0x19,0x1D,0x1D,0x19,0x03,0x1E};

/* #endregion */

uint8_t current_charset = 0;

void requestCharset(uint8_t charset) {

    if(current_charset == charset) return;
    current_charset = charset;

    switch(charset) {

    case LCD_CHARSET_BASIC:
        lcd.createChar(LCD_CHAR_WIFI, char_wifi);
        lcd.createChar(LCD_CHAR_SELECTION, char_selection);
        lcd.createChar(LCD_CHAR_OMEGA, char_omega);
        lcd.createChar(LCD_CHAR_SMILE1, char_smile1);
        lcd.createChar(LCD_CHAR_SMILE2, char_smile2);
        lcd.createChar(LCD_CHAR_SMILE3, char_smile3);
        lcd.createChar(LCD_CHAR_SMILE4, char_smile4);
        lcd.createChar(LCD_CHAR_SMILE5, char_smile5);
        break;

    case LCD_CHARSET_PROG_BAR:
        lcd.createChar(LCD_CHAR_PROG_STR_0, char_prog_str_0);
        lcd.createChar(LCD_CHAR_PROG_STR_1, char_prog_str_1);
        lcd.createChar(LCD_CHAR_PROG_MID_0, char_prog_mid_0);
        lcd.createChar(LCD_CHAR_PROG_MID_1, char_prog_mid_1);
        lcd.createChar(LCD_CHAR_PROG_MID_2, char_prog_mid_2);
        lcd.createChar(LCD_CHAR_PROG_END_0, char_prog_end_0);
        lcd.createChar(LCD_CHAR_PROG_END_1, char_prog_end_1);
        break;

    }

}

};
