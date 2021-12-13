
#pragma once

#include "LiquidCrystal_I2C.h"

class LiquidCrystal : public LiquidCrystal_I2C {
   public:
   LiquidCrystal() : LiquidCrystal_I2C(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE) {}
};
