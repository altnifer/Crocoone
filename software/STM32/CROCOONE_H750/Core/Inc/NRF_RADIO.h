/*
 * NRF_RADIO.h
 *
 *  Created on: Nov 5, 2023
 *      Author: nikit
 */

#ifndef INC_NRF_RADIO_H_
#define INC_NRF_RADIO_H_

#include "SETTINGS.h"
#include "stm32h7xx_hal.h"
#include "button.h"
#include "GFX_FUNCTIONS.h"
#include "nrf24l01.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#define CHANNELS_COUNT 129
#define NUW_REPS 100
#define PLOT_LINE_COUNT 48

void Radio_Init();

void NRF_ChannelAnalyzer_loop();

void NRF_Jammer();

#endif /* INC_NRF_RADIO_H_ */
