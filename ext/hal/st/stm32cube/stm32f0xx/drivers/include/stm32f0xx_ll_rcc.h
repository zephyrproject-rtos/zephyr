/**
  ******************************************************************************
  * @file    stm32f0xx_ll_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC LL module.
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
#ifndef __STM32F0xx_LL_RCC_H
#define __STM32F0xx_LL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/** @addtogroup STM32F0xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @defgroup RCC_LL RCC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup RCC_LL_Private_Constants RCC Private Constants
  * @{
  */
/* Defines used for the bit position in the register and perform offsets*/
#define RCC_POSITION_HPRE       (uint32_t)4U  /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PPRE1      (uint32_t)8U  /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PLLMUL     (uint32_t)18U /*!< field position in register RCC_CFGR */
#define RCC_POSITION_HSICAL     (uint32_t)8U  /*!< field position in register RCC_CR */
#define RCC_POSITION_HSITRIM    (uint32_t)3U  /*!< field position in register RCC_CR */
#define RCC_POSITION_HSI14TRIM  (uint32_t)3U  /*!< field position in register RCC_CR2 */
#define RCC_POSITION_HSI14CAL   (uint32_t)8U  /*!< field position in register RCC_CR2 */
#if defined(RCC_HSI48_SUPPORT)
#define RCC_POSITION_HSI48CAL   (uint32_t)24U /*!< field position in register RCC_CR2 */
#endif /* RCC_HSI48_SUPPORT */
#define RCC_POSITION_USART1SW   (uint32_t)0U  /*!< field position in register RCC_CFGR3 */
#define RCC_POSITION_USART2SW   (uint32_t)16U /*!< field position in register RCC_CFGR3 */
#define RCC_POSITION_USART3SW   (uint32_t)18U /*!< field position in register RCC_CFGR3 */

/**
  * @}
  */

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
#define HSE_VALUE    8000000U  /*!< Value of the HSE oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    8000000U  /*!< Value of the HSI oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE    32768U    /*!< Value of the LSE oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE    32000U    /*!< Value of the LSI oscillator in Hz */
#endif /* LSI_VALUE */
#if defined(RCC_HSI48_SUPPORT)

