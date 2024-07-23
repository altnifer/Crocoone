/*
 * handshake_attack.c
 *
 *  Created on: Jul 11, 2024
 *      Author: nikit
 */
#include "handshake_attack.h"
#include "pcap_parser.h"
#include "SD_card.h"
#include "deauth_attack.h"
#include "uart_controller.h"
#include "wifi_scan.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include "button.h"
#include "title.h"
#include "eeprom.h"
#include <stdio.h>
#include <string.h>

typedef enum {
	SAVE_ERROR,
	SAVE_OK,
	NO_DATA
} data_save_res_t;

typedef enum {
	NOT_READY,
	PREPARATION,
	RUNNING,
	SAVING_DATA,
    ERROR_HANDLER,
	EXIT
} attack_state_t;

typedef enum {
	HCCAPX,
	HC22000
} handshake_data_t;

extern SemaphoreHandle_t SPI2_mutex;

static bool save_pcap = false;
static uint8_t method = 0;
static uint8_t timeout = 0;
uint32_t eapol_sum = 0;

void handshake_attack_main_task(void *main_task_handle);
bool handshake_attack_start(uint8_t timeout, uint8_t method, bool save_pcap, char *error_buff);
bool handshake_attack_stop(char *error_buff);
data_save_res_t handshake_attack_get_data(handshake_data_t data_t, char *error_buff);
attack_state_t handshake_prepare_page(char* error_buff);
attack_state_t handshake_not_ready_page(char* error_buff);
attack_state_t handshake_running_page(char* error_buff);
attack_state_t handshake_saving_data_page(char* error_buff);
static void draw_no_target_page();
static void darw_settings_page(uint8_t set, uint8_t timeout, uint8_t method, bool save_pcap, bool button, bool full_refresh);
static void darw_running_page(uint8_t seconds_left, uint8_t timeout, uint32_t eapol_sum, bool button, bool full_refresh);
void draw_saving_data_page(uint32_t eapol_sum, bool saving, bool save_ok, bool button, bool full_refresh);

