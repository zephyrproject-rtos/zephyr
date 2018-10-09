/**
  ******************************************************************************
  * @file    stm32f0xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extension module.
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
#ifndef __STM32F0xx_HAL_RCC_EX_H
#define __STM32F0xx_HAL_RCC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal_def.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

/** @addtogroup RCC
  * @{
  */

/** @addtogroup RCC_Private_Macros
 * @{
 */
#if defined(RCC_HSI48_SUPPORT)
#define IS_RCC_OSCILLATORTYPE(OSCILLATOR) (((OSCILLATOR) == RCC_OSCILLATORTYPE_NONE)                               || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI14) == RCC_OSCILLATORTYPE_HSI14) || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI48) == RCC_OSCILLATORTYPE_HSI48))

#define IS_RCC_SYSCLKSOURCE(SOURCE)  (((SOURCE) == RCC_SYSCLKSOURCE_HSI)    || \
                                      ((SOURCE) == RCC_SYSCLKSOURCE_HSE)    || \
                                      ((SOURCE) == RCC_SYSCLKSOURCE_PLLCLK) || \
                                      ((SOURCE) == RCC_SYSCLKSOURCE_HSI48))

#define IS_RCC_SYSCLKSOURCE_STATUS(SOURCE) (((SOURCE) == RCC_SYSCLKSOURCE_STATUS_HSI)    || \
                                            ((SOURCE) == RCC_SYSCLKSOURCE_STATUS_HSE)    || \
                                            ((SOURCE) == RCC_SYSCLKSOURCE_STATUS_PLLCLK) || \
                                            ((SOURCE) == RCC_SYSCLKSOURCE_STATUS_HSI48))

#define IS_RCC_PLLSOURCE(SOURCE) (((SOURCE) == RCC_PLLSOURCE_HSI)   || \
                                  ((SOURCE) == RCC_PLLSOURCE_HSI48) || \
                                  ((SOURCE) == RCC_PLLSOURCE_HSE))

#define IS_RCC_HSI48(HSI48) (((HSI48) == RCC_HSI48_OFF) || ((HSI48) == RCC_HSI48_ON))

#else

#define IS_RCC_OSCILLATORTYPE(OSCILLATOR) (((OSCILLATOR) == RCC_OSCILLATORTYPE_NONE)                               || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE)     || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI14) == RCC_OSCILLATORTYPE_HSI14))
#define IS_RCC_SYSCLKSOURCE(SOURCE)  (((SOURCE) == RCC_SYSCLKSOURCE_HSI)    || \
                                      ((SOURCE) == RCC_SYSCLKSOURCE_HSE)    || \
                                      ((SOURCE) == RCC_SYSCLKSOURCE_PLLCLK))

#define IS_RCC_SYSCLKSOURCE_STATUS(SOURCE) (((SOURCE) == RCC_SYSCLKSOURCE_STATUS_HSI)    || \
                                            ((SOURCE) == RCC_SYSCLKSOURCE_STATUS_HSE)    || \
                                            ((SOURCE) == RCC_SYSCLKSOURCE_STATUS_PLLCLK))
#define IS_RCC_PLLSOURCE(SOURCE) (((SOURCE) == RCC_PLLSOURCE_HSI)   || \
                                  ((SOURCE) == RCC_PLLSOURCE_HSE))

#endif /* RCC_HSI48_SUPPORT */

#if defined(RCC_CFGR_PLLNODIV) && !defined(RCC_CFGR_MCO_HSI48)

#define IS_RCC_MCO1SOURCE(SOURCE)  (((SOURCE) == RCC_MCO1SOURCE_NOCLOCK)     || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_SYSCLK)       || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_PLLCLK)       || \
                                   ((SOURCE) == RCC_MCO1SOURCE_PLLCLK_DIV2)  || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI14))

#elif defined(RCC_CFGR_PLLNODIV) && defined(RCC_CFGR_MCO_HSI48)

#define IS_RCC_MCO1SOURCE(SOURCE)  (((SOURCE) == RCC_MCO1SOURCE_NOCLOCK)     || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_SYSCLK)       || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_PLLCLK)       || \
                                   ((SOURCE) == RCC_MCO1SOURCE_PLLCLK_DIV2)  || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI14)        || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI48))

#elif !defined(RCC_CFGR_PLLNODIV) && !defined(RCC_CFGR_MCO_HSI48)

#define IS_RCC_MCO1SOURCE(SOURCE)  (((SOURCE) == RCC_MCO1SOURCE_NOCLOCK)     || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_SYSCLK)       || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSE)          || \
                                   ((SOURCE) == RCC_MCO1SOURCE_PLLCLK_DIV2)  || \
                                   ((SOURCE) == RCC_MCO1SOURCE_HSI14))

#endif /* RCC_CFGR_PLLNODIV && !RCC_CFGR_MCO_HSI48 */

/**
  * @}
  */

/** @addtogroup RCC_Exported_Constants
 * @{
 */
#if defined(RCC_HSI48_SUPPORT)

/** @addtogroup RCC_PLL_Clock_Source
  * @{
  */
#define RCC_PLLSOURCE_HSI                RCC_CFGR_PLLSRC_HSI_PREDIV
#define RCC_PLLSOURCE_HSI48              RCC_CFGR_PLLSRC_HSI48_PREDIV

/**
  * @}
  */

/** @addtogroup RCC_Interrupt
  * @{
  */
#define RCC_IT_HSI48                   RCC_CIR_HSI48RDYF /*!< HSI48 Ready Interrupt flag */
/**
  * @}
  */

/** @addtogroup RCC_Flag
  * @{
  */
#define RCC_FLAG_HSI48RDY                ((uint8_t)((CR2_REG_INDEX << 5U) | RCC_CR2_HSI48RDY_BitNumber))
/**
  * @}
  */

/** @addtogroup RCC_System_Clock_Source
  * @{
  */
#define RCC_SYSCLKSOURCE_HSI48           RCC_CFGR_SW_HSI48
/**
  * @}
  */

/** @addtogroup RCC_System_Clock_Source_Status
  * @{
  */
#define RCC_SYSCLKSOURCE_STATUS_HSI48    RCC_CFGR_SWS_HSI48
/**
  * @}
  */

#else
/** @addtogroup RCC_PLL_Clock_Source
  * @{
  */

#if defined(STM32F070xB) || defined(STM32F070x6) || defined(STM32F030xC)
#define RCC_PLLSOURCE_HSI                RCC_CFGR_PLLSRC_HSI_PREDIV
#else
#define RCC_PLLSOURCE_HSI                RCC_CFGR_PLLSRC_HSI_DIV2
#endif

/**
  * @}
  */

#endif /* RCC_HSI48_SUPPORT */

/** @addtogroup RCC_MCO_Clock_Source
  * @{
  */

#if defined(RCC_CFGR_PLLNODIV)

#define RCC_MCO1SOURCE_PLLCLK       (RCC_CFGR_MCO_PLL | RCC_CFGR_PLLNODIV)

#endif /* RCC_CFGR_PLLNODIV */

#if defined(RCC_CFGR_MCO_HSI48)

#define RCC_MCO1SOURCE_HSI48        RCC_CFGR_MCO_HSI48

