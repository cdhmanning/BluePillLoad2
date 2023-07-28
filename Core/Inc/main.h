/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
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
#include <stdint.h>
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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void systick_hook(void);
uint32_t get_tick(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ON_LED_Pin GPIO_PIN_13
#define ON_LED_GPIO_Port GPIOC
#define DEBUG0_Pin GPIO_PIN_14
#define DEBUG0_GPIO_Port GPIOC
#define DEBUG1_Pin GPIO_PIN_15
#define DEBUG1_GPIO_Port GPIOC
#define ROTB0_Pin GPIO_PIN_0
#define ROTB0_GPIO_Port GPIOB
#define ROTB1_Pin GPIO_PIN_1
#define ROTB1_GPIO_Port GPIOB
#define ROTBPRESS_Pin GPIO_PIN_2
#define ROTBPRESS_GPIO_Port GPIOB
#define ROTAPRESS_Pin GPIO_PIN_12
#define ROTAPRESS_GPIO_Port GPIOB
#define ROTA1_Pin GPIO_PIN_13
#define ROTA1_GPIO_Port GPIOB
#define ROTA0_Pin GPIO_PIN_14
#define ROTA0_GPIO_Port GPIOB
#define PBUTTONA_Pin GPIO_PIN_15
#define PBUTTONA_GPIO_Port GPIOB
#define PBUTTONB_Pin GPIO_PIN_8
#define PBUTTONB_GPIO_Port GPIOA
#define INC_DEC_LED_Pin GPIO_PIN_15
#define INC_DEC_LED_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

#define ON_LED 		ON_LED_GPIO_Port, ON_LED_Pin
#define INC_DEC_LED INC_DEC_LED_GPIO_Port, INC_DEC_LED_Pin

#define ROTA0		ROTA0_GPIO_Port, ROTA0_Pin
#define ROTA1		ROTA1_GPIO_Port, ROTA1_Pin
#define ROTAPRESS 	ROTAPRESS_GPIO_Port, ROTAPRESS_Pin

#define ROTB0		ROTB0_GPIO_Port, ROTB0_Pin
#define ROTB1		ROTB1_GPIO_Port, ROTB1_Pin
#define ROTBPRESS	ROTBPRESS_GPIO_Port, ROTBPRESS_Pin

#define PBUTTONA	PBUTTONA_GPIO_Port, PBUTTONA_Pin
#define PBUTTONB	PBUTTONB_GPIO_Port, PBUTTONB_Pin

#define DEBUG0		DEBUG0_GPIO_Port, DEBUG0_Pin
#define DEBUG1		DEBUG1_GPIO_Port, DEBUG1_Pin

/* HAL peripheral handles declared in main.c */
extern ADC_HandleTypeDef hadc1;

extern CAN_HandleTypeDef hcan;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern TIM_HandleTypeDef htim2;

extern UART_HandleTypeDef huart1;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