void ESP32_start_handshake_attack(TaskHandle_t *main_task_handle) {
	xTaskCreate(handshake_attack_main_task, "handshake_attack", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void handshake_attack_main_task(void *main_task_handle) {
	char error_buff[100];
	attack_state_t attack_state = (get_target() == -1) ? NOT_READY : PREPARATION;

	update_title_text("handshake");

	while (1) {
		if (attack_state == NOT_READY) {
			attack_state = handshake_not_ready_page(error_buff);
		} else if (attack_state == PREPARATION) {
			attack_state = handshake_prepare_page(error_buff);
		} else if (attack_state == RUNNING) {
			attack_state = handshake_running_page(error_buff);
		} else if (attack_state == SAVING_DATA) {
			attack_state = handshake_saving_data_page(error_buff);
		} else if (attack_state == ERROR_HANDLER || attack_state == EXIT) {
			break;
		}
		update_title_SD_flag(false);
	}

	send_cmd_without_check((cmd_data_t){STOP_CMD, {0,0,0,0,0}});
	pcap_parser_stop();
	update_title_SD_flag(false);

	if (attack_state == ERROR_HANDLER) {
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


bool handshake_attack_start(uint8_t timeout, uint8_t method, bool save_pcap, char *error_buff) {
	FRESULT sd_setup_error_code;
	uint32_t file_num;
	char file_name[strlen(HANDSHAKE_PCAP_FILE_NAME) + 18];

	if (save_pcap) {
		eeprom_read_set(EEPROM_HANDSHAKE_PCAP_ADDR, &file_num);
		sprintf(file_name, "%s_%lu.pcap", HANDSHAKE_PCAP_FILE_NAME, file_num);
		sd_setup_error_code = SD_setup_write_from_ringBuff(HANDSHAKE_FOLDER_NAME, file_name, 12);
		if (sd_setup_error_code != FR_OK) {
			sprintf(error_buff, "SD setup error with code %d", sd_setup_error_code);
			return false;
		}
		file_num++;
		eeprom_write_set(EEPROM_HANDSHAKE_PCAP_ADDR, &file_num);
		pcap_parser_set_use_sd_flag(true);
	}

	if (!send_cmd_with_check((cmd_data_t){HANDSHAKE_CMD, {get_target(),method,true,timeout,0}}, error_buff, 2000)) {
		if (save_pcap) SD_unsetup_write_from_ringBuff();
		return false;
	}

	pcap_parser_start();

	return true;
}

bool handshake_attack_stop(char *error_buff) {
	pcap_parser_stop();
	if (!send_cmd_with_check((cmd_data_t){STOP_CMD, {0,0,0,0,0}}, error_buff, 2000))
		return false;
	return true;
}

data_save_res_t handshake_attack_get_data(handshake_data_t data_t, char *error_buff) {
	const uint16_t SIZEOF_HCCAPX = 394; //see hccapx_serializer component of ESP32 code
	const uint16_t RECEIVE_TIMEOUT_MS = 500;
	data_save_res_t ret_val = SAVE_ERROR;
	FRESULT sd_setup_error_code;
	uint32_t file_num;
	char file_name[strlen(HANDSHAKE_FILE_NAME) + 18];
	uint16_t file_num_eeprom_addr = (data_t == HC22000) ? EEPROM_HANDSHAKE_HC22000_ADDR : EEPROM_HANDSHAKE_HCCAPX_ADDR;
	cmd_data_t get_data_cmd = {};
	memcpy(
			get_data_cmd.cmdName,
			(data_t == HC22000) ? HC22000_CMD : HCCAPX_CMD,
			(data_t == HC22000) ? strlen(HC22000_CMD) : strlen(HCCAPX_CMD)
		  );
	char *file_format = (data_t == HC22000) ? (".hc22000") : (".hccapx");

	eeprom_read_set(file_num_eeprom_addr, &file_num);
	sprintf(file_name, "%s_%lu%s", HANDSHAKE_FILE_NAME, file_num, file_format);
	sd_setup_error_code = SD_setup_write_from_ringBuff(HANDSHAKE_FOLDER_NAME, file_name, 9);
	if (sd_setup_error_code != FR_OK) {
		sprintf(error_buff, "SD setup error with code %d", sd_setup_error_code);
		return ret_val;
	}
	file_num++;
	eeprom_write_set(file_num_eeprom_addr, &file_num);

	if (!send_cmd_with_check(get_data_cmd, error_buff, 2000)) {
		SD_unsetup_write_from_ringBuff();
		return ret_val;
	}

	char *pkt_header = ((data_t == HC22000) ? ("HC22000 data: ") : ("HCCAPX data: "));
	uint16_t pkt_header_len = strlen(pkt_header);
	uint16_t handshake_data_len;
	int32_t msg_start;
	uint32_t curr_time = HAL_GetTick();
	ring_buffer_t *uart_ring_buffer = get_ring_buff();
	while ((HAL_GetTick() - curr_time) < RECEIVE_TIMEOUT_MS) {
		if (!get_recieved_pkts_count()) continue;

		UART_ringBuffer_mutex_take();
		if ((msg_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)pkt_header, pkt_header_len)) == -1) {
			UART_ringBuffer_mutex_give();
			continue;
		}
		ringBuffer_clearNBytes(uart_ring_buffer, (uint16_t)msg_start + pkt_header_len);

		if (ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"NONE\n", 5) != -1) {
			ret_val = NO_DATA;
		} else {
			if (data_t == HC22000)
				handshake_data_len = (uint16_t)ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"\n", 1);
			else if (data_t == HCCAPX)
				handshake_data_len = SIZEOF_HCCAPX;

			copy_ringBuff_to_SD_buff(uart_ring_buffer, handshake_data_len);
			if (!SD_end_write_from_ringBuff()) {
				memcpy(error_buff, "SD write error", strlen("SD write error") + 1);
				ret_val = SAVE_ERROR;
			} else {
				ret_val = SAVE_OK;
			}
		}
		SD_unsetup_write_from_ringBuff();
		UART_ringBuffer_mutex_give();
		return ret_val;
	}

	SD_unsetup_write_from_ringBuff();
	return ret_val;
}


