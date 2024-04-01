/*
 * EEPROM.c
 *
 *  Created on: Mar 23, 2024
 *      Author: nikit
 */
#include "EEPROM.h"

uint16_t eeprom_settings_end_addr = 0;

bool eeprom_init(void) {
	if (!at24_isConnected())
		return false;

	uint8_t eeprom_init = 0xFF;
	if (!at24_read(EEPROM_INIT_ADDR, &eeprom_init, 1))
		return false;
	if (eeprom_init != EEPROM_INIT_NUM) {
		eeprom_init = EEPROM_INIT_NUM;
		eeprom_settings_t eeprom_settings = {
				.pcap_counter = 1,
				.hccapx_counter = 1,
				.hc22000_hnd_counter = 1,
				.hc22000_pmk_counter = 1
		};
		if (!at24_write(EEPROM_SETTINGS_ADDR, (uint8_t *)&eeprom_settings, sizeof(eeprom_settings_t)))
			return false;
		if (!at24_write(EEPROM_INIT_ADDR, &eeprom_init, 1))
			return false;
	}

	eeprom_settings_end_addr = EEPROM_SETTINGS_ADDR + sizeof(eeprom_settings_t);
	return true;
}

bool eeprom_read_settings(eeprom_settings_t *eeprom_settings) {
	if (!at24_read(EEPROM_SETTINGS_ADDR, (uint8_t *)eeprom_settings, sizeof(eeprom_settings_t)))
		return false;
	return true;
}

bool eeprom_write_settings(eeprom_settings_t *eeprom_settings) {
	if (!at24_write(EEPROM_SETTINGS_ADDR, (uint8_t *)eeprom_settings, sizeof(eeprom_settings_t)))
		return false;
	return true;
}

uint16_t get_eeprom_settings_end_addr(void) {
	return eeprom_settings_end_addr;
}
