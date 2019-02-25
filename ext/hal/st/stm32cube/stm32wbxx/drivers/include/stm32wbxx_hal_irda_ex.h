/**
  ******************************************************************************
  * @file    stm32wbxx_hal_irda_ex.h
  * @author  MCD Application Team
  * @brief   Header file of IRDA HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef STM32WBxx_HAL_IRDA_EX_H
#define STM32WBxx_HAL_IRDA_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup IRDAEx IRDAEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/** @defgroup IRDAEx_Private_Macros IRDAEx Private Macros
  * @{
  */

/** @brief  Report the IRDA clock source.
  * @param  __HANDLE__ specifies the IRDA Handle.
  * @param  __CLOCKSOURCE__ output variable.
  * @retval IRDA clocking source, written in __CLOCKSOURCE__.
  */
#define IRDA_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__)        \
  do {                                                         \
    if((__HANDLE__)->Instance == USART1)                       \
    {                                                          \
       switch(__HAL_RCC_GET_USART1_SOURCE())                   \
       {                                                       \
        case RCC_USART1CLKSOURCE_PCLK2:                        \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_PCLK2;          \
          break;                                               \
        case RCC_USART1CLKSOURCE_HSI:                          \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_HSI;            \
          break;                                               \
        case RCC_USART1CLKSOURCE_SYSCLK:                       \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_SYSCLK;         \
          break;                                               \
        case RCC_USART1CLKSOURCE_LSE:                          \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_LSE;            \
          break;                                               \
        default:                                               \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_UNDEFINED;      \
          break;                                               \
       }                                                       \
    }                                                          \
    else                                                       \
    {                                                          \
      (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_UNDEFINED;          \
    }                                                          \
  } while(0U)

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_IRDA_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