#if !defined  (HSI48_VALUE)
#define HSI48_VALUE  48000000U /*!< Value of the HSI48 oscillator in Hz */
#endif /* HSI48_VALUE */
#endif /* RCC_HSI48_SUPPORT */
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
#define LL_RCC_CIR_HSI14RDYC               RCC_CIR_HSI14RDYC  /*!< HSI14 Ready Interrupt Clear */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CIR_HSI48RDYC               RCC_CIR_HSI48RDYC  /*!< HSI48 Ready Interrupt Clear */
#endif /* RCC_HSI48_SUPPORT */
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
#define LL_RCC_CIR_HSI14RDYF               RCC_CIR_HSI14RDYF  /*!< HSI14 Ready Interrupt flag */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CIR_HSI48RDYF               RCC_CIR_HSI48RDYF  /*!< HSI48 Ready Interrupt flag */
#endif /* RCC_HSI48_SUPPORT */
#define LL_RCC_CIR_CSSF                   RCC_CIR_CSSF       /*!< Clock Security System Interrupt flag */
#define LL_RCC_CSR_OBLRSTF                RCC_CSR_OBLRSTF         /*!< OBL reset flag */
#define LL_RCC_CSR_PINRSTF                RCC_CSR_PINRSTF         /*!< PIN reset flag */
#define LL_RCC_CSR_PORRSTF                RCC_CSR_PORRSTF         /*!< POR/PDR reset flag */
#define LL_RCC_CSR_SFTRSTF                RCC_CSR_SFTRSTF         /*!< Software Reset flag */
#define LL_RCC_CSR_IWDGRSTF               RCC_CSR_IWDGRSTF        /*!< Independent Watchdog reset flag */
#define LL_RCC_CSR_WWDGRSTF               RCC_CSR_WWDGRSTF        /*!< Window watchdog reset flag */
#define LL_RCC_CSR_LPWRRSTF               RCC_CSR_LPWRRSTF        /*!< Low-Power reset flag */
#if defined(RCC_CSR_V18PWRRSTF)
#define LL_RCC_CSR_V18PWRRSTF             RCC_CSR_V18PWRRSTF      /*!< Reset flag of the 1.8 V domain. */
#endif /* RCC_CSR_V18PWRRSTF */
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
#define LL_RCC_CIR_HSI14RDYIE              RCC_CIR_HSI14RDYIE   /*!< HSI14 Ready Interrupt Enable */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CIR_HSI48RDYIE              RCC_CIR_HSI48RDYIE   /*!< HSI48 Ready Interrupt Enable */
#endif /* RCC_HSI48_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSEDRIVE  LSE oscillator drive capability
  * @{
  */
#define LL_RCC_LSEDRIVE_LOW                ((uint32_t)0x00000000U) /*!< Xtal mode lower driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMLOW          RCC_BDCR_LSEDRV_1 /*!< Xtal mode medium low driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMHIGH         RCC_BDCR_LSEDRV_0 /*!< Xtal mode medium high driving capability */
#define LL_RCC_LSEDRIVE_HIGH               RCC_BDCR_LSEDRV   /*!< Xtal mode higher driving capability */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE  System clock switch
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_HSI           RCC_CFGR_SW_HSI    /*!< HSI selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_HSE           RCC_CFGR_SW_HSE    /*!< HSE selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_PLL           RCC_CFGR_SW_PLL    /*!< PLL selection as system clock */
#if defined(RCC_CFGR_SW_HSI48)
#define LL_RCC_SYS_CLKSOURCE_HSI48         RCC_CFGR_SW_HSI48  /*!< HSI48 selection as system clock */
#endif /* RCC_CFGR_SW_HSI48 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE_STATUS  System clock switch status
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSI    RCC_CFGR_SWS_HSI   /*!< HSI used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSE    RCC_CFGR_SWS_HSE   /*!< HSE used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_PLL    RCC_CFGR_SWS_PLL   /*!< PLL used as system clock */
#if defined(RCC_CFGR_SWS_HSI48)
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSI48  RCC_CFGR_SWS_HSI48 /*!< HSI48 used as system clock */
#endif /* RCC_CFGR_SWS_HSI48 */
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
#define LL_RCC_APB1_DIV_1                  RCC_CFGR_PPRE_DIV1  /*!< HCLK not divided */
#define LL_RCC_APB1_DIV_2                  RCC_CFGR_PPRE_DIV2  /*!< HCLK divided by 2 */
#define LL_RCC_APB1_DIV_4                  RCC_CFGR_PPRE_DIV4  /*!< HCLK divided by 4 */
#define LL_RCC_APB1_DIV_8                  RCC_CFGR_PPRE_DIV8  /*!< HCLK divided by 8 */
#define LL_RCC_APB1_DIV_16                 RCC_CFGR_PPRE_DIV16 /*!< HCLK divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1SOURCE  MCO1 SOURCE selection
  * @{
  */
#define LL_RCC_MCO1SOURCE_NOCLOCK          RCC_CFGR_MCOSEL_NOCLOCK      /*!< MCO output disabled, no clock on MCO */
#define LL_RCC_MCO1SOURCE_HSI14            RCC_CFGR_MCOSEL_HSI14        /*!< HSI14 oscillator clock selected */
#define LL_RCC_MCO1SOURCE_SYSCLK           RCC_CFGR_MCOSEL_SYSCLK       /*!< SYSCLK selection as MCO source */
#define LL_RCC_MCO1SOURCE_HSI              RCC_CFGR_MCOSEL_HSI          /*!< HSI selection as MCO source */
#define LL_RCC_MCO1SOURCE_HSE              RCC_CFGR_MCOSEL_HSE          /*!< HSE selection as MCO source */
#define LL_RCC_MCO1SOURCE_LSI              RCC_CFGR_MCOSEL_LSI          /*!< LSI selection as MCO source */
#define LL_RCC_MCO1SOURCE_LSE              RCC_CFGR_MCOSEL_LSE          /*!< LSE selection as MCO source */
#if defined(RCC_CFGR_MCOSEL_HSI48)
#define LL_RCC_MCO1SOURCE_HSI48            RCC_CFGR_MCOSEL_HSI48        /*!< HSI48 selection as MCO source */
#endif /* RCC_CFGR_MCOSEL_HSI48 */
#define LL_RCC_MCO1SOURCE_PLLCLK_DIV_2     RCC_CFGR_MCOSEL_PLL_DIV2     /*!< PLL clock divided by 2*/
#if defined(RCC_CFGR_PLLNODIV)
#define LL_RCC_MCO1SOURCE_PLLCLK           (RCC_CFGR_MCOSEL_PLL_DIV2 | RCC_CFGR_PLLNODIV) /*!< PLL clock selected*/
#endif /* RCC_CFGR_PLLNODIV */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1_DIV  MCO1 prescaler
  * @{
  */
#define LL_RCC_MCO1_DIV_1                  ((uint32_t)0x00000000U)/*!< MCO Clock divided by 1 */
#if defined(RCC_CFGR_MCOPRE)
#define LL_RCC_MCO1_DIV_2                  RCC_CFGR_MCOPRE_DIV2   /*!< MCO Clock divided by 2 */
#define LL_RCC_MCO1_DIV_4                  RCC_CFGR_MCOPRE_DIV4   /*!< MCO Clock divided by 4 */
#define LL_RCC_MCO1_DIV_8                  RCC_CFGR_MCOPRE_DIV8   /*!< MCO Clock divided by 8 */
#define LL_RCC_MCO1_DIV_16                 RCC_CFGR_MCOPRE_DIV16  /*!< MCO Clock divided by 16 */
#define LL_RCC_MCO1_DIV_32                 RCC_CFGR_MCOPRE_DIV32  /*!< MCO Clock divided by 32 */
#define LL_RCC_MCO1_DIV_64                 RCC_CFGR_MCOPRE_DIV64  /*!< MCO Clock divided by 64 */
#define LL_RCC_MCO1_DIV_128                RCC_CFGR_MCOPRE_DIV128 /*!< MCO Clock divided by 128 */
#endif /* RCC_CFGR_MCOPRE */
/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EC_PERIPH_FREQUENCY Peripheral clock frequency
  * @{
  */
#define LL_RCC_PERIPH_FREQUENCY_NO         0x00000000U      /*!< No clock enabled for the peripheral            */
#define LL_RCC_PERIPH_FREQUENCY_NA         0xFFFFFFFFU      /*!< Frequency cannot be provided as external clock */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/** @defgroup RCC_LL_EC_USART1_CLKSOURCE Peripheral USART clock source selection
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE_PCLK1      (uint32_t)((RCC_POSITION_USART1SW << 24) | RCC_CFGR3_USART1SW_PCLK)   /*!< PCLK1 clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_SYSCLK     (uint32_t)((RCC_POSITION_USART1SW << 24) | RCC_CFGR3_USART1SW_SYSCLK) /*!< System clock selected as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_LSE        (uint32_t)((RCC_POSITION_USART1SW << 24) | RCC_CFGR3_USART1SW_LSE)    /*!< LSE oscillator clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_HSI        (uint32_t)((RCC_POSITION_USART1SW << 24) | RCC_CFGR3_USART1SW_HSI)    /*!< HSI oscillator clock used as USART1 clock source */
#if defined(RCC_CFGR3_USART2SW)
#define LL_RCC_USART2_CLKSOURCE_PCLK1      (uint32_t)((RCC_POSITION_USART2SW << 24) | RCC_CFGR3_USART2SW_PCLK)   /*!< PCLK1 clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_SYSCLK     (uint32_t)((RCC_POSITION_USART2SW << 24) | RCC_CFGR3_USART2SW_SYSCLK) /*!< System clock selected as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_LSE        (uint32_t)((RCC_POSITION_USART2SW << 24) | RCC_CFGR3_USART2SW_LSE)    /*!< LSE oscillator clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_HSI        (uint32_t)((RCC_POSITION_USART2SW << 24) | RCC_CFGR3_USART2SW_HSI)    /*!< HSI oscillator clock used as USART2 clock source */
#endif /* RCC_CFGR3_USART2SW */
#if defined(RCC_CFGR3_USART3SW)
#define LL_RCC_USART3_CLKSOURCE_PCLK1      (uint32_t)((RCC_POSITION_USART3SW << 24) | RCC_CFGR3_USART3SW_PCLK)   /*!< PCLK1 clock used as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_SYSCLK     (uint32_t)((RCC_POSITION_USART3SW << 24) | RCC_CFGR3_USART3SW_SYSCLK) /*!< System clock selected as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_LSE        (uint32_t)((RCC_POSITION_USART3SW << 24) | RCC_CFGR3_USART3SW_LSE)    /*!< LSE oscillator clock used as USART3 clock source */
#define LL_RCC_USART3_CLKSOURCE_HSI        (uint32_t)((RCC_POSITION_USART3SW << 24) | RCC_CFGR3_USART3SW_HSI)    /*!< HSI oscillator clock used as USART3 clock source */
#endif /* RCC_CFGR3_USART3SW */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2C1_CLKSOURCE Peripheral I2C clock source selection
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE_HSI          RCC_CFGR3_I2C1SW_HSI    /*!< HSI oscillator clock used as I2C1 clock source */
#define LL_RCC_I2C1_CLKSOURCE_SYSCLK       RCC_CFGR3_I2C1SW_SYSCLK /*!< System clock selected as I2C1 clock source */
/**
  * @}
  */

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC_CLKSOURCE Peripheral CEC clock source selection
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE_HSI_DIV244    RCC_CFGR3_CECSW_HSI_DIV244 /*!< HSI clock divided by 244 selected as HDMI CEC entry clock source */
#define LL_RCC_CEC_CLKSOURCE_LSE           RCC_CFGR3_CECSW_LSE        /*!< LSE clock selected as HDMI CEC entry clock source */
/**
  * @}
  */

#endif /* CEC */

#if defined(USB)
/** @defgroup RCC_LL_EC_USB_CLKSOURCE Peripheral USB clock source selection
  * @{
  */
#if defined(RCC_CFGR3_USBSW_HSI48)
#define LL_RCC_USB_CLKSOURCE_HSI48         RCC_CFGR3_USBSW_HSI48   /*!< HSI48 oscillator clock used as USB clock source */
#else
#define LL_RCC_USB_CLKSOURCE_NONE          ((uint32_t)0x00000000)  /*!< USB Clock disabled */
#endif /*RCC_CFGR3_USBSW_HSI48*/
#define LL_RCC_USB_CLKSOURCE_PLL           RCC_CFGR3_USBSW_PLLCLK  /*!< PLL selected as USB clock source */
/**
  * @}
  */

#endif /* USB */

/** @defgroup RCC_LL_EC_USART1 Peripheral USART get clock source
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE            RCC_POSITION_USART1SW /*!< USART1 Clock source selection */
#if defined(RCC_CFGR3_USART2SW)
#define LL_RCC_USART2_CLKSOURCE            RCC_POSITION_USART2SW /*!< USART2 Clock source selection */
#endif /* RCC_CFGR3_USART2SW */
#if defined(RCC_CFGR3_USART3SW)
#define LL_RCC_USART3_CLKSOURCE            RCC_POSITION_USART3SW /*!< USART3 Clock source selection */
#endif /* RCC_CFGR3_USART3SW */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2C1 Peripheral I2C get clock source
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE              RCC_CFGR3_I2C1SW     /*!< I2C1 Clock source selection */
/**
  * @}
  */

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC Peripheral CEC get clock source
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE               RCC_CFGR3_CECSW            /*!< CEC Clock source selection */
/**
  * @}
  */
#endif /* CEC */

#if defined(USB)
/** @defgroup RCC_LL_EC_USB Peripheral USB get clock source
  * @{
  */
#define LL_RCC_USB_CLKSOURCE               RCC_CFGR3_USBSW         /*!< USB Clock source selection */
/**
  * @}
  */
#endif /* USB */

/** @defgroup RCC_LL_EC_RTC_CLKSOURCE  RTC clock source selection
  * @{
  */
#define LL_RCC_RTC_CLKSOURCE_NONE          0x00000000U                   /*!< No clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSE           RCC_BDCR_RTCSEL_0       /*!< LSE oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSI           RCC_BDCR_RTCSEL_1       /*!< LSI oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_HSE_DIV32     RCC_BDCR_RTCSEL         /*!< HSE oscillator clock divided by 32 used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL_MUL PLL Multiplicator factor
  * @{
  */
#define LL_RCC_PLL_MUL_2                   RCC_CFGR_PLLMUL2  /*!< PLL input clock*2 */
#define LL_RCC_PLL_MUL_3                   RCC_CFGR_PLLMUL3  /*!< PLL input clock*3 */
#define LL_RCC_PLL_MUL_4                   RCC_CFGR_PLLMUL4  /*!< PLL input clock*4 */
#define LL_RCC_PLL_MUL_5                   RCC_CFGR_PLLMUL5  /*!< PLL input clock*5 */
#define LL_RCC_PLL_MUL_6                   RCC_CFGR_PLLMUL6  /*!< PLL input clock*6 */
#define LL_RCC_PLL_MUL_7                   RCC_CFGR_PLLMUL7  /*!< PLL input clock*7 */
#define LL_RCC_PLL_MUL_8                   RCC_CFGR_PLLMUL8  /*!< PLL input clock*8 */
#define LL_RCC_PLL_MUL_9                   RCC_CFGR_PLLMUL9  /*!< PLL input clock*9 */
#define LL_RCC_PLL_MUL_10                  RCC_CFGR_PLLMUL10  /*!< PLL input clock*10 */
#define LL_RCC_PLL_MUL_11                  RCC_CFGR_PLLMUL11  /*!< PLL input clock*11 */
#define LL_RCC_PLL_MUL_12                  RCC_CFGR_PLLMUL12  /*!< PLL input clock*12 */
#define LL_RCC_PLL_MUL_13                  RCC_CFGR_PLLMUL13  /*!< PLL input clock*13 */
#define LL_RCC_PLL_MUL_14                  RCC_CFGR_PLLMUL14  /*!< PLL input clock*14 */
#define LL_RCC_PLL_MUL_15                  RCC_CFGR_PLLMUL15  /*!< PLL input clock*15 */
#define LL_RCC_PLL_MUL_16                  RCC_CFGR_PLLMUL16  /*!< PLL input clock*16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLSOURCE PLL SOURCE
  * @{
  */
#define LL_RCC_PLLSOURCE_HSE               RCC_CFGR_PLLSRC_HSE_PREDIV                    /*!< HSE/PREDIV clock selected as PLL entry clock source */
#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
#define LL_RCC_PLLSOURCE_HSI               RCC_CFGR_PLLSRC_HSI_PREDIV                    /*!< HSI/PREDIV clock selected as PLL entry clock source */
#if defined(RCC_CFGR_SW_HSI48)
#define LL_RCC_PLLSOURCE_HSI48             RCC_CFGR_PLLSRC_HSI48_PREDIV                  /*!< HSI48/PREDIV clock selected as PLL entry clock source */
#endif /* RCC_CFGR_SW_HSI48 */
#else
#define LL_RCC_PLLSOURCE_HSI_DIV_2         RCC_CFGR_PLLSRC_HSI_DIV2                      /*!< HSI clock divided by 2 selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_1         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV1)    /*!< HSE clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_2         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV2)    /*!< HSE/2 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_3         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV3)    /*!< HSE/3 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_4         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV4)    /*!< HSE/4 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_5         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV5)    /*!< HSE/5 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_6         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV6)    /*!< HSE/6 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_7         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV7)    /*!< HSE/7 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_8         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV8)    /*!< HSE/8 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_9         (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV9)    /*!< HSE/9 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_10        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV10)   /*!< HSE/10 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_11        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV11)   /*!< HSE/11 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_12        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV12)   /*!< HSE/12 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_13        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV13)   /*!< HSE/13 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_14        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV14)   /*!< HSE/14 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_15        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV15)   /*!< HSE/15 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE_DIV_16        (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR2_PREDIV_DIV16)   /*!< HSE/16 clock selected as PLL entry clock source */
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PREDIV_DIV PREDIV Division factor
  * @{
  */
#define LL_RCC_PREDIV_DIV_1                RCC_CFGR2_PREDIV_DIV1   /*!< PREDIV input clock not divided */
#define LL_RCC_PREDIV_DIV_2                RCC_CFGR2_PREDIV_DIV2   /*!< PREDIV input clock divided by 2 */
#define LL_RCC_PREDIV_DIV_3                RCC_CFGR2_PREDIV_DIV3   /*!< PREDIV input clock divided by 3 */
#define LL_RCC_PREDIV_DIV_4                RCC_CFGR2_PREDIV_DIV4   /*!< PREDIV input clock divided by 4 */
#define LL_RCC_PREDIV_DIV_5                RCC_CFGR2_PREDIV_DIV5   /*!< PREDIV input clock divided by 5 */
#define LL_RCC_PREDIV_DIV_6                RCC_CFGR2_PREDIV_DIV6   /*!< PREDIV input clock divided by 6 */
#define LL_RCC_PREDIV_DIV_7                RCC_CFGR2_PREDIV_DIV7   /*!< PREDIV input clock divided by 7 */
#define LL_RCC_PREDIV_DIV_8                RCC_CFGR2_PREDIV_DIV8   /*!< PREDIV input clock divided by 8 */
#define LL_RCC_PREDIV_DIV_9                RCC_CFGR2_PREDIV_DIV9   /*!< PREDIV input clock divided by 9 */
#define LL_RCC_PREDIV_DIV_10               RCC_CFGR2_PREDIV_DIV10  /*!< PREDIV input clock divided by 10 */
#define LL_RCC_PREDIV_DIV_11               RCC_CFGR2_PREDIV_DIV11  /*!< PREDIV input clock divided by 11 */
#define LL_RCC_PREDIV_DIV_12               RCC_CFGR2_PREDIV_DIV12  /*!< PREDIV input clock divided by 12 */
#define LL_RCC_PREDIV_DIV_13               RCC_CFGR2_PREDIV_DIV13  /*!< PREDIV input clock divided by 13 */
#define LL_RCC_PREDIV_DIV_14               RCC_CFGR2_PREDIV_DIV14  /*!< PREDIV input clock divided by 14 */
#define LL_RCC_PREDIV_DIV_15               RCC_CFGR2_PREDIV_DIV15  /*!< PREDIV input clock divided by 15 */
#define LL_RCC_PREDIV_DIV_16               RCC_CFGR2_PREDIV_DIV16  /*!< PREDIV input clock divided by 16 */
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

#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
/**
  * @brief  Helper macro to calculate the PLLCLK frequency
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_FREQ (HSE_VALUE, @ref LL_RCC_PLL_GetMultiplicator()
  *             , @ref LL_RCC_PLL_GetPrediv());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI/HSI48)
  * @param  __PLLMUL__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_2
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_5
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_7
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_9
  *         @arg @ref LL_RCC_PLL_MUL_10
  *         @arg @ref LL_RCC_PLL_MUL_11
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_13
  *         @arg @ref LL_RCC_PLL_MUL_14
  *         @arg @ref LL_RCC_PLL_MUL_15
  *         @arg @ref LL_RCC_PLL_MUL_16
  * @param  __PLLPREDIV__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PREDIV_DIV_1
  *         @arg @ref LL_RCC_PREDIV_DIV_2
  *         @arg @ref LL_RCC_PREDIV_DIV_3
  *         @arg @ref LL_RCC_PREDIV_DIV_4
  *         @arg @ref LL_RCC_PREDIV_DIV_5
  *         @arg @ref LL_RCC_PREDIV_DIV_6
  *         @arg @ref LL_RCC_PREDIV_DIV_7
  *         @arg @ref LL_RCC_PREDIV_DIV_8
  *         @arg @ref LL_RCC_PREDIV_DIV_9
  *         @arg @ref LL_RCC_PREDIV_DIV_10
  *         @arg @ref LL_RCC_PREDIV_DIV_11
  *         @arg @ref LL_RCC_PREDIV_DIV_12
  *         @arg @ref LL_RCC_PREDIV_DIV_13
  *         @arg @ref LL_RCC_PREDIV_DIV_14
  *         @arg @ref LL_RCC_PREDIV_DIV_15
  *         @arg @ref LL_RCC_PREDIV_DIV_16
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_FREQ(__INPUTFREQ__, __PLLMUL__, __PLLPREDIV__) \
          (((__INPUTFREQ__) / ((((__PLLPREDIV__) & RCC_CFGR2_PREDIV) + 1U))) * ((((__PLLMUL__) & RCC_CFGR_PLLMUL) >> RCC_POSITION_PLLMUL) + 2U))

#else
/**
  * @brief  Helper macro to calculate the PLLCLK frequency
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_FREQ (HSE_VALUE / (@ref LL_RCC_PLL_GetPrediv () + 1), @ref LL_RCC_PLL_GetMultiplicator());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE div Prediv / HSI div 2)
  * @param  __PLLMUL__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_2
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_5
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_7
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_9
  *         @arg @ref LL_RCC_PLL_MUL_10
  *         @arg @ref LL_RCC_PLL_MUL_11
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_13
  *         @arg @ref LL_RCC_PLL_MUL_14
  *         @arg @ref LL_RCC_PLL_MUL_15
  *         @arg @ref LL_RCC_PLL_MUL_16
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_FREQ(__INPUTFREQ__, __PLLMUL__) \
          ((__INPUTFREQ__) * ((((__PLLMUL__) & RCC_CFGR_PLLMUL) >> RCC_POSITION_PLLMUL) + 2U))
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
/**
  * @brief  Helper macro to calculate the HCLK frequency
  * @note: __AHBPRESCALER__ be retrieved by @ref LL_RCC_GetAHBPrescaler
  *        ex: __LL_RCC_CALC_HCLK_FREQ(LL_RCC_GetAHBPrescaler())
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
  * @note: __APB1PRESCALER__ be retrieved by @ref LL_RCC_GetAPB1Prescaler
  *        ex: __LL_RCC_CALC_PCLK1_FREQ(LL_RCC_GetAPB1Prescaler())
  * @param  __HCLKFREQ__ HCLK frequency
  * @param  __APB1PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  * @retval PCLK1 clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PCLK1_FREQ(__HCLKFREQ__, __APB1PRESCALER__) ((__HCLKFREQ__) >> APBPrescTable[(__APB1PRESCALER__) >>  RCC_CFGR_PPRE_Pos])

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
  * @brief  Disable the Clock Security System.
  * @note Cannot be disabled in HSE is ready (only by hardware)
  * @rmtoll CR           CSSON         LL_RCC_HSE_DisableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_DisableCSS(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_CSSON);
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
  * @param  Value between Min_Data = 0x00 and Max_Data = 0x1F
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->CR, RCC_CR_HSITRIM, Value << RCC_CR_HSITRIM_Pos);
}

/**
  * @brief  Get HSI Calibration trimming
  * @rmtoll CR        HSITRIM       LL_RCC_HSI_GetCalibTrimming
  * @retval Between Min_Data = 0x00 and Max_Data = 0x1F
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->CR, RCC_CR_HSITRIM) >> RCC_CR_HSITRIM_Pos);
}

/**
  * @}
  */

#if defined(RCC_HSI48_SUPPORT)
/** @defgroup RCC_LL_EF_HSI48 HSI48
  * @{
  */

/**
  * @brief  Enable HSI48
  * @rmtoll CR2          HSI48ON       LL_RCC_HSI48_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_Enable(void)
{
  SET_BIT(RCC->CR2, RCC_CR2_HSI48ON);
}

/**
  * @brief  Disable HSI48
  * @rmtoll CR2          HSI48ON       LL_RCC_HSI48_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_Disable(void)
{
  CLEAR_BIT(RCC->CR2, RCC_CR2_HSI48ON);
}

/**
  * @brief  Check if HSI48 oscillator Ready
  * @rmtoll CR2          HSI48RDY      LL_RCC_HSI48_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI48_IsReady(void)
{
  return (READ_BIT(RCC->CR2, RCC_CR2_HSI48RDY) == (RCC_CR2_HSI48RDY));
}

/**
  * @brief  Get HSI48 Calibration value
  * @rmtoll CR2          HSI48CAL      LL_RCC_HSI48_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI48_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->CR2, RCC_CR2_HSI48CAL) >> RCC_POSITION_HSI48CAL);
}

/**
  * @}
  */

#endif /* RCC_HSI48_SUPPORT */

/** @defgroup RCC_LL_EF_HSI14 HSI14
  * @{
  */

/**
  * @brief  Enable HSI14
  * @rmtoll CR2          HSI14ON       LL_RCC_HSI14_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI14_Enable(void)
{
  SET_BIT(RCC->CR2, RCC_CR2_HSI14ON);
}

/**
  * @brief  Disable HSI14
  * @rmtoll CR2          HSI14ON       LL_RCC_HSI14_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI14_Disable(void)
{
  CLEAR_BIT(RCC->CR2, RCC_CR2_HSI14ON);
}

/**
  * @brief  Check if HSI14 oscillator Ready
  * @rmtoll CR2          HSI14RDY      LL_RCC_HSI14_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI14_IsReady(void)
{
  return (READ_BIT(RCC->CR2, RCC_CR2_HSI14RDY) == (RCC_CR2_HSI14RDY));
}

/**
  * @brief  ADC interface can turn on the HSI14 oscillator
  * @rmtoll CR2          HSI14DIS      LL_RCC_HSI14_EnableADCControl
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI14_EnableADCControl(void)
{
  CLEAR_BIT(RCC->CR2, RCC_CR2_HSI14DIS);
}

/**
  * @brief  ADC interface can not turn on the HSI14 oscillator
  * @rmtoll CR2          HSI14DIS      LL_RCC_HSI14_DisableADCControl
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI14_DisableADCControl(void)
{
  SET_BIT(RCC->CR2, RCC_CR2_HSI14DIS);
}

/**
  * @brief  Set HSI14 Calibration trimming
  * @note user-programmable trimming value that is added to the HSI14CAL
  * @note Default value is 16, which, when added to the HSI14CAL value,
  *       should trim the HSI14 to 14 MHz +/- 1 %
  * @rmtoll CR2          HSI14TRIM     LL_RCC_HSI14_SetCalibTrimming
  * @param  Value between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI14_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->CR2, RCC_CR2_HSI14TRIM, Value << RCC_POSITION_HSI14TRIM);
}

/**
  * @brief  Get HSI14 Calibration value
  * @note When HSI14TRIM is written, HSI14CAL is updated with the sum of
  *       HSI14TRIM and the factory trim value
  * @rmtoll CR2          HSI14TRIM     LL_RCC_HSI14_GetCalibTrimming
  * @retval Between Min_Data = 0x00 and Max_Data = 0x1F
  */
__STATIC_INLINE uint32_t LL_RCC_HSI14_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->CR2, RCC_CR2_HSI14TRIM) >> RCC_POSITION_HSI14TRIM);
}

/**
  * @brief  Get HSI14 Calibration trimming
  * @rmtoll CR2          HSI14CAL      LL_RCC_HSI14_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0x1F
  */
__STATIC_INLINE uint32_t LL_RCC_HSI14_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->CR2, RCC_CR2_HSI14CAL) >> RCC_POSITION_HSI14CAL);
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
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
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
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_HSI48 (*)
  *
  *         (*) value not defined in all devices
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_HSI48 (*)
  *
  *         (*) value not defined in all devices
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
  * @rmtoll CFGR         PPRE         LL_RCC_SetAPB1Prescaler
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
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE, Prescaler);
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
  * @rmtoll CFGR         PPRE         LL_RCC_GetAPB1Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB1Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PPRE));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_MCO MCO
  * @{
  */

/**
  * @brief  Configure MCOx
  * @rmtoll CFGR         MCO           LL_RCC_ConfigMCO\n
  *         CFGR         MCOPRE        LL_RCC_ConfigMCO\n
  *         CFGR         PLLNODIV      LL_RCC_ConfigMCO
  * @param  MCOxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1SOURCE_NOCLOCK
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI14
  *         @arg @ref LL_RCC_MCO1SOURCE_SYSCLK
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI48 (*)
  *         @arg @ref LL_RCC_MCO1SOURCE_PLLCLK (*)
  *         @arg @ref LL_RCC_MCO1SOURCE_PLLCLK_DIV_2
  *
  *         (*) value not defined in all devices
  * @param  MCOxPrescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1_DIV_1
  *         @arg @ref LL_RCC_MCO1_DIV_2 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_4 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_8 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_16 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_32 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_64 (*)
  *         @arg @ref LL_RCC_MCO1_DIV_128 (*)
  *
  *         (*) value not defined in all devices
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ConfigMCO(uint32_t MCOxSource, uint32_t MCOxPrescaler)
{
#if defined(RCC_CFGR_MCOPRE)
#if defined(RCC_CFGR_PLLNODIV)
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE | RCC_CFGR_PLLNODIV, MCOxSource | MCOxPrescaler);
#else
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE, MCOxSource | MCOxPrescaler);
#endif /* RCC_CFGR_PLLNODIV */
#else
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL, MCOxSource);
#endif /* RCC_CFGR_MCOPRE */
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_Peripheral_Clock_Source Peripheral Clock Source
  * @{
  */

/**
  * @brief  Configure USARTx clock source
  * @rmtoll CFGR3        USART1SW      LL_RCC_SetUSARTClockSource\n
  *         CFGR3        USART2SW      LL_RCC_SetUSARTClockSource\n
  *         CFGR3        USART3SW      LL_RCC_SetUSARTClockSource
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_HSI (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSARTClockSource(uint32_t USARTxSource)
{
  MODIFY_REG(RCC->CFGR3, (RCC_CFGR3_USART1SW << ((USARTxSource  & 0xFF000000U) >> 24U)), (USARTxSource & 0x00FFFFFFU));
}

/**
  * @brief  Configure I2Cx clock source
  * @rmtoll CFGR3        I2C1SW        LL_RCC_SetI2CClockSource
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2CClockSource(uint32_t I2CxSource)
{
  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_I2C1SW, I2CxSource);
}

#if defined(CEC)
/**
  * @brief  Configure CEC clock source
  * @rmtoll CFGR3        CECSW         LL_RCC_SetCECClockSource
  * @param  CECxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV244
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCECClockSource(uint32_t CECxSource)
{
  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_CECSW, CECxSource);
}
#endif /* CEC */

#if defined(USB)
/**
  * @brief  Configure USB clock source
  * @rmtoll CFGR3        USBSW         LL_RCC_SetUSBClockSource
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE_HSI48 (*)
  *         @arg @ref LL_RCC_USB_CLKSOURCE_NONE (*)
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLL
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSBClockSource(uint32_t USBxSource)
{
  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USBSW, USBxSource);
}
#endif /* USB */

/**
  * @brief  Get USARTx clock source
  * @rmtoll CFGR3        USART1SW      LL_RCC_GetUSARTClockSource\n
  *         CFGR3        USART2SW      LL_RCC_GetUSARTClockSource\n
  *         CFGR3        USART3SW      LL_RCC_GetUSARTClockSource
  * @param  USARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices.
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE_HSI (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSARTClockSource(uint32_t USARTx)
{
  return (uint32_t)(READ_BIT(RCC->CFGR3, (RCC_CFGR3_USART1SW << USARTx)) | (USARTx << 24U));
}

/**
  * @brief  Get I2Cx clock source
  * @rmtoll CFGR3        I2C1SW        LL_RCC_GetI2CClockSource
  * @param  I2Cx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2CClockSource(uint32_t I2Cx)
{
  return (uint32_t)(READ_BIT(RCC->CFGR3, I2Cx));
}

#if defined(CEC)
/**
  * @brief  Get CEC clock source
  * @rmtoll CFGR3        CECSW         LL_RCC_GetCECClockSource
  * @param  CECx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV244
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetCECClockSource(uint32_t CECx)
{
  return (uint32_t)(READ_BIT(RCC->CFGR3, CECx));
}
#endif /* CEC */

#if defined(USB)
/**
  * @brief  Get USBx clock source
  * @rmtoll CFGR3        USBSW         LL_RCC_GetUSBClockSource
  * @param  USBx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE_HSI48 (*)
  *         @arg @ref LL_RCC_USB_CLKSOURCE_NONE (*)
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLL
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSBClockSource(uint32_t USBx)
{
  return (uint32_t)(READ_BIT(RCC->CFGR3, USBx));
}
#endif /* USB */

/**
  * @}
  */

/** @defgroup RCC_LL_EF_RTC RTC
  * @{
  */

/**
  * @brief  Set RTC Clock Source
  * @note Once the RTC clock source has been selected, it cannot be changed any more unless
  *       the Backup domain is reset. The BDRST bit can be used to reset them.
  * @rmtoll BDCR         RTCSEL        LL_RCC_SetRTCClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE_DIV32
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
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE_DIV32
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

#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
/**
  * @brief  Configure PLL used for SYSCLK Domain
  * @rmtoll CFGR         PLLSRC        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR         PLLMUL        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR2        PREDIV        LL_RCC_PLL_ConfigDomain_SYS
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI48 (*)
  *
  *         (*) value not defined in all devices
  * @param  PLLMul This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_2
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_5
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_7
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_9
  *         @arg @ref LL_RCC_PLL_MUL_10
  *         @arg @ref LL_RCC_PLL_MUL_11
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_13
  *         @arg @ref LL_RCC_PLL_MUL_14
  *         @arg @ref LL_RCC_PLL_MUL_15
  *         @arg @ref LL_RCC_PLL_MUL_16
  * @param  PLLDiv This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PREDIV_DIV_1
  *         @arg @ref LL_RCC_PREDIV_DIV_2
  *         @arg @ref LL_RCC_PREDIV_DIV_3
  *         @arg @ref LL_RCC_PREDIV_DIV_4
  *         @arg @ref LL_RCC_PREDIV_DIV_5
  *         @arg @ref LL_RCC_PREDIV_DIV_6
  *         @arg @ref LL_RCC_PREDIV_DIV_7
  *         @arg @ref LL_RCC_PREDIV_DIV_8
  *         @arg @ref LL_RCC_PREDIV_DIV_9
  *         @arg @ref LL_RCC_PREDIV_DIV_10
  *         @arg @ref LL_RCC_PREDIV_DIV_11
  *         @arg @ref LL_RCC_PREDIV_DIV_12
  *         @arg @ref LL_RCC_PREDIV_DIV_13
  *         @arg @ref LL_RCC_PREDIV_DIV_14
  *         @arg @ref LL_RCC_PREDIV_DIV_15
  *         @arg @ref LL_RCC_PREDIV_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_SYS(uint32_t Source, uint32_t PLLMul, uint32_t PLLDiv)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL, Source | PLLMul);
  MODIFY_REG(RCC->CFGR2, RCC_CFGR2_PREDIV, PLLDiv);
}

#else

/**
  * @brief  Configure PLL used for SYSCLK Domain
  * @rmtoll CFGR         PLLSRC        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR         PLLMUL        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR2        PREDIV        LL_RCC_PLL_ConfigDomain_SYS
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI_DIV_2
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_1
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_2
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_3
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_4
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_5
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_6
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_7
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_8
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_9
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_10
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_11
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_12
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_13
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_14
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_15
  *         @arg @ref LL_RCC_PLLSOURCE_HSE_DIV_16
  * @param  PLLMul This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_2
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_5
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_7
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_9
  *         @arg @ref LL_RCC_PLL_MUL_10
  *         @arg @ref LL_RCC_PLL_MUL_11
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_13
  *         @arg @ref LL_RCC_PLL_MUL_14
  *         @arg @ref LL_RCC_PLL_MUL_15
  *         @arg @ref LL_RCC_PLL_MUL_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_SYS(uint32_t Source, uint32_t PLLMul)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL, (Source & RCC_CFGR_PLLSRC) | PLLMul);
  MODIFY_REG(RCC->CFGR2, RCC_CFGR2_PREDIV, (Source & RCC_CFGR2_PREDIV));
}
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */

/**
  * @brief  Get the oscillator used as PLL clock source.
  * @rmtoll CFGR         PLLSRC        LL_RCC_PLL_GetMainSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI (*)
  *         @arg @ref LL_RCC_PLLSOURCE_HSI_DIV_2 (*)
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI48 (*)
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetMainSource(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLSRC));
}

/**
  * @brief  Get PLL multiplication Factor
  * @rmtoll CFGR         PLLMUL        LL_RCC_PLL_GetMultiplicator
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_2
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_5
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_7
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_9
  *         @arg @ref LL_RCC_PLL_MUL_10
  *         @arg @ref LL_RCC_PLL_MUL_11
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_13
  *         @arg @ref LL_RCC_PLL_MUL_14
  *         @arg @ref LL_RCC_PLL_MUL_15
  *         @arg @ref LL_RCC_PLL_MUL_16
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetMultiplicator(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLMUL));
}

/**
  * @brief  Get PREDIV division factor for the main PLL
  * @note They can be written only when the PLL is disabled
  * @rmtoll CFGR2        PREDIV        LL_RCC_PLL_GetPrediv
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PREDIV_DIV_1
  *         @arg @ref LL_RCC_PREDIV_DIV_2
  *         @arg @ref LL_RCC_PREDIV_DIV_3
  *         @arg @ref LL_RCC_PREDIV_DIV_4
  *         @arg @ref LL_RCC_PREDIV_DIV_5
  *         @arg @ref LL_RCC_PREDIV_DIV_6
  *         @arg @ref LL_RCC_PREDIV_DIV_7
  *         @arg @ref LL_RCC_PREDIV_DIV_8
  *         @arg @ref LL_RCC_PREDIV_DIV_9
  *         @arg @ref LL_RCC_PREDIV_DIV_10
  *         @arg @ref LL_RCC_PREDIV_DIV_11
  *         @arg @ref LL_RCC_PREDIV_DIV_12
  *         @arg @ref LL_RCC_PREDIV_DIV_13
  *         @arg @ref LL_RCC_PREDIV_DIV_14
  *         @arg @ref LL_RCC_PREDIV_DIV_15
  *         @arg @ref LL_RCC_PREDIV_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetPrediv(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR2, RCC_CFGR2_PREDIV));
}

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
  * @brief  Clear HSI14 ready interrupt flag
  * @rmtoll CIR          HSI14RDYC     LL_RCC_ClearFlag_HSI14RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSI14RDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSI14RDYC);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Clear HSI48 ready interrupt flag
  * @rmtoll CIR          HSI48RDYC     LL_RCC_ClearFlag_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSI48RDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSI48RDYC);
}
#endif /* RCC_HSI48_SUPPORT */

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
  * @brief  Check if HSI14 ready interrupt occurred or not
  * @rmtoll CIR          HSI14RDYF     LL_RCC_IsActiveFlag_HSI14RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSI14RDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSI14RDYF) == (RCC_CIR_HSI14RDYF));
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Check if HSI48 ready interrupt occurred or not
  * @rmtoll CIR          HSI48RDYF     LL_RCC_IsActiveFlag_HSI48RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSI48RDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSI48RDYF) == (RCC_CIR_HSI48RDYF));
}
#endif /* RCC_HSI48_SUPPORT */

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
  * @brief  Check if RCC flag is set or not.
  * @rmtoll CSR          OBLRSTF       LL_RCC_IsActiveFlag_OBLRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_OBLRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_OBLRSTF) == (RCC_CSR_OBLRSTF));
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

