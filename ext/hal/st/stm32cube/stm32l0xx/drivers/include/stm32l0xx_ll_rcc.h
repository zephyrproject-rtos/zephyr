/**
  ******************************************************************************
  * @file    stm32l0xx_ll_rcc.h
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
#ifndef __STM32L0xx_LL_RCC_H
#define __STM32L0xx_LL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"

/** @addtogroup STM32L0xx_LL_Driver
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

/**
  * @}
  */

/* Private constants ---------------------------------------------------------*/
/** @defgroup RCC_LL_Private_Constants RCC Private Constants
  * @{
  */
/* Defines used for the bit position in the register and perform offsets*/
#define RCC_POSITION_HPRE       (uint32_t)4U  /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PPRE1      (uint32_t)8U  /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PPRE2      (uint32_t)11U /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PLLDIV     (uint32_t)22U /*!< field position in register RCC_CFGR */
#define RCC_POSITION_PLLMUL     (uint32_t)18U /*!< field position in register RCC_CFGR */
#define RCC_POSITION_HSICAL     (uint32_t)0U  /*!< field position in register RCC_ICSCR */
#define RCC_POSITION_HSITRIM    (uint32_t)8U  /*!< field position in register RCC_ICSCR */
#define RCC_POSITION_MSIRANGE   (uint32_t)13U /*!< field position in register RCC_ICSCR */
#define RCC_POSITION_MSICAL     (uint32_t)16U /*!< field position in register RCC_ICSCR */
#define RCC_POSITION_MSITRIM    (uint32_t)24U /*!< field position in register RCC_ICSCR */
#if defined(RCC_HSI48_SUPPORT)
#define RCC_POSITION_HSI48CAL   (uint32_t)8U  /*!< field position in register RCC_CRRCR */
#endif /* RCC_HSI48_SUPPORT */

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
#define HSE_VALUE    ((uint32_t)8000000U)  /*!< Value of the HSE oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    ((uint32_t)16000000U) /*!< Value of the HSI oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE    ((uint32_t)32768U)    /*!< Value of the LSE oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE    ((uint32_t)37000U)    /*!< Value of the LSI oscillator in Hz */
#endif /* LSI_VALUE */
#if defined(RCC_HSI48_SUPPORT)

