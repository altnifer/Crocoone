/*
 * packet_monitor.c
 *
 *  Created on: May 22, 2024
 *      Author: nikit
 */
#include "packet_monitor.h"
#include "stm32h7xx_hal.h"
#include "SD_card.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include "uart_controller.h"
#include "flash_controller.h"
#include "button.h"
#include "wifi_scan.h"
#include "title.h"
#include "pcap_parser.h"
#include <stdio.h>
#include <string.h>

extern SemaphoreHandle_t SPI2_mutex;

void packet_monitor_main_task(void *main_task_handle);
bool packet_monitor_restart(bool use_sd, uint8_t ch, uint8_t filter_mask, bool use_target, char *error_buff);
uint8_t invert_bit(const uint8_t byte, const uint8_t bit);
void draw_packet_monitor_garph(uint16_t pktPerSecData[PKT_TIME_SEC], uint16_t maxPktPerSecData, uint8_t ch, bool sd, bool refresh);
void draw_packet_monitor_settings(uint8_t set, uint8_t mask_set, uint8_t ch, uint8_t filter_mask, bool button, bool use_target, bool full_update);

void ESP32_start_packet_monitor(TaskHandle_t *main_task_handle) {
	xTaskCreate(packet_monitor_main_task, "pkt_monitor", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void packet_monitor_main_task(void *main_task_handle) {
	static uint16_t pktPerSec[MAX_WIFI_CH][PKT_TIME_SEC] = {};
	static uint16_t maxPktPerSec[MAX_WIFI_CH] = {};
	char error_buff[100];
	uint8_t set = 0;
	uint8_t mask_set = 0;
	static uint8_t ch = 1;
	static uint8_t filter_mask = 7;
	uint8_t ch_temp = ch;
	uint8_t filter_mask_temp = filter_mask;
	eButtonEvent button_up, button_down, button_left, button_right, button_confirm;
	uint32_t ticks = 0;
	bool button_flag = false;
	bool use_sd = false;
	bool exit_with_error = true;
	bool graph_page = true;
	bool full_update_screen = true;
	bool restart_monitor = true;
	bool use_target_flag = false;

	pcap_parser_stop();
	pcap_parser_set_use_sd_flag(false);
	ClearAllButtons();
	update_title_text("monitor");

	while (1) {
		//drawing on display
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		update_title_SD_flag(use_sd);
		if (full_update_screen) {
			refresh_title();
			fillScreenExceptTitle();
		}
		if (graph_page)
			draw_packet_monitor_garph(pktPerSec[ch - 1], maxPktPerSec[ch - 1], ch, use_sd, full_update_screen);
		else
			draw_packet_monitor_settings(set, mask_set, ch_temp, filter_mask_temp, button_flag, use_target_flag, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);

		//buttons handling
		//get buttons states
		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		//handling common buttons
		if (button_left == DOUBLE_CLICK && !use_sd) {
			if (graph_page) {
				filter_mask_temp = filter_mask;
				ch_temp = ch;
			}
			graph_page = !graph_page;
			full_update_screen = true;
		}

		if (button_right == DOUBLE_CLICK) {
			exit_with_error = false;
			break; //exit
		}

		//handling buttons on the graph page
		if (button_confirm == SINGLE_CLICK && graph_page) {
			use_sd = !use_sd;
			restart_monitor = true;
			full_update_screen = true;
		}

		//handling buttons on the settings page
		if (!graph_page && button_confirm == SINGLE_CLICK && set == 0 && mask_set == 0)
			filter_mask_temp = invert_bit(filter_mask_temp, 2); //data

		if (!graph_page && button_confirm == SINGLE_CLICK && set == 0 && mask_set == 1)
			filter_mask_temp = invert_bit(filter_mask_temp, 1); //mgmt

		if (!graph_page && button_confirm == SINGLE_CLICK && set == 0 && mask_set == 2)
			filter_mask_temp = invert_bit(filter_mask_temp, 0); //ctrl

		if (!graph_page && button_confirm == SINGLE_CLICK && set == 1 && get_target() != -1)
			use_target_flag = !use_target_flag;

		if (!graph_page && button_confirm == SINGLE_CLICK && button_flag) {
			restart_monitor = true;
			full_update_screen = true;
			graph_page = true;
		}

		if (!graph_page && button_down == SINGLE_CLICK && !button_flag)
			set = (set + 1) % 2;

		if (!graph_page && button_up == SINGLE_CLICK && !button_flag)
			set = (set - 1) % 2;

		if (!graph_page && button_right == SINGLE_CLICK && set == 0 && !button_flag)
			mask_set = (mask_set + 1) % 3;

		if (!graph_page && button_left == SINGLE_CLICK && set == 0 && !button_flag)
			mask_set = (mask_set - 1) % 3;

		if (!graph_page && button_right == SINGLE_CLICK && set == 1 && !button_flag) {
			ch_temp++;
			if (ch_temp > MAX_WIFI_CH) ch_temp = MIN_WIFI_CH;
		}

		if (!graph_page && button_left == SINGLE_CLICK && set == 1 && !button_flag) {
			ch_temp--;
			if (ch_temp < MIN_WIFI_CH) ch_temp = MAX_WIFI_CH;
		}

		if (!graph_page && (button_up == DOUBLE_CLICK || button_down == DOUBLE_CLICK)) button_flag = !button_flag;

		//monitor control
		if (restart_monitor) {
			if (!packet_monitor_restart(use_sd, ch_temp, filter_mask_temp, use_target_flag, error_buff)) break;
			filter_mask = filter_mask_temp;
			ch = ch_temp;
			restart_monitor = false;
		}

		if (HAL_GetTick() - ticks > 1000) {
			for (int i = PKT_TIME_SEC - 1; i > 0; i--)
				pktPerSec[ch - 1][i] = pktPerSec[ch - 1][i-1];
			pktPerSec[ch - 1][0] = get_pkts_sum();
			if (pktPerSec[ch - 1][0] > maxPktPerSec[ch - 1])
				maxPktPerSec[ch - 1] = pktPerSec[ch - 1][0];
			ticks = HAL_GetTick();
		}

		if (check_sd_write_error()){
			sprintf(error_buff, "SD write error");
			break;
		}

		osDelay(250 / portTICK_PERIOD_MS);
	}

	//monitor deinit
	send_cmd_without_check((cmd_data_t){STOP_CMD, {}});
	pcap_parser_stop();
	update_title_SD_flag(false);

	//error handling
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

	//resume main task
	vTaskResume(*((TaskHandle_t *)main_task_handle));
	vTaskDelete(NULL);
}

bool packet_monitor_restart(bool use_sd, uint8_t ch, uint8_t filter_mask, bool use_target, char *error_buff) {
	FRESULT sd_setup_error_code;
	char file_name[strlen(MONITOR_FILE_NAME) + 18];

	pcap_parser_stop();
	osDelay(500 / portTICK_PERIOD_MS);

	if (!send_cmd_with_check((cmd_data_t){STOP_CMD, {}}, error_buff, 2000)) {
		return false;
	}

	pcap_parser_set_use_sd_flag(false);

	if (use_sd) {
		file_info_t *file_info_p = flash_read_file_info();
		sprintf(file_name, "%s_%lu.pcap", MONITOR_FILE_NAME, file_info_p->monitor_pcap_num);
		sd_setup_error_code = SD_setup_write_from_ringBuff(MONITOR_FOLDER_NAME, file_name, 12);
		if (sd_setup_error_code != FR_OK) {
			sprintf(error_buff, "SD setup error with code %d", sd_setup_error_code);
			return false;
		}
		file_info_p->monitor_pcap_num++;
		flash_write_file_info();
		pcap_parser_set_use_sd_flag(true);
	}

	int target = 0;
	if (use_target)
		target = get_target();

	if (!send_cmd_with_check((cmd_data_t){MONITOR_CMD, {ch, filter_mask, target, 0, 0}}, error_buff, 2000)) {
		SD_unsetup_write_from_ringBuff();
		return false;
	}

	pcap_parser_start();

	return true;
}

uint8_t invert_bit(const uint8_t byte, const uint8_t bit)
{
  uint8_t mask = 1 << bit;
  return (byte & mask) ? (byte & ~mask) : (byte | mask);
}


void draw_packet_monitor_garph(uint16_t pktPerSecData[PKT_TIME_SEC], uint16_t maxPktPerSecData, uint8_t ch, bool sd, bool refresh) {
	static const uint16_t YStart = TITLE_END_Y + 9;
	static const uint16_t MAX_DISP_PKTS = 80;
	static char buff[30] = {};
	static uint32_t everage_ps = 0;

	if (refresh) {
		drawRect(6, YStart + 10, PKT_TIME_SEC * 2 + 2, MAX_DISP_PKTS + 2, MAIN_TXT_COLOR);
		drawFastVLine(7, YStart + 10 + MAX_DISP_PKTS + 2, 6, MAIN_TXT_COLOR);
		for (uint16_t i = 10; i <= PKT_TIME_SEC * 2; i += 10)
			if (i % 60 == 0)
				drawFastVLine(6 + i, YStart + 10 + MAX_DISP_PKTS + 2, 6, MAIN_TXT_COLOR);
			else
				drawFastVLine(6 + i, YStart + 10 + MAX_DISP_PKTS + 2, 3, MAIN_TXT_COLOR);

		drawFastHLine(3, YStart + 10 + MAX_DISP_PKTS, 3, MAIN_TXT_COLOR);
		for (uint16_t i = 40; i <= MAX_DISP_PKTS; i += 40)
			drawFastHLine(3, YStart + 10 + MAX_DISP_PKTS + 1 - i, 3, MAIN_TXT_COLOR);

		ST7735_WriteString(5, YStart, "80 p/s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(0, YStart + 100, "-1s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(56, YStart + 100, "-30s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(107, YStart + 100, "-60s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		if (sd) {
			fillRoundRect(44, YStart + 125, 40, 13, 5, MAIN_TXT_COLOR);
			ST7735_WriteString(49, YStart + 128, "use SD", Font_5x7, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		} else {
			fillRoundRect(44, YStart + 125, 40, 13, 5, MAIN_BG_COLOR);
			drawRoundRect(44, YStart + 125, 40, 13, 5, MAIN_TXT_COLOR);
			ST7735_WriteString(49, YStart + 128, "use SD", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
	}

	uint16_t y_line_start = YStart + 11 + MAX_DISP_PKTS;
	uint16_t ps = 0;
	everage_ps = 0;
	for (uint16_t i = 0; i < PKT_TIME_SEC; i++) {
		ps = pktPerSecData[i];
		drawFastVLine(7 + i * 2, YStart + 11, MAX_DISP_PKTS, MAIN_BG_COLOR);
		drawFastVLine(7 + i * 2 + 1, YStart + 11, MAX_DISP_PKTS, MAIN_BG_COLOR);
		if (ps) {
			if (ps > MAX_DISP_PKTS)
				ps = MAX_DISP_PKTS;
			else if (ps < 2)
				ps = 2;

			drawFastVLine(7 + i * 2, y_line_start - ps, ps, MAIN_TXT_COLOR);
			drawFastVLine(7 + i * 2 + 1, y_line_start - ps, ps, MAIN_TXT_COLOR);
		}
		everage_ps += (uint32_t)ps;
	}

	sprintf(buff, "ch:%2u", ch % 100);
	ST7735_WriteString(100, YStart, buff, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(buff, "p/s:%3u,Mp/s:%3u,Ep/s:%3lu", pktPerSecData[0] % 1000, maxPktPerSecData % 10000, (everage_ps / PKT_TIME_SEC) % 1000);
	ST7735_WriteString(2, YStart + 112, buff, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
}

void draw_packet_monitor_settings(uint8_t set, uint8_t mask_set, uint8_t ch, uint8_t filter_mask, bool button, bool use_target, bool full_update) {
	const uint16_t YStart = TITLE_END_Y + 9;
	char buffer[35] = {};
	static uint8_t prev_set;
	static uint8_t prev_mask_set;
	static uint8_t prev_ch;
	static uint8_t prev_filter_mask;
	static bool prev_button;
	static bool prev_use_target;

	if (full_update || prev_button != button) {
		drawIcon("parameters", !button, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("restart", 142, Font_7x10, button, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (prev_use_target != use_target)
		fillRect(10, YStart + 72, ST7735_WIDTH - 20, 24, MAIN_BG_COLOR);

	if (full_update ||
		prev_filter_mask != filter_mask ||
		(prev_set == 0 && prev_button != button) ||
		prev_set != set ||
		prev_mask_set != mask_set ||
		prev_use_target != use_target
	) {
		ST7735_WriteString(
				(ST7735_WIDTH - strlen(" sniffer filter ") * 7) / 2,
				YStart + 20,
				(set == 0 && !button) ? ">sniffer filter<" : " sniffer filter ",
				Font_7x10,
				MAIN_TXT_COLOR,
				MAIN_BG_COLOR
				);
		drawFastHLine(7, YStart + 46, 114, MAIN_TXT_COLOR);
		drawFastVLine(45, YStart + 35, 23, MAIN_TXT_COLOR);
		drawFastVLine(85, YStart + 35, 23, MAIN_TXT_COLOR);
		ST7735_WriteString(10, YStart + 35, "data", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(52, YStart + 35, "mgmt", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(92, YStart + 35, "ctrl", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(buffer, "%c", filter_mask & 0b00000100 ? '1' : '0'); //data
		ST7735_WriteString(20, YStart + 49, buffer, Font_7x10,
				(mask_set == 0 && set == 0 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(mask_set == 0 && set == 0 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
				);
		sprintf(buffer, "%c", filter_mask & 0b00000010 ? '1' : '0'); //mgmt
		ST7735_WriteString(62, YStart + 49, buffer, Font_7x10,
				(mask_set == 1 && set == 0 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(mask_set == 1 && set == 0 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
				);
		sprintf(buffer, "%c", filter_mask & 0b00000001 ? '1' : '0'); //ctrl
		ST7735_WriteString(102, YStart + 49, buffer, Font_7x10,
				(mask_set == 2 && set == 0 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
				(mask_set == 2 && set == 0 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
				);
	}

	if (full_update || prev_ch != ch || (prev_set == 1 && prev_button != button) || prev_set != set) {
		if (use_target && get_target() != -1) {
			ST7735_WriteString(
					(ST7735_WIDTH - strlen(" target ") * 7) / 2,
					YStart + 72,
					(set == 1 && !button) ? ">target<" : " target ",
					Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
					);
			char *target_ssid = *(get_AP_list() + (get_target() - 1)) + 1;
			ST7735_WriteString((128 - (strlen(target_ssid) - 1) * 7) / 2, YStart + 84, target_ssid, Font_7x10,
					(set == 1 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
					(set == 1 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
					);
		} else {
			ST7735_WriteString(
					(ST7735_WIDTH - strlen(" channel ") * 7) / 2,
					YStart + 72,
					(set == 1 && !button) ? ">channel<" : " channel ",
					Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR
					);
			sprintf(buffer, "%02d", ch);
			ST7735_WriteString(57, YStart + 84, buffer, Font_7x10,
					(set == 1 && !button) ? MAIN_BG_COLOR : MAIN_TXT_COLOR,
					(set == 1 && !button) ? MAIN_TXT_COLOR : MAIN_BG_COLOR
					);
		}
	}

	prev_set = set;
	prev_mask_set = mask_set;
	prev_ch = ch;
	prev_filter_mask = filter_mask;
	prev_button = button;
	prev_use_target = use_target;
}
