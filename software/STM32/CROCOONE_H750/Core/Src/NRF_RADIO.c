/*
 * HRF_RADIO.c
 *
 *  Created on: Nov 5, 2023
 *      Author: nikit
 */

#include "NRF_RADIO.h"

void Radio_Init() {
	DWT_Init();
	NRF_Init();
}

void NRF_ChannelAnalyzer_loop() {
	powerUp();
	setAutoAck(false);
	static uint8_t values[24][CHANNELS_COUNT] = {};
	static uint8_t max_values[CHANNELS_COUNT] = {};

	static const int GRAPH_X = 15;
	static const int GRAPH_Y = 10;
	static const int GRAPH_SCALE = 3;

	static const int palette_len = 16;
	char palette_red[palette_len], palette_green[palette_len], palette_blue[palette_len];

	for (int i = 0; i < 4; i++) {
		palette_red[i] = i * 4;
		palette_green[i] = 0;
		palette_blue[i] = i * 8;
	}
	for (int i = 4; i < 8; i++) {
		palette_red[i] = i * 4;
		palette_green[i] = 0;
		palette_blue[i] = 31 - i * 8;
	}
	for (int i = 8; i < 12; i++) {
		palette_red[i] = 31;
		palette_green[i] = (i * 8 - 64) * 2;
		palette_blue[i] = 0;
	}
	for (int i = 12; i < 16; i++) {
		palette_red[i] = 31;
		palette_green[i] = 63;
		palette_blue[i] = i * 8 - 96;
	}

	ClearAllButtons();

	fillScreen(MAIN_BG_COLOR);
	drawRect(GRAPH_X, GRAPH_Y, CHANNELS_COUNT + 2, 99, MAIN_TXT_COLOR);
	drawFastHLine(GRAPH_X + 1, GRAPH_Y + 49, CHANNELS_COUNT, MAIN_TXT_COLOR);

	for (int i = 0; i < 65; i++) {
	  if (i % 10) {
		  drawFastVLine(GRAPH_X + i * 2 + 1, GRAPH_Y + 99, 3, MAIN_TXT_COLOR);
	  } else {
		  drawFastVLine(GRAPH_X + i * 2 + 1, GRAPH_Y + 99, 5, MAIN_TXT_COLOR);
		  char str[4] = {};
		  sprintf(str, "%d", i*2);
		  ST7735_WriteString(GRAPH_X + i * 2 - strlen(str) * 2, GRAPH_Y + 99 + 7, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	  }
	}

	while(1) {
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;

		for (int i = 0; i < CHANNELS_COUNT; i++) {
			values[0][i] = (values[0][i] < 16) ? values[0][i] : 15;
			if (max_values[i] < values[0][i]) max_values[i] = values[0][i];
			drawFastVLine(GRAPH_X + i + 1, GRAPH_Y + 1, 16 * GRAPH_SCALE, MAIN_BG_COLOR);
			if (max_values[i] > 0) drawFastVLine(GRAPH_X + i + 1, GRAPH_Y + 1 + (16 - max_values[i]) * GRAPH_SCALE, max_values[i] * GRAPH_SCALE, MAIN_ERROR_COLOR);
			if (values[0][i] > 0 && max_values[i] > 0) drawFastVLine(GRAPH_X + i + 1, GRAPH_Y + 1 + (16 - values[0][i]) * GRAPH_SCALE, values[0][i] * GRAPH_SCALE, YELLOW);
		}

		for (int j = 0; j < 24; j++) {
			for (int i = 0; i < CHANNELS_COUNT; i++) {
			  unsigned int color = palette_red[values[j][i]] << 11 | palette_green[values[j][i]] << 5 | palette_blue[values[j][i]];
			  drawFastVLine(GRAPH_X + i + 1, GRAPH_Y + 16 * GRAPH_SCALE + 1 + j*2 + 1, 2, color);
			}
		}

		int rep_counter = NUW_REPS;
		uint8_t ScanedValues[CHANNELS_COUNT] = {};
		while (rep_counter--) {
			int i = CHANNELS_COUNT;
			while (i--) {
			  if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) {
				  ClearAllButtons();
				  powerDown();
				  return;
			  }
			  setChannel(i);
			  startListening();
			  delay_us(128);
			  stopListening();
			  if ( testCarrier() )
				  ++ScanedValues[i];
			}
		}

		for (int i = 23; i > -1; i--) {
		  memcpy(values[i], values[i-1], sizeof(values[i]));
		}

		memcpy(values[0], ScanedValues, sizeof(ScanedValues));
	}
	ClearAllButtons();
	powerDown();
}

