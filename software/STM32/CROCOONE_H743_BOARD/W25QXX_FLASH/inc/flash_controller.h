/*
 * flash_controller.h
 *
 *  Created on: Jul 27, 2024
 *      Author: nikit
 */

#ifndef INC_FLASH_CONTROLLER_H_
#define INC_FLASH_CONTROLLER_H_

#include "stdint.h"
#include "stdbool.h"
#include "quadspi.h"

typedef struct {
	uint8_t init_num;
	uint32_t pmkid_file_num;
	uint32_t monitor_pcap_num;
	uint32_t handshake_pcap_num;
	uint32_t handshake_hc22000_num;
	uint32_t handshake_hccapx_num;
} file_info_t;

#define FILE_INFO_SECTOR            0
#define INIT_NUM                    0x11

HAL_StatusTypeDef flash_init();
HAL_StatusTypeDef flash_write_file_info();
file_info_t *flash_read_file_info();

#endif /* INC_FLASH_CONTROLLER_H_ */
