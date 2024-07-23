/*
 * wifi_scan.c
 *
 *  Created on: Jun 27, 2024
 *      Author: nikit
 */
#include "wifi_scan.h"
#include "stm32h7xx_hal.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include "uart_controller.h"
#include "button.h"
#include "adt.h"
#include "stdio.h"
#include "string.h"
#include "title.h"
#include "cmsis_os.h"
#include "semphr.h"

static char ** AP_list = NULL;
static uint16_t AP_count = 0;
static int target = -1;
extern SemaphoreHandle_t SPI2_mutex;


void scan_main_task(void *main_task_handle);
bool start_scan(uint32_t timeout, char *error_buff);
bool parse_APs(char ***AP_list, uint16_t *AP_count);
void daraw_scan_menu(char ** AP_list, uint16_t AP_count, int curr_menu_set, bool menu_flag, bool update_menu, bool update_icon);


void ESP32_scan_wifi(TaskHandle_t *main_task_handle) {
	xTaskCreate(scan_main_task, "scan_wifi", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void scan_main_task(void *main_task_handle) {
	static int previous_target = -1;
	bool exit_with_error = true;
	bool update_menu = true;
	bool update_icon = true;
	int scan_list_curr_set = 0;
	bool menu_flag = false;
	bool rescan = false;
	char error_buff[100];
	eButtonEvent button_up, button_down;

	update_title_text("scanner");
	xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
	fillScreenExceptTitle(MAIN_BG_COLOR);
	refresh_title();
	xSemaphoreGive(SPI2_mutex);

	while (1) {
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);

		if (button_up == DOUBLE_CLICK || button_down == DOUBLE_CLICK) {
			menu_flag = !menu_flag;
			update_icon = true;
			update_menu = true;
		}

		if (button_up == SINGLE_CLICK && menu_flag) {
			scan_list_curr_set = (scan_list_curr_set - 1) % (int)AP_count;
			if (scan_list_curr_set < 0) scan_list_curr_set =  (int)(AP_count - 1);
			update_menu = true;
		}

		if (button_down == SINGLE_CLICK && menu_flag) {
			scan_list_curr_set = (scan_list_curr_set + 1) % AP_count;
			update_menu = true;
		}

		if (GetButtonState(BUTTON_CONFIRM) == SINGLE_CLICK) {
			if (menu_flag) {
				if (scan_list_curr_set != target) {
					AP_list[scan_list_curr_set][0] = '>';
					target = scan_list_curr_set + 1;
					if (previous_target != -1)
						AP_list[previous_target][0] = ' ';
					previous_target = scan_list_curr_set;
				} else {
					AP_list[scan_list_curr_set][0] = ' ';
					previous_target = -1;
				}
			} else {
				rescan = true;
				update_icon = true;
			}
			update_menu = true;
		}

		if (GetButtonState(BUTTON_RIGHT) == DOUBLE_CLICK) {
			exit_with_error = false;
			break;
		}


		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		if (rescan) {
			fillRect(7, 40, 114, 70, MAIN_BG_COLOR);
			ST7735_WriteString(25, 65, "scanning...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
			xSemaphoreGive(SPI2_mutex);
			if (!start_scan(SCAN_RESPONCE_TIME_MS, error_buff)) break;
			xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
			fillRect(7, 40, 114, 70, MAIN_BG_COLOR);
			target = -1;
			previous_target = -1;
			rescan = false;
		}

		if (update_menu || update_icon) {
			daraw_scan_menu(AP_list, AP_count, scan_list_curr_set, menu_flag, update_menu, update_icon);
			update_menu = false;
			update_icon = false;
		}
		xSemaphoreGive(SPI2_mutex);

		osDelay(250 / portTICK_PERIOD_MS);
	}

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

uint16_t get_AP_count() {
	return AP_count;
}

char **get_AP_list() {
	return AP_list;
}

int get_target() {
	return target;
}

bool start_scan(uint32_t timeout, char *error_buff) {
	if (!send_cmd_with_check((cmd_data_t){SCAN_CMD, {0,0,0,0,0}}, error_buff, 2000)) {
		return false;
	}

	uint32_t tick_counter = HAL_GetTick();

	while (HAL_GetTick() - tick_counter < timeout)
		if (get_recieved_pkts_count()) {
			UART_ringBuffer_mutex_take();
			if (parse_APs(&AP_list, &AP_count)) {
				UART_ringBuffer_mutex_give();
				return true;
			}
			UART_ringBuffer_mutex_give();
		}

	memcpy(error_buff,
			"failure while parsing msg\0",
			strlen("failure while parsing msg") + 1);
	return false;
}

bool parse_APs(char ***AP_list, uint16_t *AP_count) {
	int32_t packet_start;
	ring_buffer_t *uart_ring_buffer = get_ring_buff();

    packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)SCAN_PACKET_HEADER, SCAN_PACKET_HEADER_LEN);

	if (packet_start == -1) {
		return false;
	}

    ringBuffer_clearNBytes(uart_ring_buffer, (uint16_t)(packet_start + SCAN_PACKET_HEADER_LEN));

	if (*AP_list != NULL) {
		for (uint16_t i = 0; i < *AP_count; i++)
			free(*(*AP_list + i));
		free(*AP_list);
		*AP_list = NULL;
	}

	char ap_count_buff[3];

	ringBuffer_get(uart_ring_buffer, (uint8_t *)ap_count_buff, 2);
    ap_count_buff[2] = '\0';
	if (!sscanf(ap_count_buff, "%hu", AP_count)) return false;

	*AP_list = (char**) malloc(*AP_count * sizeof(char*));

    uint16_t ssid_len;
	for (uint16_t i = 0; i < *AP_count; i++) {
		packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"_SSID: ", 7);
		if (packet_start == -1)	break;
		ringBuffer_clearNBytes(uart_ring_buffer, packet_start + 7);
		ssid_len = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"\n", 1);
		if (ssid_len == -1)	break;

        *(*AP_list + i) = (char *)malloc((ssid_len + 2) * sizeof(char));

        ringBuffer_get(uart_ring_buffer, (uint8_t *)(*(*AP_list + i) + 1), ssid_len);
        **(*AP_list + i) = ' ';
        *(*(*AP_list + i) + ssid_len + 1) = '\0';
	}

	ringBuffer_clear(uart_ring_buffer);

	return true;
}


void daraw_scan_menu(char ** AP_list, uint16_t AP_count, int curr_menu_set, bool menu_flag, bool update_menu, bool update_icon) {
	static const uint16_t YStart = 20;

	if (AP_list != NULL && update_menu)
		drawMenu(AP_list, AP_count, 7, YStart + 20, 121, YStart + 100, !menu_flag ? -1 : curr_menu_set, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	if (update_icon) {
		drawIcon("scan list", menu_flag, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		darwMiddleButton("restart", YStart + 110, Font_7x10, !menu_flag, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}
}