#endif /* SRCC_CFGR_MCO_HSI48 */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup RCCEx
  * @{
  */

/* Private Constants -------------------------------------------------------------*/
#if defined(CRS)
/** @addtogroup RCCEx_Private_Constants
 * @{
 */

/* CRS IT Error Mask */
#define  RCC_CRS_IT_ERROR_MASK   ((uint32_t)(RCC_CRS_IT_TRIMOVF | RCC_CRS_IT_SYNCERR | RCC_CRS_IT_SYNCMISS))

/* CRS Flag Error Mask */
#define RCC_CRS_FLAG_ERROR_MASK  ((uint32_t)(RCC_CRS_FLAG_TRIMOVF | RCC_CRS_FLAG_SYNCERR | RCC_CRS_FLAG_SYNCMISS))

/**
  * @}
  */
#endif /* CRS */

/* Private macro -------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Macros RCCEx Private Macros
  * @{
  */
#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F030xC)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1 | \
                                                     RCC_PERIPHCLK_RTC))
#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F038xx ||
          STM32F030xC */

#if defined(STM32F070x6) || defined(STM32F070xB)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1 | \
                                                     RCC_PERIPHCLK_RTC    | RCC_PERIPHCLK_USB))
#endif /* STM32F070x6 || STM32F070xB */

#if defined(STM32F042x6) || defined(STM32F048xx)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1   | \
                                                     RCC_PERIPHCLK_CEC    | RCC_PERIPHCLK_RTC    | \
                                                     RCC_PERIPHCLK_USB))
#endif /* STM32F042x6 || STM32F048xx */

#if defined(STM32F051x8) || defined(STM32F058xx)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1 | \
                                                     RCC_PERIPHCLK_CEC    | RCC_PERIPHCLK_RTC))
#endif /* STM32F051x8 || STM32F058xx */

#if defined(STM32F071xB)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | \
                                                     RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_CEC    | \
                                                     RCC_PERIPHCLK_RTC))
#endif /* STM32F071xB */

#if defined(STM32F072xB) || defined(STM32F078xx)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | \
                                                     RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_CEC    | \
                                                     RCC_PERIPHCLK_RTC    | RCC_PERIPHCLK_USB))
#endif /* STM32F072xB || STM32F078xx */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define IS_RCC_PERIPHCLOCK(SELECTION) ((SELECTION) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | \
                                                     RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_CEC    | \
                                                     RCC_PERIPHCLK_RTC    | RCC_PERIPHCLK_USART3 ))
#endif /* STM32F091xC || STM32F098xx */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) || defined(STM32F078xx)

#define IS_RCC_USBCLKSOURCE(SOURCE)  (((SOURCE) == RCC_USBCLKSOURCE_HSI48) || \
                                      ((SOURCE) == RCC_USBCLKSOURCE_PLL))

#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || STM32F078xx */

#if defined(STM32F070x6) || defined(STM32F070xB)

#define IS_RCC_USBCLKSOURCE(SOURCE)  (((SOURCE) == RCC_USBCLKSOURCE_NONE) || \
                                      ((SOURCE) == RCC_USBCLKSOURCE_PLL))

#endif /* STM32F070x6 || STM32F070xB */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define IS_RCC_USART2CLKSOURCE(SOURCE)  (((SOURCE) == RCC_USART2CLKSOURCE_PCLK1)  || \
                                         ((SOURCE) == RCC_USART2CLKSOURCE_SYSCLK) || \
                                         ((SOURCE) == RCC_USART2CLKSOURCE_LSE)    || \
                                         ((SOURCE) == RCC_USART2CLKSOURCE_HSI))

#endif /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define IS_RCC_USART3CLKSOURCE(SOURCE)  (((SOURCE) == RCC_USART3CLKSOURCE_PCLK1)  || \
                                         ((SOURCE) == RCC_USART3CLKSOURCE_SYSCLK) || \
                                         ((SOURCE) == RCC_USART3CLKSOURCE_LSE)    || \
                                         ((SOURCE) == RCC_USART3CLKSOURCE_HSI))
#endif /* STM32F091xC || STM32F098xx */


#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define IS_RCC_CECCLKSOURCE(SOURCE)  (((SOURCE) == RCC_CECCLKSOURCE_HSI) || \
                                      ((SOURCE) == RCC_CECCLKSOURCE_LSE))
#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(RCC_CFGR_MCOPRE)

#define IS_RCC_MCODIV(DIV) (((DIV) == RCC_MCODIV_1)  || ((DIV) == RCC_MCODIV_2)   || \
                            ((DIV) == RCC_MCODIV_4)  || ((DIV) == RCC_MCODIV_8)   || \
                            ((DIV) == RCC_MCODIV_16) || ((DIV) == RCC_MCODIV_32)  || \
                            ((DIV) == RCC_MCODIV_64) || ((DIV) == RCC_MCODIV_128))
#else

#define IS_RCC_MCODIV(DIV) (((DIV) == RCC_MCODIV_1))

#endif /* RCC_CFGR_MCOPRE */

#define IS_RCC_LSE_DRIVE(__DRIVE__) (((__DRIVE__) == RCC_LSEDRIVE_LOW)        || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMLOW)  || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMHIGH) || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_HIGH))

#if defined(CRS)

#define IS_RCC_CRS_SYNC_SOURCE(_SOURCE_) (((_SOURCE_) == RCC_CRS_SYNC_SOURCE_GPIO) || \
                                          ((_SOURCE_) == RCC_CRS_SYNC_SOURCE_LSE)  || \
                                          ((_SOURCE_) == RCC_CRS_SYNC_SOURCE_USB))
#define IS_RCC_CRS_SYNC_DIV(_DIV_) (((_DIV_) == RCC_CRS_SYNC_DIV1)  || ((_DIV_) == RCC_CRS_SYNC_DIV2)  || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV4)  || ((_DIV_) == RCC_CRS_SYNC_DIV8)  || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV16) || ((_DIV_) == RCC_CRS_SYNC_DIV32) || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV64) || ((_DIV_) == RCC_CRS_SYNC_DIV128))
#define IS_RCC_CRS_SYNC_POLARITY(_POLARITY_) (((_POLARITY_) == RCC_CRS_SYNC_POLARITY_RISING) || \
                                              ((_POLARITY_) == RCC_CRS_SYNC_POLARITY_FALLING))
#define IS_RCC_CRS_RELOADVALUE(_VALUE_) (((_VALUE_) <= 0xFFFFU))
#define IS_RCC_CRS_ERRORLIMIT(_VALUE_) (((_VALUE_) <= 0xFFU))
#define IS_RCC_CRS_HSI48CALIBRATION(_VALUE_) (((_VALUE_) <= 0x3FU))
#define IS_RCC_CRS_FREQERRORDIR(_DIR_) (((_DIR_) == RCC_CRS_FREQERRORDIR_UP) || \
                                        ((_DIR_) == RCC_CRS_FREQERRORDIR_DOWN))
#endif /* CRS */
/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */

/**
  * @brief  RCC extended clocks structure definition
  */
#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F030xC)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F038xx ||
          STM32F030xC */

#if defined(STM32F070x6) || defined(STM32F070xB)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t UsbClockSelection;    /*!< USB clock source
                                      This parameter can be a value of @ref RCCEx_USB_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F070x6 || STM32F070xB */

