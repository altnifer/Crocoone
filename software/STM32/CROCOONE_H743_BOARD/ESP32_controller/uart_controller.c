/*
 * uart_parser.c
 *
 *  Created on: May 22, 2024
 *      Author: nikit
 */
#include <uart_controller.h>
#include "cmsis_os.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

static SemaphoreHandle_t IDLE_semaphore;
static SemaphoreHandle_t UART_ringBuffer_mutex;
static SemaphoreHandle_t ESP_UART_mutex;
static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

static ring_buffer_t ring_buffer;
static uint8_t rx_buff[RX_BUFF_SIZE] = {};
static uint8_t tx_buff[TX_BUFF_SIZE] = {};
static uint16_t pkts_recieved = 0;
static uint16_t last_pkt_size = 0;

uint16_t add_cmd_to_buff(cmd_data_t cmd_data, char *buff);
void IDLE_ISR_handler_task(void * args);

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &ESP32_AT_UART_Port)
	{
		last_pkt_size = Size;
		xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(IDLE_semaphore, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void IDLE_ISR_handler_task(void * args) {
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);

	while (1) {
		xSemaphoreTake(IDLE_semaphore, portMAX_DELAY);

		xSemaphoreTake(UART_ringBuffer_mutex, portMAX_DELAY);
		if (ringBuffer_add(&ring_buffer, rx_buff, last_pkt_size)) {
			last_pkt_size = 0;
			pkts_recieved++;
		}
		xSemaphoreGive(UART_ringBuffer_mutex);

		xSemaphoreTake(ESP_UART_mutex, portMAX_DELAY);
		HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
		__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
		xSemaphoreGive(ESP_UART_mutex);

		osDelay(1);
	}

	vTaskDelete(NULL);
}

void UART_ringBuffer_mutex_take() {
	xSemaphoreTake(UART_ringBuffer_mutex, portMAX_DELAY);
}

void UART_ringBuffer_mutex_give() {
	xSemaphoreGive(UART_ringBuffer_mutex);
}

void UART_parser_init() {
	ringBuffer_init(&ring_buffer, 13);
	IDLE_semaphore = xSemaphoreCreateBinary();
	UART_ringBuffer_mutex = xSemaphoreCreateMutex();
	ESP_UART_mutex = xSemaphoreCreateMutex();
	xTaskCreate(IDLE_ISR_handler_task, "IDLE_handler", 1024 * 4, NULL, osPriorityRealtime, NULL);
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
	bool ret_val = false;

	vTaskDelay(MIN_TIMEOUT_BETWEEN_SEND_MS / portTICK_PERIOD_MS);

	xSemaphoreTake(UART_ringBuffer_mutex, portMAX_DELAY);
	ringBuffer_clear(&ring_buffer);

	pkts_recieved = 0;
	uint16_t responce_len = strlen("OK - cmd accepted: ");
	memcpy(tx_buff, "OK - cmd accepted: ", responce_len);
	uint16_t cmd_len = add_cmd_to_buff(cmd_data, (char *)(tx_buff + responce_len));

	xSemaphoreTake(ESP_UART_mutex, portMAX_DELAY);
	HAL_UART_Transmit(&ESP32_AT_UART_Port, tx_buff + responce_len, cmd_len, 100);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
	xSemaphoreGive(ESP_UART_mutex);

	xSemaphoreGive(UART_ringBuffer_mutex);

	uint32_t curr_time = HAL_GetTick();
	while ((HAL_GetTick() - curr_time) < timeout_ms) {
		if (pkts_recieved) {
			pkts_recieved = 0;
			xSemaphoreTake(UART_ringBuffer_mutex, portMAX_DELAY);
			if (ringBuffer_findSequence(&ring_buffer, tx_buff, responce_len + cmd_len) != -1) ret_val = true;
			xSemaphoreGive(UART_ringBuffer_mutex);
		}
		if (ret_val) break;
	}

	return ret_val;
}

void send_cmd_without_check(cmd_data_t cmd_data) {
	vTaskDelay(MIN_TIMEOUT_BETWEEN_SEND_MS / portTICK_PERIOD_MS);

	uint16_t cmd_len = add_cmd_to_buff(cmd_data, (char *)tx_buff);

	xSemaphoreTake(ESP_UART_mutex, portMAX_DELAY);
	HAL_UART_Transmit(&ESP32_AT_UART_Port, tx_buff, cmd_len, 500);
	HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_AT_UART_Port, rx_buff, RX_BUFF_SIZE);
	__HAL_DMA_DISABLE_IT(&ESP32_UART_DMA_LINE, DMA_IT_HT);
	xSemaphoreGive(ESP_UART_mutex);
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
