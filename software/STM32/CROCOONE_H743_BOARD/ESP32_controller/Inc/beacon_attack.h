/*
 * beacon_attack.h
 *
 *  Created on: Jul 10, 2024
 *      Author: nikit
 */

#ifndef INC_BEACON_ATTACK_H_
#define INC_BEACON_ATTACK_H_

#include "cmsis_os.h"
#include "semphr.h"
#include "stdbool.h"
#include "stdint.h"

void ESP32_start_beacon_attack(TaskHandle_t *main_task_handle);

#endif /* INC_BEACON_ATTACK_H_ */
