/*
 * SD_card.c
 *
 *  Created on: Jun 5, 2024
 *      Author: nikit
 */
#include "SD_card.h"
#include "cmsis_os.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

#define SD_MIN_PKT_SIZE 5
#define SD_MAX_PKT_SIZE 12

static FATFS SDFatFs;
static FIL MyFile;
static bool SD_setup_flag = false;
static uint16_t bytesWrittenSum;
static uint16_t curr_sync_sum;
static uint16_t curr_pkt_size;
static ring_buffer_t SD_ring_buffer = {};

int32_t contains_symbol(char *symbols, uint16_t symbols_len, char symbol);
int32_t contains_symbol_reversed(char *symbols, uint16_t symbols_len, char symbol);

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

FRESULT SD_mkpath(const char *path) {
	FRESULT res;
	uint16_t path_size = strlen(path) + 1;
	char path_copy[path_size];
	memcpy(path_copy, path, path_size);

	char *path_p = path_copy;
	int32_t token_len;
	path_size--;

	while ((token_len = contains_symbol(path_p, path_size, '/')) != -1) {
		path_p[token_len] = '\0';
		res = f_mkdir(path_copy);
		if (res != FR_OK && res != FR_EXIST)
			return res;
		path_p[token_len] = '/';

		path_p += (token_len + 1);
		path_size -= (token_len + 1);
    }

	if (path_size)
		res = f_mkdir(path_copy);

	return res;
}

FRESULT scan_files_recursion(char *path, char *extension_filter, list_t **out_list) {
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int path_len;
	char *fn;

	res = f_opendir(&dir, path);

	if (res != FR_OK)
		return res;

	path_len = strlen(path);
	while(1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == '\0')
		   break;
		if (fno.fname[0] == '.')
		   continue;
		fn = fno.fname;
		if (fno.fattrib & AM_DIR) {
			sprintf(path + path_len, "/%s", fn);
			res = scan_files_recursion(path, extension_filter, out_list);
			if (res != FR_OK) break;
			path[path_len] = '\0';
		} else {
			if (extension_filter != NULL) {
				uint16_t ext_start = contains_symbol_reversed(fn, strlen(fn), '.');
				if (strcmp(fn + ext_start, extension_filter))
					continue;
			}
			sprintf(path + path_len, "/%s", fn);
			list_push_back(out_list, path, strlen(path) + 1);
			path[path_len] = '\0';
		}
	}
	return res;
}

FRESULT SD_scan_files(char *path, char *extension_filter, list_t **out_list) {
	char path_workspace[1024];
	memcpy(path_workspace, path, strlen(path) + 1);
	FRESULT res = scan_files_recursion(path_workspace, extension_filter, out_list);

	if (res == FR_NO_PATH)
		SD_mkpath(path);

	return res;
}

FRESULT SD_open_file_to_read(char *path) {
	f_open(&MyFile, path, FA_READ);
}

char *SD_f_gets(char *buff, int buff_len) {
	return f_gets(buff, buff_len, &MyFile);
}

int SD_f_eof() {
	return f_eof(&MyFile);
}

FRESULT SD_close_current_file() {
	return f_close(&MyFile);
}

FRESULT SD_setup_write_from_ringBuff(char *path, char *file, uint8_t pkt_size) {
	if (pkt_size > SD_MAX_PKT_SIZE || pkt_size < SD_MIN_PKT_SIZE || file == NULL)
		return FR_INVALID_PARAMETER;

	FRESULT res;
	uint16_t path_len = (path == NULL) ? 0 : strlen(path);
	uint16_t file_len = strlen(file);
	char file_path[path_len + file_len + 2];

	res = SD_mount_card();
	if (res != FR_OK) return res;

	if (path != NULL) {
		res = SD_mkpath(path);
		if (res != FR_OK && res != FR_EXIST) {
			SD_unmount_card();
			return res;
		}
	}

	memcpy(file_path, path, path_len);
	file_path[path_len] = '/';
	memcpy(file_path + path_len + 1, file, file_len);
	file_path[file_len + path_len + 1] = '\0';
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

void SD_unsetup_write_from_ringBuff() {
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




