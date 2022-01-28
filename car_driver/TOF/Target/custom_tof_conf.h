/**
 ******************************************************************************
 * @file    custom_tof_conf.h
 * @author  IMG SW Application Team
 * @brief   This file contains definitions of the TOF components bus interfaces
 *          for custom boards
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CUSTOM_TOF_CONF_H__
#define __CUSTOM_TOF_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo_bus.h"
#include "stm32f4xx_nucleo_errno.h"

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#define USE_CUSTOM_RANGING_VL53L5CX (1U)

#define CUSTOM_VL53L5CX_PWR_EN_PORT    GPIOC
#define CUSTOM_VL53L5CX_PWR_EN_PIN     GPIO_PIN_0

#define CUSTOM_VL53L5CX_I2C_RST_PORT    GPIOB
#define CUSTOM_VL53L5CX_I2C_RST_PIN     GPIO_PIN_5

#define CUSTOM_VL53L5CX_LPn_PORT    GPIOB
#define CUSTOM_VL53L5CX_LPn_PIN     GPIO_PIN_0

#define CUSTOM_VL53L5CX_I2C_Init      BSP_I2C1_Init
#define CUSTOM_VL53L5CX_I2C_DeInit    BSP_I2C1_DeInit
#define CUSTOM_VL53L5CX_I2C_WriteReg  BSP_I2C1_WriteReg16
#define CUSTOM_VL53L5CX_I2C_ReadReg   BSP_I2C1_ReadReg16

#ifdef __cplusplus
}
#endif

#endif /* __CUSTOM_TOF_CONF_H__*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