#if defined(STM32F042x6) || defined(STM32F048xx)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t CecClockSelection;    /*!< HDMI CEC clock source
                                      This parameter can be a value of @ref RCCEx_CEC_Clock_Source */

  uint32_t UsbClockSelection;    /*!< USB clock source
                                      This parameter can be a value of @ref RCCEx_USB_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F042x6 || STM32F048xx */

#if defined(STM32F051x8) || defined(STM32F058xx)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t CecClockSelection;    /*!< HDMI CEC clock source
                                      This parameter can be a value of @ref RCCEx_CEC_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F051x8 || STM32F058xx */

#if defined(STM32F071xB)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t Usart2ClockSelection; /*!< USART2 clock source
                                      This parameter can be a value of @ref RCCEx_USART2_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t CecClockSelection;    /*!< HDMI CEC clock source
                                      This parameter can be a value of @ref RCCEx_CEC_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F071xB */

#if defined(STM32F072xB) || defined(STM32F078xx)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t Usart2ClockSelection; /*!< USART2 clock source
                                      This parameter can be a value of @ref RCCEx_USART2_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t CecClockSelection;    /*!< HDMI CEC clock source
                                      This parameter can be a value of @ref RCCEx_CEC_Clock_Source */

  uint32_t UsbClockSelection;    /*!< USB clock source
                                      This parameter can be a value of @ref RCCEx_USB_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F072xB || STM32F078xx */


#if defined(STM32F091xC) || defined(STM32F098xx)
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;    /*!< Specifies RTC Clock Prescalers Selection
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t Usart1ClockSelection; /*!< USART1 clock source
                                      This parameter can be a value of @ref RCC_USART1_Clock_Source */

  uint32_t Usart2ClockSelection; /*!< USART2 clock source
                                      This parameter can be a value of @ref RCCEx_USART2_Clock_Source */

  uint32_t Usart3ClockSelection; /*!< USART3 clock source
                                      This parameter can be a value of @ref RCCEx_USART3_Clock_Source */

  uint32_t I2c1ClockSelection;   /*!< I2C1 clock source
                                      This parameter can be a value of @ref RCC_I2C1_Clock_Source */

  uint32_t CecClockSelection;    /*!< HDMI CEC clock source
                                      This parameter can be a value of @ref RCCEx_CEC_Clock_Source */

}RCC_PeriphCLKInitTypeDef;
#endif /* STM32F091xC || STM32F098xx */

#if defined(CRS)

/**
  * @brief RCC_CRS Init structure definition
  */
typedef struct
{
  uint32_t Prescaler;             /*!< Specifies the division factor of the SYNC signal.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroDivider */

  uint32_t Source;                /*!< Specifies the SYNC signal source.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroSource */

  uint32_t Polarity;              /*!< Specifies the input polarity for the SYNC signal source.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroPolarity */

  uint32_t ReloadValue;           /*!< Specifies the value to be loaded in the frequency error counter with each SYNC event.
                                      It can be calculated in using macro @ref __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)
                                     This parameter must be a number between 0 and 0xFFFF or a value of @ref RCCEx_CRS_ReloadValueDefault .*/

  uint32_t ErrorLimitValue;       /*!< Specifies the value to be used to evaluate the captured frequency error value.
                                     This parameter must be a number between 0 and 0xFF or a value of @ref RCCEx_CRS_ErrorLimitDefault */

  uint32_t HSI48CalibrationValue; /*!< Specifies a user-programmable trimming value to the HSI48 oscillator.
                                     This parameter must be a number between 0 and 0x3F or a value of @ref RCCEx_CRS_HSI48CalibrationDefault */

}RCC_CRSInitTypeDef;

/**
  * @brief RCC_CRS Synchronization structure definition
  */
typedef struct
{
  uint32_t ReloadValue;           /*!< Specifies the value loaded in the Counter reload value.
                                     This parameter must be a number between 0 and 0xFFFFU */

  uint32_t HSI48CalibrationValue; /*!< Specifies value loaded in HSI48 oscillator smooth trimming.
                                     This parameter must be a number between 0 and 0x3FU */

  uint32_t FreqErrorCapture;      /*!< Specifies the value loaded in the .FECAP, the frequency error counter
                                                                    value latched in the time of the last SYNC event.
                                    This parameter must be a number between 0 and 0xFFFFU */

  uint32_t FreqErrorDirection;    /*!< Specifies the value loaded in the .FEDIR, the counting direction of the
                                                                    frequency error counter latched in the time of the last SYNC event.
                                                                    It shows whether the actual frequency is below or above the target.
                                    This parameter must be a value of @ref RCCEx_CRS_FreqErrorDirection*/

}RCC_CRSSynchroInfoTypeDef;

#endif /* CRS */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCCEx_Periph_Clock_Selection RCCEx Periph Clock Selection
  * @{
  */
#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F030xC)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)

#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F038xx ||
          STM32F030xC */

#if defined(STM32F070x6) || defined(STM32F070xB)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)
#define RCC_PERIPHCLK_USB              (0x00020000U)

#endif /* STM32F070x6 || STM32F070xB */

#if defined(STM32F042x6) || defined(STM32F048xx)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_CEC              (0x00000400U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)
#define RCC_PERIPHCLK_USB              (0x00020000U)

#endif /* STM32F042x6 || STM32F048xx */

#if defined(STM32F051x8) || defined(STM32F058xx)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_CEC              (0x00000400U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)

#endif /* STM32F051x8 || STM32F058xx */

#if defined(STM32F071xB)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_USART2           (0x00000002U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_CEC              (0x00000400U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)

#endif /* STM32F071xB */

#if defined(STM32F072xB) || defined(STM32F078xx)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_USART2           (0x00000002U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_CEC              (0x00000400U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)
#define RCC_PERIPHCLK_USB              (0x00020000U)

#endif /* STM32F072xB || STM32F078xx */

#if defined(STM32F091xC) || defined(STM32F098xx)
#define RCC_PERIPHCLK_USART1           (0x00000001U)
#define RCC_PERIPHCLK_USART2           (0x00000002U)
#define RCC_PERIPHCLK_I2C1             (0x00000020U)
#define RCC_PERIPHCLK_CEC              (0x00000400U)
#define RCC_PERIPHCLK_RTC              (0x00010000U)
#define RCC_PERIPHCLK_USART3           (0x00040000U)

#endif /* STM32F091xC || STM32F098xx */

/**
  * @}
  */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) || defined(STM32F078xx)

/** @defgroup RCCEx_USB_Clock_Source RCCEx USB Clock Source
  * @{
  */
#define RCC_USBCLKSOURCE_HSI48         RCC_CFGR3_USBSW_HSI48  /*!< HSI48 clock selected as USB clock source */
#define RCC_USBCLKSOURCE_PLL           RCC_CFGR3_USBSW_PLLCLK /*!< PLL clock (PLLCLK) selected as USB clock */

/**
  * @}
  */

#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || STM32F078xx */

#if defined(STM32F070x6) || defined(STM32F070xB)

/** @defgroup RCCEx_USB_Clock_Source RCCEx USB Clock Source
  * @{
  */
#define RCC_USBCLKSOURCE_NONE          (0x00000000U) /*!< USB clock disabled */
#define RCC_USBCLKSOURCE_PLL           RCC_CFGR3_USBSW_PLLCLK /*!< PLL clock (PLLCLK) selected as USB clock */