attack_state_t handshake_not_ready_page(char* error_buff) {
	ClearAllButtons();

	xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
	draw_no_target_page();
	xSemaphoreGive(SPI2_mutex);

	while (1) {
		osDelay(250);
		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) return EXIT;
	}
}

attack_state_t handshake_prepare_page(char* error_buff) {
	bool full_update_screen = true;
	bool button = false;
	static uint8_t set = 0;
	eButtonEvent button_up, button_down, button_left, button_right, button_confirm;
	eapol_sum = 0;

	ClearAllButtons();

	while (1) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		darw_settings_page(set, timeout, method, save_pcap, button, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);

		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_down == SINGLE_CLICK)
			set = (set + 1) % 3;
		if (button_up == SINGLE_CLICK)
			set = ((uint8_t)(set - 1) > 2) ? (2) : (set - 1);
		if (button_down == DOUBLE_CLICK || button_up == DOUBLE_CLICK)
			button = !button;
		if ((button_right == SINGLE_CLICK || button_right == HOLD) && set == 0)
			timeout++;
		if ((button_left == SINGLE_CLICK || button_left == HOLD) && set == 0)
			timeout--;
		if (button_right == SINGLE_CLICK && set == 1)
			method = (method + 1) % 3;
		if (button_left == SINGLE_CLICK && set == 1)
			method = ((uint8_t)(method - 1) > 2) ? (2) : (method - 1);
		if ((button_right == SINGLE_CLICK || button_left == SINGLE_CLICK) && set == 2)
			save_pcap = !save_pcap;

		if (button_right == DOUBLE_CLICK)
			return EXIT;
		if (button_confirm == SINGLE_CLICK && button) {
			if (!handshake_attack_start(timeout, method, save_pcap, error_buff))
				return ERROR_HANDLER;
			else
				return RUNNING;
		}

		osDelay(100 / portTICK_PERIOD_MS);
	}
}

attack_state_t handshake_running_page(char* error_buff) {
	bool full_update_screen = true;
	eButtonEvent button_up, button_down, button_right, button_confirm;
	bool button = false;
	uint16_t time_elapsed = timeout;
	uint32_t ticks = HAL_GetTick();

	update_title_SD_flag(true);

	while (1) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		darw_running_page(time_elapsed, timeout, eapol_sum, button, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);

		button_right = GetButtonState(BUTTON_RIGHT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_down == DOUBLE_CLICK || button_up == DOUBLE_CLICK)
			button = !button;
		if (button_right == DOUBLE_CLICK)
			return EXIT;
		if (button_confirm == SINGLE_CLICK && button) {
			if (handshake_attack_stop(error_buff))
				return SAVING_DATA;
			else
				return ERROR_HANDLER;
		}


		if (check_sd_write_error()) {
			memcpy(error_buff, "SD write error", strlen("SD write error") + 1);
			return ERROR_HANDLER;
		}

		if (timeout && HAL_GetTick() - ticks > 1000) {
			time_elapsed--;
			if (!time_elapsed) {
				pcap_parser_stop();
				return SAVING_DATA;
			}
			ticks = HAL_GetTick();
		}
		eapol_sum += get_pkts_sum();

		osDelay(100 / portTICK_PERIOD_MS);
	}
}

attack_state_t handshake_saving_data_page(char* error_buff) {
	bool full_update_screen = true;
	bool button = false;
	eButtonEvent button_up, button_down, button_right, button_confirm;
	data_save_res_t save_res;

	xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
	update_title_SD_flag(true);
	draw_saving_data_page(0, true, false, false, full_update_screen);
	xSemaphoreGive(SPI2_mutex);

	osDelay(100 / portTICK_PERIOD_MS); //wait while pcap parser task ended
	save_res = handshake_attack_get_data(HC22000, error_buff);
	save_res = handshake_attack_get_data(HCCAPX, error_buff);
	if (save_res == SAVE_ERROR) return ERROR_HANDLER;

	update_title_SD_flag(false);

	while (1) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		draw_saving_data_page(eapol_sum, false, save_res == SAVE_OK, button, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);

		button_right = GetButtonState(BUTTON_RIGHT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_down == DOUBLE_CLICK || button_up == DOUBLE_CLICK)
			button = !button;
		if (button_right == DOUBLE_CLICK)
			return EXIT;
		if (button_confirm == SINGLE_CLICK && button)
			return PREPARATION;

		osDelay(100 / portTICK_PERIOD_MS);
	}
}


