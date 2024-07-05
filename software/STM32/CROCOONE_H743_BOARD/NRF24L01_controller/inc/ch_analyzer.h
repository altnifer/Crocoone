/*
 * ch_analyzer.h
 *
 *  Created on: Jun 16, 2024
 *      Author: nikit
 */

#ifndef CH_ANALYZER_H_
#define CH_ANALYZER_H_

#include "nrf24l01.h"
#include "cmsis_os.h"


void nrf_init();

void NRF24_ch_analyzer(TaskHandle_t *main_task_handle);


#endif /* INC_CH_ANALYZER_H_ */
