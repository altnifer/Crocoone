/*
 * uart_parser.h
 *
 *  Created on: May 22, 2024
 *      Author: nikit
 */

#ifndef INC_UART_PARSER_H
#define INC_UART_PARSER_H

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "ring_buffer.h"

//uart definition//
#define ESP32_AT_UART_Port huart8
extern UART_HandleTypeDef ESP32_AT_UART_Port;

#define ESP32_UART_DMA_LINE hdma_uart8_rx
extern DMA_HandleTypeDef ESP32_UART_DMA_LINE;

#define DEBUG_UART huart7
extern UART_HandleTypeDef DEBUG_UART;

#define RX_BUFF_SIZE 4096
#define RING_BUFF_SIZE 8192
#define TX_BUFF_SIZE 64
//uart definition//

//cmd type definition//
#define CMD_START "AT+"
#define CMD_PARAM_START '='
#define EMPTY_PARAM 0
#define EMPTY_HASH 0

#define STOP_CMD "STOP_CURRENT_TASK"
#define SCAN_CMD "WIFI_SCAN"
#define DEAUTH_CMD "DEAUTH_ATTACK"
#define BEACON_CMD "BEACON_ATTACK"
#define HANDSHAKE_CMD "HANDSHAKE_ATTACK"
#define PMKID_CMD "PMKID_ATTACK"
#define MONITOR_CMD "PKT_MONITOR"
#define HCCAPX_CMD "GET_HCCAPX"
#define PCAP_CMD "GET_PCAP"
#define HC22000_CMD "GET_HC22000"

#define MAX_CMD_NAME_LEN 32
#define MAX_PARAM_COUNT 5

typedef struct {
    char cmdName[MAX_CMD_NAME_LEN + 1];
    int param[MAX_PARAM_COUNT];
} cmd_data_t;
//cmd type definition//

void UART_parser_init();

void UART_parser_handler_register(void (* parser_task)(uint8_t *buffer, uint16_t Size));

void UART_parser_curr_handler_unregister();

void clear_rx_buff();

void clear_tx_buff();

uint16_t get_recieved_pkts_count();

ring_buffer_t *get_ring_buff();

bool send_cmd_with_check(cmd_data_t cmd_data, uint32_t timeout_ms);

void send_cmd_without_check(cmd_data_t cmd_data);

#endif /* INC_UART_PARSER_H_ */
