/**
  ******************************************************************************
  * @file    stm32f7xx_ll_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC LL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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
#ifndef __STM32F7xx_LL_RCC_H
#define __STM32F7xx_LL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"

/** @addtogroup STM32F7xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @defgroup RCC_LL RCC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup RCC_LL_Private_Variables RCC Private Variables
  * @{
  */

#if defined(RCC_DCKCFGR1_PLLSAIDIVR)
static const uint8_t aRCC_PLLSAIDIVRPrescTable[4] = {2, 4, 8, 16};
#endif /* RCC_DCKCFGR1_PLLSAIDIVR */

/**
  * @}
  */
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_Private_Macros RCC Private Macros
  * @{
  */
/**
  * @}
  */
#endif /*USE_FULL_LL_DRIVER*/
/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_Exported_Types RCC Exported Types
  * @{
  */

/** @defgroup LL_ES_CLOCK_FREQ Clocks Frequency Structure
  * @{
  */

/**
  * @brief  RCC Clocks Frequency Structure
  */
typedef struct
{
  uint32_t SYSCLK_Frequency;        /*!< SYSCLK clock frequency */
  uint32_t HCLK_Frequency;          /*!< HCLK clock frequency */
  uint32_t PCLK1_Frequency;         /*!< PCLK1 clock frequency */
  uint32_t PCLK2_Frequency;         /*!< PCLK2 clock frequency */
} LL_RCC_ClocksTypeDef;

/**
  * @}
  */

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCC_LL_Exported_Constants RCC Exported Constants
  * @{
  */

/** @defgroup RCC_LL_EC_OSC_VALUES Oscillator Values adaptation
  * @brief    Defines used to adapt values of different oscillators
  * @note     These values could be modified in the user environment according to 
  *           HW set-up.
  * @{
  */
#if !defined  (HSE_VALUE)
#define HSE_VALUE    25000000U  /*!< Value of the HSE oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    16000000U  /*!< Value of the HSI oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE    32768U     /*!< Value of the LSE oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE    32000U     /*!< Value of the LSI oscillator in Hz */
#endif /* LSI_VALUE */

#if !defined  (EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE    12288000U /*!< Value of the I2S_CKIN external oscillator in Hz */
#endif /* EXTERNAL_CLOCK_VALUE */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CLEAR_FLAG Clear Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_WriteReg function
  * @{
  */
#define LL_RCC_CIR_LSIRDYC                RCC_CIR_LSIRDYC     /*!< LSI Ready Interrupt Clear */
#define LL_RCC_CIR_LSERDYC                RCC_CIR_LSERDYC     /*!< LSE Ready Interrupt Clear */
#define LL_RCC_CIR_HSIRDYC                RCC_CIR_HSIRDYC     /*!< HSI Ready Interrupt Clear */
#define LL_RCC_CIR_HSERDYC                RCC_CIR_HSERDYC     /*!< HSE Ready Interrupt Clear */
#define LL_RCC_CIR_PLLRDYC                RCC_CIR_PLLRDYC     /*!< PLL Ready Interrupt Clear */
#define LL_RCC_CIR_PLLI2SRDYC             RCC_CIR_PLLI2SRDYC  /*!< PLLI2S Ready Interrupt Clear */
#define LL_RCC_CIR_PLLSAIRDYC             RCC_CIR_PLLSAIRDYC  /*!< PLLSAI Ready Interrupt Clear */
#define LL_RCC_CIR_CSSC                   RCC_CIR_CSSC        /*!< Clock Security System Interrupt Clear */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_ReadReg function
  * @{
  */
#define LL_RCC_CIR_LSIRDYF                RCC_CIR_LSIRDYF     /*!< LSI Ready Interrupt flag */
#define LL_RCC_CIR_LSERDYF                RCC_CIR_LSERDYF     /*!< LSE Ready Interrupt flag */
#define LL_RCC_CIR_HSIRDYF                RCC_CIR_HSIRDYF     /*!< HSI Ready Interrupt flag */
#define LL_RCC_CIR_HSERDYF                RCC_CIR_HSERDYF     /*!< HSE Ready Interrupt flag */
#define LL_RCC_CIR_PLLRDYF                RCC_CIR_PLLRDYF     /*!< PLL Ready Interrupt flag */
#define LL_RCC_CIR_PLLI2SRDYF             RCC_CIR_PLLI2SRDYF  /*!< PLLI2S Ready Interrupt flag */
#define LL_RCC_CIR_PLLSAIRDYF             RCC_CIR_PLLSAIRDYF  /*!< PLLSAI Ready Interrupt flag */
#define LL_RCC_CIR_CSSF                   RCC_CIR_CSSF        /*!< Clock Security System Interrupt flag */
#define LL_RCC_CSR_LPWRRSTF                RCC_CSR_LPWRRSTF   /*!< Low-Power reset flag */
#define LL_RCC_CSR_PINRSTF                 RCC_CSR_PINRSTF    /*!< PIN reset flag */
#define LL_RCC_CSR_PORRSTF                 RCC_CSR_PORRSTF    /*!< POR/PDR reset flag */
#define LL_RCC_CSR_SFTRSTF                 RCC_CSR_SFTRSTF    /*!< Software Reset flag */
#define LL_RCC_CSR_IWDGRSTF                RCC_CSR_IWDGRSTF   /*!< Independent Watchdog reset flag */
#define LL_RCC_CSR_WWDGRSTF                RCC_CSR_WWDGRSTF   /*!< Window watchdog reset flag */
#define LL_RCC_CSR_BORRSTF                 RCC_CSR_BORRSTF    /*!< BOR reset flag */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_RCC_ReadReg and  LL_RCC_WriteReg functions
  * @{
  */
#define LL_RCC_CIR_LSIRDYIE               RCC_CIR_LSIRDYIE      /*!< LSI Ready Interrupt Enable */
#define LL_RCC_CIR_LSERDYIE               RCC_CIR_LSERDYIE      /*!< LSE Ready Interrupt Enable */
#define LL_RCC_CIR_HSIRDYIE               RCC_CIR_HSIRDYIE      /*!< HSI Ready Interrupt Enable */
#define LL_RCC_CIR_HSERDYIE               RCC_CIR_HSERDYIE      /*!< HSE Ready Interrupt Enable */
#define LL_RCC_CIR_PLLRDYIE               RCC_CIR_PLLRDYIE      /*!< PLL Ready Interrupt Enable */
#define LL_RCC_CIR_PLLI2SRDYIE            RCC_CIR_PLLI2SRDYIE   /*!< PLLI2S Ready Interrupt Enable */
#define LL_RCC_CIR_PLLSAIRDYIE            RCC_CIR_PLLSAIRDYIE   /*!< PLLSAI Ready Interrupt Enable */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSEDRIVE  LSE oscillator drive capability
  * @{
  */
#define LL_RCC_LSEDRIVE_LOW                0x00000000U             /*!< Xtal mode lower driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMHIGH         RCC_BDCR_LSEDRV_0       /*!< Xtal mode medium high driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMLOW          RCC_BDCR_LSEDRV_1       /*!< Xtal mode medium low driving capability */
#define LL_RCC_LSEDRIVE_HIGH               RCC_BDCR_LSEDRV         /*!< Xtal mode higher driving capability */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE  System clock switch
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_HSI           RCC_CFGR_SW_HSI    /*!< HSI selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_HSE           RCC_CFGR_SW_HSE    /*!< HSE selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_PLL           RCC_CFGR_SW_PLL    /*!< PLL selection as system clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE_STATUS  System clock switch status
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSI    RCC_CFGR_SWS_HSI   /*!< HSI used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSE    RCC_CFGR_SWS_HSE   /*!< HSE used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_PLL    RCC_CFGR_SWS_PLL   /*!< PLL used as system clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYSCLK_DIV  AHB prescaler
  * @{
  */
#define LL_RCC_SYSCLK_DIV_1                RCC_CFGR_HPRE_DIV1   /*!< SYSCLK not divided */
#define LL_RCC_SYSCLK_DIV_2                RCC_CFGR_HPRE_DIV2   /*!< SYSCLK divided by 2 */
#define LL_RCC_SYSCLK_DIV_4                RCC_CFGR_HPRE_DIV4   /*!< SYSCLK divided by 4 */
#define LL_RCC_SYSCLK_DIV_8                RCC_CFGR_HPRE_DIV8   /*!< SYSCLK divided by 8 */
#define LL_RCC_SYSCLK_DIV_16               RCC_CFGR_HPRE_DIV16  /*!< SYSCLK divided by 16 */
#define LL_RCC_SYSCLK_DIV_64               RCC_CFGR_HPRE_DIV64  /*!< SYSCLK divided by 64 */
#define LL_RCC_SYSCLK_DIV_128              RCC_CFGR_HPRE_DIV128 /*!< SYSCLK divided by 128 */
#define LL_RCC_SYSCLK_DIV_256              RCC_CFGR_HPRE_DIV256 /*!< SYSCLK divided by 256 */
#define LL_RCC_SYSCLK_DIV_512              RCC_CFGR_HPRE_DIV512 /*!< SYSCLK divided by 512 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB1_DIV  APB low-speed prescaler (APB1)
  * @{
  */
#define LL_RCC_APB1_DIV_1                  RCC_CFGR_PPRE1_DIV1  /*!< HCLK not divided */
#define LL_RCC_APB1_DIV_2                  RCC_CFGR_PPRE1_DIV2  /*!< HCLK divided by 2 */
#define LL_RCC_APB1_DIV_4                  RCC_CFGR_PPRE1_DIV4  /*!< HCLK divided by 4 */
#define LL_RCC_APB1_DIV_8                  RCC_CFGR_PPRE1_DIV8  /*!< HCLK divided by 8 */
#define LL_RCC_APB1_DIV_16                 RCC_CFGR_PPRE1_DIV16 /*!< HCLK divided by 16 */
/**
  * @}
  */
/** @defgroup RCC_LL_EC_APB2_DIV  APB high-speed prescaler (APB2)
  * @{
  */
#define LL_RCC_APB2_DIV_1                  RCC_CFGR_PPRE2_DIV1  /*!< HCLK not divided */
#define LL_RCC_APB2_DIV_2                  RCC_CFGR_PPRE2_DIV2  /*!< HCLK divided by 2 */
#define LL_RCC_APB2_DIV_4                  RCC_CFGR_PPRE2_DIV4  /*!< HCLK divided by 4 */
#define LL_RCC_APB2_DIV_8                  RCC_CFGR_PPRE2_DIV8  /*!< HCLK divided by 8 */
#define LL_RCC_APB2_DIV_16                 RCC_CFGR_PPRE2_DIV16 /*!< HCLK divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCOxSOURCE  MCO source selection
  * @{
  */
#define LL_RCC_MCO1SOURCE_HSI              (uint32_t)(RCC_CFGR_MCO1|0x00000000U)                    /*!< HSI selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_LSE              (uint32_t)(RCC_CFGR_MCO1|(RCC_CFGR_MCO1_0 >> 16U))       /*!< LSE selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_HSE              (uint32_t)(RCC_CFGR_MCO1|(RCC_CFGR_MCO1_1 >> 16U))       /*!< HSE selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_PLLCLK           (uint32_t)(RCC_CFGR_MCO1|((RCC_CFGR_MCO1_1|RCC_CFGR_MCO1_0) >> 16U))       /*!< PLLCLK selection as MCO1 source */
#define LL_RCC_MCO2SOURCE_SYSCLK           (uint32_t)(RCC_CFGR_MCO2|0x00000000U)                    /*!< SYSCLK selection as MCO2 source */
#define LL_RCC_MCO2SOURCE_PLLI2S           (uint32_t)(RCC_CFGR_MCO2|(RCC_CFGR_MCO2_0 >> 16U))       /*!< PLLI2S selection as MCO2 source */
#define LL_RCC_MCO2SOURCE_HSE              (uint32_t)(RCC_CFGR_MCO2|(RCC_CFGR_MCO2_1 >> 16U))       /*!< HSE selection as MCO2 source */
#define LL_RCC_MCO2SOURCE_PLLCLK           (uint32_t)(RCC_CFGR_MCO2|((RCC_CFGR_MCO2_1|RCC_CFGR_MCO2_0) >> 16U))       /*!< PLLCLK selection as MCO2 source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCOx_DIV  MCO prescaler
  * @{
  */
#define LL_RCC_MCO1_DIV_1                  (uint32_t)(RCC_CFGR_MCO1PRE|0x00000000U)                       /*!< MCO1 not divided */
#define LL_RCC_MCO1_DIV_2                  (uint32_t)(RCC_CFGR_MCO1PRE|(RCC_CFGR_MCO1PRE_2 >> 16U))       /*!< MCO1 divided by 2 */
#define LL_RCC_MCO1_DIV_3                  (uint32_t)(RCC_CFGR_MCO1PRE|((RCC_CFGR_MCO1PRE_2|RCC_CFGR_MCO1PRE_0) >> 16U))       /*!< MCO1 divided by 3 */
#define LL_RCC_MCO1_DIV_4                  (uint32_t)(RCC_CFGR_MCO1PRE|((RCC_CFGR_MCO1PRE_2|RCC_CFGR_MCO1PRE_1) >> 16U))       /*!< MCO1 divided by 4 */
#define LL_RCC_MCO1_DIV_5                  (uint32_t)(RCC_CFGR_MCO1PRE|(RCC_CFGR_MCO1PRE >> 16U))         /*!< MCO1 divided by 5 */
#define LL_RCC_MCO2_DIV_1                  (uint32_t)(RCC_CFGR_MCO2PRE|0x00000000U)                       /*!< MCO2 not divided */
#define LL_RCC_MCO2_DIV_2                  (uint32_t)(RCC_CFGR_MCO2PRE|(RCC_CFGR_MCO2PRE_2 >> 16U))       /*!< MCO2 divided by 2 */
#define LL_RCC_MCO2_DIV_3                  (uint32_t)(RCC_CFGR_MCO2PRE|((RCC_CFGR_MCO2PRE_2|RCC_CFGR_MCO2PRE_0) >> 16U))       /*!< MCO2 divided by 3 */
#define LL_RCC_MCO2_DIV_4                  (uint32_t)(RCC_CFGR_MCO2PRE|((RCC_CFGR_MCO2PRE_2|RCC_CFGR_MCO2PRE_1) >> 16U))       /*!< MCO2 divided by 4 */
#define LL_RCC_MCO2_DIV_5                  (uint32_t)(RCC_CFGR_MCO2PRE|(RCC_CFGR_MCO2PRE >> 16U))         /*!< MCO2 divided by 5 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RTC_HSEDIV  HSE prescaler for RTC clock
  * @{
  */
#define LL_RCC_RTC_NOCLOCK                  0x00000000U             /*!< HSE not divided */
#define LL_RCC_RTC_HSE_DIV_2                RCC_CFGR_RTCPRE_1       /*!< HSE clock divided by 2 */
#define LL_RCC_RTC_HSE_DIV_3                (RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 3 */
#define LL_RCC_RTC_HSE_DIV_4                RCC_CFGR_RTCPRE_2       /*!< HSE clock divided by 4 */
#define LL_RCC_RTC_HSE_DIV_5                (RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 5 */
#define LL_RCC_RTC_HSE_DIV_6                (RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 6 */
#define LL_RCC_RTC_HSE_DIV_7                (RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 7 */
#define LL_RCC_RTC_HSE_DIV_8                RCC_CFGR_RTCPRE_3       /*!< HSE clock divided by 8 */
#define LL_RCC_RTC_HSE_DIV_9                (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 9 */
#define LL_RCC_RTC_HSE_DIV_10               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 10 */
#define LL_RCC_RTC_HSE_DIV_11               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 11 */
#define LL_RCC_RTC_HSE_DIV_12               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2)       /*!< HSE clock divided by 12 */
#define LL_RCC_RTC_HSE_DIV_13               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 13 */
#define LL_RCC_RTC_HSE_DIV_14               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 14 */
#define LL_RCC_RTC_HSE_DIV_15               (RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 15 */
#define LL_RCC_RTC_HSE_DIV_16               RCC_CFGR_RTCPRE_4       /*!< HSE clock divided by 16 */
#define LL_RCC_RTC_HSE_DIV_17               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 17 */
#define LL_RCC_RTC_HSE_DIV_18               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 18 */
#define LL_RCC_RTC_HSE_DIV_19               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 19 */
#define LL_RCC_RTC_HSE_DIV_20               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_2)       /*!< HSE clock divided by 20 */
#define LL_RCC_RTC_HSE_DIV_21               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 21 */
#define LL_RCC_RTC_HSE_DIV_22               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 22 */
#define LL_RCC_RTC_HSE_DIV_23               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 23 */
#define LL_RCC_RTC_HSE_DIV_24               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3)       /*!< HSE clock divided by 24 */
#define LL_RCC_RTC_HSE_DIV_25               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 25 */
#define LL_RCC_RTC_HSE_DIV_26               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 26 */
#define LL_RCC_RTC_HSE_DIV_27               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 27 */
#define LL_RCC_RTC_HSE_DIV_28               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2)       /*!< HSE clock divided by 28 */
#define LL_RCC_RTC_HSE_DIV_29               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 29 */
#define LL_RCC_RTC_HSE_DIV_30               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1)       /*!< HSE clock divided by 30 */
#define LL_RCC_RTC_HSE_DIV_31               (RCC_CFGR_RTCPRE_4|RCC_CFGR_RTCPRE_3|RCC_CFGR_RTCPRE_2|RCC_CFGR_RTCPRE_1|RCC_CFGR_RTCPRE_0)       /*!< HSE clock divided by 31 */
/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EC_PERIPH_FREQUENCY Peripheral clock frequency
  * @{
  */
#define LL_RCC_PERIPH_FREQUENCY_NO         0x00000000U                 /*!< No clock enabled for the peripheral            */
#define LL_RCC_PERIPH_FREQUENCY_NA         0xFFFFFFFFU                 /*!< Frequency cannot be provided as external clock */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/** @defgroup RCC_LL_EC_USARTx_CLKSOURCE  Peripheral USART clock source selection
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE_PCLK2      (uint32_t)((RCC_DCKCFGR2_USART1SEL << 16U) | 0x00000000U)   /*!< PCLK2 clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_USART1SEL << 16U) | RCC_DCKCFGR2_USART1SEL_0) /*!< SYSCLK clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_USART1SEL << 16U) | RCC_DCKCFGR2_USART1SEL_1) /*!< HSI clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_USART1SEL << 16U) | RCC_DCKCFGR2_USART1SEL)   /*!< LSE clock used as USART1 clock source */
#define LL_RCC_USART2_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_USART2SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_USART2SEL << 16U) | RCC_DCKCFGR2_USART2SEL_0) /*!< SYSCLK clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_USART2SEL << 16U) | RCC_DCKCFGR2_USART2SEL_1) /*!< HSI clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_USART2SEL << 16U) | RCC_DCKCFGR2_USART2SEL)   /*!< LSE clock used as USART2 clock source */
#define LL_RCC_USART3_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_USART3SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_USART3SEL << 16U) | RCC_DCKCFGR2_USART3SEL_0) /*!< SYSCLK clock used as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_USART3SEL << 16U) | RCC_DCKCFGR2_USART3SEL_1) /*!< HSI clock used as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_USART3SEL << 16U) | RCC_DCKCFGR2_USART3SEL)   /*!< LSE clock used as USART3 clock source */
#define LL_RCC_USART6_CLKSOURCE_PCLK2      (uint32_t)((RCC_DCKCFGR2_USART6SEL << 16U) | 0x00000000U)   /*!< PCLK2 clock used as USART6 clock source */
#define LL_RCC_USART6_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_USART6SEL << 16U) | RCC_DCKCFGR2_USART6SEL_0) /*!< SYSCLK clock used as USART6 clock source */
#define LL_RCC_USART6_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_USART6SEL << 16U) | RCC_DCKCFGR2_USART6SEL_1) /*!< HSI clock used as USART6 clock source */
#define LL_RCC_USART6_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_USART6SEL << 16U) | RCC_DCKCFGR2_USART6SEL)   /*!< LSE clock used as USART6 clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_UARTx_CLKSOURCE  Peripheral UART clock source selection
  * @{
  */
#define LL_RCC_UART4_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_UART4SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as UART4 clock source */
#define LL_RCC_UART4_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_UART4SEL << 16U) | RCC_DCKCFGR2_UART4SEL_0) /*!< SYSCLK clock used as UART4 clock source */
#define LL_RCC_UART4_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_UART4SEL << 16U) | RCC_DCKCFGR2_UART4SEL_1) /*!< HSI clock used as UART4 clock source */
#define LL_RCC_UART4_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_UART4SEL << 16U) | RCC_DCKCFGR2_UART4SEL)   /*!< LSE clock used as UART4 clock source */
#define LL_RCC_UART5_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_UART5SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as UART5 clock source */
#define LL_RCC_UART5_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_UART5SEL << 16U) | RCC_DCKCFGR2_UART5SEL_0) /*!< SYSCLK clock used as UART5 clock source */
#define LL_RCC_UART5_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_UART5SEL << 16U) | RCC_DCKCFGR2_UART5SEL_1) /*!< HSI clock used as UART5 clock source */
#define LL_RCC_UART5_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_UART5SEL << 16U) | RCC_DCKCFGR2_UART5SEL)   /*!< LSE clock used as UART5 clock source */
#define LL_RCC_UART7_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_UART7SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as UART7 clock source */
#define LL_RCC_UART7_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_UART7SEL << 16U) | RCC_DCKCFGR2_UART7SEL_0) /*!< SYSCLK clock used as UART7 clock source */
#define LL_RCC_UART7_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_UART7SEL << 16U) | RCC_DCKCFGR2_UART7SEL_1) /*!< HSI clock used as UART7 clock source */
#define LL_RCC_UART7_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_UART7SEL << 16U) | RCC_DCKCFGR2_UART7SEL)   /*!< LSE clock used as UART7 clock source */
#define LL_RCC_UART8_CLKSOURCE_PCLK1      (uint32_t)((RCC_DCKCFGR2_UART8SEL << 16U) | 0x00000000U)   /*!< PCLK1 clock used as UART8 clock source */
#define LL_RCC_UART8_CLKSOURCE_SYSCLK     (uint32_t)((RCC_DCKCFGR2_UART8SEL << 16U) | RCC_DCKCFGR2_UART8SEL_0) /*!< SYSCLK clock used as UART8 clock source */
#define LL_RCC_UART8_CLKSOURCE_HSI        (uint32_t)((RCC_DCKCFGR2_UART8SEL << 16U) | RCC_DCKCFGR2_UART8SEL_1) /*!< HSI clock used as UART8 clock source */
#define LL_RCC_UART8_CLKSOURCE_LSE        (uint32_t)((RCC_DCKCFGR2_UART8SEL << 16U) | RCC_DCKCFGR2_UART8SEL)   /*!< LSE clock used as UART8 clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2Cx_CLKSOURCE  Peripheral I2C clock source selection
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE_PCLK1        (uint32_t)(RCC_DCKCFGR2_I2C1SEL|0x00000000U) /*!< PCLK1 clock used as I2C1 clock source */
#define LL_RCC_I2C1_CLKSOURCE_SYSCLK       (uint32_t)(RCC_DCKCFGR2_I2C1SEL|(RCC_DCKCFGR2_I2C1SEL_0 >> 16U)) /*!< SYSCLK clock used as I2C1 clock source */
#define LL_RCC_I2C1_CLKSOURCE_HSI          (uint32_t)(RCC_DCKCFGR2_I2C1SEL|(RCC_DCKCFGR2_I2C1SEL_1 >> 16U)) /*!< HSI clock used as I2C1 clock source */
#define LL_RCC_I2C2_CLKSOURCE_PCLK1        (uint32_t)(RCC_DCKCFGR2_I2C2SEL|0x00000000U) /*!< PCLK1 clock used as I2C2 clock source */
#define LL_RCC_I2C2_CLKSOURCE_SYSCLK       (uint32_t)(RCC_DCKCFGR2_I2C2SEL|(RCC_DCKCFGR2_I2C2SEL_0 >> 16U)) /*!< SYSCLK clock used as I2C2 clock source */
#define LL_RCC_I2C2_CLKSOURCE_HSI          (uint32_t)(RCC_DCKCFGR2_I2C2SEL|(RCC_DCKCFGR2_I2C2SEL_1 >> 16U)) /*!< HSI clock used as I2C2 clock source */
#define LL_RCC_I2C3_CLKSOURCE_PCLK1        (uint32_t)(RCC_DCKCFGR2_I2C3SEL|0x00000000U) /*!< PCLK1 clock used as I2C3 clock source */
#define LL_RCC_I2C3_CLKSOURCE_SYSCLK       (uint32_t)(RCC_DCKCFGR2_I2C3SEL|(RCC_DCKCFGR2_I2C3SEL_0 >> 16U)) /*!< SYSCLK clock used as I2C3 clock source */
#define LL_RCC_I2C3_CLKSOURCE_HSI          (uint32_t)(RCC_DCKCFGR2_I2C3SEL|(RCC_DCKCFGR2_I2C3SEL_1 >> 16U)) /*!< HSI clock used as I2C3 clock source */
#if defined(I2C4)
#define LL_RCC_I2C4_CLKSOURCE_PCLK1        (uint32_t)(RCC_DCKCFGR2_I2C4SEL|0x00000000U) /*!< PCLK1 clock used as I2C4 clock source */
#define LL_RCC_I2C4_CLKSOURCE_SYSCLK       (uint32_t)(RCC_DCKCFGR2_I2C4SEL|(RCC_DCKCFGR2_I2C4SEL_0 >> 16U)) /*!< SYSCLK clock used as I2C4 clock source */
#define LL_RCC_I2C4_CLKSOURCE_HSI          (uint32_t)(RCC_DCKCFGR2_I2C4SEL|(RCC_DCKCFGR2_I2C4SEL_1 >> 16U)) /*!< HSI clock used as I2C4 clock source */
#endif /* I2C4 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIM1_CLKSOURCE  Peripheral LPTIM clock source selection
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE_PCLK1       0x00000000U                 /*!< PCLK1 clock used as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSI         RCC_DCKCFGR2_LPTIM1SEL_0    /*!< LSI oscillator clock used as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_HSI         RCC_DCKCFGR2_LPTIM1SEL_1    /*!< HSI oscillator clock used as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSE         (uint32_t)(RCC_DCKCFGR2_LPTIM1SEL_1 | RCC_DCKCFGR2_LPTIM1SEL_0)      /*!< LSE oscillator clock used as LPTIM1 clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SAIx_CLKSOURCE  Peripheral SAI clock source selection
  * @{
  */
#define LL_RCC_SAI1_CLKSOURCE_PLLSAI       (uint32_t)(RCC_DCKCFGR1_SAI1SEL | 0x00000000U)                      /*!< PLLSAI clock used as SAI1 clock source */
#define LL_RCC_SAI1_CLKSOURCE_PLLI2S       (uint32_t)(RCC_DCKCFGR1_SAI1SEL | (RCC_DCKCFGR1_SAI1SEL_0 >> 16U))  /*!< PLLI2S clock used as SAI1 clock source */
#define LL_RCC_SAI1_CLKSOURCE_PIN          (uint32_t)(RCC_DCKCFGR1_SAI1SEL | (RCC_DCKCFGR1_SAI1SEL_1 >> 16U))  /*!< External pin clock used as SAI1 clock source */
#if defined(RCC_SAI1SEL_PLLSRC_SUPPORT)
#define LL_RCC_SAI1_CLKSOURCE_PLLSRC       (uint32_t)(RCC_DCKCFGR1_SAI1SEL | (RCC_DCKCFGR1_SAI1SEL >> 16U))    /*!< Main source clock used as SAI1 clock source */
#endif /* RCC_SAI1SEL_PLLSRC_SUPPORT */
#define LL_RCC_SAI2_CLKSOURCE_PLLSAI       (uint32_t)(RCC_DCKCFGR1_SAI2SEL | 0x00000000U)                      /*!< PLLSAI clock used as SAI2 clock source */
#define LL_RCC_SAI2_CLKSOURCE_PLLI2S       (uint32_t)(RCC_DCKCFGR1_SAI2SEL | (RCC_DCKCFGR1_SAI2SEL_0 >> 16U))  /*!< PLLI2S clock used as SAI2 clock source */
#define LL_RCC_SAI2_CLKSOURCE_PIN          (uint32_t)(RCC_DCKCFGR1_SAI2SEL | (RCC_DCKCFGR1_SAI2SEL_1 >> 16U))  /*!< External pin clock used as SAI2 clock source */
#if defined(RCC_SAI2SEL_PLLSRC_SUPPORT)
#define LL_RCC_SAI2_CLKSOURCE_PLLSRC       (uint32_t)(RCC_DCKCFGR1_SAI2SEL | (RCC_DCKCFGR1_SAI2SEL >> 16U))    /*!< Main source clock used as SAI2 clock source */
#endif /* RCC_SAI2SEL_PLLSRC_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SDMMCx_CLKSOURCE  Peripheral SDMMC clock source selection
  * @{
  */
#define LL_RCC_SDMMC1_CLKSOURCE_PLL48CLK       (uint32_t)(RCC_DCKCFGR2_SDMMC1SEL | 0x00000000U)                        /*!< PLL 48M domain clock used as SDMMC1 clock */
#define LL_RCC_SDMMC1_CLKSOURCE_SYSCLK         (uint32_t)(RCC_DCKCFGR2_SDMMC1SEL | (RCC_DCKCFGR2_SDMMC1SEL >> 16U))    /*!< System clock clock used as SDMMC1 clock */
#if defined(SDMMC2)
#define LL_RCC_SDMMC2_CLKSOURCE_PLL48CLK       (uint32_t)(RCC_DCKCFGR2_SDMMC2SEL | 0x00000000U)                        /*!< PLL 48M domain clock used as SDMMC2 clock */
#define LL_RCC_SDMMC2_CLKSOURCE_SYSCLK         (uint32_t)(RCC_DCKCFGR2_SDMMC2SEL | (RCC_DCKCFGR2_SDMMC2SEL >> 16U))    /*!< System clock clock used as SDMMC2 clock */
#endif /* SDMMC2 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RNG_CLKSOURCE  Peripheral RNG clock source selection
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE_PLL          0x00000000U            /*!< PLL clock used as RNG clock source */
#define LL_RCC_RNG_CLKSOURCE_PLLSAI       RCC_DCKCFGR2_CK48MSEL  /*!< PLLSAI clock used as RNG clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USB_CLKSOURCE  Peripheral USB clock source selection
  * @{
  */
#define LL_RCC_USB_CLKSOURCE_PLL          0x00000000U            /*!< PLL clock used as USB clock source */
#define LL_RCC_USB_CLKSOURCE_PLLSAI       RCC_DCKCFGR2_CK48MSEL  /*!< PLLSAI1 clock used as USB clock source */
/**
  * @}
  */

#if defined(DSI)
/** @defgroup RCC_LL_EC_DSI_CLKSOURCE  Peripheral DSI clock source selection
  * @{
  */
#define LL_RCC_DSI_CLKSOURCE_PHY          0x00000000U           /*!< DSI-PHY clock used as DSI byte lane clock source */
#define LL_RCC_DSI_CLKSOURCE_PLL          RCC_DCKCFGR2_DSISEL   /*!< PLL clock used as DSI byte lane clock source */
/**
  * @}
  */
#endif /* DSI */

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC_CLKSOURCE  Peripheral CEC clock source selection
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE_LSE               0x00000000U                /*!< LSE oscillator clock used as CEC clock */
#define LL_RCC_CEC_CLKSOURCE_HSI_DIV488        RCC_DCKCFGR2_CECSEL        /*!< HSI oscillator clock divided by 488 used as CEC clock */
/**
  * @}
  */
#endif /* CEC */

/** @defgroup RCC_LL_EC_I2S1_CLKSOURCE  Peripheral I2S clock source selection
  * @{
  */
#define LL_RCC_I2S1_CLKSOURCE_PLLI2S           0x00000000U                /*!< I2S oscillator clock used as I2S1 clock */
#define LL_RCC_I2S1_CLKSOURCE_PIN              RCC_CFGR_I2SSRC            /*!< External pin clock used as I2S1 clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CK48M_CLKSOURCE  Peripheral 48Mhz domain clock source selection
  * @{
  */
#define LL_RCC_CK48M_CLKSOURCE_PLL             0x00000000U                /*!< PLL oscillator clock used as 48Mhz domain clock */
#define LL_RCC_CK48M_CLKSOURCE_PLLSAI          RCC_DCKCFGR2_CK48MSEL      /*!< PLLSAI oscillator clock used as 48Mhz domain clock */
/**
  * @}
  */

#if defined(DFSDM1_Channel0)
/** @defgroup RCC_LL_EC_DFSDM1_AUDIO_CLKSOURCE  Peripheral DFSDM Audio clock source selection
  * @{
  */
#define LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI1     0x00000000U                /*!< SAI1 clock used as DFSDM1 Audio clock */
#define LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI2     RCC_DCKCFGR1_ADFSDM1SEL    /*!< SAI2 clock used as DFSDM1 Audio clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_DFSDM1_CLKSOURCE  Peripheral DFSDM clock source selection
  * @{
  */
#define LL_RCC_DFSDM1_CLKSOURCE_PCLK2          0x00000000U                /*!< PCLK2 clock used as DFSDM1 clock */
#define LL_RCC_DFSDM1_CLKSOURCE_SYSCLK         RCC_DCKCFGR1_DFSDM1SEL     /*!< System clock used as DFSDM1 clock */
/**
  * @}
  */
#endif /* DFSDM1_Channel0 */

/** @defgroup RCC_LL_EC_USARTx  Peripheral USART get clock source
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE            RCC_DCKCFGR2_USART1SEL /*!< USART1 Clock source selection */
#define LL_RCC_USART2_CLKSOURCE            RCC_DCKCFGR2_USART2SEL /*!< USART2 Clock source selection */
#define LL_RCC_USART3_CLKSOURCE            RCC_DCKCFGR2_USART3SEL /*!< USART3 Clock source selection */
#define LL_RCC_USART6_CLKSOURCE            RCC_DCKCFGR2_USART6SEL /*!< USART6 Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_UARTx  Peripheral UART get clock source
  * @{
  */
#define LL_RCC_UART4_CLKSOURCE             RCC_DCKCFGR2_UART4SEL /*!< UART4 Clock source selection */
#define LL_RCC_UART5_CLKSOURCE             RCC_DCKCFGR2_UART5SEL /*!< UART5 Clock source selection */
#define LL_RCC_UART7_CLKSOURCE             RCC_DCKCFGR2_UART7SEL /*!< UART7 Clock source selection */
#define LL_RCC_UART8_CLKSOURCE             RCC_DCKCFGR2_UART8SEL /*!< UART8 Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2Cx  Peripheral I2C get clock source
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE              RCC_DCKCFGR2_I2C1SEL  /*!< I2C1 Clock source selection */
#define LL_RCC_I2C2_CLKSOURCE              RCC_DCKCFGR2_I2C2SEL  /*!< I2C2 Clock source selection */
#define LL_RCC_I2C3_CLKSOURCE              RCC_DCKCFGR2_I2C3SEL  /*!< I2C3 Clock source selection */
#if defined(I2C4)
#define LL_RCC_I2C4_CLKSOURCE              RCC_DCKCFGR2_I2C4SEL  /*!< I2C4 Clock source selection */
#endif /* I2C4 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIM1  Peripheral LPTIM get clock source
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE            RCC_DCKCFGR2_LPTIM1SEL /*!< LPTIM1 Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SAIx  Peripheral SAI get clock source
  * @{
  */
#define LL_RCC_SAI1_CLKSOURCE              RCC_DCKCFGR1_SAI1SEL /*!< SAI1 Clock source selection */
#define LL_RCC_SAI2_CLKSOURCE              RCC_DCKCFGR1_SAI2SEL /*!< SAI2 Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SDMMCx  Peripheral SDMMC get clock source
  * @{
  */
#define LL_RCC_SDMMC1_CLKSOURCE            RCC_DCKCFGR2_SDMMC1SEL /*!< SDMMC1 Clock source selection */
#if defined(SDMMC2)
#define LL_RCC_SDMMC2_CLKSOURCE            RCC_DCKCFGR2_SDMMC2SEL /*!< SDMMC2 Clock source selection */
#endif /* SDMMC2 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CK48M  Peripheral CK48M get clock source
  * @{
  */
#define LL_RCC_CK48M_CLKSOURCE             RCC_DCKCFGR2_CK48MSEL /*!< CK48M Domain clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RNG  Peripheral RNG get clock source
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE               RCC_DCKCFGR2_CK48MSEL /*!< RNG Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USB  Peripheral USB get clock source
  * @{
  */
#define LL_RCC_USB_CLKSOURCE               RCC_DCKCFGR2_CK48MSEL /*!< USB Clock source selection */
/**
  * @}
  */

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC  Peripheral CEC get clock source
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE               RCC_DCKCFGR2_CECSEL /*!< CEC Clock source selection */
/**
  * @}
  */
#endif /* CEC */

/** @defgroup RCC_LL_EC_I2S1  Peripheral I2S get clock source
  * @{
  */
#define LL_RCC_I2S1_CLKSOURCE              RCC_CFGR_I2SSRC /*!< I2S Clock source selection */
/**
  * @}
  */
#if defined(DFSDM1_Channel0)
/** @defgroup RCC_LL_EC_DFSDM_AUDIO  Peripheral DFSDM Audio get clock source
  * @{
  */
#define LL_RCC_DFSDM1_AUDIO_CLKSOURCE      RCC_DCKCFGR1_ADFSDM1SEL /*!< DFSDM Audio Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_DFSDM  Peripheral DFSDM get clock source
  * @{
  */
#define LL_RCC_DFSDM1_CLKSOURCE            RCC_DCKCFGR1_DFSDM1SEL /*!< DFSDM Clock source selection */
/**
  * @}
  */
#endif /* DFSDM1_Channel0 */

#if defined(DSI)
/** @defgroup RCC_LL_EC_DSI  Peripheral DSI get clock source
  * @{
  */
#define LL_RCC_DSI_CLKSOURCE               RCC_DCKCFGR2_DSISEL /*!< DSI Clock source selection */
/**
  * @}
  */
#endif /* DSI */

#if defined(LTDC)
/** @defgroup RCC_LL_EC_LTDC  Peripheral LTDC get clock source
  * @{
  */
#define LL_RCC_LTDC_CLKSOURCE              RCC_DCKCFGR1_PLLSAIDIVR /*!< LTDC Clock source selection */
/**
  * @}
  */
#endif /* LTDC */

#if defined(SPDIFRX)
/** @defgroup RCC_LL_EC_SPDIFRX  Peripheral SPDIFRX get clock source
  * @{
  */
#define LL_RCC_SPDIFRX1_CLKSOURCE           RCC_PLLI2SCFGR_PLLI2SP /*!< SPDIFRX Clock source selection */
/**
  * @}
  */
#endif /* SPDIFRX */

/** @defgroup RCC_LL_EC_RTC_CLKSOURCE  RTC clock source selection
  * @{
  */
#define LL_RCC_RTC_CLKSOURCE_NONE          0x00000000U             /*!< No clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSE           RCC_BDCR_RTCSEL_0       /*!< LSE oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSI           RCC_BDCR_RTCSEL_1       /*!< LSI oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_HSE           RCC_BDCR_RTCSEL         /*!< HSE oscillator clock divided by HSE prescaler used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_TIM_CLKPRESCALER  Timers clocks prescalers selection
  * @{
  */
#define LL_RCC_TIM_PRESCALER_TWICE          0x00000000U                  /*!< Timers clock to twice PCLK */
#define LL_RCC_TIM_PRESCALER_FOUR_TIMES     RCC_DCKCFGR1_TIMPRE          /*!< Timers clock to four time PCLK */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLSOURCE  PLL, PLLI2S and PLLSAI entry clock source
  * @{
  */
#define LL_RCC_PLLSOURCE_HSI               RCC_PLLCFGR_PLLSRC_HSI  /*!< HSI16 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE               RCC_PLLCFGR_PLLSRC_HSE  /*!< HSE clock selected as PLL entry clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLM_DIV  PLL, PLLI2S and PLLSAI division factor
  * @{
  */
#define LL_RCC_PLLM_DIV_2                  (RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 2 */
#define LL_RCC_PLLM_DIV_3                  (RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 3 */
#define LL_RCC_PLLM_DIV_4                  (RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 4 */
#define LL_RCC_PLLM_DIV_5                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 5 */
#define LL_RCC_PLLM_DIV_6                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 6 */
#define LL_RCC_PLLM_DIV_7                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 7 */
#define LL_RCC_PLLM_DIV_8                  (RCC_PLLCFGR_PLLM_3) /*!< PLL, PLLI2S and PLLSAI division factor by 8 */
#define LL_RCC_PLLM_DIV_9                  (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 9 */
#define LL_RCC_PLLM_DIV_10                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 10 */
#define LL_RCC_PLLM_DIV_11                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 11 */
#define LL_RCC_PLLM_DIV_12                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 12 */
#define LL_RCC_PLLM_DIV_13                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 13 */
#define LL_RCC_PLLM_DIV_14                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 14 */
#define LL_RCC_PLLM_DIV_15                 (RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 15 */
#define LL_RCC_PLLM_DIV_16                 (RCC_PLLCFGR_PLLM_4) /*!< PLL, PLLI2S and PLLSAI division factor by 16 */
#define LL_RCC_PLLM_DIV_17                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 17 */
#define LL_RCC_PLLM_DIV_18                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 18 */
#define LL_RCC_PLLM_DIV_19                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 19 */
#define LL_RCC_PLLM_DIV_20                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 20 */
#define LL_RCC_PLLM_DIV_21                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 21 */
#define LL_RCC_PLLM_DIV_22                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 22 */
#define LL_RCC_PLLM_DIV_23                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 23 */
#define LL_RCC_PLLM_DIV_24                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3) /*!< PLL, PLLI2S and PLLSAI division factor by 24 */
#define LL_RCC_PLLM_DIV_25                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 25 */
#define LL_RCC_PLLM_DIV_26                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 26 */
#define LL_RCC_PLLM_DIV_27                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 27 */
#define LL_RCC_PLLM_DIV_28                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 28 */
#define LL_RCC_PLLM_DIV_29                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 29 */
#define LL_RCC_PLLM_DIV_30                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 30 */
#define LL_RCC_PLLM_DIV_31                 (RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 31 */
#define LL_RCC_PLLM_DIV_32                 (RCC_PLLCFGR_PLLM_5) /*!< PLL, PLLI2S and PLLSAI division factor by 32 */
#define LL_RCC_PLLM_DIV_33                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 33 */
#define LL_RCC_PLLM_DIV_34                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 34 */
#define LL_RCC_PLLM_DIV_35                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 35 */
#define LL_RCC_PLLM_DIV_36                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 36 */
#define LL_RCC_PLLM_DIV_37                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 37 */
#define LL_RCC_PLLM_DIV_38                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 38 */
#define LL_RCC_PLLM_DIV_39                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 39 */
#define LL_RCC_PLLM_DIV_40                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3) /*!< PLL, PLLI2S and PLLSAI division factor by 40 */
#define LL_RCC_PLLM_DIV_41                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 41 */
#define LL_RCC_PLLM_DIV_42                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 42 */
#define LL_RCC_PLLM_DIV_43                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 43 */
#define LL_RCC_PLLM_DIV_44                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 44 */
#define LL_RCC_PLLM_DIV_45                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 45 */
#define LL_RCC_PLLM_DIV_46                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 46 */
#define LL_RCC_PLLM_DIV_47                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 47 */
#define LL_RCC_PLLM_DIV_48                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4) /*!< PLL, PLLI2S and PLLSAI division factor by 48 */
#define LL_RCC_PLLM_DIV_49                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 49 */
#define LL_RCC_PLLM_DIV_50                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 50 */
#define LL_RCC_PLLM_DIV_51                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 51 */
#define LL_RCC_PLLM_DIV_52                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 52 */
#define LL_RCC_PLLM_DIV_53                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 53 */
#define LL_RCC_PLLM_DIV_54                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 54 */
#define LL_RCC_PLLM_DIV_55                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 55 */
#define LL_RCC_PLLM_DIV_56                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3) /*!< PLL, PLLI2S and PLLSAI division factor by 56 */
#define LL_RCC_PLLM_DIV_57                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 57 */
#define LL_RCC_PLLM_DIV_58                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 58 */
#define LL_RCC_PLLM_DIV_59                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 59 */
#define LL_RCC_PLLM_DIV_60                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2) /*!< PLL, PLLI2S and PLLSAI division factor by 60 */
#define LL_RCC_PLLM_DIV_61                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 61 */
#define LL_RCC_PLLM_DIV_62                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1) /*!< PLL, PLLI2S and PLLSAI division factor by 62 */
#define LL_RCC_PLLM_DIV_63                 (RCC_PLLCFGR_PLLM_5 | RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0) /*!< PLL, PLLI2S and PLLSAI division factor by 63 */
/**
  * @}
  */

#if defined(RCC_PLLCFGR_PLLR)
/** @defgroup RCC_LL_EC_PLLR_DIV  PLL division factor (PLLR)
  * @{
  */
#define LL_RCC_PLLR_DIV_2                  (RCC_PLLCFGR_PLLR_1)                     /*!< Main PLL division factor for PLLCLK (system clock) by 2 */
#define LL_RCC_PLLR_DIV_3                  (RCC_PLLCFGR_PLLR_1|RCC_PLLCFGR_PLLR_0)  /*!< Main PLL division factor for PLLCLK (system clock) by 3 */
#define LL_RCC_PLLR_DIV_4                  (RCC_PLLCFGR_PLLR_2)                     /*!< Main PLL division factor for PLLCLK (system clock) by 4 */
#define LL_RCC_PLLR_DIV_5                  (RCC_PLLCFGR_PLLR_2|RCC_PLLCFGR_PLLR_0)  /*!< Main PLL division factor for PLLCLK (system clock) by 5 */
#define LL_RCC_PLLR_DIV_6                  (RCC_PLLCFGR_PLLR_2|RCC_PLLCFGR_PLLR_1)  /*!< Main PLL division factor for PLLCLK (system clock) by 6 */
#define LL_RCC_PLLR_DIV_7                  (RCC_PLLCFGR_PLLR)                       /*!< Main PLL division factor for PLLCLK (system clock) by 7 */
/**
  * @}
  */
#endif /* RCC_PLLCFGR_PLLR */

/** @defgroup RCC_LL_EC_PLLP_DIV  PLL division factor (PLLP)
  * @{
  */
#define LL_RCC_PLLP_DIV_2                  0x00000000U            /*!< Main PLL division factor for PLLP output by 2 */
#define LL_RCC_PLLP_DIV_4                  RCC_PLLCFGR_PLLP_0     /*!< Main PLL division factor for PLLP output by 4 */
#define LL_RCC_PLLP_DIV_6                  RCC_PLLCFGR_PLLP_1     /*!< Main PLL division factor for PLLP output by 6 */
#define LL_RCC_PLLP_DIV_8                  (RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)   /*!< Main PLL division factor for PLLP output by 8 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLQ_DIV  PLL division factor (PLLQ)
  * @{
  */
#define LL_RCC_PLLQ_DIV_2                  RCC_PLLCFGR_PLLQ_1                      /*!< Main PLL division factor for PLLQ output by 2 */
#define LL_RCC_PLLQ_DIV_3                  (RCC_PLLCFGR_PLLQ_1|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 3 */
#define LL_RCC_PLLQ_DIV_4                  RCC_PLLCFGR_PLLQ_2                      /*!< Main PLL division factor for PLLQ output by 4 */
#define LL_RCC_PLLQ_DIV_5                  (RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 5 */
#define LL_RCC_PLLQ_DIV_6                  (RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_1) /*!< Main PLL division factor for PLLQ output by 6 */
#define LL_RCC_PLLQ_DIV_7                  (RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_1|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 7 */
#define LL_RCC_PLLQ_DIV_8                  RCC_PLLCFGR_PLLQ_3                      /*!< Main PLL division factor for PLLQ output by 8 */
#define LL_RCC_PLLQ_DIV_9                  (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 9 */
#define LL_RCC_PLLQ_DIV_10                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_1) /*!< Main PLL division factor for PLLQ output by 10 */
#define LL_RCC_PLLQ_DIV_11                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_1|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 11 */
#define LL_RCC_PLLQ_DIV_12                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_2) /*!< Main PLL division factor for PLLQ output by 12 */
#define LL_RCC_PLLQ_DIV_13                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 13 */
#define LL_RCC_PLLQ_DIV_14                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_1) /*!< Main PLL division factor for PLLQ output by 14 */
#define LL_RCC_PLLQ_DIV_15                 (RCC_PLLCFGR_PLLQ_3|RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_1|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 15 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL_SPRE_SEL  PLL Spread Spectrum Selection
  * @{
  */
#define LL_RCC_SPREAD_SELECT_CENTER        0x00000000U                   /*!< PLL center spread spectrum selection */
#define LL_RCC_SPREAD_SELECT_DOWN          RCC_SSCGR_SPREADSEL           /*!< PLL down spread spectrum selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLI2SQ  PLLI2SQ division factor (PLLI2SQ)
  * @{
  */
#define LL_RCC_PLLI2SQ_DIV_2              RCC_PLLI2SCFGR_PLLI2SQ_1        /*!< PLLI2S division factor for PLLI2SQ output by 2 */
#define LL_RCC_PLLI2SQ_DIV_3              (RCC_PLLI2SCFGR_PLLI2SQ_1 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 3 */
#define LL_RCC_PLLI2SQ_DIV_4              RCC_PLLI2SCFGR_PLLI2SQ_2        /*!< PLLI2S division factor for PLLI2SQ output by 4 */
#define LL_RCC_PLLI2SQ_DIV_5              (RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 5 */
#define LL_RCC_PLLI2SQ_DIV_6              (RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_1)        /*!< PLLI2S division factor for PLLI2SQ output by 6 */
#define LL_RCC_PLLI2SQ_DIV_7              (RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_1 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 7 */
#define LL_RCC_PLLI2SQ_DIV_8              RCC_PLLI2SCFGR_PLLI2SQ_3        /*!< PLLI2S division factor for PLLI2SQ output by 8 */
#define LL_RCC_PLLI2SQ_DIV_9              (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 9 */
#define LL_RCC_PLLI2SQ_DIV_10             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_1)        /*!< PLLI2S division factor for PLLI2SQ output by 10 */
#define LL_RCC_PLLI2SQ_DIV_11             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_1 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 11 */
#define LL_RCC_PLLI2SQ_DIV_12             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_2)        /*!< PLLI2S division factor for PLLI2SQ output by 12 */
#define LL_RCC_PLLI2SQ_DIV_13             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 13 */
#define LL_RCC_PLLI2SQ_DIV_14             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_1)        /*!< PLLI2S division factor for PLLI2SQ output by 14 */
#define LL_RCC_PLLI2SQ_DIV_15             (RCC_PLLI2SCFGR_PLLI2SQ_3 | RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SQ_1 | RCC_PLLI2SCFGR_PLLI2SQ_0)        /*!< PLLI2S division factor for PLLI2SQ output by 15 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLI2SDIVQ  PLLI2SDIVQ division factor (PLLI2SDIVQ)
  * @{
  */
#define LL_RCC_PLLI2SDIVQ_DIV_1           0x00000000U                        /*!< PLLI2S division factor for PLLI2SDIVQ output by 1 */
#define LL_RCC_PLLI2SDIVQ_DIV_2           RCC_DCKCFGR1_PLLI2SDIVQ_0          /*!< PLLI2S division factor for PLLI2SDIVQ output by 2 */
#define LL_RCC_PLLI2SDIVQ_DIV_3           RCC_DCKCFGR1_PLLI2SDIVQ_1          /*!< PLLI2S division factor for PLLI2SDIVQ output by 3 */
#define LL_RCC_PLLI2SDIVQ_DIV_4           (RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 4 */
#define LL_RCC_PLLI2SDIVQ_DIV_5           RCC_DCKCFGR1_PLLI2SDIVQ_2          /*!< PLLI2S division factor for PLLI2SDIVQ output by 5 */
#define LL_RCC_PLLI2SDIVQ_DIV_6           (RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 6 */
#define LL_RCC_PLLI2SDIVQ_DIV_7           (RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 7 */
#define LL_RCC_PLLI2SDIVQ_DIV_8           (RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 8 */
#define LL_RCC_PLLI2SDIVQ_DIV_9           RCC_DCKCFGR1_PLLI2SDIVQ_3          /*!< PLLI2S division factor for PLLI2SDIVQ output by 9 */
#define LL_RCC_PLLI2SDIVQ_DIV_10          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 10 */
#define LL_RCC_PLLI2SDIVQ_DIV_11          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 11 */
#define LL_RCC_PLLI2SDIVQ_DIV_12          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 12 */
#define LL_RCC_PLLI2SDIVQ_DIV_13          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 13 */
#define LL_RCC_PLLI2SDIVQ_DIV_14          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 14 */
#define LL_RCC_PLLI2SDIVQ_DIV_15          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 15 */
#define LL_RCC_PLLI2SDIVQ_DIV_16          (RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 16 */
#define LL_RCC_PLLI2SDIVQ_DIV_17          RCC_DCKCFGR1_PLLI2SDIVQ_4          /*!< PLLI2S division factor for PLLI2SDIVQ output by 17 */
#define LL_RCC_PLLI2SDIVQ_DIV_18          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 18 */
#define LL_RCC_PLLI2SDIVQ_DIV_19          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 19 */
#define LL_RCC_PLLI2SDIVQ_DIV_20          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 20 */
#define LL_RCC_PLLI2SDIVQ_DIV_21          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_2)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 21 */
#define LL_RCC_PLLI2SDIVQ_DIV_22          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 22 */
#define LL_RCC_PLLI2SDIVQ_DIV_23          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 23 */
#define LL_RCC_PLLI2SDIVQ_DIV_24          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 24 */
#define LL_RCC_PLLI2SDIVQ_DIV_25          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 25 */
#define LL_RCC_PLLI2SDIVQ_DIV_26          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 26 */
#define LL_RCC_PLLI2SDIVQ_DIV_27          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 27 */
#define LL_RCC_PLLI2SDIVQ_DIV_28          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 28 */
#define LL_RCC_PLLI2SDIVQ_DIV_29          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 29 */
#define LL_RCC_PLLI2SDIVQ_DIV_30          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 30 */
#define LL_RCC_PLLI2SDIVQ_DIV_31          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 31 */
#define LL_RCC_PLLI2SDIVQ_DIV_32          (RCC_DCKCFGR1_PLLI2SDIVQ_4 | RCC_DCKCFGR1_PLLI2SDIVQ_3 | RCC_DCKCFGR1_PLLI2SDIVQ_2 | RCC_DCKCFGR1_PLLI2SDIVQ_1 | RCC_DCKCFGR1_PLLI2SDIVQ_0)        /*!< PLLI2S division factor for PLLI2SDIVQ output by 32 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLI2SR  PLLI2SR division factor (PLLI2SR)
  * @{
  */
#define LL_RCC_PLLI2SR_DIV_2              RCC_PLLI2SCFGR_PLLI2SR_1                                     /*!< PLLI2S division factor for PLLI2SR output by 2 */
#define LL_RCC_PLLI2SR_DIV_3              (RCC_PLLI2SCFGR_PLLI2SR_1 | RCC_PLLI2SCFGR_PLLI2SR_0)        /*!< PLLI2S division factor for PLLI2SR output by 3 */
#define LL_RCC_PLLI2SR_DIV_4              RCC_PLLI2SCFGR_PLLI2SR_2                                     /*!< PLLI2S division factor for PLLI2SR output by 4 */
#define LL_RCC_PLLI2SR_DIV_5              (RCC_PLLI2SCFGR_PLLI2SR_2 | RCC_PLLI2SCFGR_PLLI2SR_0)        /*!< PLLI2S division factor for PLLI2SR output by 5 */
#define LL_RCC_PLLI2SR_DIV_6              (RCC_PLLI2SCFGR_PLLI2SR_2 | RCC_PLLI2SCFGR_PLLI2SR_1)        /*!< PLLI2S division factor for PLLI2SR output by 6 */
#define LL_RCC_PLLI2SR_DIV_7              (RCC_PLLI2SCFGR_PLLI2SR_2 | RCC_PLLI2SCFGR_PLLI2SR_1 | RCC_PLLI2SCFGR_PLLI2SR_0)        /*!< PLLI2S division factor for PLLI2SR output by 7 */
/**
  * @}
  */

#if defined(RCC_PLLI2SCFGR_PLLI2SP)
/** @defgroup RCC_LL_EC_PLLI2SP  PLLI2SP division factor (PLLI2SP)
  * @{
  */
#define LL_RCC_PLLI2SP_DIV_2              0x00000000U            /*!< PLLI2S division factor for PLLI2SP output by 2 */
#define LL_RCC_PLLI2SP_DIV_4              RCC_PLLI2SCFGR_PLLI2SP_0        /*!< PLLI2S division factor for PLLI2SP output by 4 */
#define LL_RCC_PLLI2SP_DIV_6              RCC_PLLI2SCFGR_PLLI2SP_1        /*!< PLLI2S division factor for PLLI2SP output by 6 */
#define LL_RCC_PLLI2SP_DIV_8              (RCC_PLLI2SCFGR_PLLI2SP_1 | RCC_PLLI2SCFGR_PLLI2SP_0)        /*!< PLLI2S division factor for PLLI2SP output by 8 */
/**
  * @}
  */
#endif /* RCC_PLLI2SCFGR_PLLI2SP */

/** @defgroup RCC_LL_EC_PLLSAIQ  PLLSAIQ division factor (PLLSAIQ)
  * @{
  */
#define LL_RCC_PLLSAIQ_DIV_2              RCC_PLLSAICFGR_PLLSAIQ_1        /*!< PLLSAI division factor for PLLSAIQ output by 2 */
#define LL_RCC_PLLSAIQ_DIV_3              (RCC_PLLSAICFGR_PLLSAIQ_1 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 3 */
#define LL_RCC_PLLSAIQ_DIV_4              RCC_PLLSAICFGR_PLLSAIQ_2        /*!< PLLSAI division factor for PLLSAIQ output by 4 */
#define LL_RCC_PLLSAIQ_DIV_5              (RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 5 */
#define LL_RCC_PLLSAIQ_DIV_6              (RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_1)        /*!< PLLSAI division factor for PLLSAIQ output by 6 */
#define LL_RCC_PLLSAIQ_DIV_7              (RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_1 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 7 */
#define LL_RCC_PLLSAIQ_DIV_8              RCC_PLLSAICFGR_PLLSAIQ_3        /*!< PLLSAI division factor for PLLSAIQ output by 8 */
#define LL_RCC_PLLSAIQ_DIV_9              (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 9 */
#define LL_RCC_PLLSAIQ_DIV_10             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_1)        /*!< PLLSAI division factor for PLLSAIQ output by 10 */
#define LL_RCC_PLLSAIQ_DIV_11             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_1 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 11 */
#define LL_RCC_PLLSAIQ_DIV_12             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_2)        /*!< PLLSAI division factor for PLLSAIQ output by 12 */
#define LL_RCC_PLLSAIQ_DIV_13             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 13 */
#define LL_RCC_PLLSAIQ_DIV_14             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_1)        /*!< PLLSAI division factor for PLLSAIQ output by 14 */
#define LL_RCC_PLLSAIQ_DIV_15             (RCC_PLLSAICFGR_PLLSAIQ_3 | RCC_PLLSAICFGR_PLLSAIQ_2 | RCC_PLLSAICFGR_PLLSAIQ_1 | RCC_PLLSAICFGR_PLLSAIQ_0)        /*!< PLLSAI division factor for PLLSAIQ output by 15 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLSAIDIVQ  PLLSAIDIVQ division factor (PLLSAIDIVQ)
  * @{
  */
#define LL_RCC_PLLSAIDIVQ_DIV_1           0x00000000U               /*!< PLLSAI division factor for PLLSAIDIVQ output by 1 */
#define LL_RCC_PLLSAIDIVQ_DIV_2           RCC_DCKCFGR1_PLLSAIDIVQ_0          /*!< PLLSAI division factor for PLLSAIDIVQ output by 2 */
#define LL_RCC_PLLSAIDIVQ_DIV_3           RCC_DCKCFGR1_PLLSAIDIVQ_1          /*!< PLLSAI division factor for PLLSAIDIVQ output by 3 */
#define LL_RCC_PLLSAIDIVQ_DIV_4           (RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 4 */
#define LL_RCC_PLLSAIDIVQ_DIV_5           RCC_DCKCFGR1_PLLSAIDIVQ_2          /*!< PLLSAI division factor for PLLSAIDIVQ output by 5 */
#define LL_RCC_PLLSAIDIVQ_DIV_6           (RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 6 */
#define LL_RCC_PLLSAIDIVQ_DIV_7           (RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 7 */
#define LL_RCC_PLLSAIDIVQ_DIV_8           (RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 8 */
#define LL_RCC_PLLSAIDIVQ_DIV_9           RCC_DCKCFGR1_PLLSAIDIVQ_3          /*!< PLLSAI division factor for PLLSAIDIVQ output by 9 */
#define LL_RCC_PLLSAIDIVQ_DIV_10          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 10 */
#define LL_RCC_PLLSAIDIVQ_DIV_11          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 11 */
#define LL_RCC_PLLSAIDIVQ_DIV_12          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 12 */
#define LL_RCC_PLLSAIDIVQ_DIV_13          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 13 */
#define LL_RCC_PLLSAIDIVQ_DIV_14          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 14 */
#define LL_RCC_PLLSAIDIVQ_DIV_15          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 15 */
#define LL_RCC_PLLSAIDIVQ_DIV_16          (RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 16 */
#define LL_RCC_PLLSAIDIVQ_DIV_17          RCC_DCKCFGR1_PLLSAIDIVQ_4         /*!< PLLSAI division factor for PLLSAIDIVQ output by 17 */
#define LL_RCC_PLLSAIDIVQ_DIV_18          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 18 */
#define LL_RCC_PLLSAIDIVQ_DIV_19          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 19 */
#define LL_RCC_PLLSAIDIVQ_DIV_20          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 20 */
#define LL_RCC_PLLSAIDIVQ_DIV_21          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_2)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 21 */
#define LL_RCC_PLLSAIDIVQ_DIV_22          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 22 */
#define LL_RCC_PLLSAIDIVQ_DIV_23          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 23 */
#define LL_RCC_PLLSAIDIVQ_DIV_24          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 24 */
#define LL_RCC_PLLSAIDIVQ_DIV_25          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 25 */
#define LL_RCC_PLLSAIDIVQ_DIV_26          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 26 */
#define LL_RCC_PLLSAIDIVQ_DIV_27          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 27 */
#define LL_RCC_PLLSAIDIVQ_DIV_28          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 28 */
#define LL_RCC_PLLSAIDIVQ_DIV_29          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 29 */
#define LL_RCC_PLLSAIDIVQ_DIV_30          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 30 */
#define LL_RCC_PLLSAIDIVQ_DIV_31          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 31 */
#define LL_RCC_PLLSAIDIVQ_DIV_32          (RCC_DCKCFGR1_PLLSAIDIVQ_4 | RCC_DCKCFGR1_PLLSAIDIVQ_3 | RCC_DCKCFGR1_PLLSAIDIVQ_2 | RCC_DCKCFGR1_PLLSAIDIVQ_1 | RCC_DCKCFGR1_PLLSAIDIVQ_0)        /*!< PLLSAI division factor for PLLSAIDIVQ output by 32 */
/**
  * @}
  */

#if defined(RCC_PLLSAICFGR_PLLSAIR)
/** @defgroup RCC_LL_EC_PLLSAIR  PLLSAIR division factor (PLLSAIR)
  * @{
  */
#define LL_RCC_PLLSAIR_DIV_2              RCC_PLLSAICFGR_PLLSAIR_1                                     /*!< PLLSAI division factor for PLLSAIR output by 2 */
#define LL_RCC_PLLSAIR_DIV_3              (RCC_PLLSAICFGR_PLLSAIR_1 | RCC_PLLSAICFGR_PLLSAIR_0)        /*!< PLLSAI division factor for PLLSAIR output by 3 */
#define LL_RCC_PLLSAIR_DIV_4              RCC_PLLSAICFGR_PLLSAIR_2                                     /*!< PLLSAI division factor for PLLSAIR output by 4 */
#define LL_RCC_PLLSAIR_DIV_5              (RCC_PLLSAICFGR_PLLSAIR_2 | RCC_PLLSAICFGR_PLLSAIR_0)        /*!< PLLSAI division factor for PLLSAIR output by 5 */
#define LL_RCC_PLLSAIR_DIV_6              (RCC_PLLSAICFGR_PLLSAIR_2 | RCC_PLLSAICFGR_PLLSAIR_1)        /*!< PLLSAI division factor for PLLSAIR output by 6 */
#define LL_RCC_PLLSAIR_DIV_7              (RCC_PLLSAICFGR_PLLSAIR_2 | RCC_PLLSAICFGR_PLLSAIR_1 | RCC_PLLSAICFGR_PLLSAIR_0)        /*!< PLLSAI division factor for PLLSAIR output by 7 */
/**
  * @}
  */
#endif /* RCC_PLLSAICFGR_PLLSAIR */

#if defined(RCC_DCKCFGR1_PLLSAIDIVR)
/** @defgroup RCC_LL_EC_PLLSAIDIVR  PLLSAIDIVR division factor (PLLSAIDIVR)
  * @{
  */
#define LL_RCC_PLLSAIDIVR_DIV_2           0x00000000U             /*!< PLLSAI division factor for PLLSAIDIVR output by 2 */
#define LL_RCC_PLLSAIDIVR_DIV_4           RCC_DCKCFGR1_PLLSAIDIVR_0        /*!< PLLSAI division factor for PLLSAIDIVR output by 4 */
#define LL_RCC_PLLSAIDIVR_DIV_8           RCC_DCKCFGR1_PLLSAIDIVR_1        /*!< PLLSAI division factor for PLLSAIDIVR output by 8 */
#define LL_RCC_PLLSAIDIVR_DIV_16          (RCC_DCKCFGR1_PLLSAIDIVR_1 | RCC_DCKCFGR1_PLLSAIDIVR_0)        /*!< PLLSAI division factor for PLLSAIDIVR output by 16 */
/**
  * @}
  */
#endif /* RCC_DCKCFGR1_PLLSAIDIVR */

/** @defgroup RCC_LL_EC_PLLSAIP  PLLSAIP division factor (PLLSAIP)
  * @{
  */
#define LL_RCC_PLLSAIP_DIV_2              0x00000000U               /*!< PLLSAI division factor for PLLSAIP output by 2 */
#define LL_RCC_PLLSAIP_DIV_4              RCC_PLLSAICFGR_PLLSAIP_0        /*!< PLLSAI division factor for PLLSAIP output by 4 */
#define LL_RCC_PLLSAIP_DIV_6              RCC_PLLSAICFGR_PLLSAIP_1        /*!< PLLSAI division factor for PLLSAIP output by 6 */
#define LL_RCC_PLLSAIP_DIV_8              (RCC_PLLSAICFGR_PLLSAIP_1 | RCC_PLLSAICFGR_PLLSAIP_0)        /*!< PLLSAI division factor for PLLSAIP output by 8 */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup RCC_LL_Exported_Macros RCC Exported Macros
  * @{
  */

/** @defgroup RCC_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in RCC register
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_RCC_WriteReg(__REG__, __VALUE__) WRITE_REG(RCC->__REG__, (__VALUE__))

/**
  * @brief  Read a value in RCC register
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_RCC_ReadReg(__REG__) READ_REG(RCC->__REG__)
/**
  * @}
  */

/** @defgroup RCC_LL_EM_CALC_FREQ Calculate frequencies
  * @{
  */

/**
  * @brief  Helper macro to calculate the PLLCLK frequency on system domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetP ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLN__ Between 50 and 432
  * @param  __PLLP__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_8
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLP__) ((__INPUTFREQ__) / (__PLLM__) * (__PLLN__) / \
                   ((((__PLLP__) >> RCC_PLLCFGR_PLLP_Pos ) + 1U) * 2U))

/**
  * @brief  Helper macro to calculate the PLLCLK frequency used on 48M domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_48M_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLN__ Between 50 and 432
  * @param  __PLLQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  *         @arg @ref LL_RCC_PLLQ_DIV_9
  *         @arg @ref LL_RCC_PLLQ_DIV_10
  *         @arg @ref LL_RCC_PLLQ_DIV_11
  *         @arg @ref LL_RCC_PLLQ_DIV_12
  *         @arg @ref LL_RCC_PLLQ_DIV_13
  *         @arg @ref LL_RCC_PLLQ_DIV_14
  *         @arg @ref LL_RCC_PLLQ_DIV_15
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_48M_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLQ__) ((__INPUTFREQ__) / (__PLLM__) * (__PLLN__) / \
                   ((__PLLQ__) >> RCC_PLLCFGR_PLLQ_Pos ))

#if defined(DSI)
/**
  * @brief  Helper macro to calculate the PLLCLK frequency used on DSI
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_DSI_FREQ (HSE_VALUE, @ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetR ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLN__ Between 50 and 432
  * @param  __PLLR__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLR_DIV_2
  *         @arg @ref LL_RCC_PLLR_DIV_3
  *         @arg @ref LL_RCC_PLLR_DIV_4
  *         @arg @ref LL_RCC_PLLR_DIV_5
  *         @arg @ref LL_RCC_PLLR_DIV_6
  *         @arg @ref LL_RCC_PLLR_DIV_7
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_DSI_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLR__) ((__INPUTFREQ__) / (__PLLM__) * (__PLLN__) / \
                   ((__PLLR__) >> RCC_PLLCFGR_PLLR_Pos ))
#endif /* DSI */

/**
  * @brief  Helper macro to calculate the PLLSAI frequency used for SAI1 and SAI2 domains
  * @note ex: @ref __LL_RCC_CALC_PLLSAI_SAI_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLSAI_GetN (), @ref LL_RCC_PLLSAI_GetQ (), @ref LL_RCC_PLLSAI_GetDIVQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLSAIN__ Between 50 and 432
  * @param  __PLLSAIQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_15
  * @param  __PLLSAIDIVQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_32
  * @retval PLLSAI clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLSAI_SAI_FREQ(__INPUTFREQ__, __PLLM__, __PLLSAIN__, __PLLSAIQ__, __PLLSAIDIVQ__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLSAIN__) / \
                   (((__PLLSAIQ__) >> RCC_PLLSAICFGR_PLLSAIQ_Pos) * (((__PLLSAIDIVQ__) >> RCC_DCKCFGR1_PLLSAIDIVQ_Pos) + 1U)))

/**
  * @brief  Helper macro to calculate the PLLSAI frequency used on 48Mhz domain
  * @note ex: @ref __LL_RCC_CALC_PLLSAI_48M_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLSAI_GetN (), @ref LL_RCC_PLLSAI_GetP ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLSAIN__ Between 50 and 432
  * @param  __PLLSAIP__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIP_DIV_2
  *         @arg @ref LL_RCC_PLLSAIP_DIV_4
  *         @arg @ref LL_RCC_PLLSAIP_DIV_6
  *         @arg @ref LL_RCC_PLLSAIP_DIV_8
  * @retval PLLSAI clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLSAI_48M_FREQ(__INPUTFREQ__, __PLLM__, __PLLSAIN__, __PLLSAIP__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLSAIN__) / \
                   ((((__PLLSAIP__) >> RCC_PLLSAICFGR_PLLSAIP_Pos) + 1U ) * 2U))

#if defined(LTDC)
/**
  * @brief  Helper macro to calculate the PLLSAI frequency used for LTDC domain
  * @note ex: @ref __LL_RCC_CALC_PLLSAI_LTDC_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLSAI_GetN (), @ref LL_RCC_PLLSAI_GetR (), @ref LL_RCC_PLLSAI_GetDIVR ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLSAIN__ Between 50 and 432
  * @param  __PLLSAIR__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIR_DIV_3
  *         @arg @ref LL_RCC_PLLSAIR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIR_DIV_5
  *         @arg @ref LL_RCC_PLLSAIR_DIV_6
  *         @arg @ref LL_RCC_PLLSAIR_DIV_7
  * @param  __PLLSAIDIVR__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_16
  * @retval PLLSAI clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLSAI_LTDC_FREQ(__INPUTFREQ__, __PLLM__, __PLLSAIN__, __PLLSAIR__, __PLLSAIDIVR__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLSAIN__) / \
                   (((__PLLSAIR__) >> RCC_PLLSAICFGR_PLLSAIR_Pos) * (aRCC_PLLSAIDIVRPrescTable[(__PLLSAIDIVR__) >> RCC_DCKCFGR1_PLLSAIDIVR_Pos])))
#endif /* LTDC */

/**
  * @brief  Helper macro to calculate the PLLI2S frequency used for SAI1 and SAI2 domains
  * @note ex: @ref __LL_RCC_CALC_PLLI2S_SAI_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLI2S_GetN (), @ref LL_RCC_PLLI2S_GetQ (), @ref LL_RCC_PLLI2S_GetDIVQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLI2SN__ Between 50 and 432
  * @param  __PLLI2SQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_15
  * @param  __PLLI2SDIVQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_32
  * @retval PLLI2S clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLI2S_SAI_FREQ(__INPUTFREQ__, __PLLM__, __PLLI2SN__, __PLLI2SQ__, __PLLI2SDIVQ__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLI2SN__) / \
                   (((__PLLI2SQ__) >> RCC_PLLI2SCFGR_PLLI2SQ_Pos) * (((__PLLI2SDIVQ__) >> RCC_DCKCFGR1_PLLI2SDIVQ_Pos) + 1U)))

#if defined(SPDIFRX)
/**
  * @brief  Helper macro to calculate the PLLI2S frequency used on SPDIFRX domain
  * @note ex: @ref __LL_RCC_CALC_PLLI2S_SPDIFRX_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLI2S_GetN (), @ref LL_RCC_PLLI2S_GetP ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLI2SN__ Between 50 and 432
  * @param  __PLLI2SP__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SP_DIV_2
  *         @arg @ref LL_RCC_PLLI2SP_DIV_4
  *         @arg @ref LL_RCC_PLLI2SP_DIV_6
  *         @arg @ref LL_RCC_PLLI2SP_DIV_8
  * @retval PLLI2S clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLI2S_SPDIFRX_FREQ(__INPUTFREQ__, __PLLM__, __PLLI2SN__, __PLLI2SP__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLI2SN__) / \
                   ((((__PLLI2SP__) >> RCC_PLLI2SCFGR_PLLI2SP_Pos) + 1U) * 2U))
#endif /* SPDIFRX */

/**
  * @brief  Helper macro to calculate the PLLI2S frequency used for I2S domain
  * @note ex: @ref __LL_RCC_CALC_PLLI2S_I2S_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLLI2S_GetN (), @ref LL_RCC_PLLI2S_GetR ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  __PLLI2SN__ Between 50 and 432
  * @param  __PLLI2SR__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SR_DIV_2
  *         @arg @ref LL_RCC_PLLI2SR_DIV_3
  *         @arg @ref LL_RCC_PLLI2SR_DIV_4
  *         @arg @ref LL_RCC_PLLI2SR_DIV_5
  *         @arg @ref LL_RCC_PLLI2SR_DIV_6
  *         @arg @ref LL_RCC_PLLI2SR_DIV_7
  * @retval PLLI2S clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLI2S_I2S_FREQ(__INPUTFREQ__, __PLLM__, __PLLI2SN__, __PLLI2SR__) (((__INPUTFREQ__) / (__PLLM__)) * (__PLLI2SN__) / \
                   ((__PLLI2SR__) >> RCC_PLLI2SCFGR_PLLI2SR_Pos))

/**
  * @brief  Helper macro to calculate the HCLK frequency
  * @param  __SYSCLKFREQ__ SYSCLK frequency (based on HSE/HSI/PLLCLK)
  * @param  __AHBPRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SYSCLK_DIV_1
  *         @arg @ref LL_RCC_SYSCLK_DIV_2
  *         @arg @ref LL_RCC_SYSCLK_DIV_4
  *         @arg @ref LL_RCC_SYSCLK_DIV_8
  *         @arg @ref LL_RCC_SYSCLK_DIV_16
  *         @arg @ref LL_RCC_SYSCLK_DIV_64
  *         @arg @ref LL_RCC_SYSCLK_DIV_128
  *         @arg @ref LL_RCC_SYSCLK_DIV_256
  *         @arg @ref LL_RCC_SYSCLK_DIV_512
  * @retval HCLK clock frequency (in Hz)
  */
#define __LL_RCC_CALC_HCLK_FREQ(__SYSCLKFREQ__, __AHBPRESCALER__) ((__SYSCLKFREQ__) >> AHBPrescTable[((__AHBPRESCALER__) & RCC_CFGR_HPRE) >>  RCC_CFGR_HPRE_Pos])

/**
  * @brief  Helper macro to calculate the PCLK1 frequency (ABP1)
  * @param  __HCLKFREQ__ HCLK frequency
  * @param  __APB1PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  * @retval PCLK1 clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PCLK1_FREQ(__HCLKFREQ__, __APB1PRESCALER__) ((__HCLKFREQ__) >> APBPrescTable[(__APB1PRESCALER__) >>  RCC_CFGR_PPRE1_Pos])

/**
  * @brief  Helper macro to calculate the PCLK2 frequency (ABP2)
  * @param  __HCLKFREQ__ HCLK frequency
  * @param  __APB2PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  * @retval PCLK2 clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PCLK2_FREQ(__HCLKFREQ__, __APB2PRESCALER__) ((__HCLKFREQ__) >> APBPrescTable[(__APB2PRESCALER__) >>  RCC_CFGR_PPRE2_Pos])

/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup RCC_LL_Exported_Functions RCC Exported Functions
  * @{
  */

/** @defgroup RCC_LL_EF_HSE HSE
  * @{
  */

/**
  * @brief  Enable the Clock Security System.
  * @rmtoll CR           CSSON         LL_RCC_HSE_EnableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableCSS(void)
{
  SET_BIT(RCC->CR, RCC_CR_CSSON);
}

/**
  * @brief  Enable HSE external oscillator (HSE Bypass)
  * @rmtoll CR           HSEBYP        LL_RCC_HSE_EnableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableBypass(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSEBYP);
}

/**
  * @brief  Disable HSE external oscillator (HSE Bypass)
  * @rmtoll CR           HSEBYP        LL_RCC_HSE_DisableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_DisableBypass(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
}

/**
  * @brief  Enable HSE crystal oscillator (HSE ON)
  * @rmtoll CR           HSEON         LL_RCC_HSE_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSEON);
}

/**
  * @brief  Disable HSE crystal oscillator (HSE ON)
  * @rmtoll CR           HSEON         LL_RCC_HSE_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
}

/**
  * @brief  Check if HSE oscillator Ready
  * @rmtoll CR           HSERDY        LL_RCC_HSE_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSE_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_HSERDY) == (RCC_CR_HSERDY));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_HSI HSI
  * @{
  */

/**
  * @brief  Enable HSI oscillator
  * @rmtoll CR           HSION         LL_RCC_HSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSION);
}

/**
  * @brief  Disable HSI oscillator
  * @rmtoll CR           HSION         LL_RCC_HSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSION);
}

/**
  * @brief  Check if HSI clock is ready
  * @rmtoll CR           HSIRDY        LL_RCC_HSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_HSIRDY) == (RCC_CR_HSIRDY));
}

/**
  * @brief  Get HSI Calibration value
  * @note When HSITRIM is written, HSICAL is updated with the sum of
  *       HSITRIM and the factory trim value
  * @rmtoll CR        HSICAL        LL_RCC_HSI_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->CR, RCC_CR_HSICAL) >> RCC_CR_HSICAL_Pos);
}

/**
  * @brief  Set HSI Calibration trimming
  * @note user-programmable trimming value that is added to the HSICAL
  * @note Default value is 16, which, when added to the HSICAL value,
  *       should trim the HSI to 16 MHz +/- 1 %
  * @rmtoll CR        HSITRIM       LL_RCC_HSI_SetCalibTrimming
  * @param  Value Between Min_Data = 0 and Max_Data = 31
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->CR, RCC_CR_HSITRIM, Value << RCC_CR_HSITRIM_Pos);
}

/**
  * @brief  Get HSI Calibration trimming
  * @rmtoll CR        HSITRIM       LL_RCC_HSI_GetCalibTrimming
  * @retval Between Min_Data = 0 and Max_Data = 31
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->CR, RCC_CR_HSITRIM) >> RCC_CR_HSITRIM_Pos);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_LSE LSE
  * @{
  */

/**
  * @brief  Enable  Low Speed External (LSE) crystal.
  * @rmtoll BDCR         LSEON         LL_RCC_LSE_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_Enable(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);
}

/**
  * @brief  Disable  Low Speed External (LSE) crystal.
  * @rmtoll BDCR         LSEON         LL_RCC_LSE_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_Disable(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);
}

/**
  * @brief  Enable external clock source (LSE bypass).
  * @rmtoll BDCR         LSEBYP        LL_RCC_LSE_EnableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_EnableBypass(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
}

/**
  * @brief  Disable external clock source (LSE bypass).
  * @rmtoll BDCR         LSEBYP        LL_RCC_LSE_DisableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_DisableBypass(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
}

/**
  * @brief  Set LSE oscillator drive capability
  * @note The oscillator is in Xtal mode when it is not in bypass mode.
  * @rmtoll BDCR         LSEDRV        LL_RCC_LSE_SetDriveCapability
  * @param  LSEDrive This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LSEDRIVE_LOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_SetDriveCapability(uint32_t LSEDrive)
{
  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSEDRV, LSEDrive);
}

/**
  * @brief  Get LSE oscillator drive capability
  * @rmtoll BDCR         LSEDRV        LL_RCC_LSE_GetDriveCapability
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LSEDRIVE_LOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_HIGH
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_GetDriveCapability(void)
{
  return (uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_LSEDRV));
}

/**
  * @brief  Check if LSE oscillator Ready
  * @rmtoll BDCR         LSERDY        LL_RCC_LSE_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsReady(void)
{
  return (READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == (RCC_BDCR_LSERDY));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_LSI LSI
  * @{
  */

/**
  * @brief  Enable LSI Oscillator
  * @rmtoll CSR          LSION         LL_RCC_LSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSI_Enable(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSION);
}

/**
  * @brief  Disable LSI Oscillator
  * @rmtoll CSR          LSION         LL_RCC_LSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSI_Disable(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_LSION);
}

/**
  * @brief  Check if LSI is Ready
  * @rmtoll CSR          LSIRDY        LL_RCC_LSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSI_IsReady(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == (RCC_CSR_LSIRDY));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_System System
  * @{
  */

/**
  * @brief  Configure the system clock source
  * @rmtoll CFGR         SW            LL_RCC_SetSysClkSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_PLL
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSysClkSource(uint32_t Source)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, Source);
}

/**
  * @brief  Get the system clock source
  * @rmtoll CFGR         SWS           LL_RCC_GetSysClkSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_HSI
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_HSE
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_PLL
  */
__STATIC_INLINE uint32_t LL_RCC_GetSysClkSource(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_SWS));
}

/**
  * @brief  Set AHB prescaler
  * @rmtoll CFGR         HPRE          LL_RCC_SetAHBPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SYSCLK_DIV_1
  *         @arg @ref LL_RCC_SYSCLK_DIV_2
  *         @arg @ref LL_RCC_SYSCLK_DIV_4
  *         @arg @ref LL_RCC_SYSCLK_DIV_8
  *         @arg @ref LL_RCC_SYSCLK_DIV_16
  *         @arg @ref LL_RCC_SYSCLK_DIV_64
  *         @arg @ref LL_RCC_SYSCLK_DIV_128
  *         @arg @ref LL_RCC_SYSCLK_DIV_256
  *         @arg @ref LL_RCC_SYSCLK_DIV_512
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAHBPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, Prescaler);
}

/**
  * @brief  Set APB1 prescaler
  * @rmtoll CFGR         PPRE1         LL_RCC_SetAPB1Prescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAPB1Prescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, Prescaler);
}

/**
  * @brief  Set APB2 prescaler
  * @rmtoll CFGR         PPRE2         LL_RCC_SetAPB2Prescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAPB2Prescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, Prescaler);
}

/**
  * @brief  Get AHB prescaler
  * @rmtoll CFGR         HPRE          LL_RCC_GetAHBPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SYSCLK_DIV_1
  *         @arg @ref LL_RCC_SYSCLK_DIV_2
  *         @arg @ref LL_RCC_SYSCLK_DIV_4
  *         @arg @ref LL_RCC_SYSCLK_DIV_8
  *         @arg @ref LL_RCC_SYSCLK_DIV_16
  *         @arg @ref LL_RCC_SYSCLK_DIV_64
  *         @arg @ref LL_RCC_SYSCLK_DIV_128
  *         @arg @ref LL_RCC_SYSCLK_DIV_256
  *         @arg @ref LL_RCC_SYSCLK_DIV_512
  */
__STATIC_INLINE uint32_t LL_RCC_GetAHBPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_HPRE));
}

/**
  * @brief  Get APB1 prescaler
  * @rmtoll CFGR         PPRE1         LL_RCC_GetAPB1Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB1Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PPRE1));
}

/**
  * @brief  Get APB2 prescaler
  * @rmtoll CFGR         PPRE2         LL_RCC_GetAPB2Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB2Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PPRE2));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_MCO MCO
  * @{
  */

/**
  * @brief  Configure MCOx
  * @rmtoll CFGR         MCO1          LL_RCC_ConfigMCO\n
  *         CFGR         MCO1PRE       LL_RCC_ConfigMCO\n
  *         CFGR         MCO2          LL_RCC_ConfigMCO\n
  *         CFGR         MCO2PRE       LL_RCC_ConfigMCO
  * @param  MCOxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_PLLCLK
  *         @arg @ref LL_RCC_MCO2SOURCE_SYSCLK
  *         @arg @ref LL_RCC_MCO2SOURCE_PLLI2S
  *         @arg @ref LL_RCC_MCO2SOURCE_HSE
  *         @arg @ref LL_RCC_MCO2SOURCE_PLLCLK
  * @param  MCOxPrescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1_DIV_1
  *         @arg @ref LL_RCC_MCO1_DIV_2
  *         @arg @ref LL_RCC_MCO1_DIV_3
  *         @arg @ref LL_RCC_MCO1_DIV_4
  *         @arg @ref LL_RCC_MCO1_DIV_5
  *         @arg @ref LL_RCC_MCO2_DIV_1
  *         @arg @ref LL_RCC_MCO2_DIV_2
  *         @arg @ref LL_RCC_MCO2_DIV_3
  *         @arg @ref LL_RCC_MCO2_DIV_4
  *         @arg @ref LL_RCC_MCO2_DIV_5
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ConfigMCO(uint32_t MCOxSource, uint32_t MCOxPrescaler)
{
  MODIFY_REG(RCC->CFGR, (MCOxSource & 0xFFFF0000U) | (MCOxPrescaler & 0xFFFF0000U),  (MCOxSource << 16U) | (MCOxPrescaler << 16U));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_Peripheral_Clock_Source Peripheral Clock Source
  * @{
  */

/**
  * @brief  Configure USARTx clock source
  * @rmtoll DCKCFGR2        USART1SEL     LL_RCC_SetUSARTClockSource\n
  *         DCKCFGR2        USART2SEL     LL_RCC_SetUSARTClockSource\n
  *         DCKCFGR2        USART3SEL     LL_RCC_SetUSARTClockSource\n
  *         DCKCFGR2        USART6SEL     LL_RCC_SetUSARTClockSource
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSARTClockSource(uint32_t USARTxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, (USARTxSource >> 16U), (USARTxSource & 0x0000FFFFU));
}

/**
  * @brief  Configure UARTx clock source
  * @rmtoll DCKCFGR2        UART4SEL      LL_RCC_SetUARTClockSource\n
  *         DCKCFGR2        UART5SEL      LL_RCC_SetUARTClockSource\n
  *         DCKCFGR2        UART7SEL      LL_RCC_SetUARTClockSource\n
  *         DCKCFGR2        UART8SEL      LL_RCC_SetUARTClockSource
  * @param  UARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUARTClockSource(uint32_t UARTxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, (UARTxSource >> 16U), (UARTxSource & 0x0000FFFFU));
}

/**
  * @brief  Configure I2Cx clock source
  * @rmtoll DCKCFGR2        I2C1SEL       LL_RCC_SetI2CClockSource\n
  *         DCKCFGR2        I2C2SEL       LL_RCC_SetI2CClockSource\n
  *         DCKCFGR2        I2C3SEL       LL_RCC_SetI2CClockSource\n
  *         DCKCFGR2        I2C4SEL       LL_RCC_SetI2CClockSource
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_HSI (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2CClockSource(uint32_t I2CxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, (I2CxSource & 0xFFFF0000U),  (I2CxSource << 16U));
}

/**
  * @brief  Configure LPTIMx clock source
  * @rmtoll DCKCFGR2        LPTIM1SEL     LL_RCC_SetLPTIMClockSource
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetLPTIMClockSource(uint32_t LPTIMxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_LPTIM1SEL, LPTIMxSource);
}

/**
  * @brief  Configure SAIx clock source
  * @rmtoll DCKCFGR1        SAI1SEL       LL_RCC_SetSAIClockSource\n
  *         DCKCFGR1        SAI2SEL       LL_RCC_SetSAIClockSource
  * @param  SAIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLSAI
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLSRC (*)
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLSAI
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLSRC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSAIClockSource(uint32_t SAIxSource)
{
  MODIFY_REG(RCC->DCKCFGR1, (SAIxSource & 0xFFFF0000U), (SAIxSource << 16U));
}

/**
  * @brief  Configure SDMMC clock source
  * @rmtoll DCKCFGR2        SDMMC1SEL      LL_RCC_SetSDMMCClockSource\n
  *         DCKCFGR2        SDMMC2SEL      LL_RCC_SetSDMMCClockSource
  * @param  SDMMCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE_PLL48CLK
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_SDMMC2_CLKSOURCE_PLL48CLK (*)
  *         @arg @ref LL_RCC_SDMMC2_CLKSOURCE_SYSCLK (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSDMMCClockSource(uint32_t SDMMCxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, (SDMMCxSource & 0xFFFF0000U), (SDMMCxSource << 16U));
}

/**
  * @brief  Configure 48Mhz domain clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_SetCK48MClockSource
  * @param  CK48MxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CK48M_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_CK48M_CLKSOURCE_PLLSAI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCK48MClockSource(uint32_t CK48MxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_CK48MSEL, CK48MxSource);
}

/**
  * @brief  Configure RNG clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_SetRNGClockSource
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLLSAI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRNGClockSource(uint32_t RNGxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_CK48MSEL, RNGxSource);
}

/**
  * @brief  Configure USB clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_SetUSBClockSource
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLLSAI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSBClockSource(uint32_t USBxSource)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_CK48MSEL, USBxSource);
}

#if defined(CEC)
/**
  * @brief  Configure CEC clock source
  * @rmtoll DCKCFGR2         CECSEL        LL_RCC_SetCECClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV488
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCECClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_CECSEL, Source);
}
#endif /* CEC */

/**
  * @brief  Configure I2S clock source
  * @rmtoll CFGR         I2SSRC        LL_RCC_SetI2SClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PIN
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2SClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_I2SSRC, Source);
}

#if defined(DSI)
/**
  * @brief  Configure DSI clock source
  * @rmtoll DCKCFGR2         DSISEL        LL_RCC_SetDSIClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PHY
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PLL
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetDSIClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->DCKCFGR2, RCC_DCKCFGR2_DSISEL, Source);
}
#endif /* DSI */

#if defined(DFSDM1_Channel0)
/**
  * @brief  Configure DFSDM Audio clock source
  * @rmtoll DCKCFGR1         ADFSDM1SEL        LL_RCC_SetDFSDMAudioClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI1
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI2
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetDFSDMAudioClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_ADFSDM1SEL, Source);
}

/**
  * @brief  Configure DFSDM Kernel clock source
  * @rmtoll DCKCFGR1         DFSDM1SEL        LL_RCC_SetDFSDMClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE_SYSCLK
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetDFSDMClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_DFSDM1SEL, Source);
}
#endif /* DFSDM1_Channel0 */

/**
  * @brief  Get USARTx clock source
  * @rmtoll DCKCFGR2        USART1SEL     LL_RCC_GetUSARTClockSource\n
  *         DCKCFGR2        USART2SEL     LL_RCC_GetUSARTClockSource\n
  *         DCKCFGR2        USART3SEL     LL_RCC_GetUSARTClockSource\n
  *         DCKCFGR2        USART6SEL     LL_RCC_GetUSARTClockSource
  * @param  USARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE
  *         @arg @ref LL_RCC_USART3_CLKSOURCE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSARTClockSource(uint32_t USARTx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, USARTx) | (USARTx << 16U));
}

/**
  * @brief  Get UARTx clock source
  * @rmtoll DCKCFGR2        UART4SEL      LL_RCC_GetUARTClockSource\n
  *         DCKCFGR2        UART5SEL      LL_RCC_GetUARTClockSource\n
  *         DCKCFGR2        UART7SEL      LL_RCC_GetUARTClockSource\n
  *         DCKCFGR2        UART8SEL      LL_RCC_GetUARTClockSource
  * @param  UARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_UART4_CLKSOURCE
  *         @arg @ref LL_RCC_UART5_CLKSOURCE
  *         @arg @ref LL_RCC_UART7_CLKSOURCE
  *         @arg @ref LL_RCC_UART8_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART4_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART5_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART7_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART8_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetUARTClockSource(uint32_t UARTx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, UARTx) | (UARTx << 16U));
}

/**
  * @brief  Get I2Cx clock source
  * @rmtoll DCKCFGR2        I2C1SEL       LL_RCC_GetI2CClockSource\n
  *         DCKCFGR2        I2C2SEL       LL_RCC_GetI2CClockSource\n
  *         DCKCFGR2        I2C3SEL       LL_RCC_GetI2CClockSource\n
  *         DCKCFGR2        I2C4SEL       LL_RCC_GetI2CClockSource
  * @param  I2Cx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE (*)
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE_HSI (*)
  *
  *         (*) value not defined in all devices.
 */
__STATIC_INLINE uint32_t LL_RCC_GetI2CClockSource(uint32_t I2Cx)
{
  return (uint32_t)((READ_BIT(RCC->DCKCFGR2, I2Cx) >> 16U) | I2Cx);
}

/**
  * @brief  Get LPTIMx clock source
  * @rmtoll DCKCFGR2        LPTIM1SEL     LL_RCC_GetLPTIMClockSource
  * @param  LPTIMx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetLPTIMClockSource(uint32_t LPTIMx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, RCC_DCKCFGR2_LPTIM1SEL));
}

/**
  * @brief  Get SAIx clock source
  * @rmtoll DCKCFGR1        SAI1SEL       LL_RCC_GetSAIClockSource\n
  *         DCKCFGR1        SAI2SEL       LL_RCC_GetSAIClockSource
  * @param  SAIx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLSAI
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLLSRC (*)
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLSAI
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLLSRC (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetSAIClockSource(uint32_t SAIx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, SAIx) >> 16U | SAIx);
}

/**
  * @brief  Get SDMMCx clock source
  * @rmtoll DCKCFGR2        SDMMC1SEL      LL_RCC_GetSDMMCClockSource\n
  *         DCKCFGR2        SDMMC2SEL      LL_RCC_GetSDMMCClockSource
  * @param  SDMMCx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE (*)
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE_PLL48CLK
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_SDMMC2_CLKSOURCE_PLL48CLK (*)
  *         @arg @ref LL_RCC_SDMMC2_CLKSOURCE_SYSCLK (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetSDMMCClockSource(uint32_t SDMMCx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, SDMMCx) >> 16U | SDMMCx);
}

/**
  * @brief  Get 48Mhz domain clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_GetCK48MClockSource
  * @param  CK48Mx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CK48M_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CK48M_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_CK48M_CLKSOURCE_PLLSAI
  */
__STATIC_INLINE uint32_t LL_RCC_GetCK48MClockSource(uint32_t CK48Mx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, CK48Mx));
}

/**
  * @brief  Get RNGx clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_GetRNGClockSource
  * @param  RNGx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLLSAI
  */
__STATIC_INLINE uint32_t LL_RCC_GetRNGClockSource(uint32_t RNGx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, RNGx));
}

/**
  * @brief  Get USBx clock source
  * @rmtoll DCKCFGR2        CK48MSEL      LL_RCC_GetUSBClockSource
  * @param  USBx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLLSAI
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSBClockSource(uint32_t USBx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, USBx));
}

#if defined(CEC)
/**
  * @brief  Get CEC Clock Source
  * @rmtoll DCKCFGR2         CECSEL        LL_RCC_GetCECClockSource
  * @param  CECx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV488
  */
__STATIC_INLINE uint32_t LL_RCC_GetCECClockSource(uint32_t CECx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, CECx));
}
#endif /* CEC */

/**
  * @brief  Get I2S Clock Source
  * @rmtoll CFGR         I2SSRC        LL_RCC_GetI2SClockSource
  * @param  I2Sx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PLLI2S
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PIN
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2SClockSource(uint32_t I2Sx)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, I2Sx));
}

#if defined(DFSDM1_Channel0)
/**
  * @brief  Get DFSDM Audio Clock Source
  * @rmtoll DCKCFGR1         ADFSDM1SEL        LL_RCC_GetDFSDMAudioClockSource
  * @param  DFSDMx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI1
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI2
  */
__STATIC_INLINE uint32_t LL_RCC_GetDFSDMAudioClockSource(uint32_t DFSDMx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, DFSDMx));
}

/**
  * @brief  Get DFSDM Audio Clock Source
  * @rmtoll DCKCFGR1         DFSDM1SEL        LL_RCC_GetDFSDMClockSource
  * @param  DFSDMx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE_SYSCLK
  */
__STATIC_INLINE uint32_t LL_RCC_GetDFSDMClockSource(uint32_t DFSDMx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, DFSDMx));
}
#endif /* DFSDM1_Channel0 */

#if defined(DSI)
/**
  * @brief  Get DSI Clock Source
  * @rmtoll DCKCFGR2         DSISEL        LL_RCC_GetDSIClockSource
  * @param  DSIx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PHY
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PLL
  */
__STATIC_INLINE uint32_t LL_RCC_GetDSIClockSource(uint32_t DSIx)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR2, DSIx));
}
#endif /* DSI */

/**
  * @}
  */

/** @defgroup RCC_LL_EF_RTC RTC
  * @{
  */

/**
  * @brief  Set RTC Clock Source
  * @note Once the RTC clock source has been selected, it cannot be changed anymore unless
  *       the Backup domain is reset, or unless a failure is detected on LSE (LSECSSD is
  *       set). The BDRST bit can be used to reset them.
  * @rmtoll BDCR         RTCSEL        LL_RCC_SetRTCClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTCClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, Source);
}

/**
  * @brief  Get RTC Clock Source
  * @rmtoll BDCR         RTCSEL        LL_RCC_GetRTCClockSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTCClockSource(void)
{
  return (uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL));
}

/**
  * @brief  Enable RTC
  * @rmtoll BDCR         RTCEN         LL_RCC_EnableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableRTC(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
}

/**
  * @brief  Disable RTC
  * @rmtoll BDCR         RTCEN         LL_RCC_DisableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableRTC(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
}

/**
  * @brief  Check if RTC has been enabled or not
  * @rmtoll BDCR         RTCEN         LL_RCC_IsEnabledRTC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledRTC(void)
{
  return (READ_BIT(RCC->BDCR, RCC_BDCR_RTCEN) == (RCC_BDCR_RTCEN));
}

/**
  * @brief  Force the Backup domain reset
  * @rmtoll BDCR         BDRST         LL_RCC_ForceBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ForceBackupDomainReset(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_BDRST);
}

/**
  * @brief  Release the Backup domain reset
  * @rmtoll BDCR         BDRST         LL_RCC_ReleaseBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ReleaseBackupDomainReset(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST);
}

/**
  * @brief  Set HSE Prescalers for RTC Clock
  * @rmtoll CFGR         RTCPRE        LL_RCC_SetRTC_HSEPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_NOCLOCK
  *         @arg @ref LL_RCC_RTC_HSE_DIV_2
  *         @arg @ref LL_RCC_RTC_HSE_DIV_3
  *         @arg @ref LL_RCC_RTC_HSE_DIV_4
  *         @arg @ref LL_RCC_RTC_HSE_DIV_5
  *         @arg @ref LL_RCC_RTC_HSE_DIV_6
  *         @arg @ref LL_RCC_RTC_HSE_DIV_7
  *         @arg @ref LL_RCC_RTC_HSE_DIV_8
  *         @arg @ref LL_RCC_RTC_HSE_DIV_9
  *         @arg @ref LL_RCC_RTC_HSE_DIV_10
  *         @arg @ref LL_RCC_RTC_HSE_DIV_11
  *         @arg @ref LL_RCC_RTC_HSE_DIV_12
  *         @arg @ref LL_RCC_RTC_HSE_DIV_13
  *         @arg @ref LL_RCC_RTC_HSE_DIV_14
  *         @arg @ref LL_RCC_RTC_HSE_DIV_15
  *         @arg @ref LL_RCC_RTC_HSE_DIV_16
  *         @arg @ref LL_RCC_RTC_HSE_DIV_17
  *         @arg @ref LL_RCC_RTC_HSE_DIV_18
  *         @arg @ref LL_RCC_RTC_HSE_DIV_19
  *         @arg @ref LL_RCC_RTC_HSE_DIV_20
  *         @arg @ref LL_RCC_RTC_HSE_DIV_21
  *         @arg @ref LL_RCC_RTC_HSE_DIV_22
  *         @arg @ref LL_RCC_RTC_HSE_DIV_23
  *         @arg @ref LL_RCC_RTC_HSE_DIV_24
  *         @arg @ref LL_RCC_RTC_HSE_DIV_25
  *         @arg @ref LL_RCC_RTC_HSE_DIV_26
  *         @arg @ref LL_RCC_RTC_HSE_DIV_27
  *         @arg @ref LL_RCC_RTC_HSE_DIV_28
  *         @arg @ref LL_RCC_RTC_HSE_DIV_29
  *         @arg @ref LL_RCC_RTC_HSE_DIV_30
  *         @arg @ref LL_RCC_RTC_HSE_DIV_31
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTC_HSEPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_RTCPRE, Prescaler);
}

/**
  * @brief  Get HSE Prescalers for RTC Clock
  * @rmtoll CFGR         RTCPRE        LL_RCC_GetRTC_HSEPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_NOCLOCK
  *         @arg @ref LL_RCC_RTC_HSE_DIV_2
  *         @arg @ref LL_RCC_RTC_HSE_DIV_3
  *         @arg @ref LL_RCC_RTC_HSE_DIV_4
  *         @arg @ref LL_RCC_RTC_HSE_DIV_5
  *         @arg @ref LL_RCC_RTC_HSE_DIV_6
  *         @arg @ref LL_RCC_RTC_HSE_DIV_7
  *         @arg @ref LL_RCC_RTC_HSE_DIV_8
  *         @arg @ref LL_RCC_RTC_HSE_DIV_9
  *         @arg @ref LL_RCC_RTC_HSE_DIV_10
  *         @arg @ref LL_RCC_RTC_HSE_DIV_11
  *         @arg @ref LL_RCC_RTC_HSE_DIV_12
  *         @arg @ref LL_RCC_RTC_HSE_DIV_13
  *         @arg @ref LL_RCC_RTC_HSE_DIV_14
  *         @arg @ref LL_RCC_RTC_HSE_DIV_15
  *         @arg @ref LL_RCC_RTC_HSE_DIV_16
  *         @arg @ref LL_RCC_RTC_HSE_DIV_17
  *         @arg @ref LL_RCC_RTC_HSE_DIV_18
  *         @arg @ref LL_RCC_RTC_HSE_DIV_19
  *         @arg @ref LL_RCC_RTC_HSE_DIV_20
  *         @arg @ref LL_RCC_RTC_HSE_DIV_21
  *         @arg @ref LL_RCC_RTC_HSE_DIV_22
  *         @arg @ref LL_RCC_RTC_HSE_DIV_23
  *         @arg @ref LL_RCC_RTC_HSE_DIV_24
  *         @arg @ref LL_RCC_RTC_HSE_DIV_25
  *         @arg @ref LL_RCC_RTC_HSE_DIV_26
  *         @arg @ref LL_RCC_RTC_HSE_DIV_27
  *         @arg @ref LL_RCC_RTC_HSE_DIV_28
  *         @arg @ref LL_RCC_RTC_HSE_DIV_29
  *         @arg @ref LL_RCC_RTC_HSE_DIV_30
  *         @arg @ref LL_RCC_RTC_HSE_DIV_31
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTC_HSEPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_RTCPRE));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_TIM_CLOCK_PRESCALER TIM
  * @{
  */

/**
  * @brief  Set Timers Clock Prescalers
  * @rmtoll DCKCFGR1         TIMPRE        LL_RCC_SetTIMPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIM_PRESCALER_TWICE
  *         @arg @ref LL_RCC_TIM_PRESCALER_FOUR_TIMES
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetTIMPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_TIMPRE, Prescaler);
}

/**
  * @brief  Get Timers Clock Prescalers
  * @rmtoll DCKCFGR1         TIMPRE        LL_RCC_GetTIMPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_TIM_PRESCALER_TWICE
  *         @arg @ref LL_RCC_TIM_PRESCALER_FOUR_TIMES
  */
__STATIC_INLINE uint32_t LL_RCC_GetTIMPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, RCC_DCKCFGR1_TIMPRE));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_PLL PLL
  * @{
  */

/**
  * @brief  Enable PLL
  * @rmtoll CR           PLLON         LL_RCC_PLL_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_PLLON);
}

/**
  * @brief  Disable PLL
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @rmtoll CR           PLLON         LL_RCC_PLL_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
}

/**
  * @brief  Check if PLL Ready
  * @rmtoll CR           PLLRDY        LL_RCC_PLL_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_PLLRDY) == (RCC_CR_PLLRDY));
}

/**
  * @brief  Configure PLL used for SYSCLK Domain
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLP can be written only when PLL is disabled
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLP          LL_RCC_PLL_ConfigDomain_SYS
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLP This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_SYS(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLP)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP,
             Source | PLLM | PLLN << RCC_PLLCFGR_PLLN_Pos | PLLP);
}

/**
  * @brief  Configure PLL used for 48Mhz domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLQ can be written only when PLL is disabled
  * @note This  can be selected for USB, RNG, SDMMC1
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_48M\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_48M\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_48M\n
  *         PLLCFGR      PLLQ          LL_RCC_PLL_ConfigDomain_48M
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  *         @arg @ref LL_RCC_PLLQ_DIV_9
  *         @arg @ref LL_RCC_PLLQ_DIV_10
  *         @arg @ref LL_RCC_PLLQ_DIV_11
  *         @arg @ref LL_RCC_PLLQ_DIV_12
  *         @arg @ref LL_RCC_PLLQ_DIV_13
  *         @arg @ref LL_RCC_PLLQ_DIV_14
  *         @arg @ref LL_RCC_PLLQ_DIV_15
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_48M(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ,
             Source | PLLM | PLLN << RCC_PLLCFGR_PLLN_Pos | PLLQ);
}

#if defined(DSI)
/**
  * @brief  Configure PLL used for DSI clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLR can be written only when PLL is disabled
  * @note This  can be selected for DSI
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_DSI\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_DSI\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_DSI\n
  *         PLLCFGR      PLLR          LL_RCC_PLL_ConfigDomain_DSI
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLR This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLR_DIV_2
  *         @arg @ref LL_RCC_PLLR_DIV_3
  *         @arg @ref LL_RCC_PLLR_DIV_4
  *         @arg @ref LL_RCC_PLLR_DIV_5
  *         @arg @ref LL_RCC_PLLR_DIV_6
  *         @arg @ref LL_RCC_PLLR_DIV_7
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_DSI(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLR)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLR,
             Source | PLLM | PLLN << RCC_PLLCFGR_PLLN_Pos | PLLR);
}
#endif /* DSI */

/**
  * @brief  Get Main PLL multiplication factor for VCO
  * @rmtoll PLLCFGR      PLLN          LL_RCC_PLL_GetN
  * @retval Between 50 and 432
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetN(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLN) >>  RCC_PLLCFGR_PLLN_Pos);
}

/**
  * @brief  Get Main PLL division factor for PLLP 
  * @rmtoll PLLCFGR      PLLP       LL_RCC_PLL_GetP
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetP(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLP));
}

/**
  * @brief  Get Main PLL division factor for PLLQ
  * @note used for PLL48MCLK selected for USB, RNG, SDMMC (48 MHz clock)
  * @rmtoll PLLCFGR      PLLQ          LL_RCC_PLL_GetQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  *         @arg @ref LL_RCC_PLLQ_DIV_9
  *         @arg @ref LL_RCC_PLLQ_DIV_10
  *         @arg @ref LL_RCC_PLLQ_DIV_11
  *         @arg @ref LL_RCC_PLLQ_DIV_12
  *         @arg @ref LL_RCC_PLLQ_DIV_13
  *         @arg @ref LL_RCC_PLLQ_DIV_14
  *         @arg @ref LL_RCC_PLLQ_DIV_15
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetQ(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ));
}

#if defined(RCC_PLLCFGR_PLLR)
/**
  * @brief  Get Main PLL division factor for PLLR
  * @note used for PLLCLK (system clock)
  * @rmtoll PLLCFGR      PLLR          LL_RCC_PLL_GetR
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLR_DIV_2
  *         @arg @ref LL_RCC_PLLR_DIV_3
  *         @arg @ref LL_RCC_PLLR_DIV_4
  *         @arg @ref LL_RCC_PLLR_DIV_5
  *         @arg @ref LL_RCC_PLLR_DIV_6
  *         @arg @ref LL_RCC_PLLR_DIV_7
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetR(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLR));
}
#endif /* RCC_PLLCFGR_PLLR */

/**
  * @brief  Get the oscillator used as PLL clock source.
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_GetMainSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetMainSource(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC));
}

/**
  * @brief  Get Division factor for the main PLL and other PLL
  * @rmtoll PLLCFGR      PLLM          LL_RCC_PLL_GetDivider
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetDivider(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLM));
}

/**
  * @brief  Configure Spread Spectrum used for PLL
  * @note These bits must be written before enabling PLL
  * @rmtoll SSCGR        MODPER        LL_RCC_PLL_ConfigSpreadSpectrum\n
  *         SSCGR        INCSTEP       LL_RCC_PLL_ConfigSpreadSpectrum\n
  *         SSCGR        SPREADSEL     LL_RCC_PLL_ConfigSpreadSpectrum
  * @param  Mod Between Min_Data=0 and Max_Data=8191
  * @param  Inc Between Min_Data=0 and Max_Data=32767
  * @param  Sel This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPREAD_SELECT_CENTER
  *         @arg @ref LL_RCC_SPREAD_SELECT_DOWN
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigSpreadSpectrum(uint32_t Mod, uint32_t Inc, uint32_t Sel)
{
  MODIFY_REG(RCC->SSCGR, RCC_SSCGR_MODPER | RCC_SSCGR_INCSTEP | RCC_SSCGR_SPREADSEL, Mod | (Inc << RCC_SSCGR_INCSTEP_Pos) | Sel);
}

/**
  * @brief  Get Spread Spectrum Modulation Period for PLL
  * @rmtoll SSCGR         MODPER        LL_RCC_PLL_GetPeriodModulation
  * @retval Between Min_Data=0 and Max_Data=8191
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetPeriodModulation(void)
{
  return (uint32_t)(READ_BIT(RCC->SSCGR, RCC_SSCGR_MODPER));
}

/**
  * @brief  Get Spread Spectrum Incrementation Step for PLL
  * @note Must be written before enabling PLL
  * @rmtoll SSCGR         INCSTEP        LL_RCC_PLL_GetStepIncrementation
  * @retval Between Min_Data=0 and Max_Data=32767
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetStepIncrementation(void)
{
  return (uint32_t)(READ_BIT(RCC->SSCGR, RCC_SSCGR_INCSTEP) >> RCC_SSCGR_INCSTEP_Pos);
}

/**
  * @brief  Get Spread Spectrum Selection for PLL
  * @note Must be written before enabling PLL
  * @rmtoll SSCGR         SPREADSEL        LL_RCC_PLL_GetSpreadSelection
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SPREAD_SELECT_CENTER
  *         @arg @ref LL_RCC_SPREAD_SELECT_DOWN
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetSpreadSelection(void)
{
  return (uint32_t)(READ_BIT(RCC->SSCGR, RCC_SSCGR_SPREADSEL));
}

/**
  * @brief  Enable Spread Spectrum for PLL.
  * @rmtoll SSCGR         SSCGEN         LL_RCC_PLL_SpreadSpectrum_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_SpreadSpectrum_Enable(void)
{
  SET_BIT(RCC->SSCGR, RCC_SSCGR_SSCGEN);
}

/**
  * @brief  Disable Spread Spectrum for PLL.
  * @rmtoll SSCGR         SSCGEN         LL_RCC_PLL_SpreadSpectrum_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_SpreadSpectrum_Disable(void)
{
  CLEAR_BIT(RCC->SSCGR, RCC_SSCGR_SSCGEN);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_PLLI2S PLLI2S
  * @{
  */

/**
  * @brief  Enable PLLI2S
  * @rmtoll CR           PLLI2SON     LL_RCC_PLLI2S_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLI2S_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_PLLI2SON);
}

/**
  * @brief  Disable PLLI2S
  * @rmtoll CR           PLLI2SON     LL_RCC_PLLI2S_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLI2S_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_PLLI2SON);
}

/**
  * @brief  Check if PLLI2S Ready
  * @rmtoll CR           PLLI2SRDY    LL_RCC_PLLI2S_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_PLLI2SRDY) == (RCC_CR_PLLI2SRDY));
}

/**
  * @brief  Configure PLLI2S used for SAI1 and SAI2 domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLQ can be written only when PLLI2S is disabled
  * @note This can be selected for SAI1 and SAI2
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLI2S_ConfigDomain_SAI\n
  *         PLLCFGR      PLLM          LL_RCC_PLLI2S_ConfigDomain_SAI\n
  *         PLLI2SCFGR   PLLI2SN       LL_RCC_PLLI2S_ConfigDomain_SAI\n
  *         PLLI2SCFGR   PLLI2SQ       LL_RCC_PLLI2S_ConfigDomain_SAI\n
  *         DCKCFGR1      PLLI2SDIVQ    LL_RCC_PLLI2S_ConfigDomain_SAI
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_15
  * @param  PLLDIVQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_32
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLI2S_ConfigDomain_SAI(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ, uint32_t PLLDIVQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN | RCC_PLLI2SCFGR_PLLI2SQ, PLLN << RCC_PLLI2SCFGR_PLLI2SN_Pos | PLLQ);
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLI2SDIVQ, PLLDIVQ);
}

#if defined(SPDIFRX)
/**
  * @brief Configure PLLI2S used for SPDIFRX domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLP can be written only when PLLI2S is disabled
  * @note This  can be selected for SPDIFRX
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLI2S_ConfigDomain_SPDIFRX\n
  *         PLLCFGR      PLLM          LL_RCC_PLLI2S_ConfigDomain_SPDIFRX\n
  *         PLLI2SCFGR   PLLI2SN       LL_RCC_PLLI2S_ConfigDomain_SPDIFRX\n
  *         PLLI2SCFGR   PLLI2SP       LL_RCC_PLLI2S_ConfigDomain_SPDIFRX
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLP This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SP_DIV_2
  *         @arg @ref LL_RCC_PLLI2SP_DIV_4
  *         @arg @ref LL_RCC_PLLI2SP_DIV_6
  *         @arg @ref LL_RCC_PLLI2SP_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLI2S_ConfigDomain_SPDIFRX(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLP)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN | RCC_PLLI2SCFGR_PLLI2SP, PLLN << RCC_PLLI2SCFGR_PLLI2SN_Pos | PLLP);
}
#endif /* SPDIFRX */

/**
  * @brief  Configure PLLI2S used for I2S1 domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLR can be written only when PLLI2S is disabled
  * @note This  can be selected for I2S
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLI2S_ConfigDomain_I2S\n
  *         PLLCFGR      PLLM          LL_RCC_PLLI2S_ConfigDomain_I2S\n
  *         PLLI2SCFGR   PLLI2SN       LL_RCC_PLLI2S_ConfigDomain_I2S\n
  *         PLLI2SCFGR   PLLI2SR       LL_RCC_PLLI2S_ConfigDomain_I2S
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLR This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SR_DIV_2
  *         @arg @ref LL_RCC_PLLI2SR_DIV_3
  *         @arg @ref LL_RCC_PLLI2SR_DIV_4
  *         @arg @ref LL_RCC_PLLI2SR_DIV_5
  *         @arg @ref LL_RCC_PLLI2SR_DIV_6
  *         @arg @ref LL_RCC_PLLI2SR_DIV_7
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLI2S_ConfigDomain_I2S(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLR)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN | RCC_PLLI2SCFGR_PLLI2SR, PLLN << RCC_PLLI2SCFGR_PLLI2SN_Pos | PLLR);
}

/**
  * @brief  Get I2SPLL multiplication factor for VCO
  * @rmtoll PLLI2SCFGR  PLLI2SN      LL_RCC_PLLI2S_GetN
  * @retval Between 50 and 432
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_GetN(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN) >> RCC_PLLI2SCFGR_PLLI2SN_Pos);
}

/**
  * @brief  Get I2SPLL division factor for PLLI2SQ
  * @rmtoll PLLI2SCFGR  PLLI2SQ      LL_RCC_PLLI2S_GetQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SQ_DIV_15
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_GetQ(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SQ));
}

/**
  * @brief  Get I2SPLL division factor for PLLI2SR
  * @note used for PLLI2SCLK (I2S clock)
  * @rmtoll PLLI2SCFGR  PLLI2SR      LL_RCC_PLLI2S_GetR
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SR_DIV_2
  *         @arg @ref LL_RCC_PLLI2SR_DIV_3
  *         @arg @ref LL_RCC_PLLI2SR_DIV_4
  *         @arg @ref LL_RCC_PLLI2SR_DIV_5
  *         @arg @ref LL_RCC_PLLI2SR_DIV_6
  *         @arg @ref LL_RCC_PLLI2SR_DIV_7
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_GetR(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SR));
}

#if defined(RCC_PLLI2SCFGR_PLLI2SP)
/**
  * @brief  Get I2SPLL division factor for PLLI2SP
  * @note used for PLLSPDIFRXCLK (SPDIFRX clock)
  * @rmtoll PLLI2SCFGR  PLLI2SP      LL_RCC_PLLI2S_GetP
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SP_DIV_2
  *         @arg @ref LL_RCC_PLLI2SP_DIV_4
  *         @arg @ref LL_RCC_PLLI2SP_DIV_6
  *         @arg @ref LL_RCC_PLLI2SP_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_GetP(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SP));
}
#endif /* RCC_PLLI2SCFGR_PLLI2SP */

/**
  * @brief  Get I2SPLL division factor for PLLI2SDIVQ
  * @note used PLLSAI1CLK, PLLSAI2CLK selected (SAI1 and SAI2 clock)
  * @rmtoll DCKCFGR1   PLLI2SDIVQ      LL_RCC_PLLI2S_GetDIVQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLI2SDIVQ_DIV_32
  */
__STATIC_INLINE uint32_t LL_RCC_PLLI2S_GetDIVQ(void)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLI2SDIVQ));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_PLLSAI PLLSAI
  * @{
  */

/**
  * @brief  Enable PLLSAI
  * @rmtoll CR           PLLSAION     LL_RCC_PLLSAI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLSAI_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_PLLSAION);
}

/**
  * @brief  Disable PLLSAI
  * @rmtoll CR           PLLSAION     LL_RCC_PLLSAI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLSAI_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_PLLSAION);
}

/**
  * @brief  Check if PLLSAI Ready
  * @rmtoll CR           PLLSAIRDY    LL_RCC_PLLSAI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_PLLSAIRDY) == (RCC_CR_PLLSAIRDY));
}

/**
  * @brief  Configure PLLSAI used for SAI1 and SAI2 domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLQ can be written only when PLLSAI is disabled
  * @note This can be selected for SAI1 and SAI2
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLSAI_ConfigDomain_SAI\n
  *         PLLCFGR      PLLM          LL_RCC_PLLSAI_ConfigDomain_SAI\n
  *         PLLSAICFGR   PLLSAIN       LL_RCC_PLLSAI_ConfigDomain_SAI\n
  *         PLLSAICFGR   PLLSAIQ       LL_RCC_PLLSAI_ConfigDomain_SAI\n
  *         DCKCFGR1     PLLSAIDIVQ    LL_RCC_PLLSAI_ConfigDomain_SAI
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_15
  * @param  PLLDIVQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_32
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLSAI_ConfigDomain_SAI(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ, uint32_t PLLDIVQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIN | RCC_PLLSAICFGR_PLLSAIQ, PLLN << RCC_PLLSAICFGR_PLLSAIN_Pos | PLLQ);
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLSAIDIVQ, PLLDIVQ);
}

/**
  * @brief Configure PLLSAI used for 48Mhz domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLP can be written only when PLLSAI is disabled
  * @note This  can be selected for USB, RNG, SDMMC1
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLSAI_ConfigDomain_48M\n
  *         PLLCFGR      PLLM          LL_RCC_PLLSAI_ConfigDomain_48M\n
  *         PLLSAICFGR   PLLSAIN       LL_RCC_PLLSAI_ConfigDomain_48M\n
  *         PLLSAICFGR   PLLSAIP       LL_RCC_PLLSAI_ConfigDomain_48M
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLP This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIP_DIV_2
  *         @arg @ref LL_RCC_PLLSAIP_DIV_4
  *         @arg @ref LL_RCC_PLLSAIP_DIV_6
  *         @arg @ref LL_RCC_PLLSAIP_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLSAI_ConfigDomain_48M(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLP)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIN | RCC_PLLSAICFGR_PLLSAIP, PLLN << RCC_PLLSAICFGR_PLLSAIN_Pos | PLLP);
}

#if defined(LTDC)
/**
  * @brief  Configure PLLSAI used for LTDC domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL,
  *       PLLI2S and PLLSAI are disabled
  * @note PLLN/PLLR can be written only when PLLSAI is disabled
  * @note This  can be selected for LTDC
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLLSAI_ConfigDomain_LTDC\n
  *         PLLCFGR      PLLM          LL_RCC_PLLSAI_ConfigDomain_LTDC\n
  *         PLLSAICFGR   PLLSAIN       LL_RCC_PLLSAI_ConfigDomain_LTDC\n
  *         PLLSAICFGR   PLLSAIR       LL_RCC_PLLSAI_ConfigDomain_LTDC\n
  *         DCKCFGR1     PLLSAIDIVR    LL_RCC_PLLSAI_ConfigDomain_LTDC
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  *         @arg @ref LL_RCC_PLLM_DIV_9
  *         @arg @ref LL_RCC_PLLM_DIV_10
  *         @arg @ref LL_RCC_PLLM_DIV_11
  *         @arg @ref LL_RCC_PLLM_DIV_12
  *         @arg @ref LL_RCC_PLLM_DIV_13
  *         @arg @ref LL_RCC_PLLM_DIV_14
  *         @arg @ref LL_RCC_PLLM_DIV_15
  *         @arg @ref LL_RCC_PLLM_DIV_16
  *         @arg @ref LL_RCC_PLLM_DIV_17
  *         @arg @ref LL_RCC_PLLM_DIV_18
  *         @arg @ref LL_RCC_PLLM_DIV_19
  *         @arg @ref LL_RCC_PLLM_DIV_20
  *         @arg @ref LL_RCC_PLLM_DIV_21
  *         @arg @ref LL_RCC_PLLM_DIV_22
  *         @arg @ref LL_RCC_PLLM_DIV_23
  *         @arg @ref LL_RCC_PLLM_DIV_24
  *         @arg @ref LL_RCC_PLLM_DIV_25
  *         @arg @ref LL_RCC_PLLM_DIV_26
  *         @arg @ref LL_RCC_PLLM_DIV_27
  *         @arg @ref LL_RCC_PLLM_DIV_28
  *         @arg @ref LL_RCC_PLLM_DIV_29
  *         @arg @ref LL_RCC_PLLM_DIV_30
  *         @arg @ref LL_RCC_PLLM_DIV_31
  *         @arg @ref LL_RCC_PLLM_DIV_32
  *         @arg @ref LL_RCC_PLLM_DIV_33
  *         @arg @ref LL_RCC_PLLM_DIV_34
  *         @arg @ref LL_RCC_PLLM_DIV_35
  *         @arg @ref LL_RCC_PLLM_DIV_36
  *         @arg @ref LL_RCC_PLLM_DIV_37
  *         @arg @ref LL_RCC_PLLM_DIV_38
  *         @arg @ref LL_RCC_PLLM_DIV_39
  *         @arg @ref LL_RCC_PLLM_DIV_40
  *         @arg @ref LL_RCC_PLLM_DIV_41
  *         @arg @ref LL_RCC_PLLM_DIV_42
  *         @arg @ref LL_RCC_PLLM_DIV_43
  *         @arg @ref LL_RCC_PLLM_DIV_44
  *         @arg @ref LL_RCC_PLLM_DIV_45
  *         @arg @ref LL_RCC_PLLM_DIV_46
  *         @arg @ref LL_RCC_PLLM_DIV_47
  *         @arg @ref LL_RCC_PLLM_DIV_48
  *         @arg @ref LL_RCC_PLLM_DIV_49
  *         @arg @ref LL_RCC_PLLM_DIV_50
  *         @arg @ref LL_RCC_PLLM_DIV_51
  *         @arg @ref LL_RCC_PLLM_DIV_52
  *         @arg @ref LL_RCC_PLLM_DIV_53
  *         @arg @ref LL_RCC_PLLM_DIV_54
  *         @arg @ref LL_RCC_PLLM_DIV_55
  *         @arg @ref LL_RCC_PLLM_DIV_56
  *         @arg @ref LL_RCC_PLLM_DIV_57
  *         @arg @ref LL_RCC_PLLM_DIV_58
  *         @arg @ref LL_RCC_PLLM_DIV_59
  *         @arg @ref LL_RCC_PLLM_DIV_60
  *         @arg @ref LL_RCC_PLLM_DIV_61
  *         @arg @ref LL_RCC_PLLM_DIV_62
  *         @arg @ref LL_RCC_PLLM_DIV_63
  * @param  PLLN Between 50 and 432
  * @param  PLLR This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIR_DIV_3
  *         @arg @ref LL_RCC_PLLSAIR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIR_DIV_5
  *         @arg @ref LL_RCC_PLLSAIR_DIV_6
  *         @arg @ref LL_RCC_PLLSAIR_DIV_7
  * @param  PLLDIVR This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLLSAI_ConfigDomain_LTDC(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLR, uint32_t PLLDIVR)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM, Source | PLLM);
  MODIFY_REG(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIN | RCC_PLLSAICFGR_PLLSAIR, PLLN << RCC_PLLSAICFGR_PLLSAIN_Pos | PLLR);
  MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLSAIDIVR, PLLDIVR);
}
#endif /* LTDC */

/**
  * @brief  Get SAIPLL multiplication factor for VCO
  * @rmtoll PLLSAICFGR  PLLSAIN      LL_RCC_PLLSAI_GetN
  * @retval Between 50 and 432
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetN(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIN) >> RCC_PLLSAICFGR_PLLSAIN_Pos);
}

/**
  * @brief  Get SAIPLL division factor for PLLSAIQ
  * @rmtoll PLLSAICFGR  PLLSAIQ      LL_RCC_PLLSAI_GetQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIQ_DIV_15
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetQ(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIQ));
}

#if defined(RCC_PLLSAICFGR_PLLSAIR)
/**
  * @brief  Get SAIPLL division factor for PLLSAIR
  * @note used for PLLSAICLK (SAI clock)
  * @rmtoll PLLSAICFGR  PLLSAIR      LL_RCC_PLLSAI_GetR
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIR_DIV_3
  *         @arg @ref LL_RCC_PLLSAIR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIR_DIV_5
  *         @arg @ref LL_RCC_PLLSAIR_DIV_6
  *         @arg @ref LL_RCC_PLLSAIR_DIV_7
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetR(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIR));
}
#endif /* RCC_PLLSAICFGR_PLLSAIR */

/**
  * @brief  Get SAIPLL division factor for PLLSAIP
  * @note used for PLL48MCLK (48M domain clock)
  * @rmtoll PLLSAICFGR  PLLSAIP      LL_RCC_PLLSAI_GetP
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIP_DIV_2
  *         @arg @ref LL_RCC_PLLSAIP_DIV_4
  *         @arg @ref LL_RCC_PLLSAIP_DIV_6
  *         @arg @ref LL_RCC_PLLSAIP_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetP(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLSAICFGR, RCC_PLLSAICFGR_PLLSAIP));
}

/**
  * @brief  Get SAIPLL division factor for PLLSAIDIVQ
  * @note used PLLSAI1CLK, PLLSAI2CLK selected (SAI1 and SAI2 clock)
  * @rmtoll DCKCFGR1   PLLSAIDIVQ      LL_RCC_PLLSAI_GetDIVQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_1
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_3
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_5
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_6
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_7
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_9
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_10
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_11
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_12
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_13
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_14
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_15
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_16
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_17
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_18
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_19
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_20
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_21
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_22
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_23
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_24
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_25
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_26
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_27
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_28
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_29
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_30
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_31
  *         @arg @ref LL_RCC_PLLSAIDIVQ_DIV_32
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetDIVQ(void)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLSAIDIVQ));
}

#if defined(RCC_DCKCFGR1_PLLSAIDIVR)
/**
  * @brief  Get SAIPLL division factor for PLLSAIDIVR
  * @note used for LTDC domain clock
  * @rmtoll DCKCFGR1  PLLSAIDIVR      LL_RCC_PLLSAI_GetDIVR
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_2
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_4
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_8
  *         @arg @ref LL_RCC_PLLSAIDIVR_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_PLLSAI_GetDIVR(void)
{
  return (uint32_t)(READ_BIT(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLSAIDIVR));
}
#endif /* RCC_DCKCFGR1_PLLSAIDIVR */

/**
  * @}
  */

/** @defgroup RCC_LL_EF_FLAG_Management FLAG Management
  * @{
  */

/**
  * @brief  Clear LSI ready interrupt flag
  * @rmtoll CIR         LSIRDYC       LL_RCC_ClearFlag_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_LSIRDYC);
}

/**
  * @brief  Clear LSE ready interrupt flag
  * @rmtoll CIR         LSERDYC       LL_RCC_ClearFlag_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSERDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_LSERDYC);
}

/**
  * @brief  Clear HSI ready interrupt flag
  * @rmtoll CIR         HSIRDYC       LL_RCC_ClearFlag_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSIRDYC);
}

/**
  * @brief  Clear HSE ready interrupt flag
  * @rmtoll CIR         HSERDYC       LL_RCC_ClearFlag_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSERDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSERDYC);
}

/**
  * @brief  Clear PLL ready interrupt flag
  * @rmtoll CIR         PLLRDYC       LL_RCC_ClearFlag_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLLRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLRDYC);
}

/**
  * @brief  Clear PLLI2S ready interrupt flag
  * @rmtoll CIR         PLLI2SRDYC   LL_RCC_ClearFlag_PLLI2SRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLLI2SRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLI2SRDYC);
}

/**
  * @brief  Clear PLLSAI ready interrupt flag
  * @rmtoll CIR         PLLSAIRDYC   LL_RCC_ClearFlag_PLLSAIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLLSAIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLSAIRDYC);
}

/**
  * @brief  Clear Clock security system interrupt flag
  * @rmtoll CIR         CSSC          LL_RCC_ClearFlag_HSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSECSS(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_CSSC);
}

/**
  * @brief  Check if LSI ready interrupt occurred or not
  * @rmtoll CIR         LSIRDYF       LL_RCC_IsActiveFlag_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_LSIRDYF) == (RCC_CIR_LSIRDYF));
}

/**
  * @brief  Check if LSE ready interrupt occurred or not
  * @rmtoll CIR         LSERDYF       LL_RCC_IsActiveFlag_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSERDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_LSERDYF) == (RCC_CIR_LSERDYF));
}

/**
  * @brief  Check if HSI ready interrupt occurred or not
  * @rmtoll CIR         HSIRDYF       LL_RCC_IsActiveFlag_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSIRDYF) == (RCC_CIR_HSIRDYF));
}

/**
  * @brief  Check if HSE ready interrupt occurred or not
  * @rmtoll CIR         HSERDYF       LL_RCC_IsActiveFlag_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSERDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSERDYF) == (RCC_CIR_HSERDYF));
}

/**
  * @brief  Check if PLL ready interrupt occurred or not
  * @rmtoll CIR         PLLRDYF       LL_RCC_IsActiveFlag_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLLRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLRDYF) == (RCC_CIR_PLLRDYF));
}

/**
  * @brief  Check if PLLI2S ready interrupt occurred or not
  * @rmtoll CIR         PLLI2SRDYF   LL_RCC_IsActiveFlag_PLLI2SRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLLI2SRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLI2SRDYF) == (RCC_CIR_PLLI2SRDYF));
}

/**
  * @brief  Check if PLLSAI ready interrupt occurred or not
  * @rmtoll CIR         PLLSAIRDYF   LL_RCC_IsActiveFlag_PLLSAIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLLSAIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLSAIRDYF) == (RCC_CIR_PLLSAIRDYF));
}

/**
  * @brief  Check if Clock security system interrupt occurred or not
  * @rmtoll CIR         CSSF          LL_RCC_IsActiveFlag_HSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSECSS(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_CSSF) == (RCC_CIR_CSSF));
}

/**
  * @brief  Check if RCC flag Independent Watchdog reset is set or not.
  * @rmtoll CSR          IWDGRSTF      LL_RCC_IsActiveFlag_IWDGRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_IWDGRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_IWDGRSTF) == (RCC_CSR_IWDGRSTF));
}

/**
  * @brief  Check if RCC flag Low Power reset is set or not.
  * @rmtoll CSR          LPWRRSTF      LL_RCC_IsActiveFlag_LPWRRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LPWRRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_LPWRRSTF) == (RCC_CSR_LPWRRSTF));
}

/**
  * @brief  Check if RCC flag Pin reset is set or not.
  * @rmtoll CSR          PINRSTF       LL_RCC_IsActiveFlag_PINRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PINRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_PINRSTF) == (RCC_CSR_PINRSTF));
}

/**
  * @brief  Check if RCC flag POR/PDR reset is set or not.
  * @rmtoll CSR          PORRSTF       LL_RCC_IsActiveFlag_PORRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PORRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_PORRSTF) == (RCC_CSR_PORRSTF));
}

/**
  * @brief  Check if RCC flag Software reset is set or not.
  * @rmtoll CSR          SFTRSTF       LL_RCC_IsActiveFlag_SFTRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_SFTRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_SFTRSTF) == (RCC_CSR_SFTRSTF));
}

/**
  * @brief  Check if RCC flag Window Watchdog reset is set or not.
  * @rmtoll CSR          WWDGRSTF      LL_RCC_IsActiveFlag_WWDGRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_WWDGRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_WWDGRSTF) == (RCC_CSR_WWDGRSTF));
}

/**
  * @brief  Check if RCC flag BOR reset is set or not.
  * @rmtoll CSR          BORRSTF       LL_RCC_IsActiveFlag_BORRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_BORRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_BORRSTF) == (RCC_CSR_BORRSTF));
}

/**
  * @brief  Set RMVF bit to clear the reset flags.
  * @rmtoll CSR          RMVF          LL_RCC_ClearResetFlags
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearResetFlags(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_RMVF);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_IT_Management IT Management
  * @{
  */

/**
  * @brief  Enable LSI ready interrupt
  * @rmtoll CIR         LSIRDYIE      LL_RCC_EnableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_LSIRDYIE);
}

/**
  * @brief  Enable LSE ready interrupt
  * @rmtoll CIR         LSERDYIE      LL_RCC_EnableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSERDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_LSERDYIE);
}

/**
  * @brief  Enable HSI ready interrupt
  * @rmtoll CIR         HSIRDYIE      LL_RCC_EnableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSIRDYIE);
}

/**
  * @brief  Enable HSE ready interrupt
  * @rmtoll CIR         HSERDYIE      LL_RCC_EnableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSERDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSERDYIE);
}

/**
  * @brief  Enable PLL ready interrupt
  * @rmtoll CIR         PLLRDYIE      LL_RCC_EnableIT_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLLRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLRDYIE);
}

/**
  * @brief  Enable PLLI2S ready interrupt
  * @rmtoll CIR         PLLI2SRDYIE  LL_RCC_EnableIT_PLLI2SRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLLI2SRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLI2SRDYIE);
}

/**
  * @brief  Enable PLLSAI ready interrupt
  * @rmtoll CIR         PLLSAIRDYIE  LL_RCC_EnableIT_PLLSAIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLLSAIRDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_PLLSAIRDYIE);
}

/**
  * @brief  Disable LSI ready interrupt
  * @rmtoll CIR         LSIRDYIE      LL_RCC_DisableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSIRDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_LSIRDYIE);
}

/**
  * @brief  Disable LSE ready interrupt
  * @rmtoll CIR         LSERDYIE      LL_RCC_DisableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSERDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_LSERDYIE);
}

/**
  * @brief  Disable HSI ready interrupt
  * @rmtoll CIR         HSIRDYIE      LL_RCC_DisableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSIRDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_HSIRDYIE);
}

/**
  * @brief  Disable HSE ready interrupt
  * @rmtoll CIR         HSERDYIE      LL_RCC_DisableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSERDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_HSERDYIE);
}

/**
  * @brief  Disable PLL ready interrupt
  * @rmtoll CIR         PLLRDYIE      LL_RCC_DisableIT_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLLRDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_PLLRDYIE);
}

/**
  * @brief  Disable PLLI2S ready interrupt
  * @rmtoll CIR         PLLI2SRDYIE  LL_RCC_DisableIT_PLLI2SRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLLI2SRDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_PLLI2SRDYIE);
}

/**
  * @brief  Disable PLLSAI ready interrupt
  * @rmtoll CIR         PLLSAIRDYIE  LL_RCC_DisableIT_PLLSAIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLLSAIRDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_PLLSAIRDYIE);
}

/**
  * @brief  Checks if LSI ready interrupt source is enabled or disabled.
  * @rmtoll CIR         LSIRDYIE      LL_RCC_IsEnabledIT_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_LSIRDYIE) == (RCC_CIR_LSIRDYIE));
}

/**
  * @brief  Checks if LSE ready interrupt source is enabled or disabled.
  * @rmtoll CIR         LSERDYIE      LL_RCC_IsEnabledIT_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSERDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_LSERDYIE) == (RCC_CIR_LSERDYIE));
}

/**
  * @brief  Checks if HSI ready interrupt source is enabled or disabled.
  * @rmtoll CIR         HSIRDYIE      LL_RCC_IsEnabledIT_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSIRDYIE) == (RCC_CIR_HSIRDYIE));
}

/**
  * @brief  Checks if HSE ready interrupt source is enabled or disabled.
  * @rmtoll CIR         HSERDYIE      LL_RCC_IsEnabledIT_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSERDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSERDYIE) == (RCC_CIR_HSERDYIE));
}

/**
  * @brief  Checks if PLL ready interrupt source is enabled or disabled.
  * @rmtoll CIR         PLLRDYIE      LL_RCC_IsEnabledIT_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLLRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLRDYIE) == (RCC_CIR_PLLRDYIE));
}

/**
  * @brief  Checks if PLLI2S ready interrupt source is enabled or disabled.
  * @rmtoll CIR         PLLI2SRDYIE  LL_RCC_IsEnabledIT_PLLI2SRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLLI2SRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLI2SRDYIE) == (RCC_CIR_PLLI2SRDYIE));
}

/**
  * @brief  Checks if PLLSAI ready interrupt source is enabled or disabled.
  * @rmtoll CIR         PLLSAIRDYIE  LL_RCC_IsEnabledIT_PLLSAIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLLSAIRDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_PLLSAIRDYIE) == (RCC_CIR_PLLSAIRDYIE));
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EF_Init De-initialization function
  * @{
  */
ErrorStatus LL_RCC_DeInit(void);
/**
  * @}
  */

/** @defgroup RCC_LL_EF_Get_Freq Get system and peripherals clocks frequency functions
  * @{
  */
void        LL_RCC_GetSystemClocksFreq(LL_RCC_ClocksTypeDef *RCC_Clocks);
uint32_t    LL_RCC_GetUSARTClockFreq(uint32_t USARTxSource);
uint32_t    LL_RCC_GetUARTClockFreq(uint32_t UARTxSource);
uint32_t    LL_RCC_GetI2CClockFreq(uint32_t I2CxSource);
uint32_t    LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource);
uint32_t    LL_RCC_GetSAIClockFreq(uint32_t SAIxSource);
uint32_t    LL_RCC_GetSDMMCClockFreq(uint32_t SDMMCxSource);
uint32_t    LL_RCC_GetRNGClockFreq(uint32_t RNGxSource);
uint32_t    LL_RCC_GetUSBClockFreq(uint32_t USBxSource);
#if defined(DFSDM1_Channel0)
uint32_t    LL_RCC_GetDFSDMClockFreq(uint32_t DFSDMxSource);
uint32_t    LL_RCC_GetDFSDMAudioClockFreq(uint32_t DFSDMxSource);
#endif /* DFSDM1_Channel0 */
uint32_t    LL_RCC_GetI2SClockFreq(uint32_t I2SxSource);
#if defined(CEC)
uint32_t    LL_RCC_GetCECClockFreq(uint32_t CECxSource);
#endif /* CEC */
#if defined(LTDC)
uint32_t    LL_RCC_GetLTDCClockFreq(uint32_t LTDCxSource);
#endif /* LTDC */
#if defined(SPDIFRX)
uint32_t    LL_RCC_GetSPDIFRXClockFreq(uint32_t SPDIFRXxSource);
#endif /* SPDIFRX */
#if defined(DSI)
uint32_t    LL_RCC_GetDSIClockFreq(uint32_t DSIxSource);
#endif /* DSI */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(RCC) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F7xx_LL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