/**
  * @}
  */

#endif /* STM32F070x6 || STM32F070xB */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

/** @defgroup RCCEx_USART2_Clock_Source RCCEx USART2 Clock Source
  * @{
  */
#define RCC_USART2CLKSOURCE_PCLK1        RCC_CFGR3_USART2SW_PCLK
#define RCC_USART2CLKSOURCE_SYSCLK       RCC_CFGR3_USART2SW_SYSCLK
#define RCC_USART2CLKSOURCE_LSE          RCC_CFGR3_USART2SW_LSE
#define RCC_USART2CLKSOURCE_HSI          RCC_CFGR3_USART2SW_HSI

/**
  * @}
  */

#endif /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)

/** @defgroup RCCEx_USART3_Clock_Source RCCEx USART3 Clock Source
  * @{
  */
#define RCC_USART3CLKSOURCE_PCLK1        RCC_CFGR3_USART3SW_PCLK
#define RCC_USART3CLKSOURCE_SYSCLK       RCC_CFGR3_USART3SW_SYSCLK
#define RCC_USART3CLKSOURCE_LSE          RCC_CFGR3_USART3SW_LSE
#define RCC_USART3CLKSOURCE_HSI          RCC_CFGR3_USART3SW_HSI

/**
  * @}
  */

#endif /* STM32F091xC || STM32F098xx */


#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

/** @defgroup RCCEx_CEC_Clock_Source RCCEx CEC Clock Source
  * @{
  */
#define RCC_CECCLKSOURCE_HSI             RCC_CFGR3_CECSW_HSI_DIV244
#define RCC_CECCLKSOURCE_LSE             RCC_CFGR3_CECSW_LSE

/**
  * @}
  */

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

/** @defgroup RCCEx_MCOx_Clock_Prescaler RCCEx MCOx Clock Prescaler
  * @{
  */

#if defined(RCC_CFGR_MCOPRE)

#define RCC_MCODIV_1                     (0x00000000U)
#define RCC_MCODIV_2                     (0x10000000U)
#define RCC_MCODIV_4                     (0x20000000U)
#define RCC_MCODIV_8                     (0x30000000U)
#define RCC_MCODIV_16                    (0x40000000U)
#define RCC_MCODIV_32                    (0x50000000U)
#define RCC_MCODIV_64                    (0x60000000U)
#define RCC_MCODIV_128                   (0x70000000U)

#else

#define RCC_MCODIV_1                    (0x00000000U)

#endif /* RCC_CFGR_MCOPRE */

/**
  * @}
  */

/** @defgroup RCCEx_LSEDrive_Configuration RCC LSE Drive Configuration
  * @{
  */

#define RCC_LSEDRIVE_LOW                 (0x00000000U) /*!< Xtal mode lower driving capability */
#define RCC_LSEDRIVE_MEDIUMLOW           RCC_BDCR_LSEDRV_1      /*!< Xtal mode medium low driving capability */
#define RCC_LSEDRIVE_MEDIUMHIGH          RCC_BDCR_LSEDRV_0      /*!< Xtal mode medium high driving capability */
#define RCC_LSEDRIVE_HIGH                RCC_BDCR_LSEDRV        /*!< Xtal mode higher driving capability */

/**
  * @}
  */

#if defined(CRS)

/** @defgroup RCCEx_CRS_Status RCCEx CRS Status
  * @{
  */
#define RCC_CRS_NONE      (0x00000000U)
#define RCC_CRS_TIMEOUT   (0x00000001U)
#define RCC_CRS_SYNCOK    (0x00000002U)
#define RCC_CRS_SYNCWARN  (0x00000004U)
#define RCC_CRS_SYNCERR   (0x00000008U)
#define RCC_CRS_SYNCMISS  (0x00000010U)
#define RCC_CRS_TRIMOVF   (0x00000020U)

