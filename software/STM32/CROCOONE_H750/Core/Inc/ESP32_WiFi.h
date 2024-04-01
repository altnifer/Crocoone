/*
 * ESP32_AT_cmd.h
 *
 *  Created on: 30 июл. 2023 г.
 *      Author: nikit
 */

#ifndef INC_ESP32_WIFI_H_
#define INC_ESP32_WIFI_H_

#include "SETTINGS.h"
#include "stm32h7xx_hal.h"
#include "button.h"
#include "GFX_FUNCTIONS.h"
#include "EEPROM.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "fatfs.h"
#include "main.h"
#include "AT24Cxx.h"

#define ESP32_AT_UART_Port huart8
extern UART_HandleTypeDef ESP32_AT_UART_Port;

#define DEBUG_UART huart7
extern UART_HandleTypeDef DEBUG_UART;

#define AT_UART_Buff_size 2048
#define MAX_PKT_SIZE 512
#define MAX_HANDSHAKE_TIMEOUT_SEC 240
#define MAX_DEAUTH_TIMEOUT_SEC 300
#define MIN_TIMEOUT_SEC 10

//AT commands//
#define SCANWIFI "AT+WIFI_SCAN"
#define DASTART "AT+DEAUTH_ATTACK_START->"
#define BASTART "AT+BEACON_ATTACK_START->"
#define HSTART "AT+HANDSHAKE_ATTACK->"
#define PKTASTART "AT+PKT_ANALYZER_START->"
#define STOPTASK "AT+STOP_CURRENT_TASK"
#define GETHCCAPX "AT+GET_HCCAPX"
#define GETPCAP "AT+GET_PCAP"
#define GETHC22000 "AT+GET_HC22000"
//-----------//

typedef enum {

	// esp at cmd errors //
    AT_OK = 1,
    AT_WRONG_CMD,
    AT_WRONG_NUM,
    AT_NO_TARGET,
    AT_NO_WIFI_SCAN,
    AT_WRONG_TYPE,
	//-------------------//

	// UART hardware errors //
	WRONG_AT_ANS,
	UART_ERROR,
	//---------------------//

	// SD errors //
	SD_ERROR,
	SD_MK_DIR_ERROR,
	SD_MK_FILE_ERROR,
	SD_FILE_WRITE_ERROR,
	//-----------//

	// EEPROM error //
	EEPROM_ERROR,
	//-----------//

	NO_DATA,
	TIMEOUT
} at_error_t;

typedef enum {
    NONE,
    PASSIVE,
    DEAUTH
} handshake_method_t;

typedef enum {
    PCAP,
    HCCAPX,
    HC22000
} data_type_t;

void ESP32_AT_UART_Init();

void ESP32_scan_loop();

void ESP32_handshake_loop();

void ESP32_attacks_loop();

void ESP32_pkt_analyzer_loop();

void ESP32_start_pmkid_attack();

#endif /* INC_ESP32_WIFI_H_ */
