/*
 * EEPROM.h
 *
 *  Created on: Mar 23, 2024
 *      Author: nikit
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "stdint.h"
#include "stdbool.h"
#include "AT24Cxx.h"

#define EEPROM_INIT_NUM 0
#define EEPROM_INIT_ADDR 0
#define EEPROM_SETTINGS_ADDR 1

typedef struct {
	uint32_t pcap_counter;
	uint32_t hccapx_counter;
	uint32_t hc22000_hnd_counter;
	uint32_t hc22000_pmk_counter;
} eeprom_settings_t;

bool eeprom_init(void);
bool eeprom_read_settings(eeprom_settings_t *eeprom_settings);
bool eeprom_write_settings(eeprom_settings_t *eeprom_settings);
uint16_t get_eeprom_settings_end_addr(void);

#endif /* INC_EEPROM_H_ */
