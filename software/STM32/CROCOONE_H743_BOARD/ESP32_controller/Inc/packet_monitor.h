/*
 * packet_monitor.h
 *
 *  Created on: May 22, 2024
 *      Author: nikit
 */

#ifndef PACKET_MONITOR_H
#define PACKET_MONITOR_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "semphr.h"

#define PKT_TIME_SEC 60
#define MAX_WIFI_CH  13
#define MIN_WIFI_CH  1

#define MONITOR_FOLDER_NAME "monitor"
#define MONITOR_FILE_NAME "packets"


void ESP32_start_packet_monitor(TaskHandle_t *main_task_handle);


#endif /* PACKET_MONITOR_H_ */
