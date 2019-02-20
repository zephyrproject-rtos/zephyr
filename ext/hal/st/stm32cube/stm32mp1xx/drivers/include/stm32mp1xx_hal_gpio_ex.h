/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_gpio_ex.h
  * @author  MCD Application Team
  * @brief   Header file of GPIO HAL Extension module.
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
#ifndef __STM32MP1xx_HAL_GPIO_EX_H
#define __STM32MP1xx_HAL_GPIO_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup GPIOEx GPIOEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/** @defgroup GPIOEx_Exported_Constants GPIO Exported Constants
  * @{
  */

/** @defgroup GPIO_Alternate_function_selection GPIO Alternate Function Selection
  * @{
  */
/**
  * @brief   AF 0 selection
  */
#define GPIO_AF0_RTC_50Hz      ((uint8_t)0x00)  /* RTC_50Hz Alternate Function mapping                       */
#define GPIO_AF0_MCO1          ((uint8_t)0x00)  /* MCO1 Alternate Function mapping                           */
#define GPIO_AF0_SWJ           ((uint8_t)0x00)  /* SWJ (SWD and JTAG) Alternate Function mapping             */
#define GPIO_AF0_LCDBIAS       ((uint8_t)0x00)  /* LCDBIAS Alternate Function mapping                        */
#define GPIO_AF0_TRACE         ((uint8_t)0x00)  /* TRACE Alternate Function mapping                          */
#define GPIO_AF0_HDP           ((uint8_t)0x00)  /* HDP Alternate Function mapping                            */

/**
 * @brief   AF 1 selection
 */
#define GPIO_AF1_TIM1          ((uint8_t)0x01)  /* TIM1 Alternate Function mapping */
#define GPIO_AF1_TIM2          ((uint8_t)0x01)  /* TIM2 Alternate Function mapping */
#define GPIO_AF1_TIM16         ((uint8_t)0x01)  /* TIM16 Alternate Function mapping */
#define GPIO_AF1_TIM17         ((uint8_t)0x01)  /* TIM17 Alternate Function mapping */
#define GPIO_AF1_LPTIM1        ((uint8_t)0x01)  /* LPTIM1 Alternate Function mapping */
#define GPIO_AF1_MCO2          ((uint8_t)0x01)  /* MCO2 Alternate Function mapping */
#define GPIO_AF1_RTC           ((uint8_t)0x01)  /* RTC Alternate Function mapping */

/**
  * @brief   AF 2 selection
  */
#define GPIO_AF2_TIM3          ((uint8_t)0x02)  /* TIM3 Alternate Function mapping */
#define GPIO_AF2_TIM4          ((uint8_t)0x02)  /* TIM4 Alternate Function mapping */
#define GPIO_AF2_TIM5          ((uint8_t)0x02)  /* TIM5 Alternate Function mapping */
#define GPIO_AF2_TIM12         ((uint8_t)0x02)  /* TIM12 Alternate Function mapping */
#define GPIO_AF2_SAI1          ((uint8_t)0x02)  /* SAI1 Alternate Function mapping */
#define GPIO_AF2_SAI4          ((uint8_t)0x02)  /* SAI4 Alternate Function mapping */
#define GPIO_AF2_I2C6          ((uint8_t)0x02)  /* I2C6 Alternate Function mapping */
#define GPIO_AF2_MCO1          ((uint8_t)0x02)  /* MCO1 Alternate Function mapping */
#define GPIO_AF2_MCO2          ((uint8_t)0x02)  /* MCO2 Alternate Function mapping */
#define GPIO_AF2_HDP           ((uint8_t)0x02)  /* HDP Alternate Function mapping  */
/**
 * @brief   AF 3 selection
 */
#define GPIO_AF3_TIM8          ((uint8_t)0x03)  /* TIM8 Alternate Function mapping  */
#define GPIO_AF3_LPTIM2        ((uint8_t)0x03)  /* LPTIM2 Alternate Function mapping  */
#define GPIO_AF3_DFSDM1        ((uint8_t)0x03)  /* DFSDM1 Alternate Function mapping  */
#define GPIO_AF3_I2C2          ((uint8_t)0x03)  /* I2C6 Alternate Function mapping */
#define GPIO_AF3_LPTIM3        ((uint8_t)0x03)  /* LPTIM3 Alternate Function mapping  */
#define GPIO_AF3_LPTIM4        ((uint8_t)0x03)  /* LPTIM4 Alternate Function mapping  */
#define GPIO_AF3_LPTIM5        ((uint8_t)0x03)  /* LPTIM5 Alternate Function mapping  */
#define GPIO_AF3_SAI4          ((uint8_t)0x03)  /* SAI4 Alternate Function mapping */
#define GPIO_AF3_SDIO1         ((uint8_t)0x03)  /* SDIO1 Alternate Function mapping */

/**
 * @brief   AF 4 selection
 */
#define GPIO_AF4_I2C1          ((uint8_t)0x04)  /* I2C1 Alternate Function mapping */
#define GPIO_AF4_I2C2          ((uint8_t)0x04)  /* I2C2 Alternate Function mapping */
#define GPIO_AF4_I2C3          ((uint8_t)0x04)  /* I2C3 Alternate Function mapping */
#define GPIO_AF4_I2C4          ((uint8_t)0x04)  /* I2C4 Alternate Function mapping */
#define GPIO_AF4_I2C5          ((uint8_t)0x04)  /* I2C5 Alternate Function mapping */
#define GPIO_AF4_TIM15         ((uint8_t)0x04)  /* TIM15 Alternate Function mapping */
#define GPIO_AF4_CEC           ((uint8_t)0x04)  /* CEC Alternate Function mapping */
#define GPIO_AF4_DFSDM1        ((uint8_t)0x04)  /* DFSDM1  Alternate Function mapping   */
#define GPIO_AF4_LPTIM2        ((uint8_t)0x04)  /* LPTIM2 Alternate Function mapping  */
#define GPIO_AF4_SAI4          ((uint8_t)0x04)  /* SAI4 Alternate Function mapping */
#define GPIO_AF4_USART1        ((uint8_t)0x04)  /* USART1 Alternate Function mapping */

/**
 * @brief   AF 5 selection
 */
#define GPIO_AF5_SPI1          ((uint8_t)0x05)  /* SPI1 Alternate Function mapping   */
#define GPIO_AF5_SPI2          ((uint8_t)0x05)  /* SPI2 Alternate Function mapping   */
#define GPIO_AF5_SPI3          ((uint8_t)0x05)  /* SPI3 Alternate Function mapping   */
#define GPIO_AF5_SPI4          ((uint8_t)0x05)  /* SPI4 Alternate Function mapping   */
#define GPIO_AF5_SPI5          ((uint8_t)0x05)  /* SPI5 Alternate Function mapping   */
#define GPIO_AF5_SPI6          ((uint8_t)0x05)  /* SPI6 Alternate Function mapping   */
#define GPIO_AF5_CEC           ((uint8_t)0x05)  /* CEC  Alternate Function mapping   */
#define GPIO_AF5_I2C1          ((uint8_t)0x05)  /* I2C1 Alternate Function mapping */
#define GPIO_AF5_SDIO1         ((uint8_t)0x05)  /* SDIO1 Alternate Function mapping */
#define GPIO_AF5_SDIO3         ((uint8_t)0x05)  /* SDIO3 Alternate Function mapping */

/**
 * @brief   AF 6 selection
 */
#define GPIO_AF6_SPI3          ((uint8_t)0x06)  /* SPI3 Alternate Function mapping  */
#define GPIO_AF6_SAI1          ((uint8_t)0x06)  /* SAI1 Alternate Function mapping  */
#define GPIO_AF6_SAI3          ((uint8_t)0x06)  /* SAI3 Alternate Function mapping  */
#define GPIO_AF6_SAI4          ((uint8_t)0x06)  /* SAI4 Alternate Function mapping */
#define GPIO_AF6_I2C4          ((uint8_t)0x06)  /* I2C4 Alternate Function mapping  */
#define GPIO_AF6_DFSDM1        ((uint8_t)0x06)  /* DFSDM1 Alternate Function mapping */
#define GPIO_AF6_UART4         ((uint8_t)0x06)  /* UART4 Alternate Function mapping */

/**
 * @brief   AF 7 selection
 */
#define GPIO_AF7_SPI2          ((uint8_t)0x07)  /* SPI2 Alternate Function mapping  */
#define GPIO_AF7_SPI3          ((uint8_t)0x07)  /* SPI3 Alternate Function mapping  */
#define GPIO_AF7_SPI6          ((uint8_t)0x07)  /* SPI6 Alternate Function mapping  */
#define GPIO_AF7_USART1        ((uint8_t)0x07)  /* USART1 Alternate Function mapping     */
#define GPIO_AF7_USART2        ((uint8_t)0x07)  /* USART2 Alternate Function mapping     */
#define GPIO_AF7_USART3        ((uint8_t)0x07)  /* USART3 Alternate Function mapping     */
#define GPIO_AF7_USART6        ((uint8_t)0x07)  /* USART6 Alternate Function mapping     */
#define GPIO_AF7_UART7         ((uint8_t)0x07)  /* UART7 Alternate Function mapping     */

/**
 * @brief   AF 8 selection
 */
#define GPIO_AF8_SPI6         ((uint8_t)0x08)  /* SPI6 Alternate Function mapping  */
#define GPIO_AF8_SAI2         ((uint8_t)0x08)  /* SAI2 Alternate Function mapping  */
#define GPIO_AF8_USART3       ((uint8_t)0x08)  /* USART3 Alternate Function mapping */
#define GPIO_AF8_UART4        ((uint8_t)0x08)  /* UART4 Alternate Function mapping */
#define GPIO_AF8_UART5        ((uint8_t)0x08)  /* UART5 Alternate Function mapping */
#define GPIO_AF8_UART8        ((uint8_t)0x08)  /* UART8 Alternate Function mapping */
#define GPIO_AF8_SPDIF        ((uint8_t)0x08)  /* SPDIF Alternate Function mapping */
#define GPIO_AF8_SDIO1        ((uint8_t)0x08)  /* SDIO1 Alternate Function mapping */

/**
 * @brief   AF 9 selection
 */
#define GPIO_AF9_QUADSPI       ((uint8_t)0x09)  /* QUADSPI Alternate Function mapping */
#if defined (FDCAN1)
#define GPIO_AF9_FDCAN1        ((uint8_t)0x09)  /* FDCAN1 Alternate Function mapping    */
#endif
#if defined (FDCAN2)
#define GPIO_AF9_FDCAN2        ((uint8_t)0x09)  /* FDCAN2 Alternate Function mapping    */
#endif
#define GPIO_AF9_TIM13         ((uint8_t)0x09)  /* TIM13 Alternate Function mapping   */
#define GPIO_AF9_TIM14         ((uint8_t)0x09)  /* TIM14 Alternate Function mapping   */
#define GPIO_AF9_SDIO2         ((uint8_t)0x09)  /* SDIO2 Alternate Function mapping   */
#define GPIO_AF9_LCD           ((uint8_t)0x09)  /* LCD Alternate Function mapping   */
#define GPIO_AF9_SPDIF         ((uint8_t)0x09)  /* SPDIF Alternate Function mapping   */
#define GPIO_AF9_SDIO3         ((uint8_t)0x09)  /* SDIO3 Alternate Function mapping   */
#define GPIO_AF9_SDIO2         ((uint8_t)0x09)  /* SDIO3 Alternate Function mapping   */

/**
 * @brief   AF 10 selection
 */
#define GPIO_AF10_QUADSPI       ((uint8_t)0xA)  /* QUADSPI Alternate Function mapping */
#define GPIO_AF10_SAI2          ((uint8_t)0xA)  /* SAI2 Alternate Function mapping */
#define GPIO_AF10_SAI4          ((uint8_t)0xA)  /* SAI4 Alternate Function mapping */
#define GPIO_AF10_SDIO2         ((uint8_t)0xA)  /* SDIO2 Alternate Function mapping */
#define GPIO_AF10_SDIO3         ((uint8_t)0xA)  /* SDIO3 Alternate Function mapping */
#define GPIO_AF10_OTG2_HS       ((uint8_t)0xA)  /* OTG2_HS Alternate Function mapping */
#define GPIO_AF10_OTG1_FS       ((uint8_t)0xA)  /* OTG1_FS Alternate Function mapping */

/**
 * @brief   AF 11 selection
 */
#define GPIO_AF11_DFSDM1        ((uint8_t)0x0B)  /* DFSDM1 Alternate Function mapping */
#define GPIO_AF11_QUADSPI       ((uint8_t)0x0B)  /* QUADSPI Alternate Function mapping */
#define GPIO_AF11_ETH           ((uint8_t)0x0B)  /* ETH Alternate Function mapping */
#if defined (DSI)
#define GPIO_AF11_DSI           ((uint8_t)0x0B)  /* DSI Alternate Function mapping */
#endif
#define GPIO_AF11_SDIO1         ((uint8_t)0x0B)  /* SDIO1 Alternate Function mapping */

/**
 * @brief   AF 12 selection
 */
#define GPIO_AF12_UART5         ((uint8_t)0xC)  /* UART5 Alternate Function mapping */
#define GPIO_AF12_FMC           ((uint8_t)0xC)  /* FMC Alternate Function mapping     */
#define GPIO_AF12_SDIO1         ((uint8_t)0xC)  /* SDIO1 Alternate Function mapping  */
#define GPIO_AF12_MDIOS         ((uint8_t)0xC)  /* MDIOS Alternate Function mapping   */
#define GPIO_AF12_SAI4          ((uint8_t)0xC)  /* SAI4 Alternate Function mapping */
#define GPIO_AF12_SDIO1         ((uint8_t)0xC)  /* SAI4 Alternate Function mapping */

/**
 * @brief   AF 13 selection
 */
#define GPIO_AF13_UART7         ((uint8_t)0x0D)   /* UART7 Alternate Function mapping */
#define GPIO_AF13_DCMI          ((uint8_t)0x0D)   /* DCMI Alternate Function mapping */
#define GPIO_AF13_LCD           ((uint8_t)0x0D)   /* LCD Alternate Function mapping */
#if defined (DSI)
#define GPIO_AF13_DSI           ((uint8_t)0x0D)   /* DSI Alternate Function mapping */
#endif
#define GPIO_AF13_RNG           ((uint8_t)0x0D)   /* RNG Alternate Function mapping */


/**
 * @brief   AF 14 selection
 */
#define GPIO_AF14_UART5        ((uint8_t)0x0E)   /* UART5 Alternate Function mapping */
#define GPIO_AF14_LCD          ((uint8_t)0x0E)   /* LCD Alternate Function mapping */

/**
  * @brief   AF 15 selection
  */
#define GPIO_AF15_EVENTOUT      ((uint8_t)0x0F)  /* EVENTOUT Alternate Function mapping */

#define IS_GPIO_AF(AF)   ((AF) <= (uint8_t)0x0F)



/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @defgroup GPIOEx_Private_Macros GPIO Private Macros
  * @{
  */
/** @defgroup GPIOEx_Get_Port_Index GPIO Get Port Index
  * @{
  */
#define GPIO_GET_INDEX(__GPIOx__)   (uint8_t)(((__GPIOx__) == (GPIOA))? 0U :\
                                              ((__GPIOx__) == (GPIOB))? 1U :\
                                              ((__GPIOx__) == (GPIOC))? 2U :\
                                              ((__GPIOx__) == (GPIOD))? 3U :\
                                              ((__GPIOx__) == (GPIOE))? 4U :\
                                              ((__GPIOx__) == (GPIOF))? 5U :\
                                              ((__GPIOx__) == (GPIOG))? 6U :\
                                              ((__GPIOx__) == (GPIOH))? 7U :\
                                              ((__GPIOx__) == (GPIOI))? 8U :\
                                              ((__GPIOx__) == (GPIOJ))? 9U :\
                                              ((__GPIOx__) == (GPIOK))? 10U :\
                                              ((__GPIOx__) == (GPIOZ))? 11U : 25U)
/**
  * @}
  */

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/ 
/** @addtogroup GPIOEx_Exported_Functions GPIO Extended Exported Functions
  * @{
  */

/** @addtogroup GPIOEx_Exported_Functions_Group1 Extended Peripheral Control functions 
 * @{
 */

void                HAL_GPIOEx_SecurePin(GPIO_TypeDef  *GPIOx, uint16_t GPIO_Pin);
void                HAL_GPIOEx_NonSecurePin(GPIO_TypeDef  *GPIOx, uint16_t GPIO_Pin);


GPIO_PinState       HAL_GPIOEx_IsPinSecured(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
  * @}
  */

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32MP1xx_HAL_GPIO_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
