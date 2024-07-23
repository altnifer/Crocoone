/*
 * PMKID_attack.c
 *
 *  Created on: Jun 26, 2024
 *      Author: nikit
 */
#include "PMKID_attack.h"
#include "SETTINGS.h"
#include "GFX_FUNCTIONS.h"
#include "button.h"
#include "uart_controller.h"
#include "eeprom.h"
#include "SD_card.h"
#include "string.h"
#include "stdio.h"
#include "title.h"

static char current_ssid[33];
static uint16_t captured_pmkid_count = 0;
static bool SD_write_error;
static bool info_update;
static bool pmkid_task_flag;
extern SemaphoreHandle_t SPI2_mutex;

void pmkid_attack_parser_task(void * arg);
void pmkid_attack_main_task(void *main_task_handle);
bool pmkid_attack_setup(char *error_buff);
void pmkid_attack_unsetup();
static void darw_running_page(char * ssid_name, uint16_t pmkid_count, bool full_refresh);
static void draw_restart_page(uint16_t pmkid_count);


void ESP32_start_pmkid_attack(TaskHandle_t *main_task_handle) {
	xTaskCreate(pmkid_attack_main_task, "pmkid_attack", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void pmkid_attack_main_task(void *main_task_handle) {
	char error_buff[100];
	bool exit_with_error = true;
	bool full_screen_update = true;
	bool run_page = true;
	bool button_flag = false;
	bool pmkid_setup = true;
	bool pmkid_unsetup = false;

	SD_write_error = false;
	info_update = true;

	ClearAllButtons();
	update_title_text("pmkid");

	while (1) {
		if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK) {
			button_flag = true;
			run_page = !run_page;
			full_screen_update = true;
			pmkid_setup = run_page;
			pmkid_unsetup = !run_page;
		}

		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) {
			exit_with_error = false;
			break;
		}


		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		if (button_flag && run_page) {
			darwMiddleButton("restart", 130, Font_7x10, true, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			osDelay(5 / portTICK_PERIOD_MS);
		}
		if (button_flag && !run_page) {
			darwMiddleButton("stop", 130, Font_7x10, true, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			osDelay(5 / portTICK_PERIOD_MS);
		}
		if (full_screen_update) fillScreenExceptTitle();
		if (run_page && (info_update || full_screen_update)) {
			darw_running_page(current_ssid, captured_pmkid_count, full_screen_update);
			if (button_flag || full_screen_update)
				darwMiddleButton("stop", 130, Font_7x10, false, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
		if (!run_page && full_screen_update) {
			draw_restart_page(captured_pmkid_count);
			darwMiddleButton("restart", 130, Font_7x10, false, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
		info_update = false;
		full_screen_update = false;
		button_flag = false;
		xSemaphoreGive(SPI2_mutex);


		if (pmkid_setup) {
			if (!pmkid_attack_setup(error_buff)) break;
			pmkid_setup = false;
		}

		if (pmkid_unsetup) {
			pmkid_attack_unsetup();
			pmkid_unsetup = false;
		}

		if (SD_write_error) {
			sprintf(error_buff, "SD write error");
			break;
		}

		osDelay(250 / portTICK_PERIOD_MS);
	}

	update_title_SD_flag(false);
	pmkid_attack_unsetup();

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


void pmkid_attack_parser_task(void * arg) {
	ring_buffer_t *uart_ring_buffer = get_ring_buff();
	UART_ringBuffer_mutex_take();
	ringBuffer_clear(uart_ring_buffer);
	UART_ringBuffer_mutex_give();

	while (pmkid_task_flag) {
		osDelay(50 / portTICK_PERIOD_MS);

		UART_ringBuffer_mutex_take();
		for (uint16_t i = 0; i < get_recieved_pkts_count(); i++) {

		    int32_t SSID_packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)SSID_PACKET_HEADER, SSID_PACKET_HEADER_LEN);
		    int32_t PMKID_packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)PMKID_PACKET_HEADER, PMKID_PACKET_HEADER_LEN);

		    if (SSID_packet_start == -1 && PMKID_packet_start == -1) {
		    	ringBuffer_clear(uart_ring_buffer);
		    	break;
		    }

		    if (((SSID_packet_start < PMKID_packet_start) || (PMKID_packet_start == -1)) && SSID_packet_start != -1) {
		    	ringBuffer_clearNBytes(uart_ring_buffer, SSID_packet_start + SSID_PACKET_HEADER_LEN);
				int32_t ssid_len = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"\n", 1);

				if (ssid_len != -1) {
					ringBuffer_get(uart_ring_buffer, (uint8_t *)current_ssid, ssid_len);
					current_ssid[ssid_len] = '\0';
				}

				info_update = true;
		    } else if (PMKID_packet_start != -1) {
		    	ringBuffer_clearNBytes(uart_ring_buffer, PMKID_packet_start + PMKID_PACKET_HEADER_LEN);
		    	int32_t pmkid_len = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"\n", 1);

		    	if (pmkid_len != -1) {
					copy_ringBuff_to_SD_buff(uart_ring_buffer, pmkid_len + 1);

					captured_pmkid_count++;
					info_update = true;
		    	}
		    }

		}
		UART_ringBuffer_mutex_give();

		if ((SD_write_error = !SD_write_from_ringBuff()) == true)
			break;
	}

	if (!SD_write_error)
		SD_write_error = !SD_end_write_from_ringBuff();
	SD_unsetup_write_from_ringBuff();

	vTaskDelete(NULL);
}

bool pmkid_attack_setup(char *error_buff) {
	FRESULT sd_setup_error_code;
	uint32_t file_num;
	char file_name[strlen(PMKID_FILE_NAME) + 21];

	eeprom_read_set(EEPROM_PMKID_ADDR, &file_num);

	sprintf(file_name, "%s_%lu.hc22000", PMKID_FILE_NAME, file_num);

	sd_setup_error_code = SD_setup_write_from_ringBuff(PMKID_FOLDER_NAME, file_name, 7);
	if (sd_setup_error_code != FR_OK) {
		sprintf(error_buff, "SD setup error with code %d", sd_setup_error_code);
		return false;
	}

	if (!send_cmd_with_check((cmd_data_t){PMKID_CMD, {0,0,0,0,0}}, error_buff, 2000)) {
		SD_unsetup_write_from_ringBuff();
		return false;
	}

	file_num++;
	eeprom_write_set(EEPROM_PMKID_ADDR, &file_num);

	memset(current_ssid, 0, 33);
	captured_pmkid_count = 0;
	pmkid_task_flag = true;
	xTaskCreate(pmkid_attack_parser_task, "pmkid_parser", 1024 * 4, NULL, osPriorityNormal2, NULL);

	return true;
}

void pmkid_attack_unsetup() {
	pmkid_task_flag = false;
	send_cmd_without_check((cmd_data_t){STOP_CMD, {0,0,0,0,0}});
}

static void darw_running_page(char * ssid_name, uint16_t pmkid_count, bool full_refresh) {
	const uint16_t YStart = 20;
	const uint16_t MAX_CH_IN_STR = 13;
	char str[20] = {};

	if (strlen(ssid_name) > 0) {
		ST7735_WriteString(7, YStart + 58, "                ", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
		if (strlen(ssid_name) > MAX_CH_IN_STR) {
			memcpy(str, ssid_name, MAX_CH_IN_STR);
			memcpy(str + MAX_CH_IN_STR, "...\0", 4);
			ST7735_WriteString(7, YStart + 58, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		} else {
			ST7735_WriteString(7, YStart + 58, ssid_name, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
	}

	sprintf(str, "%u", pmkid_count);
	ST7735_WriteString(7, YStart + 84, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	if (!full_refresh) return;

	drawIcon("attack info", true, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 20, "Status:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 32, "Running...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 46, "Current SSID:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 72, "PMKID captured:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	update_title_SD_flag(true);
	refresh_title();
}

static void draw_restart_page(uint16_t pmkid_count) {
	const uint16_t YStart = 20;
	char str[20] = {};

	drawIcon("attack info", true, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	ST7735_WriteString(7, YStart + 20, "Status:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 32, "Stopped", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 46, "Total PMKID", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(7, YStart + 57, "captured:", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(str, "%u", pmkid_count);
	ST7735_WriteString(7, YStart + 69, str, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	update_title_SD_flag(false);
	refresh_title();
}
