#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "stm32h7xx_hal.h"
#include <malloc.h>
#include <string.h>
#include <stdbool.h>

extern TIM_HandleTypeDef htim2;
#define BUTTON_TIM htim2

#define DEBOUNCE_TIME 200
#define DOUBLE_CLICK_TIME 400

typedef enum
{
    NO_CLICK,
    SINGLE_CLICK,
    HOLD_CLICK,
    DOUBLE_CLICK,
	HOLD
} eButtonEvent ;

typedef struct
{
	eButtonEvent ButtonEvent;
	GPIO_TypeDef* eButtonPort;
	uint16_t eButtonPin;
	bool ButtonLevelHigh;
	bool DoublePending;
	bool HoldPending;
	uint32_t ButtonTime;
	uint16_t eButtonNum;
} eButton ;


void ButtonsInit(uint16_t ButtonCount);

void AddButton(uint16_t ButtonNum, GPIO_TypeDef* ButtonPort, uint16_t ButtonPin);

eButtonEvent GetButtonState(uint16_t ButtonNum);

void ClearAllButtons();

#endif /* __BUTTON_H__ */