/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroSource RCCEx CRS Synchronization Source
  * @{
  */
#define RCC_CRS_SYNC_SOURCE_GPIO       (0x00000000U) /*!< Synchro Signal source GPIO */
#define RCC_CRS_SYNC_SOURCE_LSE        CRS_CFGR_SYNCSRC_0      /*!< Synchro Signal source LSE */
#define RCC_CRS_SYNC_SOURCE_USB        CRS_CFGR_SYNCSRC_1      /*!< Synchro Signal source USB SOF (default)*/
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroDivider RCCEx CRS Synchronization Divider
  * @{
  */
#define RCC_CRS_SYNC_DIV1        (0x00000000U)                   /*!< Synchro Signal not divided (default) */
#define RCC_CRS_SYNC_DIV2        CRS_CFGR_SYNCDIV_0                        /*!< Synchro Signal divided by 2 */
#define RCC_CRS_SYNC_DIV4        CRS_CFGR_SYNCDIV_1                        /*!< Synchro Signal divided by 4 */
#define RCC_CRS_SYNC_DIV8        (CRS_CFGR_SYNCDIV_1 | CRS_CFGR_SYNCDIV_0) /*!< Synchro Signal divided by 8 */
#define RCC_CRS_SYNC_DIV16       CRS_CFGR_SYNCDIV_2                        /*!< Synchro Signal divided by 16 */
#define RCC_CRS_SYNC_DIV32       (CRS_CFGR_SYNCDIV_2 | CRS_CFGR_SYNCDIV_0) /*!< Synchro Signal divided by 32 */
#define RCC_CRS_SYNC_DIV64       (CRS_CFGR_SYNCDIV_2 | CRS_CFGR_SYNCDIV_1) /*!< Synchro Signal divided by 64 */
#define RCC_CRS_SYNC_DIV128      CRS_CFGR_SYNCDIV                          /*!< Synchro Signal divided by 128 */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroPolarity RCCEx CRS Synchronization Polarity
  * @{
  */
#define RCC_CRS_SYNC_POLARITY_RISING   (0x00000000U) /*!< Synchro Active on rising edge (default) */
#define RCC_CRS_SYNC_POLARITY_FALLING  CRS_CFGR_SYNCPOL        /*!< Synchro Active on falling edge */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_ReloadValueDefault RCCEx CRS Default Reload Value
  * @{
  */
#define RCC_CRS_RELOADVALUE_DEFAULT    (0x0000BB7FU) /*!< The reset value of the RELOAD field corresponds
                                                                    to a target frequency of 48 MHz and a synchronization signal frequency of 1 kHz (SOF signal from USB). */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_ErrorLimitDefault RCCEx CRS Default Error Limit Value
  * @{
  */
#define RCC_CRS_ERRORLIMIT_DEFAULT     (0x00000022U) /*!< Default Frequency error limit */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_HSI48CalibrationDefault RCCEx CRS Default HSI48 Calibration vakye
  * @{
  */
#define RCC_CRS_HSI48CALIBRATION_DEFAULT (0x00000020U) /*!< The default value is 32, which corresponds to the middle of the trimming interval.
                                                                      The trimming step is around 67 kHz between two consecutive TRIM steps. A higher TRIM value
                                                                      corresponds to a higher output frequency */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_FreqErrorDirection RCCEx CRS Frequency Error Direction
  * @{
  */
#define RCC_CRS_FREQERRORDIR_UP        (0x00000000U)   /*!< Upcounting direction, the actual frequency is above the target */
#define RCC_CRS_FREQERRORDIR_DOWN      ((uint32_t)CRS_ISR_FEDIR) /*!< Downcounting direction, the actual frequency is below the target */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_Interrupt_Sources RCCEx CRS Interrupt Sources
  * @{
  */
#define RCC_CRS_IT_SYNCOK              CRS_CR_SYNCOKIE           /*!< SYNC event OK */
#define RCC_CRS_IT_SYNCWARN            CRS_CR_SYNCWARNIE         /*!< SYNC warning */
#define RCC_CRS_IT_ERR                 CRS_CR_ERRIE              /*!< Error */
#define RCC_CRS_IT_ESYNC               CRS_CR_ESYNCIE            /*!< Expected SYNC */
#define RCC_CRS_IT_SYNCERR             CRS_CR_ERRIE              /*!< SYNC error */
#define RCC_CRS_IT_SYNCMISS            CRS_CR_ERRIE              /*!< SYNC missed */
#define RCC_CRS_IT_TRIMOVF             CRS_CR_ERRIE              /*!< Trimming overflow or underflow */

/**
  * @}
  */

/** @defgroup RCCEx_CRS_Flags RCCEx CRS Flags
  * @{
  */
#define RCC_CRS_FLAG_SYNCOK            CRS_ISR_SYNCOKF           /*!< SYNC event OK flag     */
#define RCC_CRS_FLAG_SYNCWARN          CRS_ISR_SYNCWARNF         /*!< SYNC warning flag      */
#define RCC_CRS_FLAG_ERR               CRS_ISR_ERRF              /*!< Error flag        */
#define RCC_CRS_FLAG_ESYNC             CRS_ISR_ESYNCF            /*!< Expected SYNC flag     */
#define RCC_CRS_FLAG_SYNCERR           CRS_ISR_SYNCERR           /*!< SYNC error */
#define RCC_CRS_FLAG_SYNCMISS          CRS_ISR_SYNCMISS          /*!< SYNC missed*/
#define RCC_CRS_FLAG_TRIMOVF           CRS_ISR_TRIMOVF           /*!< Trimming overflow or underflow */

/**
  * @}
  */

#endif /* CRS */

/**
  * @}
  */

/* Exported macros ------------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCCEx Exported Macros
  * @{
  */

/** @defgroup RCCEx_Peripheral_Clock_Enable_Disable RCCEx_Peripheral_Clock_Enable_Disable
  * @brief  Enables or disables the AHB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#if defined(GPIOD)

#define __HAL_RCC_GPIOD_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIODEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIODEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_GPIOD_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIODEN))

#endif /* GPIOD */

#if defined(GPIOE)

#define __HAL_RCC_GPIOE_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOEEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOEEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_GPIOE_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIOEEN))

#endif /* GPIOE */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TSC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TSC_CLK_DISABLE()          (RCC->AHBENR &= ~(RCC_AHBENR_TSCEN))

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_DMA2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_DMA2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_DMA2_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_DMA2EN))

#endif /* STM32F091xC || STM32F098xx */

/** @brief  Enable or disable the Low Speed APB (APB1) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  */
#if defined(STM32F030x8)\
 || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USART2_CLK_DISABLE() (RCC->APB1ENR &= ~(RCC_APB1ENR_USART2EN))

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || */
       /* STM32F051x8 || STM32F058xx || STM32F070x6 || */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F030x8)\
 || defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_SPI2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_SPI2_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_SPI2EN))

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TIM2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM2_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM2EN))

#endif /* STM32F031x6 || STM32F038xx ||             */
       /* STM32F042x6 || STM32F048xx ||             */
       /* STM32F051x8 || STM32F058xx ||             */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F030x8) \
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM6_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_I2C2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM6_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_I2C2_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_I2C2EN))

#endif /* STM32F030x8 ||                               */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_DAC1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_DAC1_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_DACEN))

#endif /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CEC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_CECEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_CECEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_CEC_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_CECEN))

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM7_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART3_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART3EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART3EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART4_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART4EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART4EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM7_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM7EN))
#define __HAL_RCC_USART3_CLK_DISABLE() (RCC->APB1ENR &= ~(RCC_APB1ENR_USART3EN))
#define __HAL_RCC_USART4_CLK_DISABLE() (RCC->APB1ENR &= ~(RCC_APB1ENR_USART4EN))

#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)

#define __HAL_RCC_USB_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USB_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_USBEN))

#endif /* STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F072xB || STM32F078xx || STM32F070xB  */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CAN1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_CANEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_CANEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_CAN1_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_CANEN))

#endif /* STM32F042x6 || STM32F048xx || STM32F072xB  || */
       /* STM32F091xC || STM32F098xx */

#if defined(CRS)

#define __HAL_RCC_CRS_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_CRSEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_CRSEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_CRS_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_CRSEN))

#endif /* CRS */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART5_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART5EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART5EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USART5_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_USART5EN))

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

/** @brief  Enable or disable the High Speed APB (APB2) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  */
#if defined(STM32F030x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM15_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM15EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM15EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM15_CLK_DISABLE()   (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM15EN))

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART6_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USART6_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_USART6EN))

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_USART7_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART7EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART7EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART8_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART8EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART8EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USART7_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_USART7EN))
#define __HAL_RCC_USART8_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_USART8EN))

#endif /* STM32F091xC || STM32F098xx */

/**
  * @}
  */


/** @defgroup RCCEx_Force_Release_Peripheral_Reset RCCEx Force Release Peripheral Reset
  * @brief  Forces or releases peripheral reset.
  * @{
  */

/** @brief  Force or release AHB peripheral reset.
  */
#if defined(GPIOD)

#define __HAL_RCC_GPIOD_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIODRST))

#define __HAL_RCC_GPIOD_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIODRST))

#endif /* GPIOD */

#if defined(GPIOE)

#define __HAL_RCC_GPIOE_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOERST))

#define __HAL_RCC_GPIOE_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOERST))

#endif /* GPIOE */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TSC_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_TSCRST))

#define __HAL_RCC_TSC_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_TSCRST))

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

/** @brief  Force or release APB1 peripheral reset.
  */
#if defined(STM32F030x8) \
 || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART2_FORCE_RESET()   (RCC->APB1RSTR |= (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_SPI2_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_SPI2RST))

#define __HAL_RCC_USART2_RELEASE_RESET() (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_SPI2_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_SPI2RST))

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TIM2_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST))

#define __HAL_RCC_TIM2_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST))

#endif /* STM32F031x6 || STM32F038xx ||             */
       /* STM32F042x6 || STM32F048xx ||             */
       /* STM32F051x8 || STM32F058xx ||             */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F030x8) \
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM6_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_I2C2_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_I2C2RST))

#define __HAL_RCC_TIM6_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_I2C2_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C2RST))

#endif /* STM32F030x8 ||                               */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_DAC1_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_DACRST))

#define __HAL_RCC_DAC1_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_DACRST))

#endif /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CEC_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_CECRST))

#define __HAL_RCC_CEC_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_CECRST))

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM7_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_USART3_FORCE_RESET()   (RCC->APB1RSTR |= (RCC_APB1RSTR_USART3RST))
#define __HAL_RCC_USART4_FORCE_RESET()   (RCC->APB1RSTR |= (RCC_APB1RSTR_USART4RST))

