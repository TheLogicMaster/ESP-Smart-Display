/*
 * mirel.lazar@gmail.com
 * 
 * provided 'AS IS', use it at your own risk
 */
#ifndef TINYFONT_H
#define TINYFONT_H

#include <Arduino.h>
#include "Adafruit_GFX.h"

#define TF_COLS 3
#define TF_ROWS 5
struct TFFace {
  char fface[TF_ROWS]; //4 cols x 5 rows
};

void TFDrawChar (Adafruit_GFX& d, char value, uint8_t x, uint8_t y, uint16_t color, bool transparent, uint16_t backgroundColor);

void TFDrawText (Adafruit_GFX& d, const char text[], uint8_t x, uint8_t y, uint16_t color, bool transparent, uint16_t backgroundColor);

#endif
