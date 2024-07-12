/*
 * handshake_attack.h
 *
 *  Created on: Jul 11, 2024
 *      Author: nikit
 */

#ifndef INC_HANDSHAKE_ATTACK_H_
#define INC_HANDSHAKE_ATTACK_H_

#define HANDSHAKE_FOLDER_NAME "handshake"
#define HANDSHAKE_PCAP_FILE_NAME "packets"
#define HANDSHAKE_HC22000_FILE_NAME "handshake"

#include "cmsis_os.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

void ESP32_start_handshake_attack(TaskHandle_t *main_task_handle);

#endif /* INC_HANDSHAKE_ATTACK_H_ */
