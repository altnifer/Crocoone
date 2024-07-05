/*
 * SD_card.c
 *
 *  Created on: Jun 5, 2024
 *      Author: nikit
 */
#include "SD_card.h"
#include "ring_buffer.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "string.h"

#define SD_MIN_PKT_SIZE 5
#define SD_MAX_PKT_SIZE 12

static FATFS SDFatFs;
static FIL MyFile;
static bool SD_setup_flag = false;
static uint16_t bytesWrittenSum;
static uint16_t curr_sync_sum;
static uint16_t curr_pkt_size;
static ring_buffer_t SD_ring_buffer = {};

FRESULT SD_mount_card();
FRESULT SD_unmount_card();

FRESULT SD_mount_card() {
	FRESULT status;
	uint8_t attempt = 0;
	while(attempt < 255) {
		MX_FATFS_Init();
		status = f_mount(&SDFatFs, SDPath, 1);
		if(status == FR_OK){
			break;
		} else {
			SD_unmount_card();
			attempt++;
		}
	}
	return status;
}

FRESULT SD_unmount_card() {
	FRESULT res = f_mount(NULL, SDPath, 1);
	MX_FATFS_DeInit();
	return res;
}

FRESULT SD_setup(char * file_name, char * dir_name, uint8_t pkt_size) {
	if (pkt_size > SD_MAX_PKT_SIZE || pkt_size < SD_MIN_PKT_SIZE || file_name == NULL)
		return FR_INVALID_PARAMETER;

	FRESULT res;
	char file_path[256]; //255 - max LFN file name length
	uint16_t dir_name_len = 0;
	uint16_t file_name_len = 0;

	res = SD_mount_card();
	if (res != FR_OK) return res;

	if (dir_name != NULL) {
		res = f_mkdir(dir_name);
		if (res != FR_OK && res != FR_EXIST) {
			SD_unmount_card();
			return res;
		}

		dir_name_len = strlen(dir_name);
		memcpy(file_path, dir_name, dir_name_len);
		file_path[dir_name_len] = '/';
		dir_name_len++;
	}

	file_name_len = strlen(file_name);
	memcpy(file_path + dir_name_len, file_name, file_name_len);
	file_path[file_name_len + dir_name_len] = '\0';
	res = f_open(&MyFile, file_path, FA_CREATE_NEW | FA_WRITE);
	if (res != FR_OK) {
		SD_unmount_card();
		return res;
	}

	ringBuffer_init(&SD_ring_buffer, (uint8_t)pkt_size + 1);
	curr_sync_sum = 1 << (pkt_size + 2);
	curr_pkt_size = 1 << pkt_size;
	bytesWrittenSum = 0;
	SD_setup_flag = true;

	return FR_OK;
}

void SD_unsetup() {
	if (SD_setup_flag) {
		f_close(&MyFile);
		SD_unmount_card();
		ringBuffer_deInit(&SD_ring_buffer);
	}
	SD_setup_flag = false;
}

bool SD_write_from_ringBuff() {
	uint16_t bytes_to_read = ringBuffer_getBytesToRead(&SD_ring_buffer);
	if (bytes_to_read < curr_pkt_size) return true;

	for (uint8_t i = 0; i < (bytes_to_read / curr_pkt_size); i++) {
		uint32_t byteswritten = 0;
		FRESULT res = f_write(&MyFile, SD_ring_buffer.buffer + SD_ring_buffer.head_indx, curr_pkt_size, (void *)&byteswritten);
		if (res != FR_OK || byteswritten != curr_pkt_size) return false;

		bytesWrittenSum += curr_pkt_size;

		if (bytesWrittenSum >= curr_sync_sum) {
			if (f_sync(&MyFile) != FR_OK) return false;
			bytesWrittenSum = 0;
		}
		ringBuffer_clearNBytes(&SD_ring_buffer, curr_pkt_size);
		osDelay(WRITE_TIMEOUT_MS / portTICK_PERIOD_MS);
	}

	return true;
}

bool SD_end_write_from_ringBuff() {
	int32_t bytes_to_read = (int32_t)ringBuffer_getBytesToRead(&SD_ring_buffer);
	if (bytes_to_read < 0) return true;

	while (bytes_to_read > 0) {
		uint32_t byteswritten = 0;
		FRESULT res = f_write(
				&MyFile,
				SD_ring_buffer.buffer + SD_ring_buffer.head_indx,
				(bytes_to_read > curr_pkt_size) ? (curr_pkt_size) : (bytes_to_read),
				(void *)&byteswritten
				);
		if (res != FR_OK || byteswritten != bytes_to_read) return false;
		ringBuffer_clearNBytes(&SD_ring_buffer, byteswritten);
		bytes_to_read -= byteswritten;
		osDelay(WRITE_TIMEOUT_MS / portTICK_PERIOD_MS);
	}
	ringBuffer_clear(&SD_ring_buffer);

	return true;
}

bool copy_ringBuff_to_SD_buff(ring_buffer_t *buffer_source, uint16_t bytes_to_copy) {
    if (buffer_source->bytes_to_read < bytes_to_copy) {
        return false;
    }

    if (bytes_to_copy > SD_ring_buffer.bytes_to_write) {
    	return false;
    }

    for(uint16_t i = 0; i < bytes_to_copy; i++) {
    	SD_ring_buffer.buffer[(SD_ring_buffer.tail_indx + i) & SD_ring_buffer.buffer_mask] = buffer_source->buffer[(buffer_source->head_indx + i) & buffer_source->buffer_mask];
    }

    SD_ring_buffer.tail_indx = (SD_ring_buffer.tail_indx + bytes_to_copy) & SD_ring_buffer.buffer_mask;
    SD_ring_buffer.bytes_to_read += bytes_to_copy;
    SD_ring_buffer.bytes_to_write -= bytes_to_copy;

    return true;
}
