#ifndef	_AT24CXX_H
#define	_AT24CXX_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/********************************* Configuration Start *********************************/
#define		_EEPROM_SIZE_KBIT		16

#define     _EEPROM_I2C 			hi2c1
extern I2C_HandleTypeDef _EEPROM_I2C;

#define		_EEPROM_ADDRESS			0xA0
#define		_EEPROM_USE_WP_PIN		0

/* The AT24CXX has a hardware data protection scheme that allows the
user to write protect the whole memory when the WP pin is at VCC. */

#if (_EEPROM_USE_WP_PIN == 1)
#define		_EEPROM_WP_GPIO			WP_GPIO_Port
#define		_EEPROM_WP_PIN			WP_Pin
#endif

/********************************* Configuration End *********************************/

bool at24_isConnected(void);
bool at24_write(uint16_t address, uint8_t *data, size_t lenInBytes);
bool at24_read(uint16_t address, uint8_t *data, size_t lenInBytes);
bool at24_eraseChip(void);

#endif
