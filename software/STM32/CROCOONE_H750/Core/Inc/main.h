/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DIV_ENABLE_Pin GPIO_PIN_3
#define DIV_ENABLE_GPIO_Port GPIOE
#define CS_NRF_Pin GPIO_PIN_4
#define CS_NRF_GPIO_Port GPIOC
#define CE_NRF_Pin GPIO_PIN_5
#define CE_NRF_GPIO_Port GPIOC
#define LCD_LIGHT_Pin GPIO_PIN_11
#define LCD_LIGHT_GPIO_Port GPIOE
#define CARD_DETECT_Pin GPIO_PIN_11
#define CARD_DETECT_GPIO_Port GPIOB
#define BUTTON5_Pin GPIO_PIN_8
#define BUTTON5_GPIO_Port GPIOD
#define BUTTON5_EXTI_IRQn EXTI9_5_IRQn
#define BUTTON4_Pin GPIO_PIN_9
#define BUTTON4_GPIO_Port GPIOD
#define BUTTON4_EXTI_IRQn EXTI9_5_IRQn
#define BUTTON3_Pin GPIO_PIN_10
#define BUTTON3_GPIO_Port GPIOD
#define BUTTON3_EXTI_IRQn EXTI15_10_IRQn
#define BUTTON2_Pin GPIO_PIN_11
#define BUTTON2_GPIO_Port GPIOD
#define BUTTON2_EXTI_IRQn EXTI15_10_IRQn
#define BUTTON1_Pin GPIO_PIN_12
#define BUTTON1_GPIO_Port GPIOD
#define BUTTON1_EXTI_IRQn EXTI15_10_IRQn
#define LCD_CS_Pin GPIO_PIN_13
#define LCD_CS_GPIO_Port GPIOD
#define LCD_DC_Pin GPIO_PIN_14
#define LCD_DC_GPIO_Port GPIOD
#define LCD_RST_Pin GPIO_PIN_15
#define LCD_RST_GPIO_Port GPIOD
#define POWER_ON_Pin GPIO_PIN_7
#define POWER_ON_GPIO_Port GPIOD
#define ESP_RESET_Pin GPIO_PIN_5
#define ESP_RESET_GPIO_Port GPIOB
#define AT24xx_WP_Pin GPIO_PIN_6
#define AT24xx_WP_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
