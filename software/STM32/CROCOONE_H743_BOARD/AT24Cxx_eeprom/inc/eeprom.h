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

#define EEPROM_INIT_NUM 0xAA
#define EEPROM_INIT_ADDR 0
#define	EEPROM_PMKID_ADDR 1
#define EEPROM_MONITOR_PCAP_ADDR 5
#define EEPROM_HANDSHAKE_PCAP_ADDR 9
#define EEPROM_HANDSHAKE_HC22000_ADDR 13

bool eeprom_init();
bool eeprom_read_set(uint16_t adress, uint32_t *set);
bool eeprom_write_set(uint16_t adress, uint32_t *set);

#endif /* INC_EEPROM_H_ */
