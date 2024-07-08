#include "GFX_FUNCTIONS.h"

extern int16_t _width;       ///< Display width as modified by current rotation
extern int16_t _height;      ///< Display height as modified by current rotation
extern int16_t cursor_x;     ///< x location to start print()ing text
extern int16_t cursor_y;     ///< y location to start print()ing text
extern uint8_t rotation;     ///< Display rotation (0 thru 3)
extern uint8_t _colstart;   ///< Some displays need this changed to offset
extern uint8_t _rowstart;       ///< Some displays need this changed to offset
extern uint8_t _xstart;
extern uint8_t _ystart;

void drawPixel(int16_t x, int16_t y, uint16_t color)
{
	ST7735_DrawPixel(x, y, color);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	ST7735_FillRectangle(x, y, w, h, color);
}


/***********************************************************************************************************/


#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

#define min(a, b) (((a) < (b)) ? (a) : (b))


void writePixel(int16_t x, int16_t y, uint16_t color)
{
    drawPixel(x, y, color);
}

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            writePixel(y0, x0, color);
        } else {
            writePixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void  drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	writeLine(x, y, x, y + h - 1, color);
}
void  drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	writeLine(x, y, x + w - 1, y, color);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t(y0, y1);
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if(y0 == y1){
        if(x0 > x1) _swap_int16_t(x0, x1);
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        writeLine(x0, y0, x1, y1, color);
    }
}

void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    writePixel(x0  , y0+r, color);
    writePixel(x0  , y0-r, color);
    writePixel(x0+r, y0  , color);
    writePixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        writePixel(x0 + x, y0 + y, color);
        writePixel(x0 - x, y0 + y, color);
        writePixel(x0 + x, y0 - y, color);
        writePixel(x0 - x, y0 - y, color);
        writePixel(x0 + y, y0 + x, color);
        writePixel(x0 - y, y0 + x, color);
        writePixel(x0 + y, y0 - x, color);
        writePixel(x0 - y, y0 - x, color);
    }
}

void drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            writePixel(x0 + x, y0 + y, color);
            writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            writePixel(x0 + x, y0 - y, color);
            writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            writePixel(x0 - y, y0 + x, color);
            writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            writePixel(x0 - y, y0 - x, color);
            writePixel(x0 - x, y0 - y, color);
        }
    }
}

void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color)
{

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    int16_t px    = x;
    int16_t py    = y;

    delta++; // Avoid some +1's in the loop

    while(x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if(x < (y + 1)) {
            if(corners & 1) drawFastVLine(x0+x, y0-y, 2*y+delta, color);
            if(corners & 2) drawFastVLine(x0-x, y0-y, 2*y+delta, color);
        }
        if(y != py) {
            if(corners & 1) drawFastVLine(x0+py, y0-px, 2*px+delta, color);
            if(corners & 2) drawFastVLine(x0-py, y0-px, 2*px+delta, color);
            py = y;
        }
        px = x;
    }
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    drawFastVLine(x0, y0-r, 2*r+1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}



void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y+h-1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x+w-1, y, h, color);
}

void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    drawFastHLine(x+r  , y    , w-2*r, color); // Top
    drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    drawFastVLine(x    , y+r  , h-2*r, color); // Left
    drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    drawCircleHelper(x+r    , y+r    , r, 1, color);
    drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}


void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    fillRect(x+r, y, w-2*r, h, color);
    // draw four corners
    fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}


void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}


void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }

    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        drawFastHLine(a, y0, b-a+1, color);
        return;
    }

    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        drawFastHLine(a, y, b-a+1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        drawFastHLine(a, y, b-a+1, color);
    }
}

void fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}


/*/_____________________________________NEW function_____________________________________/*/

void drawMenu(char ** param, uint16_t str_count, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
	int set, FontDef font, uint16_t txt_color, uint16_t bg_color) {

	uint8_t sett = set % str_count;
	const uint8_t max_set_in_page = (y1 - y0) / (font.height + 2);
	const uint8_t max_ch_in_str = (x1 - x0) / font.width;
	const uint8_t currentPage = (set == -1) ? (0) : (sett / max_set_in_page);
	uint8_t max_set;

	if (currentPage == str_count / max_set_in_page) {
		max_set = currentPage * max_set_in_page + str_count % max_set_in_page;
	} else {
		max_set = (currentPage * max_set_in_page + max_set_in_page);
	}

	for (uint16_t i = currentPage * max_set_in_page; i < (currentPage + 1) * max_set_in_page; i++) {
		if (i < max_set) {
			char word[max_ch_in_str + 1];
			memset(word, 0, (max_ch_in_str + 1)*sizeof(char));

			if (i == sett && set != -1)
				ST7735_WriteString(x0, i * (font.height + 2) + y0 - currentPage * max_set_in_page * (font.height + 2), strncpy(word, *(param + i), max_ch_in_str), font, bg_color, txt_color);
			else
				ST7735_WriteString(x0, i * (font.height + 2) + y0 - currentPage * max_set_in_page * (font.height + 2), strncpy(word, *(param + i), max_ch_in_str), font, txt_color, bg_color);

			fillRect(x0 + strlen(word) * font.width, i * (font.height + 2) + y0 - currentPage * max_set_in_page * (font.height + 2), x1 - x0 - strlen(word) * font.width, font.height, bg_color);
		} else {
			fillRect(x0, i * (font.height + 2) + y0 - currentPage * max_set_in_page * (font.height + 2), x1 - x0, font.height, bg_color);
		}
	}

}

