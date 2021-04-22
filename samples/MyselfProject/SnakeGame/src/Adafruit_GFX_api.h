#ifndef __ADAFRUIT_GFX_API_H__
#define __ADAFRUIT_GFX_API_H__
#include <sys/byteorder.h>

#define     INIT_ST7735      0x01











void Adafruit_displayInit(uint8_t options);
void Adafruit_drawPixel(int x, int y, uint16_t color);
void Adafruit_drawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint16_t color);
void Adafruit_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
#endif