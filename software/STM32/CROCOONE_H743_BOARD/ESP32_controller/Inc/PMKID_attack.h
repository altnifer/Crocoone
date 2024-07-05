/*
 * PMKID_attack.h
 *
 *  Created on: Jun 26, 2024
 *      Author: nikit
 */

#ifndef INC_PMKID_ATTACK_H_
#define INC_PMKID_ATTACK_H_

#include "stdint.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "semphr.h"

#define SSID_PACKET_HEADER      	"Current SSID: "
#define SSID_PACKET_HEADER_LEN  	14

#define PMKID_PACKET_HEADER     	"PMKID data: "
#define PMKID_PACKET_HEADER_LEN		12

#define PMKID_FOLDER_NAME "handshakes"
#define PMKID_FILE_NAME "pmkid_data"


void ESP32_start_pmkid_attack(TaskHandle_t *main_task_handle);


#endif /* INC_PMKID_ATTACK_H_ */
