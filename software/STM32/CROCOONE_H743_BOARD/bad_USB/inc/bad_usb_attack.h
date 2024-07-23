/*
 * bad_usb_attack.h
 *
 *  Created on: Jul 16, 2024
 *      Author: nikit
 */

#ifndef INC_BAD_USB_ATTACK_H_
#define INC_BAD_USB_ATTACK_H_

#include "stdbool.h"
#include "stdint.h"
#include "cmsis_os.h"
#include "semphr.h"

#define BAD_USB_DIRR_NAME "usb_scripts"
#define BAD_USB_FILE_EXTENSION ".txt"

void USB_attack_start(TaskHandle_t *main_task_handle);

#endif /* INC_BAD_USB_ATTACK_H_ */
