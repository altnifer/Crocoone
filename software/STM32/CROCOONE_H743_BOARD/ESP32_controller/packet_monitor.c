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
#include "eeprom.h"
#include "button.h"
#include "title.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

static uint32_t pktsSumm = 0;
static bool parser_task_flag;
static bool parser_use_sd_flag;
static bool SD_write_error;
extern SemaphoreHandle_t SPI2_mutex;

void packet_monitor_main_task(void *main_task_handle);
bool packet_parser_restart(bool use_sd, uint8_t ch, uint8_t filter_mask, char *error_buff);
void packet_parser_task(void *args);
uint8_t invert_bit(const uint8_t byte, const uint8_t bit);
void draw_packet_monitor_garph(uint16_t pktPerSecData[PKT_TIME_SEC], uint16_t maxPktPerSecData, uint16_t ch, bool sd, bool refresh);
void draw_packet_monitor_settings(int curr_set, int curr_sniffer_set, int ch, uint8_t filter_mask, bool refresh);

void ESP32_start_packet_monitor(TaskHandle_t *main_task_handle) {
	xTaskCreate(packet_monitor_main_task, "pkt_monitor", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void packet_monitor_main_task(void *main_task_handle) {
	static uint16_t pktPerSec[MAX_WIFI_CH][PKT_TIME_SEC] = {};
	static uint16_t maxPktPerSec[MAX_WIFI_CH] = {};
	static const int max_set_count = 3;
	static const int max_sniffer_set_count = 3;
	char error_buff[100];
	int curr_set = 0;
	int curr_sniffer_set = 0;
	static uint8_t ch = 1;
	static uint8_t filter_mask = 7;
	uint8_t ch_temp = ch;
	uint8_t filter_mask_temp = filter_mask;
	eButtonEvent button_up, button_down, button_left, button_right, button_confirm;
	uint32_t ticks = 0;
	bool use_sd = false;
	bool exit_with_error = true;
	bool graph_page = true;
	bool full_update_screen = true;
	bool update_screen = true;
	bool restart_monitor = true;
	pktsSumm = 0;
	SD_write_error = false;

	ClearAllButtons();
	update_title_text("monitor");

	while (1) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		update_title_SD_flag(use_sd);

		if (full_update_screen) {
			refresh_title();
			fillScreenExceptTitle();
		}
		if (graph_page && (full_update_screen || update_screen)) {
			draw_packet_monitor_garph(pktPerSec[ch - 1], maxPktPerSec[ch - 1], ch, use_sd, full_update_screen);
			full_update_screen = false;
			update_screen = false;
		} else if (!graph_page && (full_update_screen || update_screen)) {
			draw_packet_monitor_settings(curr_set, curr_sniffer_set, ch_temp, filter_mask_temp, full_update_screen);
			full_update_screen = false;
			update_screen = false;
		}
		xSemaphoreGive(SPI2_mutex);


		button_right = GetButtonState(BUTTON_RIGHT);
		button_left = GetButtonState(BUTTON_LEFT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_left == DOUBLE_CLICK && !use_sd) {
			if (graph_page) {
				filter_mask_temp = filter_mask;
				ch_temp = ch;
			}
			graph_page = !graph_page;
			full_update_screen = true;
		}

		if (button_confirm == SINGLE_CLICK && graph_page) {
			use_sd = !use_sd;
			restart_monitor = true;
			full_update_screen = true;
		}

		if (!graph_page) {
			if (button_confirm == SINGLE_CLICK) {
				if (curr_set == 0) {
					if (curr_sniffer_set == 0)
						filter_mask_temp = invert_bit(filter_mask_temp, 2); //data
					else if (curr_sniffer_set == 1)
						filter_mask_temp = invert_bit(filter_mask_temp, 1); //mgmt
					else if (curr_sniffer_set == 2)
						filter_mask_temp = invert_bit(filter_mask_temp, 0); //ctrl
				} else if (curr_set == 2) {
					restart_monitor = true;
					full_update_screen = true;
					graph_page = true;
				}
				update_screen = true;
			} else if (button_down == SINGLE_CLICK) {
				curr_set = (curr_set + 1) % max_set_count;
				update_screen = true;
			} else if (button_up == SINGLE_CLICK) {
				curr_set--;
				if (curr_set < 0) curr_set = max_set_count - 1;
				update_screen = true;
			} else if (button_right == SINGLE_CLICK) {
				if (curr_set == 0)
					curr_sniffer_set = (curr_sniffer_set + 1) % max_sniffer_set_count;
				else if (curr_set == 1) {
					ch_temp++;
					if (ch_temp > MAX_WIFI_CH) ch_temp = MIN_WIFI_CH;
				}
				update_screen = true;
			} else if (button_left == SINGLE_CLICK) {
				if (curr_set == 0) {
					curr_sniffer_set--;
					if (curr_sniffer_set < 0) curr_sniffer_set = max_sniffer_set_count - 1;
				} else if (curr_set == 1) {
					ch_temp--;
					if (ch_temp < MIN_WIFI_CH) ch_temp = MAX_WIFI_CH;
				}
				update_screen = true;
			}
		}

		if (button_right == DOUBLE_CLICK) {
			exit_with_error = false;
			break; //exit
		}


		if (restart_monitor) {
			if (!packet_parser_restart(use_sd, ch_temp, filter_mask_temp, error_buff)) break;
			filter_mask = filter_mask_temp;
			ch = ch_temp;
			restart_monitor = false;
		}

		if (HAL_GetTick() - ticks > 1000) {
			for (int i = PKT_TIME_SEC - 1; i > 0; i--)
				pktPerSec[ch - 1][i] = pktPerSec[ch - 1][i-1];
			pktPerSec[ch - 1][0] = pktsSumm;
			if (pktsSumm > maxPktPerSec[ch - 1])
				maxPktPerSec[ch - 1] = pktsSumm;
			pktsSumm = 0;

			if (graph_page) update_screen = true;
			ticks = HAL_GetTick();
		}

		if (SD_write_error){
			sprintf(error_buff, "SD write error");
			break;
		}

		osDelay(250 / portTICK_PERIOD_MS);
	}

	send_cmd_without_check((cmd_data_t){STOP_CMD, {0,0,0,0,0}});
	parser_task_flag = false;
	update_title_SD_flag(false);

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

bool packet_parser_restart(bool use_sd, uint8_t ch, uint8_t filter_mask, char *error_buff) {
	FRESULT sd_setup_error_code;
	uint32_t file_num;
	char file_name[strlen(MONITOR_FILE_NAME) + 18];

	parser_task_flag = false;
	osDelay(1000 / portTICK_PERIOD_MS);

	if (!send_cmd_with_check((cmd_data_t){STOP_CMD, {0,0,0,0,0}}, 2000)) {
		memcpy(error_buff, "Failure while sending STOP_CMD cmd to ESP\0", strlen("Failure while sending STOP_CMD cmd to ESP") + 1);
		return false;
	}
	parser_use_sd_flag = false;

	if (use_sd) {
		eeprom_read_set(EEPROM_PCAP_ADDR, &file_num);
		sprintf(file_name, "%s_%lu.pcap", MONITOR_FILE_NAME, file_num);
		sd_setup_error_code = SD_setup(file_name, MONITOR_FOLDER_NAME, 12);
		if (sd_setup_error_code != FR_OK) {
			sprintf(error_buff, "SD setup error with code %d", sd_setup_error_code);
			return false;
		}
		file_num++;
		eeprom_write_set(EEPROM_PCAP_ADDR, &file_num);
		parser_use_sd_flag = true;
	}

	if (!send_cmd_with_check((cmd_data_t){MONITOR_CMD, {ch, filter_mask, 0, 0, 0}}, 2000)) {
		SD_unsetup();
		memcpy(error_buff, "Failure while sending MONITOR_CMD cmd to ESP\0", strlen("Failure while sending MONITOR_CMD cmd to ESP") + 1);
		return false;
	}

	parser_task_flag = true;
	xTaskCreate(packet_parser_task, "monitor_parser", 1024 * 4, NULL, osPriorityNormal2, NULL);

	return true;
}

void packet_parser_task(void *args) {
	static uint8_t packet_header[FRAME_HEADER_LEN + 1];
	ring_buffer_t *uart_ring_buffer = get_ring_buff();
	ringBuffer_clear(uart_ring_buffer);

	while (parser_task_flag) {
		osDelay(50 / portTICK_PERIOD_MS);

		if (uart_ring_buffer->writeFlag) continue;

		uart_ring_buffer->readFlag = true;
		for (uint16_t i = 0; i < get_recieved_pkts_count(); i++) {
			int32_t packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"PCAP data (", 11);
			if (packet_start == -1) break;

			ringBuffer_clearNBytes(uart_ring_buffer, (uint16_t)packet_start);

			ringBuffer_get(uart_ring_buffer, packet_header, FRAME_HEADER_LEN);

			ringBuffer_clearNBytes(uart_ring_buffer, FRAME_HEADER_LEN);

			uint16_t pkts = 0, bytes = 0;
			packet_header[FRAME_HEADER_LEN] = '\0';
			if (!sscanf((char *)packet_header, "PCAP data (%hu pkts, %hu bytes): ", &pkts, &bytes)) break;

			pktsSumm += pkts;

			if (parser_use_sd_flag)
				copy_ringBuff_to_SD_buff(uart_ring_buffer, bytes);
		}
		ringBuffer_clear(uart_ring_buffer);
		uart_ring_buffer->readFlag = false;

		if ((SD_write_error = !SD_write_from_ringBuff()) == true)
			break;
	}

	if (!SD_write_error)
		SD_write_error = !SD_end_write_from_ringBuff();
	SD_unsetup();

	parser_use_sd_flag = false;
	parser_task_flag = false;
	vTaskDelete(NULL);
}

uint8_t invert_bit(const uint8_t byte, const uint8_t bit)
{
  uint8_t mask = 1 << bit;
  return (byte & mask) ? (byte & ~mask) : (byte | mask);
}


void draw_packet_monitor_garph(uint16_t pktPerSecData[PKT_TIME_SEC], uint16_t maxPktPerSecData, uint16_t ch, bool sd, bool refresh) {
	static const uint16_t graphYStart = TITLE_END_Y + 9;
	static const uint16_t MAX_DISP_PKTS = 80;
	static char buff[30] = {};
	static uint32_t everage_ps = 0;

	if (refresh) {
		drawRect(6, graphYStart + 10, PKT_TIME_SEC * 2 + 2, MAX_DISP_PKTS + 2, MAIN_TXT_COLOR);
		drawFastVLine(7, graphYStart + 10 + MAX_DISP_PKTS + 2, 6, MAIN_TXT_COLOR);
		for (uint16_t i = 10; i <= PKT_TIME_SEC * 2; i += 10)
			if (i % 60 == 0)
				drawFastVLine(6 + i, graphYStart + 10 + MAX_DISP_PKTS + 2, 6, MAIN_TXT_COLOR);
			else
				drawFastVLine(6 + i, graphYStart + 10 + MAX_DISP_PKTS + 2, 3, MAIN_TXT_COLOR);

		drawFastHLine(3, graphYStart + 10 + MAX_DISP_PKTS, 3, MAIN_TXT_COLOR);
		for (uint16_t i = 40; i <= MAX_DISP_PKTS; i += 40)
			drawFastHLine(3, graphYStart + 10 + MAX_DISP_PKTS + 1 - i, 3, MAIN_TXT_COLOR);

		ST7735_WriteString(5, graphYStart, "80 p/s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(0, graphYStart + 100, "-1s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(56, graphYStart + 100, "-30s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(107, graphYStart + 100, "-60s", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		if (sd) {
			fillRoundRect(44, graphYStart + 125, 40, 13, 5, MAIN_TXT_COLOR);
			ST7735_WriteString(49, graphYStart + 128, "use SD", Font_5x7, MAIN_BG_COLOR, MAIN_TXT_COLOR);
		} else {
			fillRoundRect(44, graphYStart + 125, 40, 13, 5, MAIN_BG_COLOR);
			drawRoundRect(44, graphYStart + 125, 40, 13, 5, MAIN_TXT_COLOR);
			ST7735_WriteString(49, graphYStart + 128, "use SD", Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		}
	}

	uint16_t y_line_start = graphYStart + 11 + MAX_DISP_PKTS;
	uint16_t ps = 0;
	everage_ps = 0;
	for (uint16_t i = 0; i < PKT_TIME_SEC; i++) {
		ps = pktPerSecData[i];
		drawFastVLine(7 + i * 2, graphYStart + 11, MAX_DISP_PKTS, MAIN_BG_COLOR);
		drawFastVLine(7 + i * 2 + 1, graphYStart + 11, MAX_DISP_PKTS, MAIN_BG_COLOR);
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
	ST7735_WriteString(100, graphYStart, buff, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	sprintf(buff, "p/s:%3u,Mp/s:%3u,Ep/s:%3lu", pktPerSecData[0] % 1000, maxPktPerSecData % 10000, (everage_ps / PKT_TIME_SEC) % 1000);
	ST7735_WriteString(2, graphYStart + 112, buff, Font_5x7, MAIN_TXT_COLOR, MAIN_BG_COLOR);
}

void draw_packet_monitor_settings(int curr_set, int curr_sniffer_set, int ch, uint8_t filter_mask, bool refresh) {
	static const uint16_t graphYStart = TITLE_END_Y + 9;
	static uint16_t previous_set = 0;
	char buffer[10] = {};

	if (previous_set == 0 || refresh) {
		ST7735_WriteString(15, graphYStart, "sniffer filter", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		drawFastHLine(5, graphYStart + 26, 118, MAIN_TXT_COLOR);
		drawFastVLine(45, graphYStart + 15, 23, MAIN_TXT_COLOR);
		drawFastVLine(85, graphYStart + 15, 23, MAIN_TXT_COLOR);
		ST7735_WriteString(10, graphYStart + 15, "data", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(52, graphYStart + 15, "mgmt", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(92, graphYStart + 15, "ctrl", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(buffer, "%c", filter_mask & 0b00000100 ? '1' : '0'); //data
		ST7735_WriteString(20, graphYStart + 29, buffer, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(buffer, "%c", filter_mask & 0b00000010 ? '1' : '0'); //mgmt
		ST7735_WriteString(62, graphYStart + 29, buffer, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(buffer, "%c", filter_mask & 0b00000001 ? '1' : '0'); //ctrl
		ST7735_WriteString(102, graphYStart + 29, buffer, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(8, graphYStart, " ", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(114, graphYStart, " ", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
	}
	if (previous_set == 1 || refresh) {
		ST7735_WriteString(40, graphYStart + 52, "channel", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		sprintf(buffer, "%02d", ch);
		ST7735_WriteString(57, graphYStart + 64, buffer, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(33, graphYStart + 52, " ", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(90, graphYStart + 52, " ", Font_7x10, MAIN_BG_COLOR, MAIN_BG_COLOR);
	}
	if (previous_set == 2 || refresh) {
		fillRoundRect(34, 141, 61, 14, 4, MAIN_BG_COLOR);
		ST7735_WriteString(40, 144, "restart", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		drawRoundRect(34, 141, 61, 14, 4, MAIN_TXT_COLOR);
	}
	previous_set = curr_set;

	switch (curr_set) {
		case 0:
			ST7735_WriteString(8, graphYStart, ">", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			ST7735_WriteString(114, graphYStart, "<", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			if (curr_sniffer_set == 0) {
				sprintf(buffer, "%c", filter_mask & 0b00000100 ? '1' : '0'); //data
				ST7735_WriteString(20, graphYStart + 29, buffer, Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			} else if (curr_sniffer_set == 1) {
				sprintf(buffer, "%c", filter_mask & 0b00000010 ? '1' : '0'); //mgmt
				ST7735_WriteString(62, graphYStart + 29, buffer, Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			} else if (curr_sniffer_set == 2){
				sprintf(buffer, "%c", filter_mask & 0b00000001 ? '1' : '0'); //ctrl
				ST7735_WriteString(102, graphYStart + 29, buffer, Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			}
			break;
		case 1:
			ST7735_WriteString(33, graphYStart + 52, ">", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			ST7735_WriteString(90, graphYStart + 52, "<", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			sprintf(buffer, "%02d", ch);
			ST7735_WriteString(57, graphYStart + 64, buffer, Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			break;
		case 2:
			fillRoundRect(34, 141, 61, 14, 4, MAIN_TXT_COLOR);
			ST7735_WriteString(40, 144, "restart", Font_7x10, MAIN_BG_COLOR, MAIN_TXT_COLOR);
			break;
		default:
			break;
	}
}