static void draw_no_target_page() {
	const uint16_t YStart = TITLE_END_Y + 6;

	refresh_title();
	fillScreenExceptTitle();

	drawIcon("parameters", true, 4, YStart, 125, YStart + 124, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	ST7735_WriteString(
			(ST7735_WIDTH - strlen("Target") * 7) / 2,
			YStart + 17,
			"Target",
			Font_7x10,
			MAIN_TXT_COLOR,
			MAIN_BG_COLOR
	);
	ST7735_WriteString(
			(ST7735_WIDTH - strlen("NO TARGET") * 7) / 2,
			YStart + 29,
			"NO TARGET",
			Font_7x10,
			MAIN_ERROR_COLOR,
			MAIN_BG_COLOR
	);
}

static void darw_settings_page(uint8_t set, uint8_t timeout, uint8_t method, bool save_pcap, bool button, bool full_refresh) {
	const uint16_t YStart = TITLE_END_Y + 6;
	char temp_buff[20] = {};
	bool target_is_ok = get_target() != -1;
	button = button && target_is_ok;
	static uint8_t prev_timeout_len = 0;
	static uint8_t prev_timeout = 0;
	static uint8_t prev_method = 0;
	static uint8_t prev_method_len = 0;
	static uint8_t prev_set = 0;
	static bool prev_save_pcap = false;
	static bool prev_button = false;

	if (full_refresh) {
		refresh_title();
		fillScreenExceptTitle();
	}

	if (full_refresh || prev_button != button) {
		drawIcon("parameters", !button, 4, YStart, 125, YStart + 124, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("start", YStart + 127, Font_7x10, button, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (full_refresh) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("Target") * 7) / 2,
				YStart + 17,
				"Target",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);

		char *target_ssid = *(get_AP_list() + (get_target() - 1)) + 1;
		ST7735_WriteString(
				(128 - (strlen(target_ssid) - 1) * 7) / 2,
				YStart + 29,
				target_ssid,
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	}

	if (full_refresh ||
		(prev_button != button && prev_set == 0) ||
		prev_timeout != timeout ||
		prev_set == 0 ||
		set == 0
	) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(" Timeout ") * 7) / 2,
				YStart + 44,
				(!button && set == 0) ? (">Timeout<") : (" Timeout "),
				Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
		);
		if (!timeout)
			memcpy(temp_buff, "none", 5);
		else
			sprintf(temp_buff, "%hu", timeout);
		uint8_t timeout_len = strlen(temp_buff);
		if (prev_timeout_len != timeout_len)
			fillRect(10, YStart + 56, 108, 10, MAIN_BG_COLOR);
		prev_timeout_len = timeout_len;
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(temp_buff) * 7) / 2,
				YStart + 56,
				temp_buff,
				Font_7x10,
				(!button && set == 0) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(!button && set == 0) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
		);
	}

	if (full_refresh ||
		(prev_button != button && prev_set == 1) ||
		prev_method != method ||
		prev_set == 1 ||
		set == 1
	) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(" Method ") * 7) / 2,
				YStart + 71,
				(!button && set == 1) ? (">Method<") : (" Method "),
				Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
		);

		if (method == 0)
			memcpy(temp_buff, "PASSIVE", strlen("PASSIVE") + 1);
		else if (method == 1)
			memcpy(temp_buff, "DEAUTH", strlen("DEAUTH") + 1);
		else
			memcpy(temp_buff, "EVIL AP", strlen("EVIL AP") + 1);

		uint8_t method_len = strlen(temp_buff);
		if (prev_method_len != method_len)
			fillRect(10, YStart + 83, 108, 10, MAIN_BG_COLOR);
		prev_method_len = method_len;

		ST7735_WriteString(
				(ST7735_WIDTH - strlen(temp_buff) * 7) / 2,
				YStart + 83,
				temp_buff,
				Font_7x10,
				(set == 1 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(set == 1 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
		);
	}

	if (full_refresh ||
		(prev_button != button && prev_set == 2) ||
		prev_save_pcap != save_pcap ||
		prev_set == 2 ||
		set == 2
	) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(" Save pcap ") * 7) / 2,
				YStart + 98,
				(set == 2 && !button) ? (">Save pcap<") : (" Save pcap "),
				Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
		);
		ST7735_WriteString(
				(ST7735_WIDTH - 7) / 2,
				YStart + 110,
				save_pcap ? "1" : "0",
				Font_7x10,
				(set == 2 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(set == 2 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
		);
	}

	prev_button = button;
	prev_timeout = timeout;
	prev_method = method;
	prev_save_pcap = save_pcap;
	prev_set = set;
}

