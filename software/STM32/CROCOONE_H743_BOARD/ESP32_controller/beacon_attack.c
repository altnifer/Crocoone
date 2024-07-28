/*
 * beacon_attack.c
 *
 *  Created on: Jul 10, 2024
 *      Author: nikit
 */
#include "beacon_attack.h"
#include "uart_controller.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include "button.h"
#include "title.h"
#include <stdio.h>
#include <string.h>

extern SemaphoreHandle_t SPI2_mutex;

void beacon_attack_main_task(void *main_task_handle);
static void darw_settings_page(uint8_t timeout, bool button_state, bool full_refresh);
static void darw_running_page(uint16_t seconds_left, uint16_t timeout, bool button_state, bool full_refresh);

void ESP32_start_beacon_attack(TaskHandle_t *main_task_handle) {
	xTaskCreate(beacon_attack_main_task, "beacon_attack", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void beacon_attack_main_task(void *main_task_handle) {
	static uint8_t timeout = 0;
	uint8_t time_elapsed;
	char error_buff[100];
	bool exit_with_error = true;
	bool full_update_screen = true;
	bool button = false;
	bool button_confirm_flag = false;
	bool running_page = false;
	bool use_timer = false;
	eButtonEvent button_up, button_down, button_left, button_right, button_confirm;
	uint32_t ticks = 0;

	ClearAllButtons();
	update_title_text("beacon");

	while (1) {
	//drawing on display
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		if (full_update_screen) {
			refresh_title();
			fillScreenExceptTitle();
		}
		if (!running_page)
			darw_settings_page(timeout, button, full_update_screen);
		else
			darw_running_page(time_elapsed, timeout, button, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);
	//

	//buttons handling
		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_right == DOUBLE_CLICK) {
			exit_with_error = false;
			break; //exit
		}
		if (button_down == DOUBLE_CLICK || button_up == DOUBLE_CLICK) button = !button;
		if (button_right == SINGLE_CLICK || button_right == HOLD) timeout++;
		if (button_left == SINGLE_CLICK || button_left == HOLD) timeout--;
		if (button_confirm == SINGLE_CLICK && button) button_confirm_flag = true;
	//


		if (!running_page && button_confirm_flag) {
			if (!send_cmd_with_check((cmd_data_t){BEACON_CMD, {timeout, 0, 0, 0, 0}}, error_buff, 2000))
				break;
			if (timeout) {
				time_elapsed = timeout;
				ticks = HAL_GetTick();
				use_timer = true;
			}
			button = false;
			running_page = true;
			full_update_screen = true;
			button_confirm_flag = false;
		}

		if (running_page && button_confirm_flag) {
			if (!send_cmd_with_check((cmd_data_t){STOP_CMD, {0, 0, 0, 0, 0}}, error_buff, 2000))
				break;
			button = false;
			use_timer = false;
			running_page = false;
			full_update_screen = true;
			button_confirm_flag = false;
		}

		if (use_timer && HAL_GetTick() - ticks > 1000) {
			time_elapsed--;
			if (!time_elapsed) {
				running_page = false;
				use_timer = false;
				full_update_screen = true;
			}
			ticks = HAL_GetTick();
		}

		osDelay(100 / portTICK_PERIOD_MS);
	}

	send_cmd_without_check((cmd_data_t){STOP_CMD, {}});

	if (exit_with_error) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		drawErrorIcon(error_buff, strlen(error_buff), MAIN_ERROR_COLOR, MAIN_BG_COLOR);
		xSemaphoreGive(SPI2_mutex);

		ClearAllButtons();
		while (1) {
			osDelay(250);
			if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) break;
		}
	}

	vTaskResume(*((TaskHandle_t *)main_task_handle));
	vTaskDelete(NULL);
}


static void darw_settings_page(uint8_t timeout, bool button_state, bool full_refresh) {
	const uint16_t YStart = TITLE_END_Y + 9;
	char str[20] = {};
	static uint8_t prev_timeout_len = 0;
	static uint8_t prev_timeout = 0;
	static bool prev_button_state = false;

	if (full_refresh || prev_button_state != button_state) {
		drawIcon("parameters", !button_state, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("start", YStart + 110, Font_7x10, button_state, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (full_refresh || prev_button_state != button_state || prev_timeout != timeout) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(" Timeout ") * 7) / 2,
				YStart + 20,
				(!button_state) ? (">Timeout<") : (" Timeout "),
				Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
		);
		if (!timeout)
			memcpy(str, "none", 5);
		else
			sprintf(str, "%hu", timeout);
		uint8_t timeout_len = strlen(str);
		if (prev_timeout_len != timeout_len)
			fillRect(46, YStart + 34, 35, 10, MAIN_BG_COLOR);
		prev_timeout_len = timeout_len;
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(str) * 7) / 2,
				YStart + 34,
				str,
				Font_7x10,
				(!button_state) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(!button_state) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
		);
	}

	prev_button_state = button_state;
	prev_timeout = timeout;
}

static void darw_running_page(uint16_t seconds_left, uint16_t timeout, bool button_state, bool full_refresh) {
	const uint16_t YStart = 20;
	char str[20] = {};
	static uint8_t prev_seconds_left = 0;
	static bool prev_button_state = false;
	static uint8_t prev_timeout_len = 0;

	if (full_refresh || prev_button_state != button_state) {
		drawIcon("timeout", !button_state, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("stop", YStart + 110, Font_7x10, button_state, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (full_refresh)
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("Seconds elapsed") * 7) / 2,
				YStart + 30,
				"Seconds elapsed",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);

	if (full_refresh || prev_seconds_left != seconds_left) {
		if (!timeout)
			memcpy(str, "none", 5);
		else
			sprintf(str, "%hu/%hu", seconds_left, timeout);
		uint8_t timeout_len = strlen(str);
		if (prev_timeout_len != timeout_len)
			fillRect(10, YStart + 44, 108, 10, MAIN_BG_COLOR);
		prev_timeout_len = timeout_len;
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(str) * 7) / 2,
				YStart + 44,
				str,
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	}

	prev_button_state = button_state;
	prev_seconds_left = seconds_left;
}