#if defined(RCC_CSR_V18PWRRSTF)
/**
  * @brief  Check if RCC Reset flag of the 1.8 V domain is set or not.
  * @rmtoll CSR          V18PWRRSTF    LL_RCC_IsActiveFlag_V18PWRRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_V18PWRRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_V18PWRRSTF) == (RCC_CSR_V18PWRRSTF));
}
#endif /* RCC_CSR_V18PWRRSTF */

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
  * @brief  Enable HSI14 ready interrupt
  * @rmtoll CIR          HSI14RDYIE    LL_RCC_EnableIT_HSI14RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSI14RDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSI14RDYIE);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Enable HSI48 ready interrupt
  * @rmtoll CIR          HSI48RDYIE    LL_RCC_EnableIT_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSI48RDY(void)
{
  SET_BIT(RCC->CIR, RCC_CIR_HSI48RDYIE);
}
#endif /* RCC_HSI48_SUPPORT */

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
  * @brief  Disable HSI14 ready interrupt
  * @rmtoll CIR          HSI14RDYIE    LL_RCC_DisableIT_HSI14RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSI14RDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_HSI14RDYIE);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Disable HSI48 ready interrupt
  * @rmtoll CIR          HSI48RDYIE    LL_RCC_DisableIT_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSI48RDY(void)
{
  CLEAR_BIT(RCC->CIR, RCC_CIR_HSI48RDYIE);
}
#endif /* RCC_HSI48_SUPPORT */

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
  * @brief  Checks if HSI14 ready interrupt source is enabled or disabled.
  * @rmtoll CIR          HSI14RDYIE    LL_RCC_IsEnabledIT_HSI14RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSI14RDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSI14RDYIE) == (RCC_CIR_HSI14RDYIE));
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Checks if HSI48 ready interrupt source is enabled or disabled.
  * @rmtoll CIR          HSI48RDYIE    LL_RCC_IsEnabledIT_HSI48RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSI48RDY(void)
{
  return (READ_BIT(RCC->CIR, RCC_CIR_HSI48RDYIE) == (RCC_CIR_HSI48RDYIE));
}
#endif /* RCC_HSI48_SUPPORT */

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
uint32_t    LL_RCC_GetI2CClockFreq(uint32_t I2CxSource);
#if defined(USB_OTG_FS) || defined(USB)
uint32_t    LL_RCC_GetUSBClockFreq(uint32_t USBxSource);
#endif /* USB_OTG_FS || USB */
#if defined(CEC)
uint32_t    LL_RCC_GetCECClockFreq(uint32_t CECxSource);
#endif /* CEC */
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

#endif /* RCC */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F0xx_LL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
