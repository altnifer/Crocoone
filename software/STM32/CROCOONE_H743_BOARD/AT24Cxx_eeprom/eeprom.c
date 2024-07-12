/*
 * EEPROM.c
 *
 *  Created on: Mar 23, 2024
 *      Author: nikit
 */
#include "eeprom.h"
#include "AT24Cxx.h"

bool eeprom_init(void) {
	if (!at24_isConnected())
		return false;

	uint8_t eeprom_init;
	uint32_t buff;

	if (!at24_read(EEPROM_INIT_ADDR, &eeprom_init, 1))
		return false;

	if (eeprom_init != EEPROM_INIT_NUM) {
		eeprom_init = EEPROM_INIT_NUM;
		if (!at24_write(EEPROM_INIT_ADDR, &eeprom_init, 1))
			return false;

		buff = 0;
		if (!at24_write(EEPROM_PMKID_ADDR, (uint8_t *)&buff, 4))
			return false;

		if (!at24_write(EEPROM_MONITOR_PCAP_ADDR, (uint8_t *)&buff, 4))
			return false;

		if (!at24_write(EEPROM_HANDSHAKE_PCAP_ADDR, (uint8_t *)&buff, 4))
			return false;

		if (!at24_write(EEPROM_HANDSHAKE_HC22000_ADDR, (uint8_t *)&buff, 4))
			return false;
	}

	return true;
}

bool eeprom_read_set(uint16_t adress, uint32_t *set) {
	if (!at24_read(adress, (uint8_t *)set, 4))
		return false;
	return true;
}

bool eeprom_write_set(uint16_t adress, uint32_t *set) {
	if (!at24_write(adress, (uint8_t *)set, 4))
		return false;
	return true;
}
