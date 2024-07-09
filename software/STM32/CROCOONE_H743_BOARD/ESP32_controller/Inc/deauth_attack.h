/*
 * deauth_attack.h
 *
 *  Created on: Jul 7, 2024
 *      Author: nikit
 */
#ifndef INC_DEAUTH_ATTACK_H_
#define INC_DEAUTH_ATTACK_H_

#include "cmsis_os.h"
#include "semphr.h"
#include "stdbool.h"
#include "stdint.h"

void ESP32_start_deauth_attack(TaskHandle_t *main_task_handle);

#endif /* INC_DEAUTH_ATTACK_H_ */
