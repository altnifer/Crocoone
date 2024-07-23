/*
 * SD_card.h
 *
 *  Created on: Jun 5, 2024
 *      Author: nikit
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "fatfs.h"
#include <stdbool.h>
#include <stdint.h>
#include "adt.h"

#define WRITE_TIMEOUT_MS 250

FRESULT SD_mount_card();

FRESULT SD_unmount_card();

FRESULT SD_mkpath(const char *path);

FRESULT SD_scan_files(char *path, char *extension_filter, list_t **out_list);

FRESULT SD_open_file_to_read(char *path);

char *SD_f_gets(char *buff, int buff_len);

int SD_f_eof();

FRESULT SD_close_current_file();

FRESULT SD_setup_write_from_ringBuff(char *path, char *file, uint8_t pkt_size);

void SD_unsetup_write_from_ringBuff();

bool SD_write_from_ringBuff();

bool SD_end_write_from_ringBuff();

bool copy_ringBuff_to_SD_buff(ring_buffer_t *buffer_source, uint16_t bytes_to_copy);

#endif /* INC_SD_CARD_H_ */