#define __HAL_RCC_TIM7_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_USART3_RELEASE_RESET() (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART3RST))
#define __HAL_RCC_USART4_RELEASE_RESET() (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART4RST))

#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)

#define __HAL_RCC_USB_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_USBRST))

#define __HAL_RCC_USB_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USBRST))

#endif /* STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F072xB || STM32F078xx || STM32F070xB */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CAN1_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_CANRST))

#define __HAL_RCC_CAN1_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_CANRST))

#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || */
       /* STM32F091xC || STM32F098xx */

#if defined(CRS)

#define __HAL_RCC_CRS_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_CRSRST))

#define __HAL_RCC_CRS_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_CRSRST))

#endif /* CRS */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART5_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_USART5RST))

#define __HAL_RCC_USART5_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART5RST))

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */


/** @brief  Force or release APB2 peripheral reset.
  */
#if defined(STM32F030x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM15_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM15RST))

#define __HAL_RCC_TIM15_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM15RST))

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART6_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_USART6RST))

#define __HAL_RCC_USART6_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART6RST))

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_USART7_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_USART7RST))
#define __HAL_RCC_USART8_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_USART8RST))

#define __HAL_RCC_USART7_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART7RST))
#define __HAL_RCC_USART8_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART8RST))

#endif /* STM32F091xC || STM32F098xx */

/**
  * @}
  */

/** @defgroup RCCEx_Peripheral_Clock_Enable_Disable_Status Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
/** @brief  AHB Peripheral Clock Enable Disable Status
  */
#if defined(GPIOD)

#define __HAL_RCC_GPIOD_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIODEN)) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIODEN)) == RESET)

#endif /* GPIOD */

#if defined(GPIOE)

#define __HAL_RCC_GPIOE_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIOEEN)) != RESET)
#define __HAL_RCC_GPIOE_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIOEEN)) == RESET)

#endif /* GPIOE */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TSC_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_TSCEN)) != RESET)
#define __HAL_RCC_TSC_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_TSCEN)) == RESET)

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_DMA2_IS_CLK_ENABLED()      ((RCC->AHBENR & (RCC_AHBENR_DMA2EN)) != RESET)
#define __HAL_RCC_DMA2_IS_CLK_DISABLED()     ((RCC->AHBENR & (RCC_AHBENR_DMA2EN)) == RESET)

#endif /* STM32F091xC || STM32F098xx */

/** @brief  APB1 Peripheral Clock Enable Disable Status
  */
#if defined(STM32F030x8)\
 || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART2_IS_CLK_ENABLED()    ((RCC->APB1ENR & (RCC_APB1ENR_USART2EN)) != RESET)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_USART2EN)) == RESET)

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || */
       /* STM32F051x8 || STM32F058xx || STM32F070x6 || */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F030x8)\
 || defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_SPI2_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_SPI2EN)) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_SPI2EN)) == RESET)

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F031x6) || defined(STM32F038xx)\
 || defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_TIM2_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_TIM2EN)) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_TIM2EN)) == RESET)

#endif /* STM32F031x6 || STM32F038xx ||             */
       /* STM32F042x6 || STM32F048xx ||             */
       /* STM32F051x8 || STM32F058xx ||             */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F030x8) \
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM6_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_TIM6EN)) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_I2C2EN)) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_TIM6EN)) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_I2C2EN)) == RESET)

#endif /* STM32F030x8 ||                               */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_DAC1_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_DAC1EN)) != RESET)
#define __HAL_RCC_DAC1_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_DAC1EN)) == RESET)

#endif /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CEC_IS_CLK_ENABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_CECEN)) != RESET)
#define __HAL_RCC_CEC_IS_CLK_DISABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_CECEN)) == RESET)

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM7_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_TIM7EN)) != RESET)
#define __HAL_RCC_USART3_IS_CLK_ENABLED()    ((RCC->APB1ENR & (RCC_APB1ENR_USART3EN)) != RESET)
#define __HAL_RCC_USART4_IS_CLK_ENABLED()    ((RCC->APB1ENR & (RCC_APB1ENR_USART4EN)) != RESET)
#define __HAL_RCC_TIM7_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_TIM7EN)) == RESET)
#define __HAL_RCC_USART3_IS_CLK_DISABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_USART3EN)) == RESET)
#define __HAL_RCC_USART4_IS_CLK_DISABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_USART4EN)) == RESET)

#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)

#define __HAL_RCC_USB_IS_CLK_ENABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_USBEN)) != RESET)
#define __HAL_RCC_USB_IS_CLK_DISABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_USBEN)) == RESET)

#endif /* STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F072xB || STM32F078xx || STM32F070xB  */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB)\
 || defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_CAN1_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_CAN1EN)) != RESET)
#define __HAL_RCC_CAN1_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_CAN1EN)) == RESET)

#endif /* STM32F042x6 || STM32F048xx || STM32F072xB  || */
       /* STM32F091xC || STM32F098xx */

#if defined(CRS)

#define __HAL_RCC_CRS_IS_CLK_ENABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_CRSEN)) != RESET)
#define __HAL_RCC_CRS_IS_CLK_DISABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_CRSEN)) == RESET)

#endif /* CRS */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART5_IS_CLK_ENABLED()    ((RCC->APB1ENR & (RCC_APB1ENR_USART5EN)) != RESET)
#define __HAL_RCC_USART5_IS_CLK_DISABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_USART5EN)) == RESET)

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

/** @brief  APB1 Peripheral Clock Enable Disable Status
  */
#if defined(STM32F030x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F070x6)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)\
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_TIM15_IS_CLK_ENABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_TIM15EN)) != RESET)
#define __HAL_RCC_TIM15_IS_CLK_DISABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_TIM15EN)) == RESET)

#endif /* STM32F030x8 || STM32F042x6 || STM32F048xx || STM32F070x6 || */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB || */
       /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)

#define __HAL_RCC_USART6_IS_CLK_ENABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_USART6EN)) != RESET)
#define __HAL_RCC_USART6_IS_CLK_DISABLED()   ((RCC->APB2ENR & (RCC_APB2ENR_USART6EN)) == RESET)

#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F091xC) || defined(STM32F098xx)

#define __HAL_RCC_USART7_IS_CLK_ENABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_USART7EN)) != RESET)
#define __HAL_RCC_USART8_IS_CLK_ENABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_USART8EN)) != RESET)
#define __HAL_RCC_USART7_IS_CLK_DISABLED()   ((RCC->APB2ENR & (RCC_APB2ENR_USART7EN)) == RESET)
#define __HAL_RCC_USART8_IS_CLK_DISABLED()   ((RCC->APB2ENR & (RCC_APB2ENR_USART8EN)) == RESET)

#endif /* STM32F091xC || STM32F098xx */
/**
  * @}
  */


/** @defgroup RCCEx_HSI48_Enable_Disable RCCEx HSI48 Enable Disable
  * @brief  Macros to enable or disable the Internal 48Mhz High Speed oscillator (HSI48).
  * @note   The HSI48 is stopped by hardware when entering STOP and STANDBY modes.
  * @note   HSI48 can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI14.
  * @note   After enabling the HSI48 with __HAL_RCC_HSI48_ENABLE(), the application software
  *         should wait on HSI48RDY flag to be set indicating that HSI48 clock is stable and can be
  *         used as system clock source. This is not necessary if HAL_RCC_OscConfig() is used.
  * @note   When the HSI48 is stopped, HSI48RDY flag goes low after 6 HSI48 oscillator
  *         clock cycles.
  * @{
  */
