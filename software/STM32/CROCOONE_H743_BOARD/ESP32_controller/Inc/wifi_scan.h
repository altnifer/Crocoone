/*
 * wifi_scan.h
 *
 *  Created on: Jun 27, 2024
 *      Author: nikit
 */

#ifndef INC_WIFI_SCAN_H_
#define INC_WIFI_SCAN_H_

#include "stdbool.h"
#include "stdint.h"
#include "cmsis_os.h"

#define SCAN_PACKET_HEADER 		"total AP scanned "
#define SCAN_PACKET_HEADER_LEN 	17

#define SCAN_RESPONCE_TIME_MS 7000

void ESP32_scan_wifi(TaskHandle_t *main_task_handle);

uint16_t get_AP_count();

char **get_AP_list();

int get_target();

#endif /* INC_WIFI_SCAN_H_ */
