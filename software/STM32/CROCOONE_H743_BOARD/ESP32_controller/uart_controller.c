/*
 * uart_parser.c
 *
 *  Created on: May 22, 2024
 *      Author: nikit
 */
#include <uart_controller.h>
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

static ring_buffer_t ring_buffer;
static uint8_t rx_buff[RX_BUFF_SIZE] = {};
static uint8_t tx_buff[TX_BUFF_SIZE] = {};
static uint16_t pkts_recieved = 0;
uint16_t pkts_missing = 0;

uint16_t add_cmd_to_buff(cmd_data_t cmd_data, char *buff);

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &ESP32_AT_UART_Port)
	{
		if (ring_buffer.readFlag) {
			pkts_missing++;
		} else {
			ring_buffer.writeFlag = true;
			if (ringBuffer_add(&ring_buffer, rx_buff, Size)) {
				pkts_recieved++;
			} else
				pkts_missing++;
			ring_buffer.writeFlag = false;
		}

		HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
		__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
	}
}

void UART_parser_init() {
	ringBuffer_init(&ring_buffer, 13);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
}

void clear_rx_buff() {
	memset(rx_buff, 0, RX_BUFF_SIZE);
}

void clear_tx_buff() {
	memset(tx_buff, 0, TX_BUFF_SIZE);
}

ring_buffer_t *get_ring_buff() {
	return &ring_buffer;
}

uint16_t get_recieved_pkts_count() {
	uint16_t ret_val = pkts_recieved;
	pkts_recieved = 0;
	return ret_val;
}

bool send_cmd_with_check(cmd_data_t cmd_data, uint32_t timeout_ms) {
	//static uint32_t previous_cmd_send_timeout = 0;

	//while ((HAL_GetTick() - previous_cmd_send_timeout) < MIN_TIMEOUT_BETWEEN_SEND_MS);
	vTaskDelay(250 / portTICK_PERIOD_MS);

	bool ret_val = false;
	clear_rx_buff();
	clear_tx_buff();
	ring_buffer.readFlag = true;
	ringBuffer_clear(&ring_buffer);
	uint16_t responce_len = strlen("OK - cmd accepted: ");
	memcpy(tx_buff, "OK - cmd accepted: ", responce_len);
	uint16_t cmd_len = add_cmd_to_buff(cmd_data, (char *)(tx_buff + responce_len));
	get_recieved_pkts_count();
	ring_buffer.readFlag = false;
	HAL_UART_Transmit(&ESP32_AT_UART_Port, tx_buff + responce_len, cmd_len, 500);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);

	uint32_t curr_time = HAL_GetTick();
	while ((HAL_GetTick() - curr_time) < timeout_ms)
		if (get_recieved_pkts_count()) {
			ring_buffer.readFlag = true;
			if (ringBuffer_findSequence(&ring_buffer, tx_buff, responce_len + cmd_len) != -1) {
				ret_val = true;
				break;
			}
			ring_buffer.readFlag = false;
		}

	ring_buffer.readFlag = true;
	ringBuffer_clear(&ring_buffer);
	ring_buffer.readFlag = false;

	//previous_cmd_send_timeout = HAL_GetTick();
	return ret_val;
}

void send_cmd_without_check(cmd_data_t cmd_data) {
	clear_tx_buff();
	uint16_t cmd_len = add_cmd_to_buff(cmd_data, (char *)tx_buff);
	HAL_UART_Transmit(&ESP32_AT_UART_Port, tx_buff, cmd_len, 500);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
}

uint16_t add_cmd_to_buff(cmd_data_t cmd_data, char *buff) {
    sprintf(buff, "%s%s%c", CMD_START, cmd_data.cmdName, CMD_PARAM_START);
    uint16_t buff_len = strlen(buff);
    for (int i = 0; i < MAX_PARAM_COUNT - 1; i++) {
        sprintf(buff + buff_len, "%d,", cmd_data.param[i]);
        buff_len = strlen(buff);
    }
    sprintf(buff + buff_len, "%d", cmd_data.param[MAX_PARAM_COUNT - 1]);

    return strlen(buff);
}