#if defined(RCC_HSI48_SUPPORT)

#define __HAL_RCC_HSI48_ENABLE()  SET_BIT(RCC->CR2, RCC_CR2_HSI48ON)
#define __HAL_RCC_HSI48_DISABLE() CLEAR_BIT(RCC->CR2, RCC_CR2_HSI48ON)

/** @brief  Macro to get the Internal 48Mhz High Speed oscillator (HSI48) state.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_HSI48_ON  HSI48 enabled
  *            @arg @ref RCC_HSI48_OFF HSI48 disabled
  */
#define __HAL_RCC_GET_HSI48_STATE() \
                  (((uint32_t)(READ_BIT(RCC->CR2, RCC_CR2_HSI48ON)) != RESET) ? RCC_HSI48_ON : RCC_HSI48_OFF)

#endif /* RCC_HSI48_SUPPORT */

/**
  * @}
  */

/** @defgroup RCCEx_Peripheral_Clock_Source_Config RCCEx Peripheral Clock Source Config
  * @{
  */
#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F070x6) || defined(STM32F070xB)

/** @brief  Macro to configure the USB clock (USBCLK).
  * @param  __USBCLKSOURCE__ specifies the USB clock source.
  *         This parameter can be one of the following values:
@if STM32F070xB
@elseif STM32F070x6
@else
  *            @arg @ref RCC_USBCLKSOURCE_HSI48  HSI48 selected as USB clock
@endif
  *            @arg @ref RCC_USBCLKSOURCE_PLL PLL Clock selected as USB clock
  */
#define __HAL_RCC_USB_CONFIG(__USBCLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USBSW, (uint32_t)(__USBCLKSOURCE__))

/** @brief  Macro to get the USB clock source.
  * @retval The clock source can be one of the following values:
@if STM32F070xB
@elseif STM32F070x6
@else
  *            @arg @ref RCC_USBCLKSOURCE_HSI48  HSI48 selected as USB clock
@endif
  *            @arg @ref RCC_USBCLKSOURCE_PLL PLL Clock selected as USB clock
  */
#define __HAL_RCC_GET_USB_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_USBSW)))

#endif /* STM32F042x6 || STM32F048xx || */
       /* STM32F072xB || STM32F078xx || */
       /* STM32F070x6 || STM32F070xB    */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)

/** @brief  Macro to configure the CEC clock.
  * @param  __CECCLKSOURCE__ specifies the CEC clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_CECCLKSOURCE_HSI HSI selected as CEC clock
  *            @arg @ref RCC_CECCLKSOURCE_LSE LSE selected as CEC clock
  */
#define __HAL_RCC_CEC_CONFIG(__CECCLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_CECSW, (uint32_t)(__CECCLKSOURCE__))

/** @brief  Macro to get the HDMI CEC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_CECCLKSOURCE_HSI HSI selected as CEC clock
  *            @arg @ref RCC_CECCLKSOURCE_LSE LSE selected as CEC clock
  */
#define __HAL_RCC_GET_CEC_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_CECSW)))

#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || defined(STM32F098xx) */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)
/** @brief  Macro to configure the USART2 clock (USART2CLK).
  * @param  __USART2CLKSOURCE__ specifies the USART2 clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1 PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE LSE selected as USART2 clock
  */
#define __HAL_RCC_USART2_CONFIG(__USART2CLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USART2SW, (uint32_t)(__USART2CLKSOURCE__))

/** @brief  Macro to get the USART2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1 PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE LSE selected as USART2 clock
  */
#define __HAL_RCC_GET_USART2_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_USART2SW)))
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F091xC || STM32F098xx*/

#if defined(STM32F091xC) || defined(STM32F098xx)
/** @brief  Macro to configure the USART3 clock (USART3CLK).
  * @param  __USART3CLKSOURCE__ specifies the USART3 clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_USART3CLKSOURCE_PCLK1 PCLK1 selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_HSI HSI selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_SYSCLK System Clock selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_LSE LSE selected as USART3 clock
  */
#define __HAL_RCC_USART3_CONFIG(__USART3CLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USART3SW, (uint32_t)(__USART3CLKSOURCE__))

/** @brief  Macro to get the USART3 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART3CLKSOURCE_PCLK1 PCLK1 selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_HSI HSI selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_SYSCLK System Clock selected as USART3 clock
  *            @arg @ref RCC_USART3CLKSOURCE_LSE LSE selected as USART3 clock
  */
#define __HAL_RCC_GET_USART3_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_USART3SW)))

#endif /* STM32F091xC || STM32F098xx */
/**
  * @}
  */

/** @defgroup RCCEx_LSE_Configuration LSE Drive Configuration
  * @{
  */

/**
  * @brief  Macro to configure the External Low Speed oscillator (LSE) drive capability.
  * @param  __RCC_LSEDRIVE__ specifies the new state of the LSE drive capability.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LSEDRIVE_LOW        LSE oscillator low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMLOW  LSE oscillator medium low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMHIGH LSE oscillator medium high drive capability.
  *            @arg @ref RCC_LSEDRIVE_HIGH       LSE oscillator high drive capability.
  * @retval None
  */
#define __HAL_RCC_LSEDRIVE_CONFIG(__RCC_LSEDRIVE__) (MODIFY_REG(RCC->BDCR,\
        RCC_BDCR_LSEDRV, (uint32_t)(__RCC_LSEDRIVE__) ))

/**
  * @}
  */

#if defined(CRS)

/** @defgroup RCCEx_IT_And_Flag RCCEx IT and Flag
  * @{
  */
/* Interrupt & Flag management */

/**
  * @brief  Enable the specified CRS interrupts.
  * @param  __INTERRUPT__ specifies the CRS interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK  SYNC event OK interrupt
  *              @arg @ref RCC_CRS_IT_SYNCWARN  SYNC warning interrupt
  *              @arg @ref RCC_CRS_IT_ERR  Synchronization or trimming error interrupt
  *              @arg @ref RCC_CRS_IT_ESYNC  Expected SYNC interrupt
  * @retval None
  */
#define __HAL_RCC_CRS_ENABLE_IT(__INTERRUPT__)   SET_BIT(CRS->CR, (__INTERRUPT__))

/**
  * @brief  Disable the specified CRS interrupts.
  * @param  __INTERRUPT__ specifies the CRS interrupt sources to be disabled.
  *          This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK  SYNC event OK interrupt
  *              @arg @ref RCC_CRS_IT_SYNCWARN  SYNC warning interrupt
  *              @arg @ref RCC_CRS_IT_ERR  Synchronization or trimming error interrupt
  *              @arg @ref RCC_CRS_IT_ESYNC  Expected SYNC interrupt
  * @retval None
  */
#define __HAL_RCC_CRS_DISABLE_IT(__INTERRUPT__)  CLEAR_BIT(CRS->CR, (__INTERRUPT__))