void NRF_Jammer() {
	static const int GRAPH_Y = 44;
	int currentCh = 0;
	int ch = 0;
	char str[25] = {};
	bool screenUpdate = 0;
	bool jammerStart = 0;
	const char text = 'x'; //random char
	powerUp();
	setAutoAck(false);
	setPALevel(RF24_PA_MAX);
	setDataRate(RF24_2MBPS);
	stopListening();

	ClearAllButtons();
	eButtonEvent button_right, button_left, button_confirm;
	uint32_t hold_timer = 0;

	fillScreen(MAIN_BG_COLOR);
	ST7735_WriteString(0, 1, "current channel: 000", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	drawFastHLine(16, GRAPH_Y + 16 + 7, 129, MAIN_TXT_COLOR);
	fillTriangle(16, GRAPH_Y + 16 + 6, 13, GRAPH_Y + 16, 19, GRAPH_Y + 16, 	MAIN_TXT_COLOR);
	fillTriangle(16 + 128, GRAPH_Y + 16 + 6, 13 + 128, GRAPH_Y + 16, 19 + 128, GRAPH_Y + 16, MAIN_TXT_COLOR);
	fillTriangle(16 + ch, GRAPH_Y + 16 + 8, 13 + ch, GRAPH_Y + 16 + 14, 19 + ch, GRAPH_Y + 16 + 14, MAIN_TXT_COLOR);
	ST7735_WriteString(0, GRAPH_Y, "2400GHz", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(0, GRAPH_Y + 8, "000 Ch", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(123, GRAPH_Y, "2528GHz", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(123, GRAPH_Y + 8, "128 Ch", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(ch, GRAPH_Y + 16 + 16, "2400GHz", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(ch, GRAPH_Y + 8 + 16 + 16, "000 Ch", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	while (1) {
		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_right == SINGLE_CLICK) {
			ch = (ch + 1) % (CHANNELS_COUNT);
			screenUpdate = 1;
		} else if (button_right == HOLD && HAL_GetTick() - hold_timer > HOLD_FAST_DELLAY) {
			ch = (ch + 5) % (CHANNELS_COUNT);
			screenUpdate = 1;

			hold_timer = HAL_GetTick();
		}

		if (button_left == SINGLE_CLICK) {
			ch = (ch - 1) % (CHANNELS_COUNT);
			if (ch < 0) ch = CHANNELS_COUNT - 1;
			screenUpdate = 1;
		} else if (button_left == HOLD && HAL_GetTick() - hold_timer > HOLD_FAST_DELLAY) {
			ch = (ch - 5) % (CHANNELS_COUNT);
			if (ch < 0) ch = CHANNELS_COUNT - 1;
			screenUpdate = 1;

			hold_timer = HAL_GetTick();
		}

		if (button_confirm == SINGLE_CLICK) {
			currentCh = ch;
			setChannel(currentCh);
			jammerStart = 1;
			screenUpdate = 1;
		} else if (button_confirm == DOUBLE_CLICK) {
			jammerStart = 0;
			screenUpdate = 1;
		}

		if (jammerStart) {
			writeFast(&text, sizeof(text));
		}

		if (screenUpdate) {
			fillRect(0, GRAPH_Y + 16 + 8, 160, 30, MAIN_BG_COLOR);
			if (jammerStart) {
				fillTriangle(16 + currentCh, GRAPH_Y + 16 + 8, 13 + currentCh, GRAPH_Y + 16 + 14, 19 + currentCh, GRAPH_Y + 16 + 14, MAIN_ERROR_COLOR);
				sprintf(str, "current channel: %03d", currentCh);
				ST7735_WriteString(0, 1, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			    ST7735_WriteString(21*5, 1, "JAMMING...", Font_5x7, MAIN_ERROR_COLOR, MAIN_BG_COLOR);
			} else {
				ST7735_WriteString(21*5, 1, "CLEARCLEAR", Font_5x7, MAIN_BG_COLOR, MAIN_BG_COLOR);
			}
			fillTriangle(16 + ch, GRAPH_Y + 16 + 8, 13 + ch, GRAPH_Y + 16 + 14, 19 + ch, GRAPH_Y + 16 + 14, MAIN_TXT_COLOR);
			sprintf(str, "%dGHz", 2400 + ch);
			ST7735_WriteString(ch, GRAPH_Y + 16 + 16, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			sprintf(str, "%03d Ch", ch);
			ST7735_WriteString(ch, GRAPH_Y + 8 + 16 + 16, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			screenUpdate = 0;
		}

		if (button_right == DOUBLE_CLICK) {
			break;
		}
	}
	ClearAllButtons();
	powerDown();
}
