#ifndef INC_GFX_FUNCTIONS_H_
#define INC_GFX_FUNCTIONS_H_

#include <ST7735.h>
#include "stdint.h"
#include <stdio.h>
#include "stdlib.h"
#include <string.h>

void drawPixel(int16_t x, int16_t y, uint16_t color);
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void  drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void  drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void fillScreen(uint16_t color);

void drawMenu(char ** param, uint16_t str_count, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int set, FontDef font, uint16_t txt_color, uint16_t bg_color);
void drawIcon(const char* header_t, bool highlight, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, FontDef font, uint16_t txt_color, uint16_t bg_color);
void drawHeader(const char* header_t, FontDef font, uint16_t txt_color, uint16_t bg_color);

void drawErrorIcon(int error);

#endif /* INC_GFX_FUNCTIONS_H_ */
