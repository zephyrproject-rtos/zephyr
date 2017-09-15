/**
  ******************************************************************************
  * @file    stm32f3xx_hal_gpio_ex.h
  * @author  MCD Application Team
  * @brief   Header file of GPIO HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F3xx_HAL_GPIO_EX_H
#define __STM32F3xx_HAL_GPIO_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @defgroup GPIOEx GPIOEx
  * @brief GPIO Extended HAL module driver
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup GPIOEx_Exported_Constants GPIOEx Exported Constants
  * @{
  */

/** @defgroup GPIOEx_Alternate_function_selection GPIOEx Alternate function selection
  * @{
  */

#if defined (STM32F302xC)
/*---------------------------------- STM32F302xC ------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 14 selection
  */

#define GPIO_AF14_USB           ((uint8_t)0x0EU)  /* USB Alternate Function mapping */
/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0EU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F302xC */

#if defined (STM32F303xC)
/*---------------------------------- STM32F303xC ------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM8           ((uint8_t)0x02U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM8          ((uint8_t)0x03U)  /* TIM8 Alternate Function mapping  */
#define GPIO_AF3_COMP7         ((uint8_t)0x03U)  /* COMP7 Alternate Function mapping */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM8          ((uint8_t)0x04U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_TIM8          ((uint8_t)0x05U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_TIM8          ((uint8_t)0x06U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP3         ((uint8_t)0x07U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF7_COMP5         ((uint8_t)0x07U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP3         ((uint8_t)0x08U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP5         ((uint8_t)0x08U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM8          ((uint8_t)0x09U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM8           ((uint8_t)0xAU)  /* TIM8 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF11_TIM8           ((uint8_t)0x0BU)  /* TIM8 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 14 selection
  */

#define GPIO_AF14_USB           ((uint8_t)0x0EU)  /* USB Alternate Function mapping */
/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0EU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F303xC */

#if defined (STM32F303xE)
/*---------------------------------- STM32F303xE ------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */

