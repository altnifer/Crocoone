/*
 * pcap_parser.c
 *
 *  Created on: Jul 11, 2024
 *      Author: nikit
 */
#include "pcap_parser.h"
#include "uart_controller.h"
#include "SD_card.h"
#include "cmsis_os.h"
#include "semphr.h"
#include <stdio.h>

static uint32_t pkts_sum = 0;
static bool parser_task_flag = false;
static bool parser_use_sd_flag = false;
static bool SD_write_error = false;

void pcap_parser_task(void *args);

void pcap_parser_start() {
	SD_write_error = false;
	parser_task_flag = true;
	pkts_sum = 0;
	xTaskCreate(pcap_parser_task, "pcap_parser", 1024 * 4, NULL, osPriorityNormal2, NULL);
}

void pcap_parser_stop() {
	parser_task_flag = false;
}

void pcap_parser_set_use_sd_flag(bool flag) {
	parser_use_sd_flag = flag;
}

uint32_t get_pkts_sum() {
	uint32_t ret_val = pkts_sum;
	pkts_sum = 0;
	return ret_val;
}

bool check_sd_write_error() {
	return SD_write_error;
}

void pcap_parser_task(void *args) {
	static uint8_t packet_header[FRAME_HEADER_LEN + 1];
	ring_buffer_t *uart_ring_buffer = get_ring_buff();
	UART_ringBuffer_mutex_take();
	ringBuffer_clear(uart_ring_buffer);
	UART_ringBuffer_mutex_give();

	while (parser_task_flag) {
		osDelay(50 / portTICK_PERIOD_MS);

		UART_ringBuffer_mutex_take();
		for (uint16_t i = 0; i < get_recieved_pkts_count(); i++) {
			int32_t packet_start = ringBuffer_findSequence(uart_ring_buffer, (uint8_t *)"PCAP data (", 11);
			if (packet_start == -1) {
				ringBuffer_clear(uart_ring_buffer);
				break;
			}

			ringBuffer_clearNBytes(uart_ring_buffer, (uint16_t)packet_start);

			ringBuffer_get(uart_ring_buffer, packet_header, FRAME_HEADER_LEN);

			ringBuffer_clearNBytes(uart_ring_buffer, FRAME_HEADER_LEN);

			uint16_t pkts = 0, bytes = 0;
			packet_header[FRAME_HEADER_LEN] = '\0';
			if (!sscanf((char *)packet_header, "PCAP data (%hu pkts, %hu bytes): ", &pkts, &bytes)) break;

			pkts_sum += pkts;

			if (parser_use_sd_flag)
				copy_ringBuff_to_SD_buff(uart_ring_buffer, bytes);
		}
		UART_ringBuffer_mutex_give();

		if (parser_use_sd_flag) {
			if ((SD_write_error = !SD_write_from_ringBuff()) == true)
				break;
		}
	}

	if (parser_use_sd_flag) {
		if (!SD_write_error)
			SD_write_error = !SD_end_write_from_ringBuff();
		SD_unsetup_write_from_ringBuff();
	}

	parser_use_sd_flag = false;
	parser_task_flag = false;
	vTaskDelete(NULL);
}
