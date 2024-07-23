/*
 * script_parser.c
 *
 *  Created on: Jul 19, 2024
 *      Author: nikit
 */
#include "script_parser.h"
#include "ascii_hid_table.h"
#include "usb_device.h"
#include "cmsis_os.h"
#include "adt.h"
#include <string.h>
#include <stdio.h>

#include "GFX_FUNCTIONS.h"

extern hashmap_t *ascii_hid_map;
extern USBD_HandleTypeDef hUsbDeviceFS;

suntax_error_t parse_report_cmd(char *cmd);
void send_keyboard_report(keyboard_report_t report);
suntax_error_t parse_string_cmd(char *cmd);
suntax_error_t parse_delay_cmd(char *cmd);

void parser_init() {
	if (!ascii_hid_map_state())
		ascii_hid_map_init();
	MX_USB_DEVICE_Init();
	osDelay(500 / portTICK_PERIOD_MS);
}

void parser_deinit() {
	//MX_USB_DEVICE_DeInit();
}

suntax_error_t parse_cmd_string(char *cmd) {
	uint16_t token_len = 1;
	if (!get_first_token(cmd, strlen(cmd), " \t\n", 3, &token_len) && !token_len) //skip empty string
		return NO_ERROR;

	if (!strncmp(cmd, "REPORT", 6))
		return parse_report_cmd(cmd);
	else if (!strncmp(cmd, "STRING", 6))
		return parse_string_cmd(cmd);
	else if (!strncmp(cmd, "DELAY", 5))
		return parse_delay_cmd(cmd);

	return UNKNOWN_CMD;
}

suntax_error_t parse_report_cmd(char *cmd) {
    keyboard_report_t keyboard_report = {};
    uint16_t key_count = 0;
    uint16_t token_len;
    char *token = get_first_token(cmd, strlen(cmd) + 1, " \t\n\0", 4, &token_len);
    char *cmd_rest = token + token_len;

    if (strncmp(token, "REPORT", token_len))
        return UNKNOWN_CMD;

    while ((token = get_first_token(cmd_rest, strlen(cmd_rest) + 1, " \t\n\0", 4, &token_len))) {
        hitem_t *hitem = hashmap_get(&ascii_hid_map, (uint8_t *)token, token_len);
        if (!hitem)
            return UNKNOWN_KEY;
        hid_key_t *hid_key = (hid_key_t *)hitem->data;
        if (hid_key->KEYCODE == KEY_NONE) {
            if (keyboard_report.MODIFIER & hid_key->MODIFIER)
                return REPETITIVE_KEY;
            keyboard_report.MODIFIER = keyboard_report.MODIFIER | hid_key->MODIFIER;
        } else {
            for (uint8_t i = 0; i < key_count; i++)
                if (keyboard_report.KEYCODES[i] == hid_key->KEYCODE)
                    return REPETITIVE_KEY;
            keyboard_report.KEYCODES[key_count] = hid_key->KEYCODE;
            key_count++;
        }

        cmd_rest = token + token_len;

        if (key_count > KEYCODES_COUNT)
            return TOO_MANY_KEYS;
    }

    if (!key_count && !keyboard_report.MODIFIER)
        return EMPTY_CMD;

    send_keyboard_report(keyboard_report);
    return NO_ERROR;
}

suntax_error_t parse_string_cmd(char *cmd) {
    uint16_t keys_str_len;
    char *keys_str = get_first_token(cmd, strlen(cmd) + 1, " \n\0", 3, &keys_str_len);

    if (strncmp(keys_str, "STRING", keys_str_len))
        return UNKNOWN_CMD;

    keys_str += (keys_str_len + 1);
    keys_str_len = strlen(cmd) - keys_str_len - 1;
    if (keys_str[keys_str_len - 1] == '\n') keys_str_len--;

    if (!keys_str_len)
    	return EMPTY_CMD;

    keyboard_report_t keyboard_report = {};
    uint8_t report_key_index = 0;
    uint8_t prev_modifier = KEY_NONE;
    bool repeat_key = false;
    for (uint16_t i = 0; i < keys_str_len; i++) {
        hitem_t *hitem = hashmap_get(&ascii_hid_map, (uint8_t *)keys_str + i, 1);
        if (!hitem) {
            printf("%c\n", keys_str[i]);
            return UNKNOWN_KEY;
        }
        hid_key_t *hid_key = (hid_key_t *)hitem->data;

        for (uint8_t i = 0; i < report_key_index; i++)
            if (keyboard_report.KEYCODES[i] == hid_key->KEYCODE) {
                repeat_key = true;
                break;
            }

        if (report_key_index >= KEYCODES_COUNT || prev_modifier != hid_key->MODIFIER || repeat_key) {
        	send_keyboard_report(keyboard_report);
            memset(&keyboard_report, 0, sizeof(keyboard_report_t));
            report_key_index = 0;

            keyboard_report.MODIFIER = keyboard_report.MODIFIER | hid_key->MODIFIER;
            prev_modifier = hid_key->MODIFIER;
        }

        keyboard_report.KEYCODES[report_key_index] = hid_key->KEYCODE;
        report_key_index++;
    }

    if (report_key_index) {
    	send_keyboard_report(keyboard_report);
    }

    return NO_ERROR;
}

suntax_error_t parse_delay_cmd(char *cmd) {
    uint16_t token_len;
    char *token = get_first_token(cmd, strlen(cmd) + 1, " \n\t\0", 4, &token_len);

    if (strncmp(token, "DELAY", token_len))
        return UNKNOWN_CMD;

    cmd = token + token_len;
    token = get_first_token(cmd, strlen(cmd) + 1, " \n\t\0", 4, &token_len);

    cmd = token + token_len;
    uint16_t dummy;
    if (get_first_token(cmd, strlen(cmd) + 1, " \n\t\0", 4, &dummy))
        return TOO_MANY_PARAMS;

    uint32_t delay_ms;
    if (!sscanf(token, "%lu", &delay_ms))
        return WRONG_PARAM;

    if (token_len > 5 || delay_ms > USB_MAX_DELAY_MS)
        return TOO_BIG_NUMBER;

    osDelay(delay_ms / portTICK_PERIOD_MS);
    return NO_ERROR;
}

void send_keyboard_report(keyboard_report_t report) {
	USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&report, sizeof(keyboard_report_t));
	osDelay(MIN_PRESS_TIME_MS / portTICK_PERIOD_MS);
	memset(&report, 0, sizeof(keyboard_report_t));
	USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&report, sizeof(keyboard_report_t));
	osDelay(MIN_PRESS_TIME_MS / portTICK_PERIOD_MS);
}
