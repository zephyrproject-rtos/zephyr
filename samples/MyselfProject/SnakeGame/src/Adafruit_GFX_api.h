#ifndef __ADAFRUIT_GFX_API_H__
#define __ADAFRUIT_GFX_API_H__
#include <sys/byteorder.h>

#define     INIT_ST7735      0x01








void Adafruit_drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color);
                                
void Adafruit_drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color);
void Adafruit_clear(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color);
void Adafruit_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color);

void Adafruit_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color);

void Adafruit_drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color);

void Adafruit_displayInit(uint8_t options);
void Adafruit_drawPixel(int x, int y, uint16_t color);
void Adafruit_drawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint16_t color);
void Adafruit_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void Adafruit_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color);
#endif