static void darw_running_page(uint8_t seconds_left, uint8_t timeout, uint32_t eapol_sum, bool button, bool full_refresh) {
	const uint16_t YStart = TITLE_END_Y + 6;
	char temp_buff[20] = {};
	static uint8_t prev_seconds_left = 0;
	static bool prev_button = false;
	static uint8_t prev_timeout_len = 0;
	static uint32_t prev_eapol_sum = 0;

	if (full_refresh) {
		refresh_title();
		fillScreenExceptTitle();
	}

	if (full_refresh || prev_button != button) {
		drawIcon("timeout", !button, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("stop", YStart + 110, Font_7x10, button, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (full_refresh) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("Seconds elapsed") * 7) / 2,
				YStart + 30,
				"Seconds elapsed",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("EAPOL captured") * 7) / 2,
				YStart + 57,
				"EAPOL captured",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	}

	if (full_refresh || prev_seconds_left != seconds_left) {
		if (!timeout)
			memcpy(temp_buff, "none", 5);
		else
			sprintf(temp_buff, "%hu/%hu", seconds_left, timeout);
		uint8_t timeout_len = strlen(temp_buff);
		if (prev_timeout_len != timeout_len)
			fillRect(10, YStart + 42, 108, 10, MAIN_BG_COLOR);
		prev_timeout_len = timeout_len;
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(temp_buff) * 7) / 2,
				YStart + 42,
				temp_buff,
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	}

	if (full_refresh || prev_eapol_sum != eapol_sum) {
		sprintf(temp_buff, "%lu", eapol_sum);
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(temp_buff) * 7) / 2,
				YStart + 69,
				temp_buff,
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	}

	prev_button = button;
	prev_seconds_left = seconds_left;
	prev_eapol_sum = eapol_sum;
}

void draw_saving_data_page(uint32_t eapol_sum, bool saving, bool save_ok, bool button, bool full_refresh) {
	const uint16_t YStart = TITLE_END_Y + 6;
	char temp_buff[20] = {};
	static bool prev_button = false;

	if (full_refresh) {
		refresh_title();
		fillScreenExceptTitle();
	}

	if (full_refresh || prev_button != button) {
		drawIcon("saving data", !button, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("ok", YStart + 110, Font_7x10, button, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}
	if (saving && full_refresh) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("saving...") * 7) / 2,
				YStart + 55,
				"saving...",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
	} else if (!saving && full_refresh) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("EAPOL captured") * 7) / 2,
				YStart + 17,
				"EAPOL captured",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
		sprintf(temp_buff, "%lu", eapol_sum);
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(temp_buff) * 7) / 2,
				YStart + 29,
				temp_buff,
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
		ST7735_WriteString(
				(ST7735_WIDTH - strlen("Save status") * 7) / 2,
				YStart + 44,
				"Save status",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
		);
		ST7735_WriteString(
				save_ok ? (ST7735_WIDTH - strlen("data saved") * 7) / 2 : (ST7735_WIDTH - strlen("NO HASHES LOADED") * 7) / 2,
				YStart + 56,
				save_ok ? "data saved" : "NO HASHES LOADED",
				Font_7x10,
				save_ok ? MAIN_TXT_COLOR : MAIN_ERROR_COLOR,
				MAIN_BG_COLOR
		);
	}

	prev_button = button;
}
