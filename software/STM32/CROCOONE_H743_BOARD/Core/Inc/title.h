/*
 * title.h
 *
 *  Created on: Jun 29, 2024
 *      Author: nikit
 */

#ifndef INC_TITLE_H_
#define INC_TITLE_H_

#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include "cmsis_os.h"
#include "semphr.h"

#define BAT_DEVIDER_ADC hadc1
extern ADC_HandleTypeDef BAT_DEVIDER_ADC;

#define MAX_CHAR_IN_TITLE 10
#define TITLE_END_Y 11


void refresh_title();

void start_title_updater(SemaphoreHandle_t *LCD_mutex);

void update_title_text(char * new_title);

void update_title_SD_flag(bool flag);

void fillScreenExceptTitle();

#endif /* INC_TITLE_H_ */
