/**
  ******************************************************************************
  * @file    stm32g0xx_hal_irda_ex.h
  * @author  MCD Application Team
  * @brief   Header file of IRDA HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
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
#ifndef STM32G0xx_HAL_IRDA_EX_H
#define STM32G0xx_HAL_IRDA_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup IRDAEx IRDAEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup IRDAEx_Extended_Exported_Constants IRDAEx Extended Exported Constants
  * @{
  */

/** @defgroup IRDAEx_Word_Length IRDAEx Word Length
  * @{
  */
#define IRDA_WORDLENGTH_7B                  USART_CR1_M1   /*!< 7-bit long frame */
#define IRDA_WORDLENGTH_8B                  0x00000000U    /*!< 8-bit long frame */
#define IRDA_WORDLENGTH_9B                  USART_CR1_M0   /*!< 9-bit long frame */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/** @defgroup IRDAEx_Private_Macros IRDAEx Private Macros
  * @{
  */

/** @brief  Report the IRDA clock source.
  * @param  __HANDLE__ specifies the IRDA Handle.
  * @param  __CLOCKSOURCE__ output variable.
  * @retval IRDA clocking source, written in __CLOCKSOURCE__.
  */
#if defined(STM32G081xx) || defined(STM32G071xx) || defined(STM32G070xx)
#define IRDA_GETCLOCKSOURCE(__HANDLE__,__CLOCKSOURCE__)       \
  do {                                                        \
    if((__HANDLE__)->Instance == USART1)                      \
    {                                                         \
       switch(__HAL_RCC_GET_USART1_SOURCE())                  \
       {                                                      \
        case RCC_USART1CLKSOURCE_PCLK1:                       \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_PCLK1;         \
          break;                                              \
        case RCC_USART1CLKSOURCE_HSI:                         \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_HSI;           \
          break;                                              \
        case RCC_USART1CLKSOURCE_SYSCLK:                      \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_SYSCLK;        \
          break;                                              \
        case RCC_USART1CLKSOURCE_LSE:                         \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_LSE;           \
          break;                                              \
        default:                                              \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_UNDEFINED;     \
          break;                                              \
       }                                                      \
    }                                                         \
    else if((__HANDLE__)->Instance == USART2)                 \
    {                                                         \
       switch(__HAL_RCC_GET_USART2_SOURCE())                  \
       {                                                      \
        case RCC_USART2CLKSOURCE_PCLK1:                       \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_PCLK1;         \
          break;                                              \
        case RCC_USART2CLKSOURCE_HSI:                         \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_HSI;           \
          break;                                              \
        case RCC_USART2CLKSOURCE_SYSCLK:                      \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_SYSCLK;        \
          break;                                              \
        case RCC_USART2CLKSOURCE_LSE:                         \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_LSE;           \
          break;                                              \
        default:                                              \
          (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_UNDEFINED;     \
          break;                                              \
       }                                                      \
    }                                                         \
    else                                                      \
    {                                                         \
      (__CLOCKSOURCE__) = IRDA_CLOCKSOURCE_UNDEFINED;         \
    }                                                         \
  } while(0U)
#endif

/** @brief  Compute the mask to apply to retrieve the received data
  *         according to the word length and to the parity bits activation.
  * @param  __HANDLE__ specifies the IRDA Handle.
  * @retval None, the mask to apply to the associated UART RDR register is stored in (__HANDLE__)->Mask field.
  */
#define IRDA_MASK_COMPUTATION(__HANDLE__)                             \
  do {                                                                \
  if ((__HANDLE__)->Init.WordLength == IRDA_WORDLENGTH_9B)            \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == IRDA_PARITY_NONE)               \
     {                                                                \
        (__HANDLE__)->Mask = 0x01FFU ;                                \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU ;                                \
     }                                                                \
  }                                                                   \
  else if ((__HANDLE__)->Init.WordLength == IRDA_WORDLENGTH_8B)       \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == IRDA_PARITY_NONE)               \
     {                                                                \
        (__HANDLE__)->Mask = 0x00FFU ;                                \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x007FU ;                                \
     }                                                                \
  }                                                                   \
  else if ((__HANDLE__)->Init.WordLength == IRDA_WORDLENGTH_7B)       \
  {                                                                   \
     if ((__HANDLE__)->Init.Parity == IRDA_PARITY_NONE)               \
     {                                                                \
        (__HANDLE__)->Mask = 0x007FU ;                                \
     }                                                                \
     else                                                             \
     {                                                                \
        (__HANDLE__)->Mask = 0x003FU ;                                \
     }                                                                \
  }                                                                   \
  else                                                                \
  {                                                                   \
    (__HANDLE__)->Mask = 0x0000U;                                     \
  }                                                                   \
} while(0U)

/** @brief Ensure that IRDA frame length is valid.
  * @param __LENGTH__ IRDA frame length.
  * @retval SET (__LENGTH__ is valid) or RESET (__LENGTH__ is invalid)
  */
#define IS_IRDA_WORD_LENGTH(__LENGTH__) (((__LENGTH__) == IRDA_WORDLENGTH_7B) || \
                                         ((__LENGTH__) == IRDA_WORDLENGTH_8B) || \
                                         ((__LENGTH__) == IRDA_WORDLENGTH_9B))
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

#endif /* STM32G0xx_HAL_IRDA_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
