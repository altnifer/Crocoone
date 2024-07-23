/*
 * bad_usb_attack.c
 *
 *  Created on: Jul 16, 2024
 *      Author: nikit
 */
#include "bad_usb_attack.h"
#include "script_parser.h"
#include "title.h"
#include "GFX_FUNCTIONS.h"
#include "SETTINGS.h"
#include "button.h"
#include "SD_card.h"
#include "adt.h"
#include <stdio.h>

typedef enum {
	RUNNING,
	FILE_MANNAGER,
    ERROR_HANDLER,
	EXIT
} attack_state_t;

extern SemaphoreHandle_t SPI2_mutex;
static char *script_path = NULL;

attack_state_t file_mannager_page(char *error_buff);
attack_state_t running_page(char *error_buff);
void bad_usb_attack_main_task(void *main_task_handle);
void get_names_from_pathes(list_t **path_list, list_t **names_list);
void draw_file_mannager_page(int set, list_t **names_list, bool full_refresh);
void draw_running_page(char *script_name, bool full_refresh);

void USB_attack_start(TaskHandle_t *main_task_handle) {
	xTaskCreate(bad_usb_attack_main_task, "bad_usb", 1024 * 4, main_task_handle, osPriorityNormal1, NULL);
}

void bad_usb_attack_main_task(void *main_task_handle) {
	char error_buff[100];
	attack_state_t attack_state = FILE_MANNAGER;

	update_title_text("bad usb");
	update_title_SD_flag(true);

	while (1) {
		if (attack_state == FILE_MANNAGER) {
			attack_state = file_mannager_page(error_buff);
		} else if (attack_state == RUNNING) {
			attack_state = running_page(error_buff);
		} else if (attack_state == ERROR_HANDLER || attack_state == EXIT) {
			break;
		}
	}

	SD_unmount_card();
	update_title_SD_flag(false);
	free(script_path);
	script_path = NULL;

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



attack_state_t file_mannager_page(char *error_buff) {
	attack_state_t ret_val = ERROR_HANDLER;
	list_t *path_list, *names_list;
	bool full_update_screen = true;
	int set = 0;
	eButtonEvent button_up, button_down, button_right, button_confirm;
	FRESULT sd_res;

	sd_res = SD_mount_card();
	if (sd_res != FR_OK) {
		sprintf(error_buff, "SD mount error with code %d", sd_res);
		return ret_val;
	}

	list_init(&path_list);
	list_init(&names_list);

	sd_res = SD_scan_files(BAD_USB_DIRR_NAME, BAD_USB_FILE_EXTENSION, &path_list);
	if (sd_res != FR_OK && sd_res != FR_NO_PATH) {
		sprintf(error_buff, "SD scan error with code %d", sd_res);
		list_deinit(&path_list);
		list_deinit(&names_list);
		return ret_val;
	}

	get_names_from_pathes(&path_list, &names_list);

	ClearAllButtons();

	while (1) {
		xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
		draw_file_mannager_page(set, &names_list, full_update_screen);
		full_update_screen = false;
		xSemaphoreGive(SPI2_mutex);

		button_right = GetButtonState(BUTTON_RIGHT);
		button_up = GetButtonState(BUTTON_UP);
		button_down = GetButtonState(BUTTON_DOWN);
		button_confirm = GetButtonState(BUTTON_CONFIRM);

		if (button_down == SINGLE_CLICK)
			set = (set + 1) % list_get_len(&names_list);
		if (button_up == SINGLE_CLICK)
			set = (set - 1 < 0) ? (list_get_len(&names_list) - 1) : (set - 1);
		if (button_confirm == SINGLE_CLICK) {
			ret_val = RUNNING;
			break;
		}
		if (button_right == DOUBLE_CLICK) {
			ret_val = EXIT;
			break;
		}

		osDelay(100 / portTICK_PERIOD_MS);
	}

	if (ret_val == RUNNING) {
		if (script_path)
			free(script_path);
		item_t *item = list_get_Nth_item(&path_list, set);
		script_path = (char *)malloc(item->size);
		memcpy(script_path, item->data, item->size);
	}

	SD_unmount_card();
	list_deinit(&path_list);
	list_deinit(&names_list);

	return ret_val;
}

attack_state_t running_page(char *error_buff) {
	int32_t name_start = contains_symbol_reversed(script_path, strlen(script_path), '/') + 1;
	xSemaphoreTake(SPI2_mutex, portMAX_DELAY);
	draw_running_page(script_path + name_start, true);
	xSemaphoreGive(SPI2_mutex);

	attack_state_t ret_val = ERROR_HANDLER;
	suntax_error_t suntax_error = NO_ERROR;
	FRESULT sd_res;
	char cmd[MAX_CMD_STRING_LEN];

	sd_res = SD_mount_card();
	if (sd_res != FR_OK) {
		sprintf(error_buff, "SD mount error with code %d", sd_res);
		return ret_val;
	}

	sd_res = SD_open_file_to_read(script_path);
	if (sd_res != FR_OK) {
		sprintf(error_buff, "SD file open error with code %d", sd_res);
		return ret_val;
	}

	parser_init();

	while (SD_f_gets(cmd, MAX_CMD_STRING_LEN)) {
		suntax_error = parse_cmd_string(cmd);
		if (suntax_error != NO_ERROR)
			break;
	}

	/*if (suntax_error == UNKNOWN_CMD)
		sprintf(error_buff, "unknown cmd");
	else if (suntax_error == EMPTY_CMD)
		sprintf(error_buff, "empty cmd");
	else if (suntax_error == REPETITIVE_KEY)
		sprintf(error_buff, "repetitive key");
	else if (suntax_error == UNKNOWN_KEY)
		sprintf(error_buff, "unknown key");
	else if (suntax_error == TOO_MANY_KEYS)
		sprintf(error_buff, "too many keys");*/
	if (suntax_error != NO_ERROR)
		sprintf(error_buff, "wrong cmd in script file");
	else {
		if (!SD_f_eof())
			sprintf(error_buff, "SD file read line error");
		else
			ret_val = FILE_MANNAGER;
	}

	SD_close_current_file();
	SD_unmount_card();
	parser_deinit();

	return ret_val;
}

void get_names_from_pathes(list_t **path_list, list_t **names_list) {
    item_t *item;
    for (uint16_t i = 0; i < list_get_len(path_list); i++) {
    	item = list_get_next(path_list, i);
    	char *path = (char *)item->data;
    	int32_t name_start = contains_symbol_reversed(path, (uint16_t)(item->size - 1), '/');
    	name_start = (name_start == -1) ? (0) : (name_start + 1);
    	list_push_back(names_list, path + name_start, item->size - name_start);
    }
}

void draw_file_mannager_page(int set, list_t **names_list, bool full_refresh) {
	static const uint16_t YStart = 20;
	static int prev_set = 0;

	if (full_refresh) {
		fillScreenExceptTitle();
		drawIcon("bad usb scripts", true, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}

	if (full_refresh || prev_set != set)
		drawStringsList(names_list, 7, YStart + 20, 121, YStart + 100, set, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);

	prev_set = set;
}

void draw_running_page(char *script_name, bool full_refresh) {
	static const uint16_t YStart = 20;

	if (full_refresh) {
		fillScreenExceptTitle();
		drawIcon(script_name, true, 4, YStart, 125, YStart + 100, Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
		ST7735_WriteString(25, 65, "running...", Font_7x10, MAIN_TXT_COLOR, MAIN_BG_COLOR);
	}
}
