/*
 * ch_analyzer.c
 *
 *  Created on: Jun 16, 2024
 *      Author: nikit
 */
#include "ch_analyzer.h"
#include "SETTINGS.h"
#include "button.h"
#include "GFX_FUNCTIONS.h"
#include <string.h>
#include <stdio.h>

#define CHANNELS_COUNT 128
#define NUW_REPS 100

void ch_analyzer_main_task(void *main_task_handle);
void draw_graph(uint8_t values[24][CHANNELS_COUNT], uint8_t max_values[CHANNELS_COUNT], bool full_update);


void nrf_init() {
	DWT_Init();
	NRF_Init();
}

void NRF24_ch_analyzer(TaskHandle_t *main_task_handle) {
	xTaskCreate(ch_analyzer_main_task, "ch_analyzer", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void ch_analyzer_main_task(void *main_task_handle) {
	powerUp();
	setAutoAck(false);
	static uint8_t values[24][CHANNELS_COUNT] = {};
	static uint8_t max_values[CHANNELS_COUNT] = {};

	ClearAllButtons();

	fillScreen(MAIN_BG_COLOR);
	draw_graph(values, max_values, true);

	while(1) {
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;

		for (int i = 0; i < CHANNELS_COUNT; i++) {
			values[0][i] = (values[0][i] < 16) ? values[0][i] : 15;
			if (max_values[i] < values[0][i]) max_values[i] = values[0][i];
		}

		draw_graph(values, max_values, false);

		int rep_counter = NUW_REPS;
		uint8_t ScanedValues[CHANNELS_COUNT] = {};
		while (rep_counter--) {
			int i = CHANNELS_COUNT;
			while (i--) {
			  if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) {
					powerDown();

					vTaskResume(*((TaskHandle_t *)main_task_handle));
					vTaskDelete(NULL);
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

		osDelay(250 / portTICK_PERIOD_MS);
	}
	powerDown();

	vTaskResume(*((TaskHandle_t *)main_task_handle));
	vTaskDelete(NULL);
}

void draw_graph(uint8_t values[24][CHANNELS_COUNT], uint8_t max_values[CHANNELS_COUNT], bool full_update) {
	static const int GRAPH_Y = 10;
	static const int GRAPH_SCALE = 3;
	static char palette_red[16], palette_green[16], palette_blue[16];
	static bool pallete_init = false;

	if (!pallete_init) {
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
		pallete_init = true;
	}

	if (full_update) {
		drawFastHLine(0, GRAPH_Y, CHANNELS_COUNT, MAIN_TXT_COLOR);
		drawFastHLine(0, GRAPH_Y + GRAPH_SCALE * 16 + 1, CHANNELS_COUNT, MAIN_TXT_COLOR);
		drawFastHLine(0, GRAPH_Y + GRAPH_SCALE * 16 + 24 * 2 + 2, CHANNELS_COUNT, MAIN_TXT_COLOR);
		ST7735_WriteString(0, GRAPH_Y + GRAPH_SCALE * 16 + 24 * 2 + 3 + 7, "0", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(112, GRAPH_Y + GRAPH_SCALE * 16 + 24 * 2 + 3 + 7, "120", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		for (int i = 0; i <= 120; i += 20) {
		  drawFastVLine(i, GRAPH_Y + GRAPH_SCALE * 16 + 24 * 2 + 3, 5, MAIN_TXT_COLOR);
		  if (i != 0 && i != 120) {
			  char str[4] = {};
			  sprintf(str, "%d", i);
			  ST7735_WriteString(i - strlen(str) * 2 - 2, GRAPH_Y + GRAPH_SCALE * 16 + 24 * 2 + 3 + 7, str, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		  }
		}
	}

	for (int i = 0; i < CHANNELS_COUNT; i++) {
		drawFastVLine(i, GRAPH_Y + 1, 16 * GRAPH_SCALE, MAIN_BG_COLOR);
		if (max_values[i] > 0) drawFastVLine(i, GRAPH_Y + 1 + (16 - max_values[i]) * GRAPH_SCALE, max_values[i] * GRAPH_SCALE, MAIN_ERROR_COLOR);
		if (values[0][i] > 0 && max_values[i] > 0) drawFastVLine(i, GRAPH_Y + 1 + (16 - values[0][i]) * GRAPH_SCALE, values[0][i] * GRAPH_SCALE, YELLOW);
	}

	for (int j = 0; j < 24; j++) {
		for (int i = 0; i < CHANNELS_COUNT; i++) {
		  unsigned int color = palette_red[values[j][i]] << 11 | palette_green[values[j][i]] << 5 | palette_blue[values[j][i]];
		  drawFastVLine(i, GRAPH_Y + 16 * GRAPH_SCALE + 1 + j*2 + 1, 2, color);
		}
	}
}