void drawIcon(const char* header_t, bool highlight,
		uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
		FontDef font, uint16_t txt_color, uint16_t bg_color) {
	static bool prev_highlight = true;
	const uint8_t max_ch_in_str = (x1 - x0 - 4) / font.width;
	char header[max_ch_in_str + 1];
	memset(header, 0, (max_ch_in_str + 1)*sizeof(char));
	strncpy(header, header_t, max_ch_in_str);
	uint16_t len = strlen(header);

	if (prev_highlight != highlight) {
		fillRect(x0 + 2, y0 + 2, x1 - x0 - 4, font.height + 3, bg_color);
		drawRoundRect(x0, y0, x1 - x0, y1 - y0, 5, bg_color);
		drawRoundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 3, bg_color);
	}

	if (highlight) {
		fillRect(x0 + 2, y0 + 2, x1 - x0 - 4, font.height + 3, txt_color);
		drawRoundRect(x0, y0, x1 - x0, y1 - y0, 5, txt_color);
		drawRoundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 3, txt_color);
		ST7735_WriteString(x0 + (x1 - x0 - 4 - len * font.width) / 2 + 2, y0 + 4, header, font, bg_color, txt_color);
	} else {
		drawRoundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 5, txt_color);
		drawFastHLine(x0 + 1, y0 + 2 + font.height + 2, x1 - x0 - 2, txt_color);
		ST7735_WriteString(x0 + (x1 - x0 - 4 - len * font.width) / 2 + 2, y0 + 4, header, font, txt_color, bg_color);
	}

	prev_highlight = highlight;
}

void drawErrorIcon(char *error_text, int error_len, uint16_t txt_color, uint16_t bg_color) {
	static const uint8_t max_str_count = 4;
	static const uint8_t max_ch_in_str = 15;
	char buffer[16] = {};

	fillRoundRect(3, 38, 122, 84, 5, bg_color);
	drawRoundRect(5, 40, 118, 80, 5, txt_color);
	fillTriangle(16, 66, 29, 44, 42, 66, txt_color);
	drawFastVLine(29, 50, 10, bg_color);
	drawFastVLine(30, 51, 7, bg_color);
	drawFastVLine(28, 51, 7, bg_color);
	fillRoundRect(28, 62, 3, 3, 1, bg_color);
	ST7735_WriteString(51, 47, "ERROR", Font_11x18, txt_color, bg_color);
	drawFastHLine(5, 70, 118, txt_color);

	uint16_t len = 0;
	uint16_t str_count = 0;
	while (error_len > 0 && str_count < max_str_count) {
		if (error_len > max_ch_in_str)
			len = max_ch_in_str;
		else
			len = error_len;
		memcpy(buffer, error_text + str_count * max_ch_in_str, len);
		buffer[len] = '\0';
		ST7735_WriteString(10, 73 + str_count * 12, buffer, Font_7x10, txt_color, bg_color);
		error_len -= max_ch_in_str;
		str_count++;
	}
}

void darwMiddleButton(char *button_text, uint16_t y0, FontDef font, bool highlight, uint16_t txt_color, uint16_t bg_color) {
	static bool prev_highlight = true;
	const uint16_t max_char_count = (ST7735_WIDTH / font.width) - 2;
	char crop_button_text[max_char_count + 1];
	uint16_t button_text_len = strlen(button_text);
	if (button_text_len > max_char_count)
		button_text_len = max_char_count;
	memcpy(crop_button_text, button_text, button_text_len);
	crop_button_text[button_text_len] = '\0';
	uint16_t button_len = (button_text_len + 2) * font.width;
	uint16_t button_X_start = (ST7735_WIDTH - button_len) / 2;

	if (prev_highlight != highlight)
		fillRoundRect(button_X_start, y0, button_len, 4 + font.height, font.width, bg_color);

	if (highlight) {
		fillRoundRect(button_X_start, y0, button_len, 4 + font.height, font.width, txt_color);
		ST7735_WriteString(button_X_start + font.width, y0 + 2, crop_button_text, font, bg_color, txt_color);
	} else {
		drawRoundRect(button_X_start, y0, button_len, 4 + font.height, font.width, txt_color);
		ST7735_WriteString(button_X_start + font.width, y0 + 2, crop_button_text, font, txt_color, bg_color);
	}

	prev_highlight = highlight;
}

/*/______________________________________________________________________________________/*/
