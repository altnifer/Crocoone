#include "button.h"

eButton * ButtonsP = NULL;
uint16_t MaxButtonCount;
uint32_t debounce_millis = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (HAL_GetTick() - debounce_millis > DEBOUNCE_TIME) {
		for (uint16_t i = 0; i < MaxButtonCount; i++) {
			if (GPIO_Pin == (ButtonsP + i)->eButtonPin) {
				(ButtonsP + i)->ButtonLevelHigh = 1;
			}
		}
		debounce_millis = HAL_GetTick();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == BUTTON_TIM.Instance) //check if the interrupt comes from TIM2
	{
		for (uint16_t i = 0; i < MaxButtonCount; i++) {

			if ((ButtonsP + i)->ButtonEvent != NO_CLICK) break;

			if (!(ButtonsP + i)->HoldPending && !(ButtonsP + i)->DoublePending && (ButtonsP + i)->ButtonLevelHigh) {
				(ButtonsP + i)->DoublePending = 1; // pending
				(ButtonsP + i)->HoldPending = 1;
				(ButtonsP + i)->ButtonTime = HAL_GetTick();
				(ButtonsP + i)->ButtonLevelHigh = 0;
			}

			else if ((ButtonsP + i)->DoublePending && (ButtonsP + i)->ButtonLevelHigh && !(ButtonsP + i)->HoldPending && HAL_GetTick() - (ButtonsP + i)->ButtonTime <= DOUBLE_CLICK_TIME) {
				(ButtonsP + i)->DoublePending = 0; // double
				(ButtonsP + i)->HoldPending = 0;
				(ButtonsP + i)->ButtonLevelHigh = 0;
				(ButtonsP + i)->ButtonEvent = DOUBLE_CLICK;
			}

			else if (!(ButtonsP + i)->HoldPending && (ButtonsP + i)->DoublePending && (HAL_GetTick() - (ButtonsP + i)->ButtonTime) > DOUBLE_CLICK_TIME) {
				(ButtonsP + i)->DoublePending = 0; // single
				(ButtonsP + i)->HoldPending = 0;
				(ButtonsP + i)->ButtonLevelHigh = 0;
				(ButtonsP + i)->ButtonEvent = SINGLE_CLICK;
			}

			else if (/*!(ButtonsP + i)->ButtonLevelHigh && */(ButtonsP + i)->HoldPending && HAL_GetTick() - (ButtonsP + i)->ButtonTime > DOUBLE_CLICK_TIME) {
				(ButtonsP + i)->DoublePending = 0; // hold
				(ButtonsP + i)->ButtonLevelHigh = 0;
				(ButtonsP + i)->ButtonEvent = HOLD;
			}


			if (HAL_GPIO_ReadPin((ButtonsP + i)->eButtonPort, (ButtonsP + i)->eButtonPin) == GPIO_PIN_SET) { //GPIO_PIN_RESET - BUTTON TO HIGH
				(ButtonsP + i)->HoldPending = 0;
			}

		}
	}
}

void ButtonsInit(uint16_t ButtonCount) {
	HAL_TIM_Base_Start_IT(&BUTTON_TIM);
	ButtonsP = (eButton*) malloc(ButtonCount * sizeof(eButton));
	MaxButtonCount = ButtonCount;
}

void AddButton(uint16_t ButtonNum, GPIO_TypeDef* ButtonPort, uint16_t ButtonPin) {
	static uint16_t CurrentButton = 0;
	if (CurrentButton >= MaxButtonCount) return;
	eButton NewButton = {
			.ButtonEvent = NO_CLICK,
			.eButtonPort = ButtonPort,
			.eButtonPin = ButtonPin,
			.ButtonLevelHigh = 0,
			.DoublePending = 0,
			.HoldPending = 0,
			.ButtonTime = 0,
			.eButtonNum = ButtonNum
	};
	*(ButtonsP + CurrentButton) = NewButton;
	CurrentButton++;
}

eButtonEvent GetButtonState(uint16_t ButtonNum) {
	eButtonEvent ButtonEvent = NO_CLICK;
	for (uint16_t i = 0; i < MaxButtonCount; i++)
		if ((ButtonsP + i)->eButtonNum == ButtonNum) {
			if ((ButtonsP + i)->ButtonEvent == HOLD) {
				ButtonEvent = HOLD;
				(ButtonsP + i)->ButtonEvent = NO_CLICK;
				(ButtonsP + i)->ButtonLevelHigh = 0;
				(ButtonsP + i)->DoublePending = 0;
				break;
			} else if ((ButtonsP + i)->ButtonEvent != NO_CLICK) {
				ButtonEvent = (ButtonsP + i)->ButtonEvent;
				(ButtonsP + i)->ButtonEvent = NO_CLICK;
				(ButtonsP + i)->ButtonLevelHigh = 0;
				(ButtonsP + i)->DoublePending = 0;
				(ButtonsP + i)->HoldPending = 0;
				(ButtonsP + i)->ButtonTime = 0;
				break;
			}
		}
	return ButtonEvent;
}

void ClearAllButtons() {
	for (uint16_t i = 0; i < MaxButtonCount; i++) {
		(ButtonsP + i)->ButtonEvent = NO_CLICK;
		(ButtonsP + i)->ButtonLevelHigh = 0;
		(ButtonsP + i)->DoublePending = 0;
		(ButtonsP + i)->HoldPending = 0;
		(ButtonsP + i)->ButtonTime = 0;
	}
}