#if !defined  (HSI48_VALUE)
#define HSI48_VALUE  ((uint32_t)48000000U) /*!< Value of the HSI48 oscillator in Hz */
#endif /* HSI48_VALUE */
#endif /* RCC_HSI48_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CLEAR_FLAG Clear Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_WriteReg function
  * @{
  */
#define LL_RCC_CICR_LSIRDYC                RCC_CICR_LSIRDYC     /*!< LSI Ready Interrupt Clear */
#define LL_RCC_CICR_LSERDYC                RCC_CICR_LSERDYC     /*!< LSE Ready Interrupt Clear */
#define LL_RCC_CICR_HSIRDYC                RCC_CICR_HSIRDYC     /*!< HSI Ready Interrupt Clear */
#define LL_RCC_CICR_HSERDYC                RCC_CICR_HSERDYC     /*!< HSE Ready Interrupt Clear */
#define LL_RCC_CICR_PLLRDYC                RCC_CICR_PLLRDYC     /*!< PLL Ready Interrupt Clear */
#define LL_RCC_CICR_MSIRDYC                RCC_CICR_MSIRDYC     /*!< MSI Ready Interrupt Clear */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CICR_HSI48RDYC               RCC_CICR_HSI48RDYC  /*!< HSI48 Ready Interrupt Clear */
#endif /* RCC_HSI48_SUPPORT */
#define LL_RCC_CICR_LSECSSC                RCC_CICR_LSECSSC     /*!< LSE Clock Security System Interrupt Clear */
#define LL_RCC_CICR_CSSC                   RCC_CICR_CSSC        /*!< Clock Security System Interrupt Clear */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_ReadReg function
  * @{
  */
#define LL_RCC_CIFR_LSIRDYF                RCC_CIFR_LSIRDYF     /*!< LSI Ready Interrupt flag */
#define LL_RCC_CIFR_LSERDYF                RCC_CIFR_LSERDYF     /*!< LSE Ready Interrupt flag */
#define LL_RCC_CIFR_HSIRDYF                RCC_CIFR_HSIRDYF     /*!< HSI Ready Interrupt flag */
#define LL_RCC_CIFR_HSERDYF                RCC_CIFR_HSERDYF     /*!< HSE Ready Interrupt flag */
#define LL_RCC_CIFR_PLLRDYF                RCC_CIFR_PLLRDYF     /*!< PLL Ready Interrupt flag */
#define LL_RCC_CIFR_MSIRDYF                RCC_CIFR_MSIRDYF     /*!< MSI Ready Interrupt flag */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CIFR_HSI48RDYF               RCC_CIFR_HSI48RDYF  /*!< HSI48 Ready Interrupt flag */
#endif /* RCC_HSI48_SUPPORT */
#define LL_RCC_CIFR_LSECSSF                RCC_CIFR_LSECSSF    /*!< LSE Clock Security System Interrupt flag */
#define LL_RCC_CIFR_CSSF                   RCC_CIFR_CSSF       /*!< Clock Security System Interrupt flag */
#define LL_RCC_CSR_FWRSTF                 RCC_CSR_FWRSTF          /*!< Firewall reset flag */
#define LL_RCC_CSR_OBLRSTF                RCC_CSR_OBLRSTF         /*!< OBL reset flag */
#define LL_RCC_CSR_PINRSTF                RCC_CSR_PINRSTF         /*!< PIN reset flag */
#define LL_RCC_CSR_PORRSTF                RCC_CSR_PORRSTF         /*!< POR/PDR reset flag */
#define LL_RCC_CSR_SFTRSTF                RCC_CSR_SFTRSTF         /*!< Software Reset flag */
#define LL_RCC_CSR_IWDGRSTF               RCC_CSR_IWDGRSTF        /*!< Independent Watchdog reset flag */
#define LL_RCC_CSR_WWDGRSTF               RCC_CSR_WWDGRSTF        /*!< Window watchdog reset flag */
#define LL_RCC_CSR_LPWRRSTF               RCC_CSR_LPWRRSTF        /*!< Low-Power reset flag */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_RCC_ReadReg and  LL_RCC_WriteReg functions
  * @{
  */
#define LL_RCC_CIER_LSIRDYIE               RCC_CIER_LSIRDYIE      /*!< LSI Ready Interrupt Enable */
#define LL_RCC_CIER_LSERDYIE               RCC_CIER_LSERDYIE      /*!< LSE Ready Interrupt Enable */
#define LL_RCC_CIER_HSIRDYIE               RCC_CIER_HSIRDYIE      /*!< HSI Ready Interrupt Enable */
#define LL_RCC_CIER_HSERDYIE               RCC_CIER_HSERDYIE      /*!< HSE Ready Interrupt Enable */
#define LL_RCC_CIER_PLLRDYIE               RCC_CIER_PLLRDYIE      /*!< PLL Ready Interrupt Enable */
#define LL_RCC_CIER_MSIRDYIE               RCC_CIER_MSIRDYIE      /*!< MSI Ready Interrupt Enable */
#if defined(RCC_HSI48_SUPPORT)
#define LL_RCC_CIER_HSI48RDYIE              RCC_CIER_HSI48RDYIE   /*!< HSI48 Ready Interrupt Enable */
#endif /* RCC_HSI48_SUPPORT */
#define LL_RCC_CIER_LSECSSIE               RCC_CIER_LSECSSIE      /*!< LSE CSS Interrupt Enable */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSEDRIVE  LSE oscillator drive capability
  * @{
  */
#define LL_RCC_LSEDRIVE_LOW                ((uint32_t)0x00000000U) /*!< Xtal mode lower driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMLOW          RCC_CSR_LSEDRV_0 /*!< Xtal mode medium low driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMHIGH         RCC_CSR_LSEDRV_1 /*!< Xtal mode medium high driving capability */
#define LL_RCC_LSEDRIVE_HIGH               RCC_CSR_LSEDRV   /*!< Xtal mode higher driving capability */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RTC_HSE_DIV RTC HSE Prescaler
  * @{
  */
#define LL_RCC_RTC_HSE_DIV_2               (uint32_t)0x00000000U/*!< HSE is divided by 2 for RTC clock  */
#define LL_RCC_RTC_HSE_DIV_4               RCC_CR_RTCPRE_0      /*!< HSE is divided by 4 for RTC clock  */
#define LL_RCC_RTC_HSE_DIV_8               RCC_CR_RTCPRE_1      /*!< HSE is divided by 8 for RTC clock  */
#define LL_RCC_RTC_HSE_DIV_16              RCC_CR_RTCPRE        /*!< HSE is divided by 16 for RTC clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MSIRANGE  MSI clock ranges
  * @{
  */
#define LL_RCC_MSIRANGE_0                  RCC_ICSCR_MSIRANGE_0  /*!< MSI = 65.536 KHz */
#define LL_RCC_MSIRANGE_1                  RCC_ICSCR_MSIRANGE_1  /*!< MSI = 131.072 KHz*/
#define LL_RCC_MSIRANGE_2                  RCC_ICSCR_MSIRANGE_2  /*!< MSI = 262.144 KHz */
#define LL_RCC_MSIRANGE_3                  RCC_ICSCR_MSIRANGE_3  /*!< MSI = 524.288 KHz */
#define LL_RCC_MSIRANGE_4                  RCC_ICSCR_MSIRANGE_4  /*!< MSI = 1.048 MHz */
#define LL_RCC_MSIRANGE_5                  RCC_ICSCR_MSIRANGE_5  /*!< MSI = 2.097 MHz */
#define LL_RCC_MSIRANGE_6                  RCC_ICSCR_MSIRANGE_6  /*!< MSI = 4.194 MHz */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE  System clock switch
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_MSI           RCC_CFGR_SW_MSI    /*!< MSI selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_HSI           RCC_CFGR_SW_HSI    /*!< HSI selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_HSE           RCC_CFGR_SW_HSE    /*!< HSE selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_PLL           RCC_CFGR_SW_PLL    /*!< PLL selection as system clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE_STATUS  System clock switch status
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_STATUS_MSI    RCC_CFGR_SWS_MSI   /*!< MSI used as system clock */
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

/** @defgroup RCC_LL_EC_STOP_WAKEUPCLOCK  Wakeup from Stop and CSS backup clock selection
  * @{
  */
#define LL_RCC_STOP_WAKEUPCLOCK_MSI        ((uint32_t)0x00000000U) /*!< MSI selection after wake-up from STOP */
#define LL_RCC_STOP_WAKEUPCLOCK_HSI        RCC_CFGR_STOPWUCK       /*!< HSI selection after wake-up from STOP */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1SOURCE  MCO1 SOURCE selection
  * @{
  */
#define LL_RCC_MCO1SOURCE_NOCLOCK          RCC_CFGR_MCOSEL_NOCLOCK      /*!< MCO output disabled, no clock on MCO */
#define LL_RCC_MCO1SOURCE_SYSCLK           RCC_CFGR_MCOSEL_SYSCLK       /*!< SYSCLK selection as MCO source */
#define LL_RCC_MCO1SOURCE_HSI              RCC_CFGR_MCOSEL_HSI          /*!< HSI selection as MCO source */
#define LL_RCC_MCO1SOURCE_MSI              RCC_CFGR_MCOSEL_MSI          /*!< MSI selection as MCO source */
#define LL_RCC_MCO1SOURCE_HSE              RCC_CFGR_MCOSEL_HSE          /*!< HSE selection as MCO source */
#define LL_RCC_MCO1SOURCE_LSI              RCC_CFGR_MCOSEL_LSI          /*!< LSI selection as MCO source */
#define LL_RCC_MCO1SOURCE_LSE              RCC_CFGR_MCOSEL_LSE          /*!< LSE selection as MCO source */
#if defined(RCC_CFGR_MCOSEL_HSI48)
#define LL_RCC_MCO1SOURCE_HSI48            RCC_CFGR_MCOSEL_HSI48        /*!< HSI48 selection as MCO source */
#endif /* RCC_CFGR_MCOSEL_HSI48 */
#define LL_RCC_MCO1SOURCE_PLLCLK           RCC_CFGR_MCOSEL_PLL          /*!< PLLCLK selection as MCO source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1_DIV  MCO1 prescaler
  * @{
  */
#define LL_RCC_MCO1_DIV_1                  RCC_CFGR_MCOPRE_DIV1  /*!< MCO Clock divided by 1  */
#define LL_RCC_MCO1_DIV_2                  RCC_CFGR_MCOPRE_DIV2  /*!< MCO Clock divided by 2  */
#define LL_RCC_MCO1_DIV_4                  RCC_CFGR_MCOPRE_DIV4  /*!< MCO Clock divided by 4  */
#define LL_RCC_MCO1_DIV_8                  RCC_CFGR_MCOPRE_DIV8  /*!< MCO Clock divided by 8  */
#define LL_RCC_MCO1_DIV_16                 RCC_CFGR_MCOPRE_DIV16 /*!< MCO Clock divided by 16 */
/**
  * @}
  */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EC_PERIPH_FREQUENCY Peripheral clock frequency
  * @{
  */
#define LL_RCC_PERIPH_FREQUENCY_NO         (uint32_t)0x00000000U      /*!< No clock enabled for the peripheral            */
#define LL_RCC_PERIPH_FREQUENCY_NA         (uint32_t)0xFFFFFFFFU      /*!< Frequency cannot be provided as external clock */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/** @defgroup RCC_LL_EC_USART1_CLKSOURCE  Peripheral USART clock source selection
  * @{
  */
#if defined(RCC_CCIPR_USART1SEL)
#define LL_RCC_USART1_CLKSOURCE_PCLK2      (uint32_t)((RCC_CCIPR_USART1SEL << 16U) | 0x00000000U)             /*!< PCLK2 selected as USART1 clock */
#define LL_RCC_USART1_CLKSOURCE_SYSCLK     (uint32_t)((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL_0)   /*!< SYSCLK selected as USART1 clock */
#define LL_RCC_USART1_CLKSOURCE_HSI        (uint32_t)((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL_1)   /*!< HSI selected as USART1 clock */
#define LL_RCC_USART1_CLKSOURCE_LSE        (uint32_t)((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL)     /*!< LSE selected as USART1 clock*/
#endif /* RCC_CCIPR_USART1SEL */
#define LL_RCC_USART2_CLKSOURCE_PCLK1      (uint32_t)((RCC_CCIPR_USART2SEL << 16U) | 0x00000000U)             /*!< PCLK1 selected as USART2 clock */
#define LL_RCC_USART2_CLKSOURCE_SYSCLK     (uint32_t)((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL_0)   /*!< SYSCLK selected as USART2 clock */
#define LL_RCC_USART2_CLKSOURCE_HSI        (uint32_t)((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL_1)   /*!< HSI selected as USART2 clock */
#define LL_RCC_USART2_CLKSOURCE_LSE        (uint32_t)((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL)     /*!< LSE selected as USART2 clock*/
/**
  * @}
  */



/** @defgroup RCC_LL_EC_LPUART1_CLKSOURCE  Peripheral LPUART clock source selection
  * @{
  */
#define LL_RCC_LPUART1_CLKSOURCE_PCLK1     (uint32_t)0x00000000U   /*!< PCLK1 selected as LPUART1 clock */
#define LL_RCC_LPUART1_CLKSOURCE_SYSCLK    RCC_CCIPR_LPUART1SEL_0  /*!< SYSCLK selected as LPUART1 clock */
#define LL_RCC_LPUART1_CLKSOURCE_HSI       RCC_CCIPR_LPUART1SEL_1  /*!< HSI selected as LPUART1 clock */
#define LL_RCC_LPUART1_CLKSOURCE_LSE       RCC_CCIPR_LPUART1SEL    /*!< LSE selected as LPUART1 clock*/
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2C1_CLKSOURCE  Peripheral I2C clock source selection
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE_PCLK1        (uint32_t)((RCC_CCIPR_I2C1SEL << 4U) | (0x00000000U >> 4U))           /*!< PCLK1 selected as I2C1 clock */
#define LL_RCC_I2C1_CLKSOURCE_SYSCLK       (uint32_t)((RCC_CCIPR_I2C1SEL << 4U) | (RCC_CCIPR_I2C1SEL_0 >> 4U))  /*!< SYSCLK selected as I2C1 clock */
#define LL_RCC_I2C1_CLKSOURCE_HSI          (uint32_t)((RCC_CCIPR_I2C1SEL << 4U) | (RCC_CCIPR_I2C1SEL_1 >> 4U))  /*!< HSI selected as I2C1 clock */
#if defined(RCC_CCIPR_I2C3SEL)
#define LL_RCC_I2C3_CLKSOURCE_PCLK1        (uint32_t)((RCC_CCIPR_I2C3SEL << 4U) | (0x00000000U >> 4U))           /*!< PCLK1 selected as I2C3 clock */
#define LL_RCC_I2C3_CLKSOURCE_SYSCLK       (uint32_t)((RCC_CCIPR_I2C3SEL << 4U) | (RCC_CCIPR_I2C3SEL_0 >> 4U))  /*!< SYSCLK selected as I2C3 clock */
#define LL_RCC_I2C3_CLKSOURCE_HSI          (uint32_t)((RCC_CCIPR_I2C3SEL << 4U) | (RCC_CCIPR_I2C3SEL_1 >> 4U))  /*!< HSI selected as I2C3 clock */
#endif /*RCC_CCIPR_I2C3SEL*/
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIM1_CLKSOURCE  Peripheral LPTIM clock source selection
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE_PCLK1      (uint32_t)(0x00000000U)          /*!< PCLK1 selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSI        (uint32_t)RCC_CCIPR_LPTIM1SEL_0  /*!< LSI selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_HSI        (uint32_t)RCC_CCIPR_LPTIM1SEL_1  /*!< HSI selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSE        (uint32_t)RCC_CCIPR_LPTIM1SEL    /*!< LSE selected as LPTIM1 clock*/
/**
  * @}
  */

#if defined(RCC_CCIPR_HSI48SEL)

#if defined(RNG)
/** @defgroup RCC_LL_EC_RNG_CLKSOURCE  Peripheral RNG clock source selection
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE_PLL           (uint32_t)(0x00000000U)          /*!< PLL selected as RNG clock */
#define LL_RCC_RNG_CLKSOURCE_HSI48         (uint32_t)(RCC_CCIPR_HSI48SEL)   /*!< HSI48 selected as RNG clock*/
/**
  * @}
  */
#endif /* RNG */
#if defined(USB)
/** @defgroup RCC_LL_EC_USB_CLKSOURCE  Peripheral USB clock source selection
  * @{
  */
#define LL_RCC_USB_CLKSOURCE_PLL           (uint32_t)(0x00000000U)          /*!< PLL selected as USB clock */
#define LL_RCC_USB_CLKSOURCE_HSI48         (uint32_t)(RCC_CCIPR_HSI48SEL)   /*!< HSI48 selected as USB clock*/
/**
  * @}
  */

#endif /* USB */
#endif /* RCC_CCIPR_HSI48SEL */


/** @defgroup RCC_LL_EC_USART1 Peripheral USART get clock source
  * @{
  */
#if defined(RCC_CCIPR_USART1SEL)
#define LL_RCC_USART1_CLKSOURCE            RCC_CCIPR_USART1SEL    /*!< USART1 clock source selection bits */
#endif /* RCC_CCIPR_USART1SEL */
#define LL_RCC_USART2_CLKSOURCE            RCC_CCIPR_USART2SEL    /*!< USART2 clock source selection bits */
/**
  * @}
  */


/** @defgroup RCC_LL_EC_LPUART1 Peripheral LPUART get clock source
  * @{
  */
#define LL_RCC_LPUART1_CLKSOURCE           RCC_CCIPR_LPUART1SEL   /*!< LPUART1 clock source selection bits */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2C1 Peripheral I2C get clock source
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE              RCC_CCIPR_I2C1SEL   /*!< I2C1 clock source selection bits */
#if defined(RCC_CCIPR_I2C3SEL)
#define LL_RCC_I2C3_CLKSOURCE              RCC_CCIPR_I2C3SEL   /*!< I2C3 clock source selection bits */
#endif /*RCC_CCIPR_I2C3SEL*/
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIM1 Peripheral LPTIM get clock source
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE            RCC_CCIPR_LPTIM1SEL  /*!< LPTIM1 clock source selection bits */
/**
  * @}
  */

#if defined(RCC_CCIPR_HSI48SEL)
#if defined(RNG)
/** @defgroup RCC_LL_EC_RNG  Peripheral RNG get clock source
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE               RCC_CCIPR_HSI48SEL   /*!< HSI48 RC clock source selection bit for RNG*/
/**
  * @}
  */
#endif /* RNG */

#if defined(USB)
/** @defgroup RCC_LL_EC_USB  Peripheral USB get clock source
  * @{
  */
#define LL_RCC_USB_CLKSOURCE               RCC_CCIPR_HSI48SEL  /*!< HSI48 RC clock source selection bit for USB*/
/**
  * @}
  */

#endif /* USB */
#endif /* RCC_CCIPR_HSI48SEL */

/** @defgroup RCC_LL_EC_RTC_CLKSOURCE  RTC clock source selection
  * @{
  */
#define LL_RCC_RTC_CLKSOURCE_NONE          (uint32_t)0x00000000U         /*!< No clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSE           RCC_CSR_RTCSEL_LSE            /*!< LSE oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSI           RCC_CSR_RTCSEL_LSI            /*!< LSI oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_HSE           RCC_CSR_RTCSEL_HSE            /*!< HSE oscillator clock divided by a programmable prescaler 
                                                                             (selection through @ref LL_RCC_SetRTC_HSEPrescaler function ) */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL_MUL PLL Multiplicator factor
  * @{
  */
#define LL_RCC_PLL_MUL_3                   RCC_CFGR_PLLMUL3  /*!< PLL input clock * 3  */
#define LL_RCC_PLL_MUL_4                   RCC_CFGR_PLLMUL4  /*!< PLL input clock * 4  */
#define LL_RCC_PLL_MUL_6                   RCC_CFGR_PLLMUL6  /*!< PLL input clock * 6  */
#define LL_RCC_PLL_MUL_8                   RCC_CFGR_PLLMUL8  /*!< PLL input clock * 8  */
#define LL_RCC_PLL_MUL_12                  RCC_CFGR_PLLMUL12 /*!< PLL input clock * 12 */
#define LL_RCC_PLL_MUL_16                  RCC_CFGR_PLLMUL16 /*!< PLL input clock * 16 */
#define LL_RCC_PLL_MUL_24                  RCC_CFGR_PLLMUL24 /*!< PLL input clock * 24 */
#define LL_RCC_PLL_MUL_32                  RCC_CFGR_PLLMUL32 /*!< PLL input clock * 32 */
#define LL_RCC_PLL_MUL_48                  RCC_CFGR_PLLMUL48 /*!< PLL input clock * 48 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL_DIV PLL division factor
  * @{
  */
#define LL_RCC_PLL_DIV_2                   RCC_CFGR_PLLDIV2 /*!< PLL clock output = PLLVCO / 2 */
#define LL_RCC_PLL_DIV_3                   RCC_CFGR_PLLDIV3 /*!< PLL clock output = PLLVCO / 3 */
#define LL_RCC_PLL_DIV_4                   RCC_CFGR_PLLDIV4 /*!< PLL clock output = PLLVCO / 4 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLSOURCE PLL SOURCE
  * @{
  */
#define LL_RCC_PLLSOURCE_HSI               RCC_CFGR_PLLSRC_HSI                           /*!< HSI clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE               RCC_CFGR_PLLSRC_HSE                           /*!< HSE clock selected as PLL entry clock source */
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
  * @brief  Helper macro to calculate the PLLCLK frequency
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_FREQ (HSE_VALUE,
  *                                      @ref LL_RCC_PLL_GetMultiplicator (),
  *                                      @ref LL_RCC_PLL_GetDivider ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on MSI/HSE/HSI)
  * @param  __PLLMUL__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_16
  *         @arg @ref LL_RCC_PLL_MUL_24
  *         @arg @ref LL_RCC_PLL_MUL_32
  *         @arg @ref LL_RCC_PLL_MUL_48
  * @param  __PLLDIV__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_DIV_2
  *         @arg @ref LL_RCC_PLL_DIV_3
  *         @arg @ref LL_RCC_PLL_DIV_4
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_FREQ(__INPUTFREQ__, __PLLMUL__,  __PLLDIV__) ((__INPUTFREQ__) * (PLLMulTable[(__PLLMUL__) >> RCC_POSITION_PLLMUL]) / (((__PLLDIV__) >> RCC_POSITION_PLLDIV)+1U))

/**
  * @brief  Helper macro to calculate the HCLK frequency
  * @note: __AHBPRESCALER__ be retrieved by @ref LL_RCC_GetAHBPrescaler
  *        ex: __LL_RCC_CALC_HCLK_FREQ(LL_RCC_GetAHBPrescaler())
  * @param  __SYSCLKFREQ__ SYSCLK frequency (based on MSI/HSE/HSI/PLLCLK)
  * @param  __AHBPRESCALER__: This parameter can be one of the following values:
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
#define __LL_RCC_CALC_HCLK_FREQ(__SYSCLKFREQ__, __AHBPRESCALER__) ((__SYSCLKFREQ__) >> AHBPrescTable[((__AHBPRESCALER__) & RCC_CFGR_HPRE) >>  RCC_POSITION_HPRE])

/**
  * @brief  Helper macro to calculate the PCLK1 frequency (ABP1)
  * @note: __APB1PRESCALER__ be retrieved by @ref LL_RCC_GetAPB1Prescaler
  *        ex: __LL_RCC_CALC_PCLK1_FREQ(LL_RCC_GetAPB1Prescaler())
  * @param  __HCLKFREQ__ HCLK frequency
  * @param  __APB1PRESCALER__: This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  * @retval PCLK1 clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PCLK1_FREQ(__HCLKFREQ__, __APB1PRESCALER__) ((__HCLKFREQ__) >> APBPrescTable[(__APB1PRESCALER__) >>  RCC_POSITION_PPRE1])

/**
  * @brief  Helper macro to calculate the PCLK2 frequency (ABP2)
  * @note: __APB2PRESCALER__ be retrieved by @ref LL_RCC_GetAPB2Prescaler
  *        ex: __LL_RCC_CALC_PCLK2_FREQ(LL_RCC_GetAPB2Prescaler())
  * @param  __HCLKFREQ__ HCLK frequency
  * @param  __APB2PRESCALER__: This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  * @retval PCLK2 clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PCLK2_FREQ(__HCLKFREQ__, __APB2PRESCALER__) ((__HCLKFREQ__) >> APBPrescTable[(__APB2PRESCALER__) >>  RCC_POSITION_PPRE2])

/**
  * @brief  Helper macro to calculate the MSI frequency (in Hz)
  * @note: __MSIRANGE__can be retrieved by @ref LL_RCC_MSI_GetRange
  *        ex: __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange())
  * @param  __MSIRANGE__: This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MSIRANGE_0
  *         @arg @ref LL_RCC_MSIRANGE_1
  *         @arg @ref LL_RCC_MSIRANGE_2
  *         @arg @ref LL_RCC_MSIRANGE_3
  *         @arg @ref LL_RCC_MSIRANGE_4
  *         @arg @ref LL_RCC_MSIRANGE_5
  *         @arg @ref LL_RCC_MSIRANGE_6
  * @retval MSI clock frequency (in Hz)
  */
#define __LL_RCC_CALC_MSI_FREQ(__MSIRANGE__) ((32768U * ( 1U << (((__MSIRANGE__) >> RCC_POSITION_MSIRANGE) + 1U))))

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

#if defined(RCC_HSECSS_SUPPORT)
/**
  * @brief  Enable the Clock Security System.
  * @rmtoll CR           CSSHSEON         LL_RCC_HSE_EnableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableCSS(void)
{
  SET_BIT(RCC->CR, RCC_CR_CSSON);
}
#endif /* RCC_HSECSS_SUPPORT */

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
  * @brief  Configure the RTC prescaler (divider)
  * @rmtoll CR           RTCPRE        LL_RCC_SetRTC_HSEPrescaler
  * @param  Div This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_HSE_DIV_2
  *         @arg @ref LL_RCC_RTC_HSE_DIV_4
  *         @arg @ref LL_RCC_RTC_HSE_DIV_8
  *         @arg @ref LL_RCC_RTC_HSE_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTC_HSEPrescaler(uint32_t Div)
{
  MODIFY_REG(RCC->CR, RCC_CR_RTCPRE, Div);
}

/**
  * @brief  Get the RTC divider (prescaler)
  * @rmtoll CR           RTCPRE        LL_RCC_GetRTC_HSEPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_HSE_DIV_2
  *         @arg @ref LL_RCC_RTC_HSE_DIV_4
  *         @arg @ref LL_RCC_RTC_HSE_DIV_8
  *         @arg @ref LL_RCC_RTC_HSE_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTC_HSEPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->CR, RCC_CR_RTCPRE));
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
  * @brief  Enable HSI even in stop mode
  * @note HSI oscillator is forced ON even in Stop mode
  * @rmtoll CR           HSIKERON      LL_RCC_HSI_EnableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_EnableInStopMode(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSIKERON);
}

/**
  * @brief  Disable HSI in stop mode
  * @rmtoll CR           HSIKERON      LL_RCC_HSI_DisableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_DisableInStopMode(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSIKERON);
}

/**
  * @brief  Enable HSI Divider (it divides by 4)
  * @rmtoll CR           HSIDIVEN       LL_RCC_HSI_EnableDivider
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_EnableDivider(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSIDIVEN);
}

/**
  * @brief  Disable HSI Divider (it divides by 4)
  * @rmtoll CR           HSIDIVEN       LL_RCC_HSI_DisableDivider
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_DisableDivider(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSIDIVEN);
}



#if defined(RCC_CR_HSIOUTEN)
/**
  * @brief  Enable HSI Output
  * @rmtoll CR           HSIOUTEN       LL_RCC_HSI_EnableOutput
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_EnableOutput(void)
{
  SET_BIT(RCC->CR, RCC_CR_HSIOUTEN);
}

/**
  * @brief  Disable HSI Output
  * @rmtoll CR           HSIOUTEN       LL_RCC_HSI_DisableOutput
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_DisableOutput(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_HSIOUTEN);
}
#endif /* RCC_CR_HSIOUTEN */

/**
  * @brief  Get HSI Calibration value
  * @note When HSITRIM is written, HSICAL is updated with the sum of
  *       HSITRIM and the factory trim value
  * @rmtoll ICSCR        HSICAL        LL_RCC_HSI_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_HSICAL) >> RCC_POSITION_HSICAL);
}

/**
  * @brief  Set HSI Calibration trimming
  * @note user-programmable trimming value that is added to the HSICAL
  * @note Default value is 16, which, when added to the HSICAL value,
  *       should trim the HSI to 16 MHz +/- 1 %
  * @rmtoll ICSCR        HSITRIM       LL_RCC_HSI_SetCalibTrimming
  * @param  Value between Min_Data = 0x00 and Max_Data = 0x1F
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, Value << RCC_POSITION_HSITRIM);
}

/**
  * @brief  Get HSI Calibration trimming
  * @rmtoll ICSCR        HSITRIM       LL_RCC_HSI_GetCalibTrimming
  * @retval Between Min_Data = 0x00 and Max_Data = 0x1F
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_HSITRIM) >> RCC_POSITION_HSITRIM);
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
  * @rmtoll CRRCR          HSI48ON       LL_RCC_HSI48_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_Enable(void)
{
  SET_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);
}

/**
  * @brief  Disable HSI48
  * @rmtoll CRRCR          HSI48ON       LL_RCC_HSI48_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_Disable(void)
{
  CLEAR_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);
}

/**
  * @brief  Check if HSI48 oscillator Ready
  * @rmtoll CRRCR          HSI48RDY      LL_RCC_HSI48_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI48_IsReady(void)
{
  return (READ_BIT(RCC->CRRCR, RCC_CRRCR_HSI48RDY) == (RCC_CRRCR_HSI48RDY));
}

/**
  * @brief  Get HSI48 Calibration value
  * @rmtoll CRRCR          HSI48CAL      LL_RCC_HSI48_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI48_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->CRRCR, RCC_CRRCR_HSI48CAL) >> RCC_POSITION_HSI48CAL);
}

#if defined(RCC_CRRCR_HSI48DIV6OUTEN)
/**
  * @brief  Enable HSI48 Divider (it divides by 6)
  * @rmtoll CRRCR           HSI48DIV6OUTEN       LL_RCC_HSI48_EnableDivider
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_EnableDivider(void)
{
  SET_BIT(RCC->CRRCR, RCC_CRRCR_HSI48DIV6OUTEN);
}

/**
  * @brief  Disable HSI48 Divider (it divides by 6)
  * @rmtoll CRRCR           HSI48DIV6OUTEN       LL_RCC_HSI48_DisableDivider
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI48_DisableDivider(void)
{
  CLEAR_BIT(RCC->CRRCR, RCC_CRRCR_HSI48DIV6OUTEN);
}

/**
  * @brief  Check if HSI48 Divider is enabled (it divides by 6)
  * @rmtoll CRRCR        HSI48DIV6OUTEN        LL_RCC_HSI48_IsDivided
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI48_IsDivided(void)
{
  return (READ_BIT(RCC->CRRCR, RCC_CRRCR_HSI48DIV6OUTEN) == (RCC_CRRCR_HSI48DIV6OUTEN));
}

#endif /*RCC_CRRCR_HSI48DIV6OUTEN*/

/**
  * @}
  */

#endif /* RCC_HSI48_SUPPORT */

/** @defgroup RCC_LL_EF_LSE LSE
  * @{
  */

/**
  * @brief  Enable  Low Speed External (LSE) crystal.
  * @rmtoll CSR         LSEON         LL_RCC_LSE_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_Enable(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSEON);
}

/**
  * @brief  Disable  Low Speed External (LSE) crystal.
  * @rmtoll CSR         LSEON         LL_RCC_LSE_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_Disable(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_LSEON);
}

/**
  * @brief  Enable external clock source (LSE bypass).
  * @rmtoll CSR         LSEBYP        LL_RCC_LSE_EnableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_EnableBypass(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSEBYP);
}

/**
  * @brief  Disable external clock source (LSE bypass).
  * @rmtoll CSR         LSEBYP        LL_RCC_LSE_DisableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_DisableBypass(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_LSEBYP);
}

/**
  * @brief  Set LSE oscillator drive capability
  * @note The oscillator is in Xtal mode when it is not in bypass mode.
  * @rmtoll CSR         LSEDRV        LL_RCC_LSE_SetDriveCapability
  * @param  LSEDrive This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LSEDRIVE_LOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
  *         @arg @ref LL_RCC_LSEDRIVE_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_SetDriveCapability(uint32_t LSEDrive)
{
  MODIFY_REG(RCC->CSR, RCC_CSR_LSEDRV, LSEDrive);
}

/**
  * @brief  Get LSE oscillator drive capability
  * @rmtoll CSR         LSEDRV        LL_RCC_LSE_GetDriveCapability
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LSEDRIVE_LOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMLOW
  *         @arg @ref LL_RCC_LSEDRIVE_MEDIUMHIGH
  *         @arg @ref LL_RCC_LSEDRIVE_HIGH
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_GetDriveCapability(void)
{
  return (uint32_t)(READ_BIT(RCC->CSR, RCC_CSR_LSEDRV));
}

/**
  * @brief  Enable Clock security system on LSE.
  * @rmtoll CSR         LSECSSON      LL_RCC_LSE_EnableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_EnableCSS(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSECSSON);
}

/**
  * @brief  Disable Clock security system on LSE.
  * @note   Clock security system can be disabled only after a LSE
  *         failure detection. In that case it MUST be disabled by software.
  * @rmtoll CSR          LSECSSON      LL_RCC_LSE_DisableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_DisableCSS(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_LSECSSON);
}

/**
  * @brief  Check if LSE oscillator Ready
  * @rmtoll CSR         LSERDY        LL_RCC_LSE_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsReady(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_LSERDY) == (RCC_CSR_LSERDY));
}

/**
  * @brief  Check if CSS on LSE failure Detection
  * @rmtoll CSR         LSECSSD       LL_RCC_LSE_IsCSSDetected
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsCSSDetected(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_LSECSSD) == (RCC_CSR_LSECSSD));
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

/** @defgroup RCC_LL_EF_MSI MSI
  * @{
  */

/**
  * @brief  Enable MSI oscillator
  * @rmtoll CR           MSION         LL_RCC_MSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_MSI_Enable(void)
{
  SET_BIT(RCC->CR, RCC_CR_MSION);
}

/**
  * @brief  Disable MSI oscillator
  * @rmtoll CR           MSION         LL_RCC_MSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_MSI_Disable(void)
{
  CLEAR_BIT(RCC->CR, RCC_CR_MSION);
}

/**
  * @brief  Check if MSI oscillator Ready
  * @rmtoll CR           MSIRDY        LL_RCC_MSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_MSI_IsReady(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_MSIRDY) == (RCC_CR_MSIRDY));
}

/**
  * @brief  Configure the Internal Multi Speed oscillator (MSI) clock range in run mode.
  * @rmtoll ICSCR           MSIRANGE      LL_RCC_MSI_SetRange
  * @param  Range This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MSIRANGE_0
  *         @arg @ref LL_RCC_MSIRANGE_1
  *         @arg @ref LL_RCC_MSIRANGE_2
  *         @arg @ref LL_RCC_MSIRANGE_3
  *         @arg @ref LL_RCC_MSIRANGE_4
  *         @arg @ref LL_RCC_MSIRANGE_5
  *         @arg @ref LL_RCC_MSIRANGE_6
  * @retval None
  */
__STATIC_INLINE void LL_RCC_MSI_SetRange(uint32_t Range)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, Range);
}

/**
  * @brief  Get the Internal Multi Speed oscillator (MSI) clock range in run mode.
  * @rmtoll ICSCR           MSIRANGE      LL_RCC_MSI_GetRange
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_MSIRANGE_0
  *         @arg @ref LL_RCC_MSIRANGE_1
  *         @arg @ref LL_RCC_MSIRANGE_2
  *         @arg @ref LL_RCC_MSIRANGE_3
  *         @arg @ref LL_RCC_MSIRANGE_4
  *         @arg @ref LL_RCC_MSIRANGE_5
  *         @arg @ref LL_RCC_MSIRANGE_6
  */
__STATIC_INLINE uint32_t LL_RCC_MSI_GetRange(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_MSIRANGE));
}

/**
  * @brief  Get MSI Calibration value
  * @note When MSITRIM is written, MSICAL is updated with the sum of
  *       MSITRIM and the factory trim value
  * @rmtoll ICSCR        MSICAL        LL_RCC_MSI_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_MSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_MSICAL) >> RCC_POSITION_MSICAL);
}

/**
  * @brief  Set MSI Calibration trimming
  * @note user-programmable trimming value that is added to the MSICAL
  * @rmtoll ICSCR        MSITRIM       LL_RCC_MSI_SetCalibTrimming
  * @param  Value between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_MSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSITRIM, Value << RCC_POSITION_MSITRIM);
}

/**
  * @brief  Get MSI Calibration trimming
  * @rmtoll ICSCR        MSITRIM       LL_RCC_MSI_GetCalibTrimming
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_MSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_MSITRIM) >> RCC_POSITION_MSITRIM);
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_MSI
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_MSI
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
  * @brief  Set Clock After Wake-Up From Stop mode
  * @rmtoll CFGR         STOPWUCK      LL_RCC_SetClkAfterWakeFromStop
  * @param  Clock This parameter can be one of the following values:
  *         @arg @ref LL_RCC_STOP_WAKEUPCLOCK_MSI
  *         @arg @ref LL_RCC_STOP_WAKEUPCLOCK_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetClkAfterWakeFromStop(uint32_t Clock)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_STOPWUCK, Clock);
}

/**
  * @brief  Get Clock After Wake-Up From Stop mode
  * @rmtoll CFGR         STOPWUCK      LL_RCC_GetClkAfterWakeFromStop
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_STOP_WAKEUPCLOCK_MSI
  *         @arg @ref LL_RCC_STOP_WAKEUPCLOCK_HSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetClkAfterWakeFromStop(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_STOPWUCK));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_MCO MCO
  * @{
  */

/**
  * @brief  Configure MCOx
  * @rmtoll CFGR         MCOSEL        LL_RCC_ConfigMCO\n
  *         CFGR         MCOPRE        LL_RCC_ConfigMCO
  * @param  MCOxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1SOURCE_NOCLOCK
  *         @arg @ref LL_RCC_MCO1SOURCE_SYSCLK
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI
  *         @arg @ref LL_RCC_MCO1SOURCE_MSI
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_PLLCLK
  *         @arg @ref LL_RCC_MCO1SOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI48 (*)
  *
  *         (*) value not defined in all devices.
  * @param  MCOxPrescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1_DIV_1
  *         @arg @ref LL_RCC_MCO1_DIV_2
  *         @arg @ref LL_RCC_MCO1_DIV_4
  *         @arg @ref LL_RCC_MCO1_DIV_8
  *         @arg @ref LL_RCC_MCO1_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ConfigMCO(uint32_t MCOxSource, uint32_t MCOxPrescaler)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE, MCOxSource | MCOxPrescaler);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_Peripheral_Clock_Source Peripheral Clock Source
  * @{
  */

/**
  * @brief  Configure USARTx clock source
  * @rmtoll CCIPR        USARTxSEL     LL_RCC_SetUSARTClockSource
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK2 (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSARTClockSource(uint32_t USARTxSource)
{
  MODIFY_REG(RCC->CCIPR, (USARTxSource >> 16U), (USARTxSource & 0x0000FFFFU));
}

/**
  * @brief  Configure LPUART1x clock source
  * @rmtoll CCIPR        LPUART1SEL    LL_RCC_SetLPUARTClockSource
  * @param  LPUARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetLPUARTClockSource(uint32_t LPUARTxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, LPUARTxSource);
}

/**
  * @brief  Configure I2Cx clock source
  * @rmtoll CCIPR        I2CxSEL       LL_RCC_SetI2CClockSource
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_PCLK1 (*)
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_HSI (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2CClockSource(uint32_t I2CxSource)
{
  MODIFY_REG(RCC->CCIPR, ((I2CxSource >> 4U) & 0x000FF000U), ((I2CxSource << 4U) & 0x000FF000U));
}

/**
  * @brief  Configure LPTIMx clock source
  * @rmtoll CCIPR        LPTIMxSEL     LL_RCC_SetLPTIMClockSource
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetLPTIMClockSource(uint32_t LPTIMxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL, LPTIMxSource);
}

#if defined(RCC_CCIPR_HSI48SEL)
#if defined(RNG)
/**
  * @brief  Configure RNG clock source
  * @rmtoll CCIPR        HSI48SEL      LL_RCC_SetRNGClockSource
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_HSI48
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRNGClockSource(uint32_t RNGxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, RNGxSource);
}
#endif /* RNG */

#if defined(USB)
/**
  * @brief  Configure USB clock source
  * @rmtoll CCIPR        HSI48SEL      LL_RCC_SetUSBClockSource
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_HSI48
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSBClockSource(uint32_t USBxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, USBxSource);
}
#endif /* USB */

#endif /* RCC_CCIPR_HSI48SEL */

/**
  * @brief  Get USARTx clock source
  * @rmtoll CCIPR        USARTxSEL     LL_RCC_GetUSARTClockSource
  * @param  USARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK2 (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI (*)
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE (*)
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSARTClockSource(uint32_t USARTx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, USARTx) | (USARTx << 16U));
}



/**
  * @brief  Get LPUARTx clock source
  * @rmtoll CCIPR        LPUART1SEL    LL_RCC_GetLPUARTClockSource
  * @param  LPUARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetLPUARTClockSource(uint32_t LPUARTx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, LPUARTx));
}

/**
  * @brief  Get I2Cx clock source
  * @rmtoll CCIPR        I2CxSEL       LL_RCC_GetI2CClockSource
  * @param  I2Cx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_PCLK1  (*)
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_SYSCLK  (*)
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE_HSI   (*)
  *
  *         (*) value not defined in all devices.
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2CClockSource(uint32_t I2Cx)
{
  return (uint32_t)((READ_BIT(RCC->CCIPR, I2Cx) >> 4U) | (I2Cx << 4U));
}

/**
  * @brief  Get LPTIMx clock source
  * @rmtoll CCIPR        LPTIMxSEL     LL_RCC_GetLPTIMClockSource
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
  return (uint32_t)(READ_BIT(RCC->CCIPR, LPTIMx));
}

#if defined(RCC_CCIPR_HSI48SEL)
#if defined(RNG)
/**
  * @brief  Get RNGx clock source
  * @rmtoll CCIPR        CLK48SEL      LL_RCC_GetRNGClockSource
  * @param  RNGx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_HSI48
  */
__STATIC_INLINE uint32_t LL_RCC_GetRNGClockSource(uint32_t RNGx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, RNGx));
}
#endif /* RNG */

#if defined(USB)
/**
  * @brief  Get USBx clock source
  * @rmtoll CCIPR        CLK48SEL      LL_RCC_GetUSBClockSource
  * @param  USBx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_USB_CLKSOURCE_HSI48
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSBClockSource(uint32_t USBx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, USBx));
}
#endif /* USB */

#endif /* RCC_CCIPR_HSI48SEL */

/**
  * @}
  */

/** @defgroup RCC_LL_EF_RTC RTC
  * @{
  */

/**
  * @brief  Set RTC Clock Source
  * @note Once the RTC clock source has been selected, it cannot be changed any more unless
  *       the Backup domain is reset, or unless a failure is detected on LSE (LSECSSD is
  *       set). The RTCRST bit can be used to reset them.
  * @rmtoll CSR         RTCSEL        LL_RCC_SetRTCClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTCClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->CSR, RCC_CSR_RTCSEL, Source);
}

/**
  * @brief  Get RTC Clock Source
  * @rmtoll CSR         RTCSEL        LL_RCC_GetRTCClockSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTCClockSource(void)
{
  return (uint32_t)(READ_BIT(RCC->CSR, RCC_CSR_RTCSEL));
}

/**
  * @brief  Enable RTC
  * @rmtoll CSR         RTCEN         LL_RCC_EnableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableRTC(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_RTCEN);
}

/**
  * @brief  Disable RTC
  * @rmtoll CSR         RTCEN         LL_RCC_DisableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableRTC(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_RTCEN);
}

/**
  * @brief  Check if RTC has been enabled or not
  * @rmtoll CSR         RTCEN         LL_RCC_IsEnabledRTC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledRTC(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_RTCEN) == (RCC_CSR_RTCEN));
}

/**
  * @brief  Force the Backup domain reset
  * @rmtoll CSR         RTCRST         LL_RCC_ForceBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ForceBackupDomainReset(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_RTCRST);
}

/**
  * @brief  Release the Backup domain reset
  * @rmtoll CSR         RTCRST         LL_RCC_ReleaseBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ReleaseBackupDomainReset(void)
{
  CLEAR_BIT(RCC->CSR, RCC_CSR_RTCRST);
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
  * @rmtoll CFGR         PLLSRC        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR         PLLMUL        LL_RCC_PLL_ConfigDomain_SYS\n
  *         CFGR         PLLDIV        LL_RCC_PLL_ConfigDomain_SYS
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLMul This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_16
  *         @arg @ref LL_RCC_PLL_MUL_24
  *         @arg @ref LL_RCC_PLL_MUL_32
  *         @arg @ref LL_RCC_PLL_MUL_48
  * @param  PLLDiv This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL_DIV_2
  *         @arg @ref LL_RCC_PLL_DIV_3
  *         @arg @ref LL_RCC_PLL_DIV_4
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_SYS(uint32_t Source, uint32_t PLLMul, uint32_t PLLDiv)
{
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV, Source | PLLMul | PLLDiv);
}

/**
  * @brief  Get the oscillator used as PLL clock source.
  * @rmtoll CFGR         PLLSRC        LL_RCC_PLL_GetMainSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetMainSource(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLSRC));
}

/**
  * @brief  Get PLL multiplication Factor
  * @rmtoll CFGR         PLLMUL        LL_RCC_PLL_GetMultiplicator
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL_MUL_3
  *         @arg @ref LL_RCC_PLL_MUL_4
  *         @arg @ref LL_RCC_PLL_MUL_6
  *         @arg @ref LL_RCC_PLL_MUL_8
  *         @arg @ref LL_RCC_PLL_MUL_12
  *         @arg @ref LL_RCC_PLL_MUL_16
  *         @arg @ref LL_RCC_PLL_MUL_24
  *         @arg @ref LL_RCC_PLL_MUL_32
  *         @arg @ref LL_RCC_PLL_MUL_48
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetMultiplicator(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLMUL));
}

/**
  * @brief  Get Division factor for the main PLL and other PLL
  * @rmtoll CFGR         PLLDIV        LL_RCC_PLL_GetDivider
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL_DIV_2
  *         @arg @ref LL_RCC_PLL_DIV_3
  *         @arg @ref LL_RCC_PLL_DIV_4
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetDivider(void)
{
  return (uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLDIV));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_FLAG_Management FLAG Management
  * @{
  */

/**
  * @brief  Clear LSI ready interrupt flag
  * @rmtoll CICR         LSIRDYC       LL_RCC_ClearFlag_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSIRDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_LSIRDYC);
}

/**
  * @brief  Clear LSE ready interrupt flag
  * @rmtoll CICR         LSERDYC       LL_RCC_ClearFlag_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSERDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_LSERDYC);
}

/**
  * @brief  Clear MSI ready interrupt flag
  * @rmtoll CICR         MSIRDYC       LL_RCC_ClearFlag_MSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_MSIRDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_MSIRDYC);
}

/**
  * @brief  Clear HSI ready interrupt flag
  * @rmtoll CICR         HSIRDYC       LL_RCC_ClearFlag_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSIRDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_HSIRDYC);
}

/**
  * @brief  Clear HSE ready interrupt flag
  * @rmtoll CICR         HSERDYC       LL_RCC_ClearFlag_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSERDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_HSERDYC);
}

/**
  * @brief  Clear PLL ready interrupt flag
  * @rmtoll CICR         PLLRDYC       LL_RCC_ClearFlag_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLLRDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_PLLRDYC);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Clear HSI48 ready interrupt flag
  * @rmtoll CICR          HSI48RDYC     LL_RCC_ClearFlag_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSI48RDY(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_HSI48RDYC);
}
#endif /* RCC_HSI48_SUPPORT */

#if defined(RCC_HSECSS_SUPPORT)
/**
  * @brief  Clear Clock security system interrupt flag
  * @rmtoll CICR         CSSC          LL_RCC_ClearFlag_HSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSECSS(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_CSSC);
}
#endif /* RCC_HSECSS_SUPPORT */

/**
  * @brief  Clear LSE Clock security system interrupt flag
  * @rmtoll CICR         LSECSSC       LL_RCC_ClearFlag_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSECSS(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_LSECSSC);
}

/**
  * @brief  Check if LSI ready interrupt occurred or not
  * @rmtoll CIFR         LSIRDYF       LL_RCC_IsActiveFlag_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSIRDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_LSIRDYF) == (RCC_CIFR_LSIRDYF));
}

/**
  * @brief  Check if LSE ready interrupt occurred or not
  * @rmtoll CIFR         LSERDYF       LL_RCC_IsActiveFlag_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSERDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_LSERDYF) == (RCC_CIFR_LSERDYF));
}

/**
  * @brief  Check if MSI ready interrupt occurred or not
  * @rmtoll CIFR         MSIRDYF       LL_RCC_IsActiveFlag_MSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_MSIRDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_MSIRDYF) == (RCC_CIFR_MSIRDYF));
}

/**
  * @brief  Check if HSI ready interrupt occurred or not
  * @rmtoll CIFR         HSIRDYF       LL_RCC_IsActiveFlag_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSIRDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_HSIRDYF) == (RCC_CIFR_HSIRDYF));
}

/**
  * @brief  Check if HSE ready interrupt occurred or not
  * @rmtoll CIFR         HSERDYF       LL_RCC_IsActiveFlag_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSERDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_HSERDYF) == (RCC_CIFR_HSERDYF));
}

/**
  * @brief  Check if PLL ready interrupt occurred or not
  * @rmtoll CIFR         PLLRDYF       LL_RCC_IsActiveFlag_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLLRDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_PLLRDYF) == (RCC_CIFR_PLLRDYF));
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Check if HSI48 ready interrupt occurred or not
  * @rmtoll CIFR          HSI48RDYF     LL_RCC_IsActiveFlag_HSI48RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSI48RDY(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_HSI48RDYF) == (RCC_CIFR_HSI48RDYF));
}
#endif /* RCC_HSI48_SUPPORT */

#if defined(RCC_HSECSS_SUPPORT)
/**
  * @brief  Check if Clock security system interrupt occurred or not
  * @rmtoll CIFR         CSSF          LL_RCC_IsActiveFlag_HSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSECSS(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_CSSF) == (RCC_CIFR_CSSF));
}
#endif /* RCC_HSECSS_SUPPORT */

/**
  * @brief  Check if LSE Clock security system interrupt occurred or not
  * @rmtoll CIFR         LSECSSF       LL_RCC_IsActiveFlag_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSECSS(void)
{
  return (READ_BIT(RCC->CIFR, RCC_CIFR_LSECSSF) == (RCC_CIFR_LSECSSF));
}

/**
  * @brief  Check if HSI Divider is enabled (it divides by 4)
  * @rmtoll CR        HSIDIVF        LL_RCC_IsActiveFlag_HSIDIV
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSIDIV(void)
{
  return (READ_BIT(RCC->CR, RCC_CR_HSIDIVF) == (RCC_CR_HSIDIVF));
}

#if defined(RCC_CSR_FWRSTF)
/**
  * @brief  Check if RCC flag FW reset is set or not.
  * @rmtoll CSR          FWRSTF        LL_RCC_IsActiveFlag_FWRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_FWRST(void)
{
  return (READ_BIT(RCC->CSR, RCC_CSR_FWRSTF) == (RCC_CSR_FWRSTF));
}
#endif /* RCC_CSR_FWRSTF */

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
  * @rmtoll CIER         LSIRDYIE      LL_RCC_EnableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSIRDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_LSIRDYIE);
}

/**
  * @brief  Enable LSE ready interrupt
  * @rmtoll CIER         LSERDYIE      LL_RCC_EnableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSERDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_LSERDYIE);
}

/**
  * @brief  Enable MSI ready interrupt
  * @rmtoll CIER         MSIRDYIE      LL_RCC_EnableIT_MSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_MSIRDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_MSIRDYIE);
}

/**
  * @brief  Enable HSI ready interrupt
  * @rmtoll CIER         HSIRDYIE      LL_RCC_EnableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSIRDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_HSIRDYIE);
}

/**
  * @brief  Enable HSE ready interrupt
  * @rmtoll CIER         HSERDYIE      LL_RCC_EnableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSERDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_HSERDYIE);
}

/**
  * @brief  Enable PLL ready interrupt
  * @rmtoll CIER         PLLRDYIE      LL_RCC_EnableIT_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLLRDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_PLLRDYIE);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Enable HSI48 ready interrupt
  * @rmtoll CIER          HSI48RDYIE    LL_RCC_EnableIT_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSI48RDY(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_HSI48RDYIE);
}
#endif /* RCC_HSI48_SUPPORT */

/**
  * @brief  Enable LSE clock security system interrupt
  * @rmtoll CIER         LSECSSIE      LL_RCC_EnableIT_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSECSS(void)
{
  SET_BIT(RCC->CIER, RCC_CIER_LSECSSIE);
}

/**
  * @brief  Disable LSI ready interrupt
  * @rmtoll CIER         LSIRDYIE      LL_RCC_DisableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSIRDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_LSIRDYIE);
}

/**
  * @brief  Disable LSE ready interrupt
  * @rmtoll CIER         LSERDYIE      LL_RCC_DisableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSERDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_LSERDYIE);
}

/**
  * @brief  Disable MSI ready interrupt
  * @rmtoll CIER         MSIRDYIE      LL_RCC_DisableIT_MSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_MSIRDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_MSIRDYIE);
}

/**
  * @brief  Disable HSI ready interrupt
  * @rmtoll CIER         HSIRDYIE      LL_RCC_DisableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSIRDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_HSIRDYIE);
}

/**
  * @brief  Disable HSE ready interrupt
  * @rmtoll CIER         HSERDYIE      LL_RCC_DisableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSERDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_HSERDYIE);
}

/**
  * @brief  Disable PLL ready interrupt
  * @rmtoll CIER         PLLRDYIE      LL_RCC_DisableIT_PLLRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLLRDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_PLLRDYIE);
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Disable HSI48 ready interrupt
  * @rmtoll CIER          HSI48RDYIE    LL_RCC_DisableIT_HSI48RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSI48RDY(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_HSI48RDYIE);
}
#endif /* RCC_HSI48_SUPPORT */

/**
  * @brief  Disable LSE clock security system interrupt
  * @rmtoll CIER         LSECSSIE      LL_RCC_DisableIT_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSECSS(void)
{
  CLEAR_BIT(RCC->CIER, RCC_CIER_LSECSSIE);
}

/**
  * @brief  Checks if LSI ready interrupt source is enabled or disabled.
  * @rmtoll CIER         LSIRDYIE      LL_RCC_IsEnabledIT_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSIRDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_LSIRDYIE) == (RCC_CIER_LSIRDYIE));
}

/**
  * @brief  Checks if LSE ready interrupt source is enabled or disabled.
  * @rmtoll CIER         LSERDYIE      LL_RCC_IsEnabledIT_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSERDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_LSERDYIE) == (RCC_CIER_LSERDYIE));
}

/**
  * @brief  Checks if MSI ready interrupt source is enabled or disabled.
  * @rmtoll CIER         MSIRDYIE      LL_RCC_IsEnabledIT_MSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_MSIRDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_MSIRDYIE) == (RCC_CIER_MSIRDYIE));
}

/**
  * @brief  Checks if HSI ready interrupt source is enabled or disabled.
  * @rmtoll CIER         HSIRDYIE      LL_RCC_IsEnabledIT_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSIRDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_HSIRDYIE) == (RCC_CIER_HSIRDYIE));
}

/**
  * @brief  Checks if HSE ready interrupt source is enabled or disabled.
  * @rmtoll CIER         HSERDYIE      LL_RCC_IsEnabledIT_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSERDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_HSERDYIE) == (RCC_CIER_HSERDYIE));
}

/**
  * @brief  Checks if PLL ready interrupt source is enabled or disabled.
  * @rmtoll CIER         PLLRDYIE      LL_RCC_IsEnabledIT_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLLRDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_PLLRDYIE) == (RCC_CIER_PLLRDYIE));
}

#if defined(RCC_HSI48_SUPPORT)
/**
  * @brief  Checks if HSI48 ready interrupt source is enabled or disabled.
  * @rmtoll CIER          HSI48RDYIE    LL_RCC_IsEnabledIT_HSI48RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSI48RDY(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_HSI48RDYIE) == (RCC_CIER_HSI48RDYIE));
}
#endif /* RCC_HSI48_SUPPORT */

/**
  * @brief  Checks if LSECSS interrupt source is enabled or disabled.
  * @rmtoll CIER         LSECSSIE      LL_RCC_IsEnabledIT_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSECSS(void)
{
  return (READ_BIT(RCC->CIER, RCC_CIER_LSECSSIE) == (RCC_CIER_LSECSSIE));
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
uint32_t    LL_RCC_GetI2CClockFreq(uint32_t I2CxSource);
uint32_t    LL_RCC_GetLPUARTClockFreq(uint32_t LPUARTxSource);
uint32_t    LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource);
#if defined(USB_OTG_FS) || defined(USB)
uint32_t    LL_RCC_GetUSBClockFreq(uint32_t USBxSource);
#endif /* USB_OTG_FS || USB */
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

#endif /* __STM32L0xx_LL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
