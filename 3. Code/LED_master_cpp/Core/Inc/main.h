/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define DEBUG_ON
extern char debug_message[];
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
#define STATUS_Pin GPIO_PIN_13
#define STATUS_GPIO_Port GPIOC
#define CH1_Pin GPIO_PIN_0
#define CH1_GPIO_Port GPIOA
#define CH2_Pin GPIO_PIN_1
#define CH2_GPIO_Port GPIOA
#define CH3_Pin GPIO_PIN_2
#define CH3_GPIO_Port GPIOA
#define BATTERY_LEVEL_Pin GPIO_PIN_3
#define BATTERY_LEVEL_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_4
#define SD_CS_GPIO_Port GPIOA
#define SD_SCK_Pin GPIO_PIN_5
#define SD_SCK_GPIO_Port GPIOA
#define SD_MISO_Pin GPIO_PIN_6
#define SD_MISO_GPIO_Port GPIOA
#define SD_MOSI_Pin GPIO_PIN_7
#define SD_MOSI_GPIO_Port GPIOA
#define SD_CD_Pin GPIO_PIN_0
#define SD_CD_GPIO_Port GPIOB
#define BUTTON_LEFT_Pin GPIO_PIN_1
#define BUTTON_LEFT_GPIO_Port GPIOB
#define BUTTON_OK_Pin GPIO_PIN_10
#define BUTTON_OK_GPIO_Port GPIOB
#define BUTTON_RIGHT_Pin GPIO_PIN_11
#define BUTTON_RIGHT_GPIO_Port GPIOB
#define RADIO_CS_Pin GPIO_PIN_12
#define RADIO_CS_GPIO_Port GPIOB
#define RADIO_SCK_Pin GPIO_PIN_13
#define RADIO_SCK_GPIO_Port GPIOB
#define RADIO_MISO_Pin GPIO_PIN_14
#define RADIO_MISO_GPIO_Port GPIOB
#define RADIO_MOSI_Pin GPIO_PIN_15
#define RADIO_MOSI_GPIO_Port GPIOB
#define CH4_Pin GPIO_PIN_4
#define CH4_GPIO_Port GPIOB
#define RADIO_CE_Pin GPIO_PIN_5
#define RADIO_CE_GPIO_Port GPIOB
#define RADIO_IRQ_Pin GPIO_PIN_6
#define RADIO_IRQ_GPIO_Port GPIOB
#define RADIO_IRQ_EXTI_IRQn EXTI9_5_IRQn
#define CH5_Pin GPIO_PIN_7
#define CH5_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define CSN_GPIO_Port RADIO_CS_GPIO_Port
#define CSN_Pin RADIO_CS_Pin
#define CE_GPIO_Port RADIO_CE_GPIO_Port
#define CE_Pin RADIO_CE_Pin
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