/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM8           ((uint8_t)0x02U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
#define GPIO_AF2_I2C3           ((uint8_t)0x02U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF2_TIM20          ((uint8_t)0x02U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM8          ((uint8_t)0x03U)  /* TIM8 Alternate Function mapping  */
#define GPIO_AF3_COMP7         ((uint8_t)0x03U)  /* COMP7 Alternate Function mapping */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF3_I2C3          ((uint8_t)0x03U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF3_TIM20         ((uint8_t)0x03U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM8          ((uint8_t)0x04U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_TIM8          ((uint8_t)0x05U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
#define GPIO_AF5_SPI4          ((uint8_t)0x05U)  /* SPI4 Alternate Function mapping */

/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_TIM8          ((uint8_t)0x06U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_TIM20         ((uint8_t)0x06U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP3         ((uint8_t)0x07U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF7_COMP5         ((uint8_t)0x07U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP3         ((uint8_t)0x08U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP5         ((uint8_t)0x08U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF8_I2C3          ((uint8_t)0x08U)  /* I2C3 Alternate Function mapping */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM8          ((uint8_t)0x09U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM8           ((uint8_t)0xAU)  /* TIM8 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF11_TIM8           ((uint8_t)0x0BU)  /* TIM8 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF12_FMC             ((uint8_t)0xCU)  /* FMC Alternate Function mapping                      */
#define GPIO_AF12_SDIO            ((uint8_t)0xCU)  /* SDIO Alternate Function mapping                     */

/**
  * @brief   AF 14 selection
  */
#define GPIO_AF14_USB           ((uint8_t)0x0EU)  /* USB Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0EU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F303xE */

#if defined (STM32F302xE)
/*---------------------------------- STM32F302xE ------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */

/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
#define GPIO_AF2_I2C3           ((uint8_t)0x02U)  /* I2C3 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF3_I2C3          ((uint8_t)0x03U)  /* I2C3 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
#define GPIO_AF5_SPI4          ((uint8_t)0x05U)  /* SPI4 Alternate Function mapping */

/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF8_I2C3          ((uint8_t)0x08U)  /* I2C3 Alternate Function mapping */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF12_FMC             ((uint8_t)0xCU)  /* FMC Alternate Function mapping                      */
#define GPIO_AF12_SDIO            ((uint8_t)0xCU)  /* SDIO Alternate Function mapping                     */

/**
  * @brief   AF 14 selection
  */
#define GPIO_AF14_USB           ((uint8_t)0x0EU)  /* USB Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0EU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F302xE */

#if defined (STM32F398xx)
/*---------------------------------- STM32F398xx ------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */

/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM8           ((uint8_t)0x02U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
#define GPIO_AF2_I2C3           ((uint8_t)0x02U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF2_TIM20          ((uint8_t)0x02U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM8          ((uint8_t)0x03U)  /* TIM8 Alternate Function mapping  */
#define GPIO_AF3_COMP7         ((uint8_t)0x03U)  /* COMP7 Alternate Function mapping */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF3_I2C3          ((uint8_t)0x03U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF3_TIM20         ((uint8_t)0x03U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM8          ((uint8_t)0x04U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_TIM8          ((uint8_t)0x05U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
#define GPIO_AF5_SPI4          ((uint8_t)0x05U)  /* SPI4 Alternate Function mapping */

/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_TIM8          ((uint8_t)0x06U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_TIM20         ((uint8_t)0x06U)  /* TIM20 Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP3         ((uint8_t)0x07U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF7_COMP5         ((uint8_t)0x07U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP3         ((uint8_t)0x08U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP5         ((uint8_t)0x08U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF8_I2C3          ((uint8_t)0x08U)  /* I2C3 Alternate Function mapping */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM8          ((uint8_t)0x09U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM8           ((uint8_t)0xAU)  /* TIM8 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF11_TIM8           ((uint8_t)0x0BU)  /* TIM8 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF12_FMC             ((uint8_t)0xCU)  /* FMC Alternate Function mapping                      */
#define GPIO_AF12_SDIO            ((uint8_t)0xCU)  /* SDIO Alternate Function mapping                     */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F398xx */

#if defined (STM32F358xx)
/*---------------------------------- STM32F358xx -------------------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM8           ((uint8_t)0x02U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_COMP1          ((uint8_t)0x02U)  /* COMP1 Alternate Function mapping */
/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_TIM8          ((uint8_t)0x03U)  /* TIM8 Alternate Function mapping  */
#define GPIO_AF3_COMP7         ((uint8_t)0x03U)  /* COMP7 Alternate Function mapping */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM8          ((uint8_t)0x04U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1/I2S1 Alternate Function mapping      */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_I2S           ((uint8_t)0x05U)  /* I2S Alternate Function mapping */
#define GPIO_AF5_I2S2ext       ((uint8_t)0x05U)  /* I2S2ext Alternate Function mapping */
#define GPIO_AF5_TIM8          ((uint8_t)0x05U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
#define GPIO_AF5_UART4         ((uint8_t)0x05U)  /* UART4 Alternate Function mapping */
#define GPIO_AF5_UART5         ((uint8_t)0x05U)  /* UART5 Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_I2S3ext       ((uint8_t)0x06U)  /* I2S3ext Alternate Function mapping */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_TIM8          ((uint8_t)0x06U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_COMP3         ((uint8_t)0x07U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF7_COMP5         ((uint8_t)0x07U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF7_COMP6         ((uint8_t)0x07U)  /* COMP6 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */
#define GPIO_AF8_COMP3         ((uint8_t)0x08U)  /* COMP3 Alternate Function mapping  */
#define GPIO_AF8_COMP4         ((uint8_t)0x08U)  /* COMP4 Alternate Function mapping  */
#define GPIO_AF8_COMP5         ((uint8_t)0x08U)  /* COMP5 Alternate Function mapping  */
#define GPIO_AF8_COMP6         ((uint8_t)0x08U)  /* COMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM8          ((uint8_t)0x09U)  /* TIM8 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM8           ((uint8_t)0xAU)  /* TIM8 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1           ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF11_TIM8           ((uint8_t)0x0BU)  /* TIM8 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1            ((uint8_t)0xCU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F358xx */

#if  defined (STM32F373xC)
/*---------------------------------- STM32F373xC--------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM5           ((uint8_t)0x02U)  /* TIM5 Alternate Function mapping */
#define GPIO_AF2_TIM13          ((uint8_t)0x02U)  /* TIM13 Alternate Function mapping */
#define GPIO_AF2_TIM14          ((uint8_t)0x02U)  /* TIM14 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM19          ((uint8_t)0x02U)  /* TIM19 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI1          ((uint8_t)0x06U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_CEC           ((uint8_t)0x06U)  /* CEC Alternate Function mapping */
/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */
#define GPIO_AF7_CEC           ((uint8_t)0x07U)  /* CEC Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM12         ((uint8_t)0x09U)  /* TIM12 Alternate Function mapping */
#define GPIO_AF9_TIM13         ((uint8_t)0x09U)  /* TIM13 Alternate Function mapping */
#define GPIO_AF9_TIM14         ((uint8_t)0x09U)  /* TIM14 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */
/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM12          ((uint8_t)0xAU)  /* TIM12 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */
/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM19          ((uint8_t)0x0BU)  /* TIM19 Alternate Function mapping */


/**
  * @brief   AF 14 selection
  */
#define GPIO_AF14_USB           ((uint8_t)0x0EU)  /* USB Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0BU) || ((AF) == (uint8_t)0x0EU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F373xC */


#if defined (STM32F378xx)
/*---------------------------------------- STM32F378xx--------------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4           ((uint8_t)0x02U)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM5           ((uint8_t)0x02U)  /* TIM5 Alternate Function mapping */
#define GPIO_AF2_TIM13          ((uint8_t)0x02U)  /* TIM13 Alternate Function mapping */
#define GPIO_AF2_TIM14          ((uint8_t)0x02U)  /* TIM14 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM19          ((uint8_t)0x02U)  /* TIM19 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_SPI1          ((uint8_t)0x06U)  /* SPI1/I2S1 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_CEC           ((uint8_t)0x06U)  /* CEC Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping  */
#define GPIO_AF7_CEC           ((uint8_t)0x07U)  /* CEC Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_COMP1         ((uint8_t)0x08U)  /* COMP1 Alternate Function mapping  */
#define GPIO_AF8_COMP2         ((uint8_t)0x08U)  /* COMP2 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM12         ((uint8_t)0x09U)  /* TIM12 Alternate Function mapping */
#define GPIO_AF9_TIM13         ((uint8_t)0x09U)  /* TIM13 Alternate Function mapping */
#define GPIO_AF9_TIM14         ((uint8_t)0x09U)  /* TIM14 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM4           ((uint8_t)0xAU)  /* TIM4 Alternate Function mapping */
#define GPIO_AF10_TIM12          ((uint8_t)0xAU)  /* TIM12 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM19          ((uint8_t)0x0BU)  /* TIM19 Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0BU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F378xx */

#if defined (STM32F303x8)
/*---------------------------------- STM32F303x8--------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM16          ((uint8_t)0x02U)  /* TIM16 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_GPCOMP6       ((uint8_t)0x07U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_GPCOMP2         ((uint8_t)0x08U)  /* GPCOMP2 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP4         ((uint8_t)0x08U)  /* GPCOMP4 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP6         ((uint8_t)0x08U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */
/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1          ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1          ((uint8_t)0x0CU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 13 selection
  */
#define GPIO_AF13_OPAMP2        ((uint8_t)0x0DU)  /* OPAMP2 Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0DU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F303x8 */

#if defined (STM32F334x8) || defined (STM32F328xx)
/*---------------------------------- STM32F334x8/STM32F328xx -------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM3           ((uint8_t)0x02U)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM16          ((uint8_t)0x02U)  /* TIM16 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_HRTIM1        ((uint8_t)0x03U)  /* HRTIM1 Alternate Function mapping  */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_GPCOMP6       ((uint8_t)0x07U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_GPCOMP2         ((uint8_t)0x08U)  /* GPCOMP2 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP4         ((uint8_t)0x08U)  /* GPCOMP4 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP6         ((uint8_t)0x08U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */
/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM3           ((uint8_t)0xAU)  /* TIM3 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1          ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1          ((uint8_t)0x0CU)  /* TIM1 Alternate Function mapping */
#define GPIO_AF12_HRTIM1        ((uint8_t)0x0CU)  /* HRTIM1 Alternate Function mapping  */

/**
  * @brief   AF 13 selection
  */
#define GPIO_AF13_OPAMP2        ((uint8_t)0x0DU)  /* OPAMP2 Alternate Function mapping */
#define GPIO_AF13_HRTIM1        ((uint8_t)0x0DU)  /* HRTIM1 Alternate Function mapping  */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0DU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F334x8 || STM32F328xx */

#if defined (STM32F301x8) || defined (STM32F318xx)
/*---------------------------------- STM32F301x8 / STM32F318xx ------------------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC Alternate Function mapping     								       */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_I2C3           ((uint8_t)0x02U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_I2C3          ((uint8_t)0x03U)  /* I2C3 Alternate Function mapping  */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping  */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_GPCOMP6       ((uint8_t)0x07U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_I2C3            ((uint8_t)0x08U)  /* I2C3 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP2         ((uint8_t)0x08U)  /* GPCOMP2 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP4         ((uint8_t)0x08U)  /* GPCOMP4 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP6         ((uint8_t)0x08U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1          ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1          ((uint8_t)0x0CU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F301x8 || STM32F318xx */

#if defined (STM32F302x8)
/*---------------------------------- STM32F302x8------------------------------------------*/
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_MCO           ((uint8_t)0x00U)  /* MCO (MCO1 and MCO2) Alternate Function mapping            */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00U)  /* RTC Alternate Function mapping     								       */
#define GPIO_AF0_TAMPER        ((uint8_t)0x00U)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF0_SWJ           ((uint8_t)0x00U)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_TRACE         ((uint8_t)0x00U)  /* TRACE Alternate Function mapping                          */

/**
  * @brief   AF 1 selection
  */
#define GPIO_AF1_TIM2           ((uint8_t)0x01U)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM15          ((uint8_t)0x01U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF1_TIM16          ((uint8_t)0x01U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17          ((uint8_t)0x01U)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_EVENTOUT       ((uint8_t)0x01U)  /* EVENTOUT Alternate Function mapping */
/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_I2C3           ((uint8_t)0x02U)  /* I2C3 Alternate Function mapping */
#define GPIO_AF2_TIM1           ((uint8_t)0x02U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF2_TIM15          ((uint8_t)0x02U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF2_TIM2           ((uint8_t)0x02U)  /* TIM2 Alternate Function mapping */

/**
  * @brief   AF 3 selection
  */
#define GPIO_AF3_TSC           ((uint8_t)0x03U)  /* TSC Alternate Function mapping  */
#define GPIO_AF3_I2C3          ((uint8_t)0x03U)  /* I2C3 Alternate Function mapping  */
#define GPIO_AF3_TIM15         ((uint8_t)0x03U)  /* TIM15 Alternate Function mapping  */

/**
  * @brief   AF 4 selection
  */
#define GPIO_AF4_I2C1          ((uint8_t)0x04U)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04U)  /* I2C2 Alternate Function mapping */
#define GPIO_AF4_TIM1          ((uint8_t)0x04U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF4_TIM16         ((uint8_t)0x04U)  /* TIM16 Alternate Function mapping */
#define GPIO_AF4_TIM17         ((uint8_t)0x04U)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 5 selection
  */
#define GPIO_AF5_SPI1          ((uint8_t)0x05U)  /* SPI1 Alternate Function mapping */
#define GPIO_AF5_SPI2          ((uint8_t)0x05U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF5_SPI3          ((uint8_t)0x05U)  /* SPI3/I2S3 Alternate Function mapping */
#define GPIO_AF5_IR            ((uint8_t)0x05U)  /* IR Alternate Function mapping */
/**
  * @brief   AF 6 selection
  */
#define GPIO_AF6_TIM1          ((uint8_t)0x06U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF6_IR            ((uint8_t)0x06U)  /* IR Alternate Function mapping */
#define GPIO_AF6_SPI2          ((uint8_t)0x06U)  /* SPI2/I2S2 Alternate Function mapping */
#define GPIO_AF6_SPI3          ((uint8_t)0x06U)  /* SPI3/I2S3 Alternate Function mapping */

/**
  * @brief   AF 7 selection
  */
#define GPIO_AF7_USART1        ((uint8_t)0x07U)  /* USART1 Alternate Function mapping  */
#define GPIO_AF7_USART2        ((uint8_t)0x07U)  /* USART2 Alternate Function mapping  */
#define GPIO_AF7_USART3        ((uint8_t)0x07U)  /* USART3 Alternate Function mapping  */
#define GPIO_AF7_GPCOMP6       ((uint8_t)0x07U)  /* GPCOMP6 Alternate Function mapping */
#define GPIO_AF7_CAN           ((uint8_t)0x07U)  /* CAN Alternate Function mapping */

/**
  * @brief   AF 8 selection
  */
#define GPIO_AF8_I2C3   	 ((uint8_t)0x08U)  /* I2C3 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP2         ((uint8_t)0x08U)  /* GPCOMP2 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP4         ((uint8_t)0x08U)  /* GPCOMP4 Alternate Function mapping  */
#define GPIO_AF8_GPCOMP6         ((uint8_t)0x08U)  /* GPCOMP6 Alternate Function mapping  */

/**
  * @brief   AF 9 selection
  */
#define GPIO_AF9_TIM1          ((uint8_t)0x09U)  /* TIM1 Alternate Function mapping */
#define GPIO_AF9_TIM15         ((uint8_t)0x09U)  /* TIM15 Alternate Function mapping */
#define GPIO_AF9_CAN           ((uint8_t)0x09U)  /* CAN Alternate Function mapping */

/**
  * @brief   AF 10 selection
  */
#define GPIO_AF10_TIM2           ((uint8_t)0xAU)  /* TIM2 Alternate Function mapping */
#define GPIO_AF10_TIM17          ((uint8_t)0xAU)  /* TIM17 Alternate Function mapping */

/**
  * @brief   AF 11 selection
  */
#define GPIO_AF11_TIM1          ((uint8_t)0x0BU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 12 selection
  */
#define GPIO_AF12_TIM1          ((uint8_t)0x0CU)  /* TIM1 Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0FU)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)          (((AF) <= (uint8_t)0x0CU) || ((AF) == (uint8_t)0x0FU))
/*------------------------------------------------------------------------------------------*/
#endif /* STM32F302x8 */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup GPIOEx_Exported_Macros GPIOEx Exported Macros
  * @{
  */

/** @defgroup GPIOEx_Get_Port_Index GPIOEx_Get Port Index
* @{
  */
#if defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
#define GPIO_GET_INDEX(__GPIOx__)    (((__GPIOx__) == (GPIOA))? 0U :\
                                      ((__GPIOx__) == (GPIOB))? 1U :\
                                      ((__GPIOx__) == (GPIOC))? 2U :\
                                      ((__GPIOx__) == (GPIOD))? 3U : 5U)
#endif /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F373xC) || defined(STM32F378xx)
#define GPIO_GET_INDEX(__GPIOx__)    (((__GPIOx__) == (GPIOA))? 0U :\
                                      ((__GPIOx__) == (GPIOB))? 1U :\
                                      ((__GPIOx__) == (GPIOC))? 2U :\
                                      ((__GPIOx__) == (GPIOD))? 3U :\
                                      ((__GPIOx__) == (GPIOE))? 4U : 5U)
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define GPIO_GET_INDEX(__GPIOx__)    (((__GPIOx__) == (GPIOA))? 0U :\
                                      ((__GPIOx__) == (GPIOB))? 1U :\
                                      ((__GPIOx__) == (GPIOC))? 2U :\
                                      ((__GPIOx__) == (GPIOD))? 3U :\
                                      ((__GPIOx__) == (GPIOE))? 4U :\
                                      ((__GPIOx__) == (GPIOF))? 5U :\
                                      ((__GPIOx__) == (GPIOG))? 6U : 7U)
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

/**
  * @}
  */

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

#endif /* __STM32F3xx_HAL_GPIO_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
