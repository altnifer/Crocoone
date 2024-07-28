/*
 * title.h
 *
 *  Created on: Jun 29, 2024
 *      Author: nikit
 */
#include "title.h"
#include "main.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include <string.h>

#define POWER_SUPPLY_VOLTAGE		3.269f
#define ADC_MAX						0xFFFF
#define DIVIDER_R1 					10.04f
#define DIVIDER_R2 					18.05f
#define MIN_BATTERY_VOLTAGE 		3.0f
#define MAX_BATTERY_VOLTAGE 		4.2f

static bool SD_flag;
static int batteryPercent = 0;
static char battery_persent_disp[10];
static char title[MAX_CHAR_IN_TITLE + 1];
static bool refresh_title_text = false;


void draw_title_task(void *args);


void start_title_updater(SemaphoreHandle_t *LCD_mutex) {
	SD_flag = false;
	xTaskCreate(draw_title_task, "title_updater", 1024 * 4, LCD_mutex, osPriorityLow, NULL);
}

void draw_title_task(void *args) {
	SemaphoreHandle_t *LCD_mutex = (SemaphoreHandle_t *)args;
	float batteryVoltage = 0;
	uint16_t adcData = 0;

	while (1) {
		HAL_GPIO_WritePin(DIV_ENABLE_GPIO_Port, DIV_ENABLE_Pin, GPIO_PIN_RESET); //div enable

		HAL_ADC_Start(&hadc1);

		HAL_ADC_PollForConversion(&hadc1, 100);

		adcData = HAL_ADC_GetValue(&hadc1);

		HAL_ADC_Stop(&hadc1);

		HAL_GPIO_WritePin(DIV_ENABLE_GPIO_Port, DIV_ENABLE_Pin, GPIO_PIN_SET);

		batteryVoltage = POWER_SUPPLY_VOLTAGE * adcData / ADC_MAX;
		batteryVoltage = batteryVoltage / (DIVIDER_R2 / (DIVIDER_R1 + DIVIDER_R2));
		batteryPercent = (int)(((batteryVoltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE  - MIN_BATTERY_VOLTAGE)) * 100.0f);
		if (batteryPercent < 0)
		  batteryPercent = 0;
		else if (batteryPercent > 100)
		  batteryPercent = 100;

		xSemaphoreTake(*LCD_mutex, portMAX_DELAY);
		refresh_title();
		xSemaphoreGive(*LCD_mutex);

		osDelay(5000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

void refresh_title() {
	if (refresh_title_text) {
		fillRect(1, 1, MAX_CHAR_IN_TITLE * 7, 10, MAIN_BG_COLOR);
		ST7735_WriteString(1, 1, title, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		refresh_title_text = false;
	}
	sprintf(battery_persent_disp, "%3d%%", batteryPercent);
	ST7735_WriteString(99, 1, battery_persent_disp, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	if (SD_flag)
		ST7735_WriteString(87, 1, "SD", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	else
		ST7735_WriteString(87, 1, "  ", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
}

void update_title_text(char * new_title) {
	uint16_t new_title_len = strlen(new_title);
	new_title_len = (new_title_len > MAX_CHAR_IN_TITLE) ? (MAX_CHAR_IN_TITLE) : (new_title_len);

	memcpy(title, new_title, new_title_len);
	title[new_title_len] = '\0';
	refresh_title_text = true;
}

void update_title_SD_flag(bool flag) {
	SD_flag = flag;
}

void fillScreenExceptTitle() {
	fillRect(0, TITLE_END_Y + 1, ST7735_WIDTH, ST7735_HEIGHT - TITLE_END_Y, MAIN_BG_COLOR);
}
