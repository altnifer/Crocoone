/*
 * SD_card.h
 *
 *  Created on: Jun 5, 2024
 *      Author: nikit
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "fatfs.h"
#include "ring_buffer.h"
#include <stdbool.h>
#include <stdint.h>

#define WRITE_TIMEOUT_MS 250

FRESULT SD_setup(char * file_name, char * dir_name, uint8_t pkt_size);

void SD_unsetup();

bool SD_write_from_ringBuff();

bool SD_end_write_from_ringBuff();

bool copy_ringBuff_to_SD_buff(ring_buffer_t *buffer_source, uint16_t bytes_to_copy);

#endif /* INC_SD_CARD_H_ */
