/*
 * script_parser.h
 *
 *  Created on: Jul 19, 2024
 *      Author: nikit
 */

#ifndef INC_SCRIPT_PARSER_H_
#define INC_SCRIPT_PARSER_H_

#include "stdbool.h"
#include "stdint.h"

#define MAX_CMD_STRING_LEN 1024
#define USB_MAX_DELAY_MS 900000
#define MIN_PRESS_TIME_MS 20
#define KEYCODES_COUNT 6

typedef struct
{
	uint8_t MODIFIER;
	uint8_t RESERVED;
	uint8_t KEYCODES[KEYCODES_COUNT];
} keyboard_report_t;

typedef enum {
    NO_ERROR,
    UNKNOWN_CMD,
    EMPTY_CMD,
    REPETITIVE_KEY,
    UNKNOWN_KEY,
    TOO_MANY_KEYS,
    WRONG_PARAM,
    TOO_BIG_NUMBER,
    TOO_MANY_PARAMS
} suntax_error_t;

void parser_init();
void parser_deinit();
suntax_error_t parse_cmd_string(char *cmd);

#endif /* INC_SCRIPT_PARSER_H_ */