/** @brief  Check whether the CRS interrupt has occurred or not.
  * @param  __INTERRUPT__ specifies the CRS interrupt source to check.
  *         This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK  SYNC event OK interrupt
  *              @arg @ref RCC_CRS_IT_SYNCWARN  SYNC warning interrupt
  *              @arg @ref RCC_CRS_IT_ERR  Synchronization or trimming error interrupt
  *              @arg @ref RCC_CRS_IT_ESYNC  Expected SYNC interrupt
  * @retval The new state of __INTERRUPT__ (SET or RESET).
  */
#define __HAL_RCC_CRS_GET_IT_SOURCE(__INTERRUPT__)  ((READ_BIT(CRS->CR, (__INTERRUPT__)) != RESET) ? SET : RESET)

/** @brief  Clear the CRS interrupt pending bits
  * @param  __INTERRUPT__ specifies the interrupt pending bit to clear.
  *         This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK  SYNC event OK interrupt
  *              @arg @ref RCC_CRS_IT_SYNCWARN  SYNC warning interrupt
  *              @arg @ref RCC_CRS_IT_ERR  Synchronization or trimming error interrupt
  *              @arg @ref RCC_CRS_IT_ESYNC  Expected SYNC interrupt
  *              @arg @ref RCC_CRS_IT_TRIMOVF  Trimming overflow or underflow interrupt
  *              @arg @ref RCC_CRS_IT_SYNCERR  SYNC error interrupt
  *              @arg @ref RCC_CRS_IT_SYNCMISS  SYNC missed interrupt
  */
#define __HAL_RCC_CRS_CLEAR_IT(__INTERRUPT__)  do { \
                                                 if(((__INTERRUPT__) & RCC_CRS_IT_ERROR_MASK) != RESET) \
                                                 { \
                                                   WRITE_REG(CRS->ICR, CRS_ICR_ERRC | ((__INTERRUPT__) & ~RCC_CRS_IT_ERROR_MASK)); \
                                                 } \
                                                 else \
                                                 { \
                                                   WRITE_REG(CRS->ICR, (__INTERRUPT__)); \
                                                 } \
                                               } while(0U)

/**
  * @brief  Check whether the specified CRS flag is set or not.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_FLAG_SYNCOK  SYNC event OK
  *              @arg @ref RCC_CRS_FLAG_SYNCWARN  SYNC warning
  *              @arg @ref RCC_CRS_FLAG_ERR  Error
  *              @arg @ref RCC_CRS_FLAG_ESYNC  Expected SYNC
  *              @arg @ref RCC_CRS_FLAG_TRIMOVF  Trimming overflow or underflow
  *              @arg @ref RCC_CRS_FLAG_SYNCERR  SYNC error
  *              @arg @ref RCC_CRS_FLAG_SYNCMISS  SYNC missed
  * @retval The new state of _FLAG_ (TRUE or FALSE).
  */
#define __HAL_RCC_CRS_GET_FLAG(__FLAG__)  (READ_BIT(CRS->ISR, (__FLAG__)) == (__FLAG__))

/**
  * @brief  Clear the CRS specified FLAG.
  * @param __FLAG__ specifies the flag to clear.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_FLAG_SYNCOK  SYNC event OK
  *              @arg @ref RCC_CRS_FLAG_SYNCWARN  SYNC warning
  *              @arg @ref RCC_CRS_FLAG_ERR  Error
  *              @arg @ref RCC_CRS_FLAG_ESYNC  Expected SYNC
  *              @arg @ref RCC_CRS_FLAG_TRIMOVF  Trimming overflow or underflow
  *              @arg @ref RCC_CRS_FLAG_SYNCERR  SYNC error
  *              @arg @ref RCC_CRS_FLAG_SYNCMISS  SYNC missed
  * @note RCC_CRS_FLAG_ERR clears RCC_CRS_FLAG_TRIMOVF, RCC_CRS_FLAG_SYNCERR, RCC_CRS_FLAG_SYNCMISS and consequently RCC_CRS_FLAG_ERR
  * @retval None
  */
#define __HAL_RCC_CRS_CLEAR_FLAG(__FLAG__)     do { \
                                                 if(((__FLAG__) & RCC_CRS_FLAG_ERROR_MASK) != RESET) \
                                                 { \
                                                   WRITE_REG(CRS->ICR, CRS_ICR_ERRC | ((__FLAG__) & ~RCC_CRS_FLAG_ERROR_MASK)); \
                                                 } \
                                                 else \
                                                 { \
                                                   WRITE_REG(CRS->ICR, (__FLAG__)); \
                                                 } \
                                               } while(0U)

/**
  * @}
  */

/** @defgroup RCCEx_CRS_Extended_Features RCCEx CRS Extended Features
  * @{
  */
/**
  * @brief  Enable the oscillator clock for frequency error counter.
  * @note   when the CEN bit is set the CRS_CFGR register becomes write-protected.
  * @retval None
  */
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_ENABLE()  SET_BIT(CRS->CR, CRS_CR_CEN)

/**
  * @brief  Disable the oscillator clock for frequency error counter.
  * @retval None
  */
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_DISABLE() CLEAR_BIT(CRS->CR, CRS_CR_CEN)

/**
  * @brief  Enable the automatic hardware adjustement of TRIM bits.
  * @note   When the AUTOTRIMEN bit is set the CRS_CFGR register becomes write-protected.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_ENABLE()     SET_BIT(CRS->CR, CRS_CR_AUTOTRIMEN)

/**
  * @brief  Disable the automatic hardware adjustement of TRIM bits.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_DISABLE()    CLEAR_BIT(CRS->CR, CRS_CR_AUTOTRIMEN)

/**
  * @brief  Macro to calculate reload value to be set in CRS register according to target and sync frequencies
  * @note   The RELOAD value should be selected according to the ratio between the target frequency and the frequency
  *             of the synchronization source after prescaling. It is then decreased by one in order to
  *             reach the expected synchronization on the zero value. The formula is the following:
  *             RELOAD = (fTARGET / fSYNC) -1
  * @param  __FTARGET__ Target frequency (value in Hz)
  * @param  __FSYNC__   Synchronization signal frequency (value in Hz)
  * @retval None
  */
#define __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)  (((__FTARGET__) / (__FSYNC__)) - 1U)

/**
  * @}
  */

#endif /* CRS */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCCEx_Exported_Functions
  * @{
  */

/** @addtogroup RCCEx_Exported_Functions_Group1
  * @{
  */

HAL_StatusTypeDef     HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void                  HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
uint32_t              HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk);

/**
  * @}
  */

#if defined(CRS)

/** @addtogroup RCCEx_Exported_Functions_Group3
  * @{
  */

void              HAL_RCCEx_CRSConfig(RCC_CRSInitTypeDef *pInit);
void              HAL_RCCEx_CRSSoftwareSynchronizationGenerate(void);
void              HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo);
uint32_t          HAL_RCCEx_CRSWaitSynchronization(uint32_t Timeout);
void              HAL_RCCEx_CRS_IRQHandler(void);
void              HAL_RCCEx_CRS_SyncOkCallback(void);
void              HAL_RCCEx_CRS_SyncWarnCallback(void);
void              HAL_RCCEx_CRS_ExpectedSyncCallback(void);
void              HAL_RCCEx_CRS_ErrorCallback(uint32_t Error);

/**
  * @}
  */

#endif /* CRS */

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

#endif /* __STM32F0xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
