/**
  ******************************************************************************
  * @file    stm32g0xx_ll_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC LL module.
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
#ifndef STM32G0xx_LL_RCC_H
#define STM32G0xx_LL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
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
#define HSE_VALUE    8000000U   /*!< Value of the HSE oscillator in Hz */
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
#define EXTERNAL_CLOCK_VALUE    48000U /*!< Value of the I2S_CKIN external oscillator in Hz */
#endif /* EXTERNAL_CLOCK_VALUE */
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
#define LL_RCC_CIFR_LSECSSF                RCC_CIFR_LSECSSF     /*!< LSE Clock Security System Interrupt flag */
#define LL_RCC_CIFR_CSSF                   RCC_CIFR_CSSF        /*!< Clock Security System Interrupt flag */
#define LL_RCC_CSR_LPWRRSTF                RCC_CSR_LPWRRSTF   /*!< Low-Power reset flag */
#define LL_RCC_CSR_OBLRSTF                 RCC_CSR_OBLRSTF    /*!< OBL reset flag */
#define LL_RCC_CSR_PINRSTF                 RCC_CSR_PINRSTF    /*!< PIN reset flag */
#define LL_RCC_CSR_SFTRSTF                 RCC_CSR_SFTRSTF    /*!< Software Reset flag */
#define LL_RCC_CSR_IWDGRSTF                RCC_CSR_IWDGRSTF   /*!< Independent Watchdog reset flag */
#define LL_RCC_CSR_WWDGRSTF                RCC_CSR_WWDGRSTF   /*!< Window watchdog reset flag */
#define LL_RCC_CSR_PWRRSTF                 RCC_CSR_PWRRSTF    /*!< BOR or POR/PDR reset flag */
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
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSEDRIVE  LSE oscillator drive capability
  * @{
  */
#define LL_RCC_LSEDRIVE_LOW                0x00000000U             /*!< Xtal mode lower driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMLOW          RCC_BDCR_LSEDRV_0       /*!< Xtal mode medium low driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMHIGH         RCC_BDCR_LSEDRV_1       /*!< Xtal mode medium high driving capability */
#define LL_RCC_LSEDRIVE_HIGH               RCC_BDCR_LSEDRV         /*!< Xtal mode higher driving capability */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSCO_CLKSOURCE  LSCO Selection
  * @{
  */
#define LL_RCC_LSCO_CLKSOURCE_LSI          0x00000000U                 /*!< LSI selection for low speed clock  */
#define LL_RCC_LSCO_CLKSOURCE_LSE          RCC_BDCR_LSCOSEL            /*!< LSE selection for low speed clock  */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE  System clock switch
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_HSI           0x00000000U                        /*!< HSI selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_HSE           RCC_CFGR_SW_0                      /*!< HSE selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_PLL           RCC_CFGR_SW_1                      /*!< PLL selection as system clock */
#define LL_RCC_SYS_CLKSOURCE_LSI           (RCC_CFGR_SW_1 | RCC_CFGR_SW_0)    /*!< LSI selection used as system clock */
#define LL_RCC_SYS_CLKSOURCE_LSE           RCC_CFGR_SW_2                      /*!< LSE selection used as system clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYS_CLKSOURCE_STATUS  System clock switch status
  * @{
  */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSI    0x00000000U                         /*!< HSI used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_HSE    RCC_CFGR_SWS_0                      /*!< HSE used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_PLL    RCC_CFGR_SWS_1                      /*!< PLL used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_LSI    (RCC_CFGR_SWS_1 | RCC_CFGR_SWS_0)   /*!< LSI used as system clock */
#define LL_RCC_SYS_CLKSOURCE_STATUS_LSE    RCC_CFGR_SWS_2                      /*!< LSE used as system clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SYSCLK_DIV  AHB prescaler
  * @{
  */
#define LL_RCC_SYSCLK_DIV_1                0x00000000U                                                             /*!< SYSCLK not divided */
#define LL_RCC_SYSCLK_DIV_2                RCC_CFGR_HPRE_3                                                         /*!< SYSCLK divided by 2 */
#define LL_RCC_SYSCLK_DIV_4                (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_0)                                     /*!< SYSCLK divided by 4 */
#define LL_RCC_SYSCLK_DIV_8                (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1)                                     /*!< SYSCLK divided by 8 */
#define LL_RCC_SYSCLK_DIV_16               (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0)                   /*!< SYSCLK divided by 16 */
#define LL_RCC_SYSCLK_DIV_64               (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2)                                     /*!< SYSCLK divided by 64 */
#define LL_RCC_SYSCLK_DIV_128              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_0)                   /*!< SYSCLK divided by 128 */
#define LL_RCC_SYSCLK_DIV_256              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1)                   /*!< SYSCLK divided by 256 */
#define LL_RCC_SYSCLK_DIV_512              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0) /*!< SYSCLK divided by 512 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB1_DIV  APB low-speed prescaler (APB1)
  * @{
  */
#define LL_RCC_APB1_DIV_1                  0x00000000U                                           /*!< HCLK not divided */
#define LL_RCC_APB1_DIV_2                  RCC_CFGR_PPRE_2                                       /*!< HCLK divided by 2 */
#define LL_RCC_APB1_DIV_4                  (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_0)                   /*!< HCLK divided by 4 */
#define LL_RCC_APB1_DIV_8                  (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1)                   /*!< HCLK divided by 8 */
#define LL_RCC_APB1_DIV_16                 (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1 | RCC_CFGR_PPRE_0) /*!< HCLK divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_HSI_DIV  HSI division factor
  * @{
  */
#define LL_RCC_HSI_DIV_1                  0x00000000U                                /*!< HSI not divided */
#define LL_RCC_HSI_DIV_2                  RCC_CR_HSIDIV_0                            /*!< HSI divided by 2 */
#define LL_RCC_HSI_DIV_4                  RCC_CR_HSIDIV_1                            /*!< HSI divided by 4 */
#define LL_RCC_HSI_DIV_8                  (RCC_CR_HSIDIV_1 | RCC_CR_HSIDIV_0)        /*!< HSI divided by 8 */
#define LL_RCC_HSI_DIV_16                 RCC_CR_HSIDIV_2                            /*!< HSI divided by 16 */
#define LL_RCC_HSI_DIV_32                 (RCC_CR_HSIDIV_2 | RCC_CR_HSIDIV_0)        /*!< HSI divided by 32 */
#define LL_RCC_HSI_DIV_64                 (RCC_CR_HSIDIV_2 | RCC_CR_HSIDIV_1)        /*!< HSI divided by 64 */
#define LL_RCC_HSI_DIV_128                RCC_CR_HSIDIV                              /*!< HSI divided by 128 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1SOURCE  MCO1 SOURCE selection
  * @{
  */
#define LL_RCC_MCO1SOURCE_NOCLOCK          0x00000000U                            /*!< MCO output disabled, no clock on MCO */
#define LL_RCC_MCO1SOURCE_SYSCLK           RCC_CFGR_MCOSEL_0                      /*!< SYSCLK selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_HSI              (RCC_CFGR_MCOSEL_0| RCC_CFGR_MCOSEL_1) /*!< HSI16 selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_HSE              RCC_CFGR_MCOSEL_2                      /*!< HSE selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_PLLCLK           (RCC_CFGR_MCOSEL_0|RCC_CFGR_MCOSEL_2)  /*!< Main PLL selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_LSI              (RCC_CFGR_MCOSEL_1|RCC_CFGR_MCOSEL_2)  /*!< LSI selection as MCO1 source */
#define LL_RCC_MCO1SOURCE_LSE              (RCC_CFGR_MCOSEL_0|RCC_CFGR_MCOSEL_1|RCC_CFGR_MCOSEL_2) /*!< LSE selection as MCO1 source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1_DIV  MCO1 prescaler
  * @{
  */
#define LL_RCC_MCO1_DIV_1                  0x00000000U                                                 /*!< MCO1 not divided */
#define LL_RCC_MCO1_DIV_2                  RCC_CFGR_MCOPRE_0                                           /*!< MCO1 divided by 2 */
#define LL_RCC_MCO1_DIV_4                  RCC_CFGR_MCOPRE_1                                           /*!< MCO1 divided by 4 */
#define LL_RCC_MCO1_DIV_8                  (RCC_CFGR_MCOPRE_1 | RCC_CFGR_MCOPRE_0)                     /*!< MCO1 divided by 8 */
#define LL_RCC_MCO1_DIV_16                 RCC_CFGR_MCOPRE_2                                           /*!< MCO1 divided by 16 */
#define LL_RCC_MCO1_DIV_32                 (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_0)                     /*!< MCO1 divided by 32 */
#define LL_RCC_MCO1_DIV_64                 (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_1)                     /*!< MCO1 divided by 64 */
#define LL_RCC_MCO1_DIV_128                (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_1 | RCC_CFGR_MCOPRE_0) /*!< MCO1 divided by 128 */
/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EC_PERIPH_FREQUENCY Peripheral clock frequency
  * @{
  */
#define LL_RCC_PERIPH_FREQUENCY_NO        0x00000000U                 /*!< No clock enabled for the peripheral            */
#define LL_RCC_PERIPH_FREQUENCY_NA        0xFFFFFFFFU                 /*!< Frequency cannot be provided as external clock */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/** @defgroup RCC_LL_EC_USARTx_CLKSOURCE  Peripheral USART clock source selection
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE_PCLK1      ((RCC_CCIPR_USART1SEL << 16U) | 0x00000000U)            /*!< PCLK1 clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_SYSCLK     ((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL_0)  /*!< SYSCLK clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_HSI        ((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL_1)  /*!< HSI clock used as USART1 clock source */
#define LL_RCC_USART1_CLKSOURCE_LSE        ((RCC_CCIPR_USART1SEL << 16U) | RCC_CCIPR_USART1SEL)    /*!< LSE clock used as USART1 clock source */
#define LL_RCC_USART2_CLKSOURCE_PCLK1      ((RCC_CCIPR_USART2SEL << 16U) | 0x00000000U)            /*!< PCLK1 clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_SYSCLK     ((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL_0)  /*!< SYSCLK clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_HSI        ((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL_1)  /*!< HSI clock used as USART2 clock source */
#define LL_RCC_USART2_CLKSOURCE_LSE        ((RCC_CCIPR_USART2SEL << 16U) | RCC_CCIPR_USART2SEL)    /*!< LSE clock used as USART2 clock source */
/**
  * @}
  */

#if defined(LPUART1)
/** @defgroup RCC_LL_EC_LPUART1_CLKSOURCE Peripheral LPUART clock source selection
  * @{
  */
#define LL_RCC_LPUART1_CLKSOURCE_PCLK1     0x00000000U                     /*!< PCLK1 clock used as LPUART1 clock source */
#define LL_RCC_LPUART1_CLKSOURCE_SYSCLK    RCC_CCIPR_LPUART1SEL_0          /*!< SYSCLK clock used as LPUART1 clock source */
#define LL_RCC_LPUART1_CLKSOURCE_HSI       RCC_CCIPR_LPUART1SEL_1          /*!< HSI clock used as LPUART1 clock source */
#define LL_RCC_LPUART1_CLKSOURCE_LSE       RCC_CCIPR_LPUART1SEL            /*!< LSE clock used as LPUART1 clock source */
/**
  * @}
  */
#endif /* LPUART1 */

/** @defgroup RCC_LL_EC_I2C1_CLKSOURCE Peripheral I2C clock source selection
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE_PCLK1        0x00000000U                  /*!< PCLK1 clock used as I2C1 clock source */
#define LL_RCC_I2C1_CLKSOURCE_SYSCLK       RCC_CCIPR_I2C1SEL_0          /*!< SYSCLK clock used as I2C1 clock source */
#define LL_RCC_I2C1_CLKSOURCE_HSI          RCC_CCIPR_I2C1SEL_1          /*!< HSI clock used as I2C1 clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2S1_CLKSOURCE Peripheral I2S clock source selection
  * @{
  */
#define LL_RCC_I2S1_CLKSOURCE_SYSCLK      0x00000000U                  /*!< SYSCLK clock used as I2S1 clock source */
#define LL_RCC_I2S1_CLKSOURCE_PLL         RCC_CCIPR_I2S1SEL_0          /*!< PLL clock used as I2S1 clock source */
#define LL_RCC_I2S1_CLKSOURCE_HSI         RCC_CCIPR_I2S1SEL_1          /*!< HSI clock used as I2S1 clock source */
#define LL_RCC_I2S1_CLKSOURCE_PIN         RCC_CCIPR_I2S1SEL            /*!< External clock used as I2S1 clock source */


/**
  * @}
  */

#if defined(RCC_CCIPR_TIM1SEL)
/** @defgroup RCC_LL_EC_TIMx_CLKSOURCE Peripheral TIM clock source selection
  * @{
  */
#define LL_RCC_TIM1_CLKSOURCE_PCLK1        (RCC_CCIPR_TIM1SEL   | (0x00000000U >> 16U))          /*!< PCLK1 clock used as TIM1 clock source */
#define LL_RCC_TIM1_CLKSOURCE_PLL          (RCC_CCIPR_TIM1SEL   | (RCC_CCIPR_TIM1SEL >> 16U))    /*!< PLL used as TIM1 clock source */
/**
  * @}
  */
#endif /* RCC_CCIPR_TIM1SEL */

#if defined(RCC_CCIPR_TIM15SEL)
/** @addtogroup RCC_LL_EC_TIMx_CLKSOURCE
  * @{
  */
#define LL_RCC_TIM15_CLKSOURCE_PCLK1       (RCC_CCIPR_TIM15SEL  | (0x00000000U >> 16U))          /*!< PCLK1 clock used as TIM15 clock source */
#define LL_RCC_TIM15_CLKSOURCE_PLL         (RCC_CCIPR_TIM15SEL  | (RCC_CCIPR_TIM15SEL >> 16U))   /*!< PLL used as TIM15 clock source */
/**
  * @}
  */
#endif /* RCC_CCIPR_TIM15SEL */

#if defined(LPTIM1) && defined(LPTIM2)
/** @defgroup RCC_LL_EC_LPTIMx_CLKSOURCE Peripheral LPTIM clock source selection
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE_PCLK1      (RCC_CCIPR_LPTIM1SEL | (0x00000000U >> 16U))           /*!< PCLK1 selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSI        (RCC_CCIPR_LPTIM1SEL | (RCC_CCIPR_LPTIM1SEL_0 >> 16U)) /*!< LSI selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_HSI        (RCC_CCIPR_LPTIM1SEL | (RCC_CCIPR_LPTIM1SEL_1 >> 16U)) /*!< HSI selected as LPTIM1 clock */
#define LL_RCC_LPTIM1_CLKSOURCE_LSE        (RCC_CCIPR_LPTIM1SEL | (RCC_CCIPR_LPTIM1SEL >> 16U))   /*!< LSE selected as LPTIM1 clock */
#define LL_RCC_LPTIM2_CLKSOURCE_PCLK1      (RCC_CCIPR_LPTIM2SEL | (0x00000000U >> 16U))           /*!< PCLK1 selected as LPTIM2 clock */
#define LL_RCC_LPTIM2_CLKSOURCE_LSI        (RCC_CCIPR_LPTIM2SEL | (RCC_CCIPR_LPTIM2SEL_0 >> 16U)) /*!< LSI selected as LPTIM2 clock */
#define LL_RCC_LPTIM2_CLKSOURCE_HSI        (RCC_CCIPR_LPTIM2SEL | (RCC_CCIPR_LPTIM2SEL_1 >> 16U)) /*!< HSI selected as LPTIM2 clock */
#define LL_RCC_LPTIM2_CLKSOURCE_LSE        (RCC_CCIPR_LPTIM2SEL | (RCC_CCIPR_LPTIM2SEL >> 16U))   /*!< LSE selected as LPTIM2 clock */
/**
  * @}
  */
#endif /* LPTIM1 && LPTIM2*/

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC_CLKSOURCE_HSI Peripheral CEC clock source selection
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE_HSI_DIV488    0x00000000U                   /*!< HSI oscillator clock divided by 488 used as CEC clock */
#define LL_RCC_CEC_CLKSOURCE_LSE           RCC_CCIPR_CECSEL              /*!< LSE oscillator clock used as CEC clock */

/**
  * @}
*/
#endif /* CEC */

#if defined(RNG)
/** @defgroup RCC_LL_EC_RNG_CLKSOURCE Peripheral RNG clock source selection
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE_NONE          0x00000000U                   /*!< No clock used as RNG clock */
#define LL_RCC_RNG_CLKSOURCE_HSI_DIV8      RCC_CCIPR_RNGSEL_0            /*!< HSI oscillator clock divided by 8 used as RNG clock, available on cut2.0 */
#define LL_RCC_RNG_CLKSOURCE_SYSCLK        RCC_CCIPR_RNGSEL_1            /*!< SYSCLK divided by 1 used as RNG clock */
#define LL_RCC_RNG_CLKSOURCE_PLL           RCC_CCIPR_RNGSEL              /*!< PLL used as RNG clock */
/**
  * @}
  */
#endif /* RNG */

#if defined(RNG)
/** @defgroup RCC_LL_EC_RNG_CLK_DIV Peripheral RNG clock division factor
  * @{
  */
#define LL_RCC_RNG_CLK_DIV1                0x00000000U                    /*!< RNG clock not divided  */
#define LL_RCC_RNG_CLK_DIV2                RCC_CCIPR_RNGDIV_0             /*!< RNG clock divided by 2 */
#define LL_RCC_RNG_CLK_DIV4                RCC_CCIPR_RNGDIV_1             /*!< RNG clock divided by 4 */
#define LL_RCC_RNG_CLK_DIV8                RCC_CCIPR_RNGDIV               /*!< RNG clock divided by 8 */
/**
  * @}
  */
#endif /* RNG */

/** @defgroup RCC_LL_EC_ADC_CLKSOURCE Peripheral ADC clock source selection
  * @{
  */
#define LL_RCC_ADC_CLKSOURCE_SYSCLK        0x00000000U                   /*!< SYSCLK used as ADC clock */
#define LL_RCC_ADC_CLKSOURCE_PLL          RCC_CCIPR_ADCSEL_0             /*!< PLL used as ADC clock */
#define LL_RCC_ADC_CLKSOURCE_HSI           RCC_CCIPR_ADCSEL              /*!< HSI used as ADC clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USARTx Peripheral USARTx get clock source
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE            RCC_CCIPR_USART1SEL /*!< USART1 Clock source selection */
#define LL_RCC_USART2_CLKSOURCE            RCC_CCIPR_USART2SEL /*!< USART2 Clock source selection */
/**
  * @}
  */

#if defined(LPUART1)
/** @defgroup RCC_LL_EC_LPUART1 Peripheral LPUART get clock source
  * @{
  */
#define LL_RCC_LPUART1_CLKSOURCE           RCC_CCIPR_LPUART1SEL /*!< LPUART1 Clock source selection */
/**
  * @}
  */
#endif /* LPUART1 */

/** @defgroup RCC_LL_EC_I2C1 Peripheral I2C get clock source
  * @{
  */
#define LL_RCC_I2C1_CLKSOURCE              RCC_CCIPR_I2C1SEL /*!< I2C1 Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2S1 Peripheral I2S get clock source
  * @{
  */
#define LL_RCC_I2S1_CLKSOURCE              RCC_CCIPR_I2S1SEL /*!< I2S1 Clock source selection */
/**
  * @}
  */

#if defined(RCC_CCIPR_TIM1SEL)
/** @defgroup RCC_LL_EC_TIMx Peripheral TIMx get clock source
  * @{
  */
#define LL_RCC_TIM1_CLKSOURCE              RCC_CCIPR_TIM1SEL   /*!< TIM1 Clock source selection */
#if defined(RCC_CCIPR_TIM15SEL)
#define LL_RCC_TIM15_CLKSOURCE             RCC_CCIPR_TIM15SEL  /*!< TIM15 Clock source selection */
#endif /* RCC_CCIPR_TIM15SEL */
/**
  * @}
  */
#endif /* RCC_CCIPR_TIM1SEL */

#if defined(LPTIM1) && defined(LPTIM2)
/** @defgroup RCC_LL_EC_LPTIM1 Peripheral LPTIM get clock source
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE            RCC_CCIPR_LPTIM1SEL /*!< LPTIM2 Clock source selection */
#define LL_RCC_LPTIM2_CLKSOURCE            RCC_CCIPR_LPTIM2SEL /*!< LPTIM2 Clock source selection */
/**
  * @}
  */
#endif /* LPTIM1 && LPTIM2 */

#if defined(CEC)
/** @defgroup RCC_LL_EC_CEC Peripheral CEC get clock source
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE               RCC_CCIPR_CECSEL        /*!< CEC Clock source selection */
/**
  * @}
  */
#endif /* CEC */

#if defined(RNG)
/** @defgroup RCC_LL_EC_RNG Peripheral RNG get clock source
  * @{
  */
#define LL_RCC_RNG_CLKSOURCE               RCC_CCIPR_RNGSEL        /*!< RNG Clock source selection */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RNG_DIV Peripheral RNG get clock division factor
  * @{
  */
#define LL_RCC_RNG_CLKDIV                  RCC_CCIPR_RNGDIV        /*!< RNG Clock division factor */
/**
  * @}
  */
#endif /* RNG */

/** @defgroup RCC_LL_EC_ADC Peripheral ADC get clock source
  * @{
  */
#define LL_RCC_ADC_CLKSOURCE               RCC_CCIPR_ADCSEL        /*!< ADC Clock source selection */
/**
  * @}
  */

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


/** @defgroup RCC_LL_EC_PLLSOURCE  PLL entry clock source
  * @{
  */
#define LL_RCC_PLLSOURCE_NONE              0x00000000U             /*!< No clock */
#define LL_RCC_PLLSOURCE_HSI               RCC_PLLCFGR_PLLSRC_HSI  /*!< HSI16 clock selected as PLL entry clock source */
#define LL_RCC_PLLSOURCE_HSE               RCC_PLLCFGR_PLLSRC_HSE  /*!< HSE clock selected as PLL entry clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLM_DIV  PLL division factor (PLLM)
  * @{
  */
#define LL_RCC_PLLM_DIV_1                  0x00000000U                                 /*!< PLL division factor by 1 */
#define LL_RCC_PLLM_DIV_2                  (RCC_PLLCFGR_PLLM_0)                        /*!< PLL division factor by 2 */
#define LL_RCC_PLLM_DIV_3                  (RCC_PLLCFGR_PLLM_1)                        /*!< PLL division factor by 3 */
#define LL_RCC_PLLM_DIV_4                  ((RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0)) /*!< PLL division factor by 4 */
#define LL_RCC_PLLM_DIV_5                  (RCC_PLLCFGR_PLLM_2)                        /*!< PLL division factor by 5 */
#define LL_RCC_PLLM_DIV_6                  ((RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0)) /*!< PLL division factor by 6 */
#define LL_RCC_PLLM_DIV_7                  ((RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1)) /*!< PLL division factor by 7 */
#define LL_RCC_PLLM_DIV_8                  (RCC_PLLCFGR_PLLM)                          /*!< PLL division factor by 8 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLR_DIV  PLL division factor (PLLR)
  * @{
  */
#define LL_RCC_PLLR_DIV_2                  (RCC_PLLCFGR_PLLR_0)                     /*!< Main PLL division factor for PLLCLK (system clock) by 2 */
#define LL_RCC_PLLR_DIV_3                  (RCC_PLLCFGR_PLLR_1)                     /*!< Main PLL division factor for PLLCLK (system clock) by 3 */
#define LL_RCC_PLLR_DIV_4                  (RCC_PLLCFGR_PLLR_1|RCC_PLLCFGR_PLLR_0)  /*!< Main PLL division factor for PLLCLK (system clock) by 4 */
#define LL_RCC_PLLR_DIV_5                  (RCC_PLLCFGR_PLLR_2)                     /*!< Main PLL division factor for PLLCLK (system clock) by 5 */
#define LL_RCC_PLLR_DIV_6                  (RCC_PLLCFGR_PLLR_2|RCC_PLLCFGR_PLLR_0)  /*!< Main PLL division factor for PLLCLK (system clock) by 6 */
#define LL_RCC_PLLR_DIV_7                  (RCC_PLLCFGR_PLLR_2|RCC_PLLCFGR_PLLR_1)  /*!< Main PLL division factor for PLLCLK (system clock) by 7 */
#define LL_RCC_PLLR_DIV_8                  (RCC_PLLCFGR_PLLR)                       /*!< Main PLL division factor for PLLCLK (system clock) by 8 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLLP_DIV  PLL division factor (PLLP)
  * @{
  */
#define LL_RCC_PLLP_DIV_2                  (RCC_PLLCFGR_PLLP_0)                                              /*!< Main PLL division factor for PLLP output by 2 */
#define LL_RCC_PLLP_DIV_3                  (RCC_PLLCFGR_PLLP_1)                                              /*!< Main PLL division factor for PLLP output by 3 */
#define LL_RCC_PLLP_DIV_4                  (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1)                           /*!< Main PLL division factor for PLLP output by 4 */
#define LL_RCC_PLLP_DIV_5                  (RCC_PLLCFGR_PLLP_2)                                              /*!< Main PLL division factor for PLLP output by 5 */
#define LL_RCC_PLLP_DIV_6                  (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_2)                           /*!< Main PLL division factor for PLLP output by 6 */
#define LL_RCC_PLLP_DIV_7                  (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2)                           /*!< Main PLL division factor for PLLP output by 7 */
#define LL_RCC_PLLP_DIV_8                  (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2)        /*!< Main PLL division factor for PLLP output by 8 */
#define LL_RCC_PLLP_DIV_9                  (RCC_PLLCFGR_PLLP_3)                                              /*!< Main PLL division factor for PLLP output by 9 */
#define LL_RCC_PLLP_DIV_10                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_3)                           /*!< Main PLL division factor for PLLP output by 10 */
#define LL_RCC_PLLP_DIV_11                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_3)                           /*!< Main PLL division factor for PLLP output by 11 */
#define LL_RCC_PLLP_DIV_12                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_3)        /*!< Main PLL division factor for PLLP output by 12 */
#define LL_RCC_PLLP_DIV_13                 (RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3)                           /*!< Main PLL division factor for PLLP output by 13 */
#define LL_RCC_PLLP_DIV_14                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3)        /*!< Main PLL division factor for PLLP output by 14 */
#define LL_RCC_PLLP_DIV_15                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3)        /*!< Main PLL division factor for PLLP output by 15 */
#define LL_RCC_PLLP_DIV_16                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3)/*!< Main PLL division factor for PLLP output by 16 */
#define LL_RCC_PLLP_DIV_17                 (RCC_PLLCFGR_PLLP_4)                                              /*!< Main PLL division factor for PLLP output by 17 */
#define LL_RCC_PLLP_DIV_18                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_4)                           /*!< Main PLL division factor for PLLP output by 18 */
#define LL_RCC_PLLP_DIV_19                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_4)                           /*!< Main PLL division factor for PLLP output by 19 */
#define LL_RCC_PLLP_DIV_20                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 20 */
#define LL_RCC_PLLP_DIV_21                 (RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_4)                           /*!< Main PLL division factor for PLLP output by 21 */
#define LL_RCC_PLLP_DIV_22                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 22 */
#define LL_RCC_PLLP_DIV_23                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 23 */
#define LL_RCC_PLLP_DIV_24                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_4)/*!< Main PLL division factor for PLLP output by 24 */
#define LL_RCC_PLLP_DIV_25                 (RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)                           /*!< Main PLL division factor for PLLP output by 25 */
#define LL_RCC_PLLP_DIV_26                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 26 */
#define LL_RCC_PLLP_DIV_27                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 27*/
#define LL_RCC_PLLP_DIV_28                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)/*!< Main PLL division factor for PLLP output by 28 */
#define LL_RCC_PLLP_DIV_29                 (RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)        /*!< Main PLL division factor for PLLP output by 29 */
#define LL_RCC_PLLP_DIV_30                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)/*!< Main PLL division factor for PLLP output by 30 */
#define LL_RCC_PLLP_DIV_31                 (RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)/*!< Main PLL division factor for PLLP output by 31 */
#define LL_RCC_PLLP_DIV_32                 (RCC_PLLCFGR_PLLP_0|RCC_PLLCFGR_PLLP_1|RCC_PLLCFGR_PLLP_2|RCC_PLLCFGR_PLLP_3|RCC_PLLCFGR_PLLP_4)/*!< Main PLL division factor for PLLP output by 32 */
/**
  * @}
  */

#if defined(RCC_PLLQ_SUPPORT)
/** @defgroup RCC_LL_EC_PLLQ_DIV  PLL division factor (PLLQ)
  * @{
  */
#define LL_RCC_PLLQ_DIV_2                  (RCC_PLLCFGR_PLLQ_0)                    /*!< Main PLL division factor for PLLQ output by 2 */
#define LL_RCC_PLLQ_DIV_3                  (RCC_PLLCFGR_PLLQ_1)                    /*!< Main PLL division factor for PLLQ output by 3 */
#define LL_RCC_PLLQ_DIV_4                  (RCC_PLLCFGR_PLLQ_1|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 4 */
#define LL_RCC_PLLQ_DIV_5                  (RCC_PLLCFGR_PLLQ_2)                    /*!< Main PLL division factor for PLLQ output by 5 */
#define LL_RCC_PLLQ_DIV_6                  (RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_0) /*!< Main PLL division factor for PLLQ output by 6 */
#define LL_RCC_PLLQ_DIV_7                  (RCC_PLLCFGR_PLLQ_2|RCC_PLLCFGR_PLLQ_1) /*!< Main PLL division factor for PLLQ output by 7 */
#define LL_RCC_PLLQ_DIV_8                  (RCC_PLLCFGR_PLLQ)                      /*!< Main PLL division factor for PLLQ output by 8 */
/**
  * @}
  */
#endif

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
#define LL_RCC_WriteReg(__REG__, __VALUE__) WRITE_REG((RCC->__REG__), (__VALUE__))

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
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetR ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLR__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLR_DIV_2
  *         @arg @ref LL_RCC_PLLR_DIV_3
  *         @arg @ref LL_RCC_PLLR_DIV_4
  *         @arg @ref LL_RCC_PLLR_DIV_5
  *         @arg @ref LL_RCC_PLLR_DIV_6
  *         @arg @ref LL_RCC_PLLR_DIV_7
  *         @arg @ref LL_RCC_PLLR_DIV_8
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLR__) ((__INPUTFREQ__) * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLR__) >> RCC_PLLCFGR_PLLR_Pos) + 1U))

/**
  * @brief  Helper macro to calculate the PLLPCLK frequency used on I2S domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_I2S1_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetP ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLP__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_3
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_5
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_7
  *         @arg @ref LL_RCC_PLLP_DIV_8
  *         @arg @ref LL_RCC_PLLP_DIV_9
  *         @arg @ref LL_RCC_PLLP_DIV_10
  *         @arg @ref LL_RCC_PLLP_DIV_11
  *         @arg @ref LL_RCC_PLLP_DIV_12
  *         @arg @ref LL_RCC_PLLP_DIV_13
  *         @arg @ref LL_RCC_PLLP_DIV_14
  *         @arg @ref LL_RCC_PLLP_DIV_15
  *         @arg @ref LL_RCC_PLLP_DIV_16
  *         @arg @ref LL_RCC_PLLP_DIV_17
  *         @arg @ref LL_RCC_PLLP_DIV_18
  *         @arg @ref LL_RCC_PLLP_DIV_19
  *         @arg @ref LL_RCC_PLLP_DIV_20
  *         @arg @ref LL_RCC_PLLP_DIV_21
  *         @arg @ref LL_RCC_PLLP_DIV_22
  *         @arg @ref LL_RCC_PLLP_DIV_23
  *         @arg @ref LL_RCC_PLLP_DIV_24
  *         @arg @ref LL_RCC_PLLP_DIV_25
  *         @arg @ref LL_RCC_PLLP_DIV_26
  *         @arg @ref LL_RCC_PLLP_DIV_27
  *         @arg @ref LL_RCC_PLLP_DIV_28
  *         @arg @ref LL_RCC_PLLP_DIV_29
  *         @arg @ref LL_RCC_PLLP_DIV_30
  *         @arg @ref LL_RCC_PLLP_DIV_31
  *         @arg @ref LL_RCC_PLLP_DIV_32
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_I2S1_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLP__) ((__INPUTFREQ__)  * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLP__) >> RCC_PLLCFGR_PLLP_Pos) + 1U))

/**
  * @brief  Helper macro to calculate the PLLPCLK frequency used on ADC domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_ADC_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetP ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLP__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_3
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_5
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_7
  *         @arg @ref LL_RCC_PLLP_DIV_8
  *         @arg @ref LL_RCC_PLLP_DIV_9
  *         @arg @ref LL_RCC_PLLP_DIV_10
  *         @arg @ref LL_RCC_PLLP_DIV_11
  *         @arg @ref LL_RCC_PLLP_DIV_12
  *         @arg @ref LL_RCC_PLLP_DIV_13
  *         @arg @ref LL_RCC_PLLP_DIV_14
  *         @arg @ref LL_RCC_PLLP_DIV_15
  *         @arg @ref LL_RCC_PLLP_DIV_16
  *         @arg @ref LL_RCC_PLLP_DIV_17
  *         @arg @ref LL_RCC_PLLP_DIV_18
  *         @arg @ref LL_RCC_PLLP_DIV_19
  *         @arg @ref LL_RCC_PLLP_DIV_20
  *         @arg @ref LL_RCC_PLLP_DIV_21
  *         @arg @ref LL_RCC_PLLP_DIV_22
  *         @arg @ref LL_RCC_PLLP_DIV_23
  *         @arg @ref LL_RCC_PLLP_DIV_24
  *         @arg @ref LL_RCC_PLLP_DIV_25
  *         @arg @ref LL_RCC_PLLP_DIV_26
  *         @arg @ref LL_RCC_PLLP_DIV_27
  *         @arg @ref LL_RCC_PLLP_DIV_28
  *         @arg @ref LL_RCC_PLLP_DIV_29
  *         @arg @ref LL_RCC_PLLP_DIV_30
  *         @arg @ref LL_RCC_PLLP_DIV_31
  *         @arg @ref LL_RCC_PLLP_DIV_32
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_ADC_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLP__) ((__INPUTFREQ__) * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLP__) >> RCC_PLLCFGR_PLLP_Pos) + 1U))

#if defined(RNG)
/**
  * @brief  Helper macro to calculate the PLLQCLK frequency used on RNG domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_RNG_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_RNG_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLQ__) ((__INPUTFREQ__) * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLQ__) >> RCC_PLLCFGR_PLLQ_Pos) + 1U))
#endif /* RNG */

#if defined(RCC_PLLQ_SUPPORT)
/**
  * @brief  Helper macro to calculate the PLLQCLK frequency used on TIM1 domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_TIM1_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_TIM1_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLQ__) ((__INPUTFREQ__) * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLQ__) >> RCC_PLLCFGR_PLLQ_Pos) + 1U))
#if defined(TIM15)
/**
  * @brief  Helper macro to calculate the PLLQCLK frequency used on TIM15 domain
  * @note ex: @ref __LL_RCC_CALC_PLLCLK_TIM15_FREQ (HSE_VALUE,@ref LL_RCC_PLL_GetDivider (),
  *             @ref LL_RCC_PLL_GetN (), @ref LL_RCC_PLL_GetQ ());
  * @param  __INPUTFREQ__ PLL Input frequency (based on HSE/HSI)
  * @param  __PLLM__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  __PLLN__ Between Min_Data = 8 and Max_Data = 86
  * @param  __PLLQ__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval PLL clock frequency (in Hz)
  */
#define __LL_RCC_CALC_PLLCLK_TIM15_FREQ(__INPUTFREQ__, __PLLM__, __PLLN__, __PLLQ__) ((__INPUTFREQ__) * (__PLLN__) / ((((__PLLM__)>> RCC_PLLCFGR_PLLM_Pos) + 1U)) / \
                   (((__PLLQ__) >> RCC_PLLCFGR_PLLQ_Pos) + 1U))
#endif /* TIM15 */
#endif /* RCC_PLLQ_SUPPORT */

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
#define __LL_RCC_CALC_HCLK_FREQ(__SYSCLKFREQ__,__AHBPRESCALER__) ((__SYSCLKFREQ__) >> (AHBPrescTable[((__AHBPRESCALER__) & RCC_CFGR_HPRE) >>  RCC_CFGR_HPRE_Pos] & 0x1FU))

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
#define __LL_RCC_CALC_PCLK1_FREQ(__HCLKFREQ__, __APB1PRESCALER__) ((__HCLKFREQ__) >> (APBPrescTable[(__APB1PRESCALER__) >>  RCC_CFGR_PPRE_Pos] & 0x1FU))

/**
  * @brief  Helper macro to calculate the HSISYS frequency
  * @param  __HSIDIV__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_HSI_DIV_1
  *         @arg @ref LL_RCC_HSI_DIV_2
  *         @arg @ref LL_RCC_HSI_DIV_4
  *         @arg @ref LL_RCC_HSI_DIV_8
  *         @arg @ref LL_RCC_HSI_DIV_16
  *         @arg @ref LL_RCC_HSI_DIV_32
  *         @arg @ref LL_RCC_HSI_DIV_64
  *         @arg @ref LL_RCC_HSI_DIV_128
  * @retval HSISYS clock frequency (in Hz)
  */
#define __LL_RCC_CALC_HSI_FREQ(__HSIDIV__) (HSI_VALUE / (1U << ((__HSIDIV__)>> RCC_CR_HSIDIV_Pos)))

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
  return ((READ_BIT(RCC->CR, RCC_CR_HSERDY) == (RCC_CR_HSERDY)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_HSI HSI
  * @{
  */

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
  * @brief  Check if HSI in stop mode is enabled
  * @rmtoll CR           HSIKERON        LL_RCC_HSI_IsEnabledInStopMode
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_IsEnabledInStopMode(void)
{
  return ((READ_BIT(RCC->CR, RCC_CR_HSIKERON) == (RCC_CR_HSIKERON)) ? 1UL : 0UL);
}

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
  return ((READ_BIT(RCC->CR, RCC_CR_HSIRDY) == (RCC_CR_HSIRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Get HSI Calibration value
  * @note When HSITRIM is written, HSICAL is updated with the sum of
  *       HSITRIM and the factory trim value
  * @rmtoll ICSCR        HSICAL        LL_RCC_HSI_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_HSICAL) >> RCC_ICSCR_HSICAL_Pos);
}

/**
  * @brief  Set HSI Calibration trimming
  * @note user-programmable trimming value that is added to the HSICAL
  * @note Default value is 64, which, when added to the HSICAL value,
  *       should trim the HSI to 16 MHz +/- 1 %
  * @rmtoll ICSCR        HSITRIM       LL_RCC_HSI_SetCalibTrimming
  * @param  Value Between Min_Data = 0 and Max_Data = 127
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, Value << RCC_ICSCR_HSITRIM_Pos);
}

/**
  * @brief  Get HSI Calibration trimming
  * @rmtoll ICSCR        HSITRIM       LL_RCC_HSI_GetCalibTrimming
  * @retval Between Min_Data = 0 and Max_Data = 127
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_HSITRIM) >> RCC_ICSCR_HSITRIM_Pos);
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
  * @brief  Enable Clock security system on LSE.
  * @rmtoll BDCR         LSECSSON      LL_RCC_LSE_EnableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_EnableCSS(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_LSECSSON);
}

/**
  * @brief  Disable Clock security system on LSE.
  * @note Clock security system can be disabled only after a LSE
  *       failure detection. In that case it MUST be disabled by software.
  * @rmtoll BDCR         LSECSSON      LL_RCC_LSE_DisableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_DisableCSS(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSECSSON);
}

/**
  * @brief  Check if LSE oscillator Ready
  * @rmtoll BDCR         LSERDY        LL_RCC_LSE_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsReady(void)
{
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == (RCC_BDCR_LSERDY)) ? 1UL : 0UL);
}

/**
  * @brief  Check if CSS on LSE failure Detection
  * @rmtoll BDCR         LSECSSD       LL_RCC_LSE_IsCSSDetected
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsCSSDetected(void)
{
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_LSECSSD) == (RCC_BDCR_LSECSSD)) ? 1UL : 0UL);
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
  return ((READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == (RCC_CSR_LSIRDY)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_LSCO LSCO
  * @{
  */

/**
  * @brief  Enable Low speed clock
  * @rmtoll BDCR         LSCOEN        LL_RCC_LSCO_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSCO_Enable(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_LSCOEN);
}

/**
  * @brief  Disable Low speed clock
  * @rmtoll BDCR         LSCOEN        LL_RCC_LSCO_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSCO_Disable(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSCOEN);
}

/**
  * @brief  Configure Low speed clock selection
  * @rmtoll BDCR         LSCOSEL       LL_RCC_LSCO_SetSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LSCO_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LSCO_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSCO_SetSource(uint32_t Source)
{
  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSCOSEL, Source);
}

/**
  * @brief  Get Low speed clock selection
  * @rmtoll BDCR         LSCOSEL       LL_RCC_LSCO_GetSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LSCO_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LSCO_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_LSCO_GetSource(void)
{
  return (uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_LSCOSEL));
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_LSE
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
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_LSI
  *         @arg @ref LL_RCC_SYS_CLKSOURCE_STATUS_LSE
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
  * @brief  Set HSI16 division factor
  * @rmtoll CR         HSIDIV          LL_RCC_SetHSIDiv
  * @note  HSIDIV parameter is only applied to SYSCLK_Frequency when HSI is used as
  * system clock source.
  * @param  HSIDiv  This parameter can be one of the following values:
  *         @arg @ref LL_RCC_HSI_DIV_1
  *         @arg @ref LL_RCC_HSI_DIV_2
  *         @arg @ref LL_RCC_HSI_DIV_4
  *         @arg @ref LL_RCC_HSI_DIV_8
  *         @arg @ref LL_RCC_HSI_DIV_16
  *         @arg @ref LL_RCC_HSI_DIV_32
  *         @arg @ref LL_RCC_HSI_DIV_64
  *         @arg @ref LL_RCC_HSI_DIV_128
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetHSIDiv(uint32_t HSIDiv)
{
  MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, HSIDiv);
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
  * @brief  Get HSI16 Division factor
  * @rmtoll CR         HSIDIV         LL_RCC_GetHSIDiv
  * @note  HSIDIV parameter is only applied to SYSCLK_Frequency when HSI is used as
  * system clock source.
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_HSI_DIV_1
  *         @arg @ref LL_RCC_HSI_DIV_2
  *         @arg @ref LL_RCC_HSI_DIV_4
  *         @arg @ref LL_RCC_HSI_DIV_8
  *         @arg @ref LL_RCC_HSI_DIV_16
  *         @arg @ref LL_RCC_HSI_DIV_32
  *         @arg @ref LL_RCC_HSI_DIV_64
  *         @arg @ref LL_RCC_HSI_DIV_128
  */
__STATIC_INLINE uint32_t LL_RCC_GetHSIDiv(void)
{
  return (uint32_t)(READ_BIT(RCC->CR, RCC_CR_HSIDIV));
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
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_PLLCLK
  *         @arg @ref LL_RCC_MCO1SOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
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
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSARTClockSource(uint32_t USARTxSource)
{
  MODIFY_REG(RCC->CCIPR, (USARTxSource >> 16U), (USARTxSource & 0x0000FFFFU));
}

#if defined(LPUART1)
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
#endif /* LPUART1 */

/**
  * @brief  Configure I2Cx clock source
  * @rmtoll CCIPR        I2C1SEL       LL_RCC_SetI2CClockSource
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2CClockSource(uint32_t I2CxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C1SEL, I2CxSource);
}

#if defined(RCC_CCIPR_TIM1SEL) || defined(RCC_CCIPR_TIM15SEL)
/**
  * @brief  Configure TIMx clock source
  * @rmtoll CCIPR        TIMxSEL       LL_RCC_SetTIMClockSource
  * @param  TIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE_PCLK1
  * @if defined(STM32G081xx)
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE_PCLK1
  * @endif
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetTIMClockSource(uint32_t TIMxSource)
{
  MODIFY_REG(RCC->CCIPR, (TIMxSource & 0xFFFF0000U), (TIMxSource << 16));
}
#endif /* RCC_CCIPR_TIM1SEL && RCC_CCIPR_TIM15SEL */

#if defined(LPTIM1) && defined(LPTIM2)
/**
  * @brief  Configure LPTIMx clock source
  * @rmtoll CCIPR        LPTIMxSEL     LL_RCC_SetLPTIMClockSource
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetLPTIMClockSource(uint32_t LPTIMxSource)
{
  MODIFY_REG(RCC->CCIPR, (LPTIMxSource & 0xFFFF0000U), (LPTIMxSource << 16U));
}
#endif /* LPTIM1 && LPTIM2 */

#if defined(CEC)
/**
  * @brief  Configure CEC clock source
  * @rmtoll CCIPR        CECSEL        LL_RCC_SetCECClockSource
  * @param  CECxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV488
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCECClockSource(uint32_t CECxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_CECSEL, CECxSource);
}
#endif /* CEC */

#if defined(RCC_CCIPR_RNGDIV)
/**
  * @brief  Configure RNG division factor
  * @rmtoll CCIPR        RNGDIV        LL_RCC_SetRNGClockDiv
  * @param  RNGxDiv This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLK_DIV1
  *         @arg @ref LL_RCC_RNG_CLK_DIV2
  *         @arg @ref LL_RCC_RNG_CLK_DIV4
  *         @arg @ref LL_RCC_RNG_CLK_DIV8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRNGClockDiv(uint32_t RNGxDiv)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_RNGDIV, RNGxDiv);
}
#endif /* RNG */

#if defined (RCC_CCIPR_RNGSEL)
/**
  * @brief  Configure RNG clock source
  * @rmtoll CCIPR        RNGSEL        LL_RCC_SetRNGClockSource
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_HSI_DIV8
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRNGClockSource(uint32_t RNGxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_RNGSEL, RNGxSource);
}
#endif /* RNG */

/**
  * @brief  Configure ADC clock source
  * @rmtoll CCIPR        ADCSEL        LL_RCC_SetADCClockSource
  * @param  ADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetADCClockSource(uint32_t ADCxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_ADCSEL, ADCxSource);
}

/**
  * @brief  Configure I2Sx clock source
  * @rmtoll CCIPR        I2S1SEL       LL_RCC_SetI2SClockSource
  * @param  I2SxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2SClockSource(uint32_t I2SxSource)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2S1SEL, I2SxSource);
}

/**
  * @brief  Get USARTx clock source
  * @rmtoll CCIPR        USART1SEL     LL_RCC_GetUSARTClockSource
  * @param  USARTx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART2_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSARTClockSource(uint32_t USARTx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, USARTx) | (USARTx << 16U));
}

#if defined (RCC_CCIPR_LPUART1SEL)
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
#endif /* LPUART1 */

/**
  * @brief  Get I2Cx clock source
  * @rmtoll CCIPR        I2C1SEL       LL_RCC_GetI2CClockSource
  * @param  I2Cx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE_HSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2CClockSource(uint32_t I2Cx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, I2Cx));
}

#if defined(RCC_CCIPR_TIM1SEL) || defined(RCC_CCIPR_TIM15SEL)
/**
  * @brief  Get TIMx clock source
  * @rmtoll CCIPR        TIMxSEL       LL_RCC_GetTIMClockSource
  * @param  TIMx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE_PCLK1
  * @if defined(STM32G081xx)
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE_PCLK1
  * @endif
  */
__STATIC_INLINE uint32_t LL_RCC_GetTIMClockSource(uint32_t TIMx)
{
  return (uint32_t)((READ_BIT(RCC->CCIPR, TIMx) >> 16U) | TIMx);
}
#endif /* RCC_CCIPR_TIM1SEL || RCC_CCIPR_TIM15SEL */

#if defined(LPTIM1) && defined(LPTIM2)
/**
  * @brief  Get LPTIMx clock source
  * @rmtoll CCIPR        LPTIMxSEL     LL_RCC_GetLPTIMClockSource
  * @param  LPTIMx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetLPTIMClockSource(uint32_t LPTIMx)
{
  return (uint32_t)((READ_BIT(RCC->CCIPR, LPTIMx) >> 16U) | LPTIMx);
}
#endif /* LPTIM1 && LPTIM2 */

#if defined (RCC_CCIPR_CECSEL)
/**
  * @brief  Get CEC clock source
  * @rmtoll CCIPR        CECSEL        LL_RCC_GetCECClockSource
  * @param  CECx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_HSI_DIV488
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetCECClockSource(uint32_t CECx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, CECx));
}
#endif /* CEC */

#if defined(RNG)
/**
  * @brief  Get RNGx clock source
  * @rmtoll CCIPR        RNGSEL        LL_RCC_GetRNGClockSource
  * @param  RNGx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_HSI_DIV8
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_RNG_CLKSOURCE_PLL
  */
__STATIC_INLINE uint32_t LL_RCC_GetRNGClockSource(uint32_t RNGx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, RNGx));
}
#endif /* RNG */

#if defined(RNG)
/**
  * @brief  Get RNGx clock division factor
  * @rmtoll CCIPR        RNGDIV        LL_RCC_GetRNGClockDiv
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLK_DIV1
  *         @arg @ref LL_RCC_RNG_CLK_DIV2
  *         @arg @ref LL_RCC_RNG_CLK_DIV4
  *         @arg @ref LL_RCC_RNG_CLK_DIV8
  */
__STATIC_INLINE uint32_t LL_RCC_GetRNGClockDiv(void)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGDIV));
}
#endif /* RNG */

/**
  * @brief  Get ADCx clock source
  * @rmtoll CCIPR        ADCSEL        LL_RCC_GetADCClockSource
  * @param  ADCx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_SYSCLK
  */
__STATIC_INLINE uint32_t LL_RCC_GetADCClockSource(uint32_t ADCx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, ADCx));
}

/**
  * @brief  Get I2Sx clock source
  * @rmtoll CCIPR        I2S        LL_RCC_GetI2SClockSource
  * @param  I2Sx This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PIN
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_SYSCLK
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE_PLL
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2SClockSource(uint32_t I2Sx)
{
  return (uint32_t)(READ_BIT(RCC->CCIPR, I2Sx));
}
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
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_RTCEN) == (RCC_BDCR_RTCEN)) ? 1UL : 0UL);
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
  return ((READ_BIT(RCC->CR, RCC_CR_PLLRDY) == (RCC_CR_PLLRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Configure PLL used for SYSCLK Domain
  * @note PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note PLLN/PLLR can be written only when PLL is disabled
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_SYS\n
  *         PLLCFGR      PLLR          LL_RCC_PLL_ConfigDomain_SYS
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLR This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLR_DIV_2
  *         @arg @ref LL_RCC_PLLR_DIV_3
  *         @arg @ref LL_RCC_PLLR_DIV_4
  *         @arg @ref LL_RCC_PLLR_DIV_5
  *         @arg @ref LL_RCC_PLLR_DIV_6
  *         @arg @ref LL_RCC_PLLR_DIV_7
  *         @arg @ref LL_RCC_PLLR_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_SYS(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLR)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLR,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLR);
}

/**
  * @brief  Configure PLL used for ADC domain clock
  * @note   PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note   PLLN/PLLP can be written only when PLL is disabled
  * @note   User shall verify whether the PLL configuration is not done through
  *         other functions (ex: I2S1)
  * @note   This can be selected for ADC
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_ADC\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_ADC\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_ADC\n
  *         PLLCFGR      PLLP          LL_RCC_PLL_ConfigDomain_ADC
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLP This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_3
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_5
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_7
  *         @arg @ref LL_RCC_PLLP_DIV_8
  *         @arg @ref LL_RCC_PLLP_DIV_9
  *         @arg @ref LL_RCC_PLLP_DIV_10
  *         @arg @ref LL_RCC_PLLP_DIV_11
  *         @arg @ref LL_RCC_PLLP_DIV_12
  *         @arg @ref LL_RCC_PLLP_DIV_13
  *         @arg @ref LL_RCC_PLLP_DIV_14
  *         @arg @ref LL_RCC_PLLP_DIV_15
  *         @arg @ref LL_RCC_PLLP_DIV_16
  *         @arg @ref LL_RCC_PLLP_DIV_17
  *         @arg @ref LL_RCC_PLLP_DIV_18
  *         @arg @ref LL_RCC_PLLP_DIV_19
  *         @arg @ref LL_RCC_PLLP_DIV_20
  *         @arg @ref LL_RCC_PLLP_DIV_21
  *         @arg @ref LL_RCC_PLLP_DIV_22
  *         @arg @ref LL_RCC_PLLP_DIV_23
  *         @arg @ref LL_RCC_PLLP_DIV_24
  *         @arg @ref LL_RCC_PLLP_DIV_25
  *         @arg @ref LL_RCC_PLLP_DIV_26
  *         @arg @ref LL_RCC_PLLP_DIV_27
  *         @arg @ref LL_RCC_PLLP_DIV_28
  *         @arg @ref LL_RCC_PLLP_DIV_29
  *         @arg @ref LL_RCC_PLLP_DIV_30
  *         @arg @ref LL_RCC_PLLP_DIV_31
  *         @arg @ref LL_RCC_PLLP_DIV_32
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_ADC(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLP)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLP);
}

/**
  * @brief  Configure PLL used for I2S domain clock
  * @note   PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note   PLLN/PLLP can be written only when PLL is disabled
  * @note   User shall verify whether the PLL configuration is not done through
  *         other functions (ex: ADC)
  * @note   This can be selected for I2S1
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_I2S1\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_I2S1\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_I2S1\n
  *         PLLCFGR      PLLP          LL_RCC_PLL_ConfigDomain_I2S1
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLP This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_3
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_5
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_7
  *         @arg @ref LL_RCC_PLLP_DIV_8
  *         @arg @ref LL_RCC_PLLP_DIV_9
  *         @arg @ref LL_RCC_PLLP_DIV_10
  *         @arg @ref LL_RCC_PLLP_DIV_11
  *         @arg @ref LL_RCC_PLLP_DIV_12
  *         @arg @ref LL_RCC_PLLP_DIV_13
  *         @arg @ref LL_RCC_PLLP_DIV_14
  *         @arg @ref LL_RCC_PLLP_DIV_15
  *         @arg @ref LL_RCC_PLLP_DIV_16
  *         @arg @ref LL_RCC_PLLP_DIV_17
  *         @arg @ref LL_RCC_PLLP_DIV_18
  *         @arg @ref LL_RCC_PLLP_DIV_19
  *         @arg @ref LL_RCC_PLLP_DIV_20
  *         @arg @ref LL_RCC_PLLP_DIV_21
  *         @arg @ref LL_RCC_PLLP_DIV_22
  *         @arg @ref LL_RCC_PLLP_DIV_23
  *         @arg @ref LL_RCC_PLLP_DIV_24
  *         @arg @ref LL_RCC_PLLP_DIV_25
  *         @arg @ref LL_RCC_PLLP_DIV_26
  *         @arg @ref LL_RCC_PLLP_DIV_27
  *         @arg @ref LL_RCC_PLLP_DIV_28
  *         @arg @ref LL_RCC_PLLP_DIV_29
  *         @arg @ref LL_RCC_PLLP_DIV_30
  *         @arg @ref LL_RCC_PLLP_DIV_31
  *         @arg @ref LL_RCC_PLLP_DIV_32
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_I2S1(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLP)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLP);
}

#if defined(RNG)
/**
  * @brief  Configure PLL used for RNG domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note PLLN/PLLQ can be written only when PLL is disabled
  * @note   User shall verify whether the PLL configuration is not done through
  *         other functions (ex: TIM1, TIM15)
  * @note This  can be selected for RNG
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_RNG\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_RNG\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_RNG\n
  *         PLLCFGR      PLLQ          LL_RCC_PLL_ConfigDomain_RNG
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_RNG(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLQ);
}
#endif /* RNG */

#if defined(RCC_PLLQ_SUPPORT)
/**
  * @brief  Configure PLL used for TIM1 domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note PLLN/PLLQ can be written only when PLL is disabled
  * @note   User shall verify whether the PLL configuration is not done through
  *         other functions (ex: RNG, TIM15)
  * @note This  can be selected for TIM1
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_TIM1\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_TIM1\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_TIM1\n
  *         PLLCFGR      PLLQ          LL_RCC_PLL_ConfigDomain_TIM1
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_TIM1(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLQ);
}
#endif /* RCC_PLLQ_SUPPORT */

#if defined(RCC_PLLQ_SUPPORT) && defined(TIM15)
/**
  * @brief  Configure PLL used for TIM15 domain clock
  * @note PLL Source and PLLM Divider can be written only when PLL is disabled
  * @note PLLN/PLLQ can be written only when PLL is disabled
  * @note   User shall verify whether the PLL configuration is not done through
  *         other functions (ex: RNG, TIM1)
  * @note This  can be selected for TIM15
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_ConfigDomain_TIM15\n
  *         PLLCFGR      PLLM          LL_RCC_PLL_ConfigDomain_TIM15\n
  *         PLLCFGR      PLLN          LL_RCC_PLL_ConfigDomain_TIM15\n
  *         PLLCFGR      PLLQ          LL_RCC_PLL_ConfigDomain_TIM15
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @param  PLLM This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  * @param  PLLN Between 8 and 86
  * @param  PLLQ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_ConfigDomain_TIM15(uint32_t Source, uint32_t PLLM, uint32_t PLLN, uint32_t PLLQ)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ,
             Source | PLLM | (PLLN << RCC_PLLCFGR_PLLN_Pos) | PLLQ);
}
#endif /* RCC_PLLQ_SUPPORT && TIM15 */

/**
  * @brief  Get Main PLL multiplication factor for VCO
  * @rmtoll PLLCFGR      PLLN          LL_RCC_PLL_GetN
  * @retval Between 8 and 86
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetN(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLN) >>  RCC_PLLCFGR_PLLN_Pos);
}

/**
  * @brief  Get Main PLL division factor for PLLP
  * @note   used for PLLPCLK (ADC & I2S clock)
  * @rmtoll PLLCFGR      PLLP       LL_RCC_PLL_GetP
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLP_DIV_2
  *         @arg @ref LL_RCC_PLLP_DIV_3
  *         @arg @ref LL_RCC_PLLP_DIV_4
  *         @arg @ref LL_RCC_PLLP_DIV_5
  *         @arg @ref LL_RCC_PLLP_DIV_6
  *         @arg @ref LL_RCC_PLLP_DIV_7
  *         @arg @ref LL_RCC_PLLP_DIV_8
  *         @arg @ref LL_RCC_PLLP_DIV_9
  *         @arg @ref LL_RCC_PLLP_DIV_10
  *         @arg @ref LL_RCC_PLLP_DIV_11
  *         @arg @ref LL_RCC_PLLP_DIV_12
  *         @arg @ref LL_RCC_PLLP_DIV_13
  *         @arg @ref LL_RCC_PLLP_DIV_14
  *         @arg @ref LL_RCC_PLLP_DIV_15
  *         @arg @ref LL_RCC_PLLP_DIV_16
  *         @arg @ref LL_RCC_PLLP_DIV_17
  *         @arg @ref LL_RCC_PLLP_DIV_18
  *         @arg @ref LL_RCC_PLLP_DIV_19
  *         @arg @ref LL_RCC_PLLP_DIV_20
  *         @arg @ref LL_RCC_PLLP_DIV_21
  *         @arg @ref LL_RCC_PLLP_DIV_22
  *         @arg @ref LL_RCC_PLLP_DIV_23
  *         @arg @ref LL_RCC_PLLP_DIV_24
  *         @arg @ref LL_RCC_PLLP_DIV_25
  *         @arg @ref LL_RCC_PLLP_DIV_26
  *         @arg @ref LL_RCC_PLLP_DIV_27
  *         @arg @ref LL_RCC_PLLP_DIV_28
  *         @arg @ref LL_RCC_PLLP_DIV_29
  *         @arg @ref LL_RCC_PLLP_DIV_30
  *         @arg @ref LL_RCC_PLLP_DIV_31
  *         @arg @ref LL_RCC_PLLP_DIV_32
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetP(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLP));
}

#if defined(RCC_PLLQ_SUPPORT)
/**
  * @brief  Get Main PLL division factor for PLLQ
  * @note used for PLLQCLK selected for RNG, TIM1, TIM15 clock
  * @rmtoll PLLCFGR      PLLQ          LL_RCC_PLL_GetQ
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLQ_DIV_2
  *         @arg @ref LL_RCC_PLLQ_DIV_3
  *         @arg @ref LL_RCC_PLLQ_DIV_4
  *         @arg @ref LL_RCC_PLLQ_DIV_5
  *         @arg @ref LL_RCC_PLLQ_DIV_6
  *         @arg @ref LL_RCC_PLLQ_DIV_7
  *         @arg @ref LL_RCC_PLLQ_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetQ(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ));
}
#endif /* RCC_PLLQ_SUPPORT */

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
  *         @arg @ref LL_RCC_PLLR_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetR(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLR));
}

/**
  * @brief  Configure PLL clock source
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_SetMainSource
  * @param PLLSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_HSI
  *         @arg @ref LL_RCC_PLLSOURCE_HSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_SetMainSource(uint32_t PLLSource)
{
  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC, PLLSource);
}

/**
  * @brief  Get the oscillator used as PLL clock source.
  * @rmtoll PLLCFGR      PLLSRC        LL_RCC_PLL_GetMainSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLLSOURCE_NONE
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
  *         @arg @ref LL_RCC_PLLM_DIV_1
  *         @arg @ref LL_RCC_PLLM_DIV_2
  *         @arg @ref LL_RCC_PLLM_DIV_3
  *         @arg @ref LL_RCC_PLLM_DIV_4
  *         @arg @ref LL_RCC_PLLM_DIV_5
  *         @arg @ref LL_RCC_PLLM_DIV_6
  *         @arg @ref LL_RCC_PLLM_DIV_7
  *         @arg @ref LL_RCC_PLLM_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_PLL_GetDivider(void)
{
  return (uint32_t)(READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLM));
}

/**
  * @brief  Enable PLL output mapped on ADC domain clock
  * @rmtoll PLLCFGR      PLLPEN        LL_RCC_PLL_EnableDomain_ADC
  * @note   User shall check that PLL enable is not done through
  *         other functions (ex: I2S1)
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_ADC(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLPEN);
}

/**
  * @brief  Disable PLL output mapped on ADC domain clock
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note   User shall check that PLL is not used by any other IP
  *         (ex: I2S1)
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used,  should be 0
  * @rmtoll PLLCFGR      PLLPEN        LL_RCC_PLL_DisableDomain_ADC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_ADC(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLPEN);
}

/**
  * @brief  Enable PLL output mapped on I2S domain clock
  * @rmtoll PLLCFGR      PLLPEN        LL_RCC_PLL_EnableDomain_I2S1
  * @note   User shall check that PLL enable is not done through
  *         other functions (ex: ADC)
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_I2S1(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLPEN);
}

/**
  * @brief  Disable PLL output mapped on I2S1 domain clock
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note   User shall check that PLL is not used by any other IP
  *         (ex: RNG)
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used,  should be 0
  * @rmtoll PLLCFGR      PLLPEN        LL_RCC_PLL_DisableDomain_I2S1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_I2S1(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLPEN);
}

#if defined(RNG)
/**
  * @brief  Enable PLL output mapped on RNG domain clock
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_EnableDomain_RNG
  * @note   User shall check that PLL enable is not done through
  *         other functions (ex: TIM1, TIM15)
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_RNG(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}

/**
  * @brief  Disable PLL output mapped on RNG domain clock
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note   User shall check that PLL is not used by any other IP
  *         (ex: TIM, TIM15)
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used,  should be 0
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_DisableDomain_RNG
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_RNG(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}
#endif /* RNG */

#if defined(RCC_PLLQ_SUPPORT)
/**
  * @brief  Enable PLL output mapped on TIM1 domain clock
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_EnableDomain_TIM1
  * @note   User shall check that PLL enable is not done through
  *         other functions (ex: RNG, TIM15)
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_TIM1(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}

/**
  * @brief  Disable PLL output mapped on TIM1 domain clock
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note   User shall check that PLL is not used by any other IP
  *         (ex: RNG, TIM15)
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used,  should be 0
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_DisableDomain_TIM1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_TIM1(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}
#endif /* RCC_PLLQ_SUPPORT */

#if defined(RCC_PLLQ_SUPPORT) && defined(TIM15)
/**
  * @brief  Enable PLL output mapped on TIM15 domain clock
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_EnableDomain_TIM15
  * @note   User shall check that PLL enable is not done through
  *         other functions (ex: RNG, TIM1)
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_TIM15(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}

/**
  * @brief  Disable PLL output mapped on TIM15 domain clock
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note   User shall check that PLL is not used by any other IP
  *         (ex: RNG, TIM1)
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used,  should be 0
  * @rmtoll PLLCFGR      PLLQEN        LL_RCC_PLL_DisableDomain_TIM15
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_TIM15(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
}
#endif /* RCC_PLLQ_SUPPORT && TIM15 */

/**
  * @brief  Enable PLL output mapped on SYSCLK domain
  * @rmtoll PLLCFGR      PLLREN        LL_RCC_PLL_EnableDomain_SYS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_EnableDomain_SYS(void)
{
  SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLREN);
}

/**
  * @brief  Disable PLL output mapped on SYSCLK domain
  * @note Cannot be disabled if the PLL clock is used as the system clock
  * @note In order to save power, when the PLLCLK  of the PLL is
  *       not used, Main PLL  should be 0
  * @rmtoll PLLCFGR      PLLREN        LL_RCC_PLL_DisableDomain_SYS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL_DisableDomain_SYS(void)
{
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLREN);
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

/**
  * @brief  Clear Clock security system interrupt flag
  * @rmtoll CICR         CSSC          LL_RCC_ClearFlag_HSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSECSS(void)
{
  SET_BIT(RCC->CICR, RCC_CICR_CSSC);
}

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
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_LSIRDYF) == (RCC_CIFR_LSIRDYF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if LSE ready interrupt occurred or not
  * @rmtoll CIFR         LSERDYF       LL_RCC_IsActiveFlag_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSERDY(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_LSERDYF) == (RCC_CIFR_LSERDYF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if HSI ready interrupt occurred or not
  * @rmtoll CIFR         HSIRDYF       LL_RCC_IsActiveFlag_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSIRDY(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_HSIRDYF) == (RCC_CIFR_HSIRDYF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if HSE ready interrupt occurred or not
  * @rmtoll CIFR         HSERDYF       LL_RCC_IsActiveFlag_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSERDY(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_HSERDYF) == (RCC_CIFR_HSERDYF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL ready interrupt occurred or not
  * @rmtoll CIFR         PLLRDYF       LL_RCC_IsActiveFlag_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLLRDY(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_PLLRDYF) == (RCC_CIFR_PLLRDYF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if Clock security system interrupt occurred or not
  * @rmtoll CIFR         CSSF          LL_RCC_IsActiveFlag_HSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSECSS(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_CSSF) == (RCC_CIFR_CSSF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if LSE Clock security system interrupt occurred or not
  * @rmtoll CIFR         LSECSSF       LL_RCC_IsActiveFlag_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSECSS(void)
{
  return ((READ_BIT(RCC->CIFR, RCC_CIFR_LSECSSF) == (RCC_CIFR_LSECSSF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Independent Watchdog reset is set or not.
  * @rmtoll CSR          IWDGRSTF      LL_RCC_IsActiveFlag_IWDGRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_IWDGRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_IWDGRSTF) == (RCC_CSR_IWDGRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Low Power reset is set or not.
  * @rmtoll CSR          LPWRRSTF      LL_RCC_IsActiveFlag_LPWRRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LPWRRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_LPWRRSTF) == (RCC_CSR_LPWRRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Option byte reset is set or not.
  * @rmtoll CSR          OBLRSTF       LL_RCC_IsActiveFlag_OBLRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_OBLRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_OBLRSTF) == (RCC_CSR_OBLRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Pin reset is set or not.
  * @rmtoll CSR          PINRSTF       LL_RCC_IsActiveFlag_PINRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PINRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_PINRSTF) == (RCC_CSR_PINRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Software reset is set or not.
  * @rmtoll CSR          SFTRSTF       LL_RCC_IsActiveFlag_SFTRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_SFTRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_SFTRSTF) == (RCC_CSR_SFTRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag Window Watchdog reset is set or not.
  * @rmtoll CSR          WWDGRSTF      LL_RCC_IsActiveFlag_WWDGRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_WWDGRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_WWDGRSTF) == (RCC_CSR_WWDGRSTF)) ? 1UL : 0UL);
}

/**
  * @brief  Check if RCC flag BOR or POR/PDR reset is set or not.
  * @rmtoll CSR          PWRRSTF       LL_RCC_IsActiveFlag_PWRRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PWRRST(void)
{
  return ((READ_BIT(RCC->CSR, RCC_CSR_PWRRSTF) == (RCC_CSR_PWRRSTF)) ? 1UL : 0UL);
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

/**
  * @brief  Checks if LSI ready interrupt source is enabled or disabled.
  * @rmtoll CIER         LSIRDYIE      LL_RCC_IsEnabledIT_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSIRDY(void)
{
  return ((READ_BIT(RCC->CIER, RCC_CIER_LSIRDYIE) == (RCC_CIER_LSIRDYIE)) ? 1UL : 0UL);
}

/**
  * @brief  Checks if LSE ready interrupt source is enabled or disabled.
  * @rmtoll CIER         LSERDYIE      LL_RCC_IsEnabledIT_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSERDY(void)
{
  return ((READ_BIT(RCC->CIER, RCC_CIER_LSERDYIE) == (RCC_CIER_LSERDYIE)) ? 1UL : 0UL);
}

/**
  * @brief  Checks if HSI ready interrupt source is enabled or disabled.
  * @rmtoll CIER         HSIRDYIE      LL_RCC_IsEnabledIT_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSIRDY(void)
{
  return ((READ_BIT(RCC->CIER, RCC_CIER_HSIRDYIE) == (RCC_CIER_HSIRDYIE)) ? 1UL : 0UL);
}

/**
  * @brief  Checks if HSE ready interrupt source is enabled or disabled.
  * @rmtoll CIER         HSERDYIE      LL_RCC_IsEnabledIT_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSERDY(void)
{
  return ((READ_BIT(RCC->CIER, RCC_CIER_HSERDYIE) == (RCC_CIER_HSERDYIE)) ? 1UL : 0UL);
}

/**
  * @brief  Checks if PLL ready interrupt source is enabled or disabled.
  * @rmtoll CIER         PLLRDYIE      LL_RCC_IsEnabledIT_PLLRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLLRDY(void)
{
  return ((READ_BIT(RCC->CIER, RCC_CIER_PLLRDYIE) == (RCC_CIER_PLLRDYIE)) ? 1UL : 0UL);
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
#if defined(LPUART1)
uint32_t    LL_RCC_GetLPUARTClockFreq(uint32_t LPUARTxSource);
#endif /* LPUART1 */
#if defined(LPTIM1) && defined(LPTIM2)
uint32_t    LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource);
#endif /* LPTIM1 && LPTIM2 */
#if defined(RNG)
uint32_t    LL_RCC_GetRNGClockFreq(uint32_t RNGxSource);
#endif /* RNG */
uint32_t    LL_RCC_GetADCClockFreq(uint32_t ADCxSource);
uint32_t    LL_RCC_GetI2SClockFreq(uint32_t I2SxSource);
#if defined(CEC)
uint32_t    LL_RCC_GetCECClockFreq(uint32_t CECxSource);
#endif /* CEC */
uint32_t    LL_RCC_GetTIMClockFreq(uint32_t TIMxSource);
uint32_t    LL_RCC_GetRTCClockFreq(void);
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

#endif /* STM32G0xx_LL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
