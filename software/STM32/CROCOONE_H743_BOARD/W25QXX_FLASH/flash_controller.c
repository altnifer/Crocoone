/*
 * flash_controller.c
 *
 *  Created on: Jul 27, 2024
 *      Author: nikit
 */
#include "flash_controller.h"
#include <string.h>

static file_info_t file_info;

HAL_StatusTypeDef flash_init() {
	HAL_StatusTypeDef status = HAL_OK;

	status = CSP_QUADSPI_Init();
	if (status != HAL_OK)
		return status;

	status = CSP_QSPI_Read((uint8_t *)&file_info, FILE_INFO_SECTOR, sizeof(file_info_t));
	if (status != HAL_OK)
		return status;

	if (file_info.init_num != INIT_NUM) {
		status = CSP_QSPI_EraseSector(FILE_INFO_SECTOR, FILE_INFO_SECTOR + MEMORY_SECTOR_SIZE);
		if (status != HAL_OK)
			return status;

		memset(&file_info, 0, sizeof(file_info_t));
		file_info.init_num = INIT_NUM;
		status = CSP_QSPI_WriteMemory((uint8_t *)&file_info, FILE_INFO_SECTOR, sizeof(file_info_t));
		if (status != HAL_OK)
			return status;
	}

	return status;
}

HAL_StatusTypeDef flash_write_file_info() {
	HAL_StatusTypeDef status = HAL_OK;

	status = CSP_QSPI_EraseSector(FILE_INFO_SECTOR, FILE_INFO_SECTOR + MEMORY_SECTOR_SIZE);
	if (status != HAL_OK)
		return status;

	status = CSP_QSPI_WriteMemory((uint8_t *)&file_info, FILE_INFO_SECTOR, sizeof(file_info_t));

	return status;
}

file_info_t *flash_read_file_info() {
	if (CSP_QSPI_Read((uint8_t *)&file_info, FILE_INFO_SECTOR, sizeof(file_info_t)) != HAL_OK)
		return NULL;
	return &file_info;
}
