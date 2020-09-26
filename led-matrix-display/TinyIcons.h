/*
 * Adapted from:
 * 
 * mirel.lazar@gmail.com
 * 
 * provided 'AS IS', use it at your own risk
 */
//
#ifndef TINYICONS_H
#define TINYICONS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#define TI_COLS 10
#define TI_ROWS 5

#define TI_PIXELS TI_COLS * TI_ROWS

void TIDrawIcon(Adafruit_GFX& d, uint16_t id, uint8_t x, uint8_t y, uint8_t frame, bool daytime, bool transparent, uint16_t backgroundColor);

#endif

