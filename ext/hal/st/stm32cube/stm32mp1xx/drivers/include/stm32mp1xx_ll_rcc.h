/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_rcc.h
  * @author  MCD Application Team
  * @version $VERSION$
  * @date    $DATE$
  * @brief   Header file of RCC LL module.
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
#ifndef STM32MP1xx_LL_RCC_H
#define STM32MP1xx_LL_RCC_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx.h"

/** @addtogroup STM32MP1xx_LL_Driver
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
/* Defines used to perform offsets*/

/* Clock source register offset Vs I2C46CKSELR register */
#define RCC_OFFSET_I2C46CKSELR     0x000UL
#define RCC_OFFSET_SPI6CKSELR      0x004UL
#define RCC_OFFSET_UART1CKSELR     0x008UL
#define RCC_OFFSET_RNG1CKSELR      0x00CUL
#define RCC_OFFSET_MCO1CFGR        0x740UL
#define RCC_OFFSET_MCO2CFGR        0x744UL
#define RCC_OFFSET_TIMG1PRER       0x768UL
#define RCC_OFFSET_TIMG2PRER       0x76CUL
#define RCC_OFFSET_I2C12CKSELR     0x800UL
#define RCC_OFFSET_I2C35CKSELR     0x804UL
#define RCC_OFFSET_SAI1CKSELR      0x808UL
#define RCC_OFFSET_SAI2CKSELR      0x80CUL
#define RCC_OFFSET_SAI3CKSELR      0x810UL
#define RCC_OFFSET_SAI4CKSELR      0x814UL
#define RCC_OFFSET_SPI2S1CKSELR    0x818UL
#define RCC_OFFSET_SPI2S23CKSELR   0x81CUL
#define RCC_OFFSET_SPI45CKSELR     0x820UL
#define RCC_OFFSET_UART6CKSELR     0x824UL
#define RCC_OFFSET_UART24CKSELR    0x828UL
#define RCC_OFFSET_UART35CKSELR    0x82CUL
#define RCC_OFFSET_UART78CKSELR    0x830UL
#define RCC_OFFSET_SDMMC12CKSELR   0x834UL
#define RCC_OFFSET_SDMMC3CKSELR    0x838UL
#define RCC_OFFSET_RNG2CKSELR      0x860UL
#define RCC_OFFSET_LPTIM45CKSELR   0x86CUL
#define RCC_OFFSET_LPTIM23CKSELR   0x870UL
#define RCC_OFFSET_LPTIM1CKSELR    0x874UL

#define RCC_CONFIG_SHIFT  0U
#define RCC_MASK_SHIFT    8U
#define RCC_REG_SHIFT     16U


/* Define all reset flags mask */
#define LL_RCC_MC_RSTSCLRR_ALL  0x000007FFU
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_Private_Macros RCC Private Macros
  * @{
  */

/* 32     28     24           16            8             0
   --------------------------------------------------------
   | Free  |     Register      |    Mask    | ClkSource   |
   |       |      Offset       |            | Config      |
   --------------------------------------------------------*/

#define LL_CLKSOURCE_MASK(__CLKSOURCE__) \
          (((__CLKSOURCE__) >> RCC_MASK_SHIFT  ) & 0xFFUL)

#define LL_CLKSOURCE_CONFIG(__CLKSOURCE__) \
          (((__CLKSOURCE__) >> RCC_CONFIG_SHIFT) & 0xFFUL)

#define LL_CLKSOURCE_REG(__CLKSOURCE__) \
          (((__CLKSOURCE__) >> RCC_REG_SHIFT   ) & 0xFFFUL)

#define LL_CLKSOURCE(__REG__, __MSK__, __CLK__)      \
          ((uint32_t)((((__REG__) ) << RCC_REG_SHIFT)  | \
          (( __MSK__              ) << RCC_MASK_SHIFT) | \
          (( __CLK__              ) << RCC_CONFIG_SHIFT)))

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
  uint32_t MPUSS_Frequency;   /*!< MPUSS clock frequency */
  uint32_t AXISS_Frequency;   /*!< AXISS clock frequency */
  uint32_t MCUSS_Frequency;   /*!< MCUSS clock frequency */
  uint32_t ACLK_Frequency;    /*!< ACLK clock frequency */
  uint32_t HCLK1_Frequency;   /*!< HCLK1 clock frequency */
  uint32_t HCLK2_Frequency;   /*!< HCLK2 clock frequency */
  uint32_t HCLK3_Frequency;   /*!< HCLK3 clock frequency */
  uint32_t HCLK4_Frequency;   /*!< HCLK4 clock frequency */
  uint32_t HCLK5_Frequency;   /*!< HCLK5 clock frequency */
  uint32_t HCLK6_Frequency;   /*!< HCLK6 clock frequency */
  uint32_t MCU_Frequency;     /*!< MCU clock frequency */
  uint32_t MLHCLK_Frequency;  /*!< MLHCLK clock frequency */
  uint32_t PCLK1_Frequency;   /*!< PCLK1 clock frequency */
  uint32_t PCLK2_Frequency;   /*!< PCLK2 clock frequency */
  uint32_t PCLK3_Frequency;   /*!< PCLK3 clock frequency */
  uint32_t PCLK4_Frequency;   /*!< PCLK4 clock frequency */
  uint32_t PCLK5_Frequency;   /*!< PCLK5 clock frequency */
} LL_RCC_ClocksTypeDef;

/**
  * @}
  */

/**
  * @brief  PLL Clocks Frequency Structure
  */
typedef struct
{
  uint32_t PLL_P_Frequency;
  uint32_t PLL_Q_Frequency;
  uint32_t PLL_R_Frequency;
} LL_PLL_ClocksTypeDef;

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
#define HSE_VALUE    24000000U   /*!< Value of the HSE oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    64000000U  /*!< Value of the HSI oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE    32768U     /*!< Value of the LSE oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE    32000U     /*!< Value of the LSI oscillator in Hz */
#endif /* LSI_VALUE */

#if !defined  (CSI_VALUE)
#define CSI_VALUE    4000000U     /*!< Value of the CSI oscillator in Hz */
#endif /* LSI_VALUE */

#if !defined  (EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE    12288000U /*!< Value of the I2S_CKIN external oscillator in Hz */
#endif /* EXTERNAL_CLOCK_VALUE */

#if !defined  (USBO_48M_VALUE)
#define USBO_48M_VALUE    48000000U /*!< Value of the rcc_ck_usbo_48m oscillator in Hz */
#endif /* USBO_48M_VALUE */

/**
  * @}
  */

/** @defgroup RCC_LL_EC_CLEAR_FLAG Clear Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_WriteReg function
  * @{
  */
#define LL_RCC_CIFR_LSIRDYC      RCC_MC_CIFR_LSIRDYF /*!< LSI Ready Interrupt Clear */
#define LL_RCC_CIFR_LSERDYC      RCC_MC_CIFR_LSERDYF /*!< LSE Ready Interrupt Clear */
#define LL_RCC_CIFR_HSIRDYC      RCC_MC_CIFR_HSIRDYF /*!< HSI Ready Interrupt Clear */
#define LL_RCC_CIFR_HSERDYC      RCC_MC_CIFR_HSERDYF /*!< HSE Ready Interrupt Clear */
#define LL_RCC_CIFR_CSIRDYC      RCC_MC_CIFR_CSIRDYF /*!< CSI Ready Interrupt Clear */
#define LL_RCC_CIFR_PLL1RDYC     RCC_MC_CIFR_PLL1DYF /*!< PLL1 Ready Interrupt Clear */
#define LL_RCC_CIFR_PLL2RDYC     RCC_MC_CIFR_PLL2DYF /*!< PLL2 Ready Interrupt Clear */
#define LL_RCC_CIFR_PLL3RDYC     RCC_MC_CIFR_PLL3DYF /*!< PLL3 Ready Interrupt Clear */
#define LL_RCC_CIFR_PLL4RDYC     RCC_MC_CIFR_PLL4DYF /*!< PLL4 Ready Interrupt Clear */
#define LL_RCC_CIFR_LSECSSC      RCC_MC_CIFR_LSECSSF /*!< LSE Clock Security System Interrupt Clear */
#define LL_RCC_CIFR_WKUPC        RCC_MC_CIFR_WKUPF   /*!< Wake up from CStop Interrupt Clear */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_RCC_ReadReg function
  * @{
  */
#define LL_RCC_CIFR_LSIRDYF      RCC_MC_CIFR_LSIRDYF /*!< LSI Ready Interrupt flag */
#define LL_RCC_CIFR_LSERDYF      RCC_MC_CIFR_LSERDYF /*!< LSE Ready Interrupt flag */
#define LL_RCC_CIFR_HSIRDYF      RCC_MC_CIFR_HSIRDYF /*!< HSI Ready Interrupt flag */
#define LL_RCC_CIFR_HSERDYF      RCC_MC_CIFR_HSERDYF /*!< HSE Ready Interrupt flag */
#define LL_RCC_CIFR_CSIRDYF      RCC_MC_CIFR_CSIRDYF /*!< CSI Ready Interrupt flag */
#define LL_RCC_CIFR_PLL1RDYF     RCC_MC_CIFR_PLL1DYF /*!< PLL1 Ready Interrupt flag */
#define LL_RCC_CIFR_PLL2RDYF     RCC_MC_CIFR_PLL2DYF /*!< PLL2 Ready Interrupt flag */
#define LL_RCC_CIFR_PLL3RDYF     RCC_MC_CIFR_PLL3DYF /*!< PLL3 Ready Interrupt flag */
#define LL_RCC_CIFR_PLL4RDYF     RCC_MC_CIFR_PLL4DYF /*!< PLL4 Ready Interrupt flag */
#define LL_RCC_CIFR_LSECSSF      RCC_MC_CIFR_LSECSSF /*!< LSE Clock Security System Interrupt flag */
#define LL_RCC_CIFR_WKUPF        RCC_MC_CIFR_WKUPF   /*!< Wake up from CStop Interrupt flag */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_RCC_ReadReg and  LL_RCC_WriteReg functions
  * @{
  */
#define LL_RCC_CIER_LSIRDYIE      RCC_MC_CIER_LSIRDYIE /*!< LSI Ready Interrupt Enable */
#define LL_RCC_CIER_LSERDYIE      RCC_MC_CIER_LSERDYIE /*!< LSE Ready Interrupt Enable */
#define LL_RCC_CIER_HSIRDYIE      RCC_MC_CIER_HSIRDYIE /*!< HSI Ready Interrupt Enable */
#define LL_RCC_CIER_HSERDYIE      RCC_MC_CIER_HSERDYIE /*!< HSE Ready Interrupt Enable */
#define LL_RCC_CIER_CSIRDYIE      RCC_MC_CIER_CSIRDYIE /*!< CSI Ready Interrupt Enable */
#define LL_RCC_CIER_PLL1RDYIE     RCC_MC_CIER_PLL1DYIE /*!< PLL1 Ready Interrupt Enable */
#define LL_RCC_CIER_PLL2RDYIE     RCC_MC_CIER_PLL2DYIE /*!< PLL2 Ready Interrupt Enable */
#define LL_RCC_CIER_PLL3RDYIE     RCC_MC_CIER_PLL3DYIE /*!< PLL3 Ready Interrupt Enable */
#define LL_RCC_CIER_PLL4RDYIE     RCC_MC_CIER_PLL4DYIE /*!< PLL4 Ready Interrupt Enable */
#define LL_RCC_CIER_LSECSSIE      RCC_MC_CIER_LSECSSIE /*!< LSE Clock Security System Interrupt Enable */
#define LL_RCC_CIER_WKUPIE        RCC_MC_CIER_WKUPIE   /*!< Wake up from CStop Interrupt Enable */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_HSIDIV  HSI oscillator divider
  * @{
  */
#define LL_RCC_HSI_DIV_1                   RCC_HSICFGR_HSIDIV_0
#define LL_RCC_HSI_DIV_2                   RCC_HSICFGR_HSIDIV_1
#define LL_RCC_HSI_DIV_4                   RCC_HSICFGR_HSIDIV_2
#define LL_RCC_HSI_DIV_8                   RCC_HSICFGR_HSIDIV_3
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCOxSOURCE  MCO SOURCE selection
  * @{
  */
#define LL_RCC_MCO1SOURCE_HSI      LL_CLKSOURCE(RCC_OFFSET_MCO1CFGR, RCC_MCO1CFGR_MCO1SEL, RCC_MCO1CFGR_MCO1SEL_0)
#define LL_RCC_MCO1SOURCE_HSE      LL_CLKSOURCE(RCC_OFFSET_MCO1CFGR, RCC_MCO1CFGR_MCO1SEL, RCC_MCO1CFGR_MCO1SEL_1)
#define LL_RCC_MCO1SOURCE_CSI      LL_CLKSOURCE(RCC_OFFSET_MCO1CFGR, RCC_MCO1CFGR_MCO1SEL, RCC_MCO1CFGR_MCO1SEL_2)
#define LL_RCC_MCO1SOURCE_LSI      LL_CLKSOURCE(RCC_OFFSET_MCO1CFGR, RCC_MCO1CFGR_MCO1SEL, RCC_MCO1CFGR_MCO1SEL_3)
#define LL_RCC_MCO1SOURCE_LSE      LL_CLKSOURCE(RCC_OFFSET_MCO1CFGR, RCC_MCO1CFGR_MCO1SEL, RCC_MCO1CFGR_MCO1SEL_4)

#define LL_RCC_MCO2SOURCE_MPU      LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_0)
#define LL_RCC_MCO2SOURCE_AXI      LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_1)
#define LL_RCC_MCO2SOURCE_MCU      LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_2)
#define LL_RCC_MCO2SOURCE_PLL4     LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_3)
#define LL_RCC_MCO2SOURCE_HSE      LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_4)
#define LL_RCC_MCO2SOURCE_HSI      LL_CLKSOURCE(RCC_OFFSET_MCO2CFGR, RCC_MCO2CFGR_MCO2SEL, RCC_MCO2CFGR_MCO2SEL_5)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO1_DIV  MCO1 prescaler
  * @{
  */
#define LL_RCC_MCO1_DIV_1                  RCC_MCO1CFGR_MCO1DIV_0   /*!< MCO not divided */
#define LL_RCC_MCO1_DIV_2                  RCC_MCO1CFGR_MCO1DIV_1   /*!< MCO divided by 2 */
#define LL_RCC_MCO1_DIV_3                  RCC_MCO1CFGR_MCO1DIV_2   /*!< MCO divided by 3 */
#define LL_RCC_MCO1_DIV_4                  RCC_MCO1CFGR_MCO1DIV_3   /*!< MCO divided by 4 */
#define LL_RCC_MCO1_DIV_5                  RCC_MCO1CFGR_MCO1DIV_4   /*!< MCO divided by 5 */
#define LL_RCC_MCO1_DIV_6                  RCC_MCO1CFGR_MCO1DIV_5   /*!< MCO divided by 6 */
#define LL_RCC_MCO1_DIV_7                  RCC_MCO1CFGR_MCO1DIV_6   /*!< MCO divided by 7 */
#define LL_RCC_MCO1_DIV_8                  RCC_MCO1CFGR_MCO1DIV_7   /*!< MCO divided by 8 */
#define LL_RCC_MCO1_DIV_9                  RCC_MCO1CFGR_MCO1DIV_8   /*!< MCO divided by 9 */
#define LL_RCC_MCO1_DIV_10                 RCC_MCO1CFGR_MCO1DIV_9   /*!< MCO divided by 10 */
#define LL_RCC_MCO1_DIV_11                 RCC_MCO1CFGR_MCO1DIV_10  /*!< MCO divided by 11 */
#define LL_RCC_MCO1_DIV_12                 RCC_MCO1CFGR_MCO1DIV_11  /*!< MCO divided by 12 */
#define LL_RCC_MCO1_DIV_13                 RCC_MCO1CFGR_MCO1DIV_12  /*!< MCO divided by 13 */
#define LL_RCC_MCO1_DIV_14                 RCC_MCO1CFGR_MCO1DIV_13  /*!< MCO divided by 14 */
#define LL_RCC_MCO1_DIV_15                 RCC_MCO1CFGR_MCO1DIV_14  /*!< MCO divided by 15 */
#define LL_RCC_MCO1_DIV_16                 RCC_MCO1CFGR_MCO1DIV_15  /*!< MCO divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCO2_DIV  MCO2 prescaler
  * @{
  */
#define LL_RCC_MCO2_DIV_1                  RCC_MCO2CFGR_MCO2DIV_0   /*!< MCO not divided */
#define LL_RCC_MCO2_DIV_2                  RCC_MCO2CFGR_MCO2DIV_1   /*!< MCO divided by 2 */
#define LL_RCC_MCO2_DIV_3                  RCC_MCO2CFGR_MCO2DIV_2   /*!< MCO divided by 3 */
#define LL_RCC_MCO2_DIV_4                  RCC_MCO2CFGR_MCO2DIV_3   /*!< MCO divided by 4 */
#define LL_RCC_MCO2_DIV_5                  RCC_MCO2CFGR_MCO2DIV_4   /*!< MCO divided by 5 */
#define LL_RCC_MCO2_DIV_6                  RCC_MCO2CFGR_MCO2DIV_5   /*!< MCO divided by 6 */
#define LL_RCC_MCO2_DIV_7                  RCC_MCO2CFGR_MCO2DIV_6   /*!< MCO divided by 7 */
#define LL_RCC_MCO2_DIV_8                  RCC_MCO2CFGR_MCO2DIV_7   /*!< MCO divided by 8 */
#define LL_RCC_MCO2_DIV_9                  RCC_MCO2CFGR_MCO2DIV_8   /*!< MCO divided by 9 */
#define LL_RCC_MCO2_DIV_10                 RCC_MCO2CFGR_MCO2DIV_9   /*!< MCO divided by 10 */
#define LL_RCC_MCO2_DIV_11                 RCC_MCO2CFGR_MCO2DIV_10  /*!< MCO divided by 11 */
#define LL_RCC_MCO2_DIV_12                 RCC_MCO2CFGR_MCO2DIV_11  /*!< MCO divided by 12 */
#define LL_RCC_MCO2_DIV_13                 RCC_MCO2CFGR_MCO2DIV_12  /*!< MCO divided by 13 */
#define LL_RCC_MCO2_DIV_14                 RCC_MCO2CFGR_MCO2DIV_13  /*!< MCO divided by 14 */
#define LL_RCC_MCO2_DIV_15                 RCC_MCO2CFGR_MCO2DIV_14  /*!< MCO divided by 15 */
#define LL_RCC_MCO2_DIV_16                 RCC_MCO2CFGR_MCO2DIV_15  /*!< MCO divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RTC_HSEDIV  HSE prescaler for RTC clock
  * @{
  */
#define LL_RCC_RTC_HSE_DIV_1                RCC_RTCDIVR_RTCDIV_1
#define LL_RCC_RTC_HSE_DIV_2                RCC_RTCDIVR_RTCDIV_2
#define LL_RCC_RTC_HSE_DIV_3                RCC_RTCDIVR_RTCDIV_3
#define LL_RCC_RTC_HSE_DIV_4                RCC_RTCDIVR_RTCDIV_4
#define LL_RCC_RTC_HSE_DIV_5                RCC_RTCDIVR_RTCDIV_5
#define LL_RCC_RTC_HSE_DIV_6                RCC_RTCDIVR_RTCDIV_6
#define LL_RCC_RTC_HSE_DIV_7                RCC_RTCDIVR_RTCDIV_7
#define LL_RCC_RTC_HSE_DIV_8                RCC_RTCDIVR_RTCDIV_8
#define LL_RCC_RTC_HSE_DIV_9                RCC_RTCDIVR_RTCDIV_9
#define LL_RCC_RTC_HSE_DIV_10               RCC_RTCDIVR_RTCDIV_10
#define LL_RCC_RTC_HSE_DIV_11               RCC_RTCDIVR_RTCDIV_11
#define LL_RCC_RTC_HSE_DIV_12               RCC_RTCDIVR_RTCDIV_12
#define LL_RCC_RTC_HSE_DIV_13               RCC_RTCDIVR_RTCDIV_13
#define LL_RCC_RTC_HSE_DIV_14               RCC_RTCDIVR_RTCDIV_14
#define LL_RCC_RTC_HSE_DIV_15               RCC_RTCDIVR_RTCDIV_15
#define LL_RCC_RTC_HSE_DIV_16               RCC_RTCDIVR_RTCDIV_16
#define LL_RCC_RTC_HSE_DIV_17               RCC_RTCDIVR_RTCDIV_17
#define LL_RCC_RTC_HSE_DIV_18               RCC_RTCDIVR_RTCDIV_18
#define LL_RCC_RTC_HSE_DIV_19               RCC_RTCDIVR_RTCDIV_19
#define LL_RCC_RTC_HSE_DIV_20               RCC_RTCDIVR_RTCDIV_20
#define LL_RCC_RTC_HSE_DIV_21               RCC_RTCDIVR_RTCDIV_21
#define LL_RCC_RTC_HSE_DIV_22               RCC_RTCDIVR_RTCDIV_22
#define LL_RCC_RTC_HSE_DIV_23               RCC_RTCDIVR_RTCDIV_23
#define LL_RCC_RTC_HSE_DIV_24               RCC_RTCDIVR_RTCDIV_24
#define LL_RCC_RTC_HSE_DIV_25               RCC_RTCDIVR_RTCDIV_25
#define LL_RCC_RTC_HSE_DIV_26               RCC_RTCDIVR_RTCDIV_26
#define LL_RCC_RTC_HSE_DIV_27               RCC_RTCDIVR_RTCDIV_27
#define LL_RCC_RTC_HSE_DIV_28               RCC_RTCDIVR_RTCDIV_28
#define LL_RCC_RTC_HSE_DIV_29               RCC_RTCDIVR_RTCDIV_29
#define LL_RCC_RTC_HSE_DIV_30               RCC_RTCDIVR_RTCDIV_30
#define LL_RCC_RTC_HSE_DIV_31               RCC_RTCDIVR_RTCDIV_31
#define LL_RCC_RTC_HSE_DIV_32               RCC_RTCDIVR_RTCDIV_32
#define LL_RCC_RTC_HSE_DIV_33               RCC_RTCDIVR_RTCDIV_33
#define LL_RCC_RTC_HSE_DIV_34               RCC_RTCDIVR_RTCDIV_34
#define LL_RCC_RTC_HSE_DIV_35               RCC_RTCDIVR_RTCDIV_35
#define LL_RCC_RTC_HSE_DIV_36               RCC_RTCDIVR_RTCDIV_36
#define LL_RCC_RTC_HSE_DIV_37               RCC_RTCDIVR_RTCDIV_37
#define LL_RCC_RTC_HSE_DIV_38               RCC_RTCDIVR_RTCDIV_38
#define LL_RCC_RTC_HSE_DIV_39               RCC_RTCDIVR_RTCDIV_39
#define LL_RCC_RTC_HSE_DIV_40               RCC_RTCDIVR_RTCDIV_40
#define LL_RCC_RTC_HSE_DIV_41               RCC_RTCDIVR_RTCDIV_41
#define LL_RCC_RTC_HSE_DIV_42               RCC_RTCDIVR_RTCDIV_42
#define LL_RCC_RTC_HSE_DIV_43               RCC_RTCDIVR_RTCDIV_43
#define LL_RCC_RTC_HSE_DIV_44               RCC_RTCDIVR_RTCDIV_44
#define LL_RCC_RTC_HSE_DIV_45               RCC_RTCDIVR_RTCDIV_45
#define LL_RCC_RTC_HSE_DIV_46               RCC_RTCDIVR_RTCDIV_46
#define LL_RCC_RTC_HSE_DIV_47               RCC_RTCDIVR_RTCDIV_47
#define LL_RCC_RTC_HSE_DIV_48               RCC_RTCDIVR_RTCDIV_48
#define LL_RCC_RTC_HSE_DIV_49               RCC_RTCDIVR_RTCDIV_49
#define LL_RCC_RTC_HSE_DIV_50               RCC_RTCDIVR_RTCDIV_50
#define LL_RCC_RTC_HSE_DIV_51               RCC_RTCDIVR_RTCDIV_51
#define LL_RCC_RTC_HSE_DIV_52               RCC_RTCDIVR_RTCDIV_52
#define LL_RCC_RTC_HSE_DIV_53               RCC_RTCDIVR_RTCDIV_53
#define LL_RCC_RTC_HSE_DIV_54               RCC_RTCDIVR_RTCDIV_54
#define LL_RCC_RTC_HSE_DIV_55               RCC_RTCDIVR_RTCDIV_55
#define LL_RCC_RTC_HSE_DIV_56               RCC_RTCDIVR_RTCDIV_56
#define LL_RCC_RTC_HSE_DIV_57               RCC_RTCDIVR_RTCDIV_57
#define LL_RCC_RTC_HSE_DIV_58               RCC_RTCDIVR_RTCDIV_58
#define LL_RCC_RTC_HSE_DIV_59               RCC_RTCDIVR_RTCDIV_59
#define LL_RCC_RTC_HSE_DIV_60               RCC_RTCDIVR_RTCDIV_60
#define LL_RCC_RTC_HSE_DIV_61               RCC_RTCDIVR_RTCDIV_61
#define LL_RCC_RTC_HSE_DIV_62               RCC_RTCDIVR_RTCDIV_62
#define LL_RCC_RTC_HSE_DIV_63               RCC_RTCDIVR_RTCDIV_63
#define LL_RCC_RTC_HSE_DIV_64               RCC_RTCDIVR_RTCDIV_64
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MPU_CLKSOURCE  MPU clock switch
  * @{
  */
#define LL_RCC_MPU_CLKSOURCE_HSI                RCC_MPCKSELR_MPUSRC_0 /*!< HSI selection as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_HSE                RCC_MPCKSELR_MPUSRC_1 /*!< HSE selection as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_PLL1               RCC_MPCKSELR_MPUSRC_2 /*!< PLL1 selection as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_MPUDIV             RCC_MPCKSELR_MPUSRC_3 /*!< MPUDIV selection as MPU clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MPU_CLKSOURCE_STATUS  MPU clock switch status
  * @{
  */
#define LL_RCC_MPU_CLKSOURCE_STATUS_HSI         RCC_MPCKSELR_MPUSRC_0 /*!< HSI used as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_STATUS_HSE         RCC_MPCKSELR_MPUSRC_1 /*!< HSE used as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_STATUS_PLL1        RCC_MPCKSELR_MPUSRC_2 /*!< PLL1 used as MPU clock */
#define LL_RCC_MPU_CLKSOURCE_STATUS_MPUDIV      RCC_MPCKSELR_MPUSRC_3 /*!< MPUDIV used as MPU clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MPU_DIV  MPUDIV prescaler
  * @{
  */
#define LL_RCC_MPU_DIV_OFF                      RCC_MPCKDIVR_MPUDIV_0  /*!< MPU div is disabled, no clock generated */
#define LL_RCC_MPU_DIV_2                        RCC_MPCKDIVR_MPUDIV_1  /*!< MPUSS is equal to pll1_p_ck divided by 2 */
#define LL_RCC_MPU_DIV_4                        RCC_MPCKDIVR_MPUDIV_2  /*!< MPUSS is equal to pll1_p_ck divided by 4 */
#define LL_RCC_MPU_DIV_8                        RCC_MPCKDIVR_MPUDIV_3  /*!< MPUSS is equal to pll1_p_ck divided by 8 */
#define LL_RCC_MPU_DIV_16                       RCC_MPCKDIVR_MPUDIV_4  /*!< MPUSS is equal to pll1_p_ck divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_AXISS_CLKSOURCE  AXISS clock switch
  * @{
  */
#define LL_RCC_AXISS_CLKSOURCE_HSI              RCC_ASSCKSELR_AXISSRC_0 /*!< HSI selection as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_HSE              RCC_ASSCKSELR_AXISSRC_1 /*!< HSE selection as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_PLL2             RCC_ASSCKSELR_AXISSRC_2 /*!< PLL2 selection as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_OFF              RCC_ASSCKSELR_AXISSRC_3 /*!< AXISS is gated */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_AXISS_CLKSOURCE_STATUS  AXISS clock switch status
  * @{
  */
#define LL_RCC_AXISS_CLKSOURCE_STATUS_HSI       RCC_ASSCKSELR_AXISSRC_0 /*!< HSI used as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_STATUS_HSE       RCC_ASSCKSELR_AXISSRC_1 /*!< HSE used as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_STATUS_PLL2      RCC_ASSCKSELR_AXISSRC_2 /*!< PLL2 used as AXISS clock */
#define LL_RCC_AXISS_CLKSOURCE_STATUS_OFF       RCC_ASSCKSELR_AXISSRC_3 /*!< AXISS is gated */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_AXI_DIV  AXI, AHB5 and AHB6 prescaler
  * @{
  */
#define LL_RCC_AXI_DIV_1                        RCC_AXIDIVR_AXIDIV_0  /*!< AXISS not divided */
#define LL_RCC_AXI_DIV_2                        RCC_AXIDIVR_AXIDIV_1  /*!< AXISS divided by 2 */
#define LL_RCC_AXI_DIV_3                        RCC_AXIDIVR_AXIDIV_2  /*!< AXISS divided by 3 */
#define LL_RCC_AXI_DIV_4                        RCC_AXIDIVR_AXIDIV_3  /*!< AXISS divided by 4 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCUSS_CLKSOURCE  MCUSS clock switch
  * @{
  */
#define LL_RCC_MCUSS_CLKSOURCE_HSI              RCC_MSSCKSELR_MCUSSRC_0 /*!< HSI selection as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_HSE              RCC_MSSCKSELR_MCUSSRC_1 /*!< HSE selection as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_CSI              RCC_MSSCKSELR_MCUSSRC_2 /*!< CSI selection as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_PLL3             RCC_MSSCKSELR_MCUSSRC_3 /*!< PLL3 selection as MCUSS clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCUSS_CLKSOURCE_STATUS  MCUSS clock switch status
  * @{
  */
#define LL_RCC_MCUSS_CLKSOURCE_STATUS_HSI       RCC_MSSCKSELR_MCUSSRC_0 /*!< HSI used as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_STATUS_HSE       RCC_MSSCKSELR_MCUSSRC_1 /*!< HSE used as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_STATUS_CSI       RCC_MSSCKSELR_MCUSSRC_2 /*!< CSI used as MCUSS clock */
#define LL_RCC_MCUSS_CLKSOURCE_STATUS_PLL3      RCC_MSSCKSELR_MCUSSRC_3 /*!< PLL3 used as MCUSS clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_MCU_DIV  MCUDIV prescaler
  * @{
  */
#define LL_RCC_MCU_DIV_1                        RCC_MCUDIVR_MCUDIV_0 /*!< MCUSS not divided */
#define LL_RCC_MCU_DIV_2                        RCC_MCUDIVR_MCUDIV_1 /*!< MCUSS divided by 2 */
#define LL_RCC_MCU_DIV_4                        RCC_MCUDIVR_MCUDIV_2 /*!< MCUSS divided by 4 */
#define LL_RCC_MCU_DIV_8                        RCC_MCUDIVR_MCUDIV_3 /*!< MCUSS divided by 8 */
#define LL_RCC_MCU_DIV_16                       RCC_MCUDIVR_MCUDIV_4 /*!< MCUSS divided by 16 */
#define LL_RCC_MCU_DIV_32                       RCC_MCUDIVR_MCUDIV_5 /*!< MCUSS divided by 32 */
#define LL_RCC_MCU_DIV_64                       RCC_MCUDIVR_MCUDIV_6 /*!< MCUSS divided by 64 */
#define LL_RCC_MCU_DIV_128                      RCC_MCUDIVR_MCUDIV_7 /*!< MCUSS divided by 128 */
#define LL_RCC_MCU_DIV_256                      RCC_MCUDIVR_MCUDIV_8 /*!< MCUSS divided by 256 */
#define LL_RCC_MCU_DIV_512                      RCC_MCUDIVR_MCUDIV_9 /*!< MCUSS divided by 512 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB1_DIV  APB1 prescaler
  * @{
  */
#define LL_RCC_APB1_DIV_1                       RCC_APB1DIVR_APB1DIV_0  /*!< mlhclk not divided (default after reset) */
#define LL_RCC_APB1_DIV_2                       RCC_APB1DIVR_APB1DIV_1  /*!< mlhclk divided by 2 */
#define LL_RCC_APB1_DIV_4                       RCC_APB1DIVR_APB1DIV_2  /*!< mlhclk divided by 4 */
#define LL_RCC_APB1_DIV_8                       RCC_APB1DIVR_APB1DIV_3  /*!< mlhclk divided by 8 */
#define LL_RCC_APB1_DIV_16                      RCC_APB1DIVR_APB1DIV_4  /*!< mlhclk divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB2_DIV  APB2 prescaler
  * @{
  */
#define LL_RCC_APB2_DIV_1                       RCC_APB2DIVR_APB2DIV_0  /*!< mlhclk not divided (default after reset) */
#define LL_RCC_APB2_DIV_2                       RCC_APB2DIVR_APB2DIV_1  /*!< mlhclk divided by 2 */
#define LL_RCC_APB2_DIV_4                       RCC_APB2DIVR_APB2DIV_2  /*!< mlhclk divided by 4 */
#define LL_RCC_APB2_DIV_8                       RCC_APB2DIVR_APB2DIV_3  /*!< mlhclk divided by 8 */
#define LL_RCC_APB2_DIV_16                      RCC_APB2DIVR_APB2DIV_4  /*!< mlhclk divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB3_DIV  APB3 prescaler
  * @{
  */
#define LL_RCC_APB3_DIV_1                       RCC_APB3DIVR_APB3DIV_0  /*!< mlhclk not divided (default after reset) */
#define LL_RCC_APB3_DIV_2                       RCC_APB3DIVR_APB3DIV_1  /*!< mlhclk divided by 2 */
#define LL_RCC_APB3_DIV_4                       RCC_APB3DIVR_APB3DIV_2  /*!< mlhclk divided by 4 */
#define LL_RCC_APB3_DIV_8                       RCC_APB3DIVR_APB3DIV_3  /*!< mlhclk divided by 8 */
#define LL_RCC_APB3_DIV_16                      RCC_APB3DIVR_APB3DIV_4  /*!< mlhclk divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB4_DIV  APB4 prescaler
  * @{
  */
#define LL_RCC_APB4_DIV_1                       RCC_APB4DIVR_APB4DIV_0  /*!< aclk not divided (default after reset) */
#define LL_RCC_APB4_DIV_2                       RCC_APB4DIVR_APB4DIV_1  /*!< aclk divided by 2 */
#define LL_RCC_APB4_DIV_4                       RCC_APB4DIVR_APB4DIV_2  /*!< aclk divided by 4 */
#define LL_RCC_APB4_DIV_8                       RCC_APB4DIVR_APB4DIV_3  /*!< aclk divided by 8 */
#define LL_RCC_APB4_DIV_16                      RCC_APB4DIVR_APB4DIV_4  /*!< aclk divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_APB5_DIV  APB5 prescaler
  * @{
  */
#define LL_RCC_APB5_DIV_1                       RCC_APB5DIVR_APB5DIV_0  /*!< aclk not divided (default after reset) */
#define LL_RCC_APB5_DIV_2                       RCC_APB5DIVR_APB5DIV_1  /*!< aclk divided by 2 */
#define LL_RCC_APB5_DIV_4                       RCC_APB5DIVR_APB5DIV_2  /*!< aclk divided by 4 */
#define LL_RCC_APB5_DIV_8                       RCC_APB5DIVR_APB5DIV_3  /*!< aclk divided by 8 */
#define LL_RCC_APB5_DIV_16                      RCC_APB5DIVR_APB5DIV_4  /*!< aclk divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LSEDRIVE  LSE oscillator drive capability
  * @{
  */
#define LL_RCC_LSEDRIVE_LOW                     RCC_BDCR_LSEDRV_0 /*!< Xtal mode lower driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMLOW               RCC_BDCR_LSEDRV_1 /*!< Xtal mode medium low driving capability */
#define LL_RCC_LSEDRIVE_MEDIUMHIGH              RCC_BDCR_LSEDRV_2 /*!< Xtal mode medium high driving capability */
#define LL_RCC_LSEDRIVE_HIGH                    RCC_BDCR_LSEDRV_3 /*!< Xtal mode higher driving capability */
/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RCC_LL_EC_PERIPH_FREQUENCY Peripheral clock frequency
  * @{
  */
#define LL_RCC_PERIPH_FREQUENCY_NO              0x00000000U /*!< No clock enabled for the peripheral */
#define LL_RCC_PERIPH_FREQUENCY_NA              0xFFFFFFFFU /*!< Frequency cannot be provided as external clock */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */


/** @defgroup RCC_LL_EC_I2Cx_CLKSOURCE  Peripheral I2C clock source selection
  * @{
  */
#define LL_RCC_I2C12_CLKSOURCE_PCLK1        LL_CLKSOURCE(RCC_OFFSET_I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, RCC_I2C12CKSELR_I2C12SRC_0)
#define LL_RCC_I2C12_CLKSOURCE_PLL4R        LL_CLKSOURCE(RCC_OFFSET_I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, RCC_I2C12CKSELR_I2C12SRC_1)
#define LL_RCC_I2C12_CLKSOURCE_HSI          LL_CLKSOURCE(RCC_OFFSET_I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, RCC_I2C12CKSELR_I2C12SRC_2)
#define LL_RCC_I2C12_CLKSOURCE_CSI          LL_CLKSOURCE(RCC_OFFSET_I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, RCC_I2C12CKSELR_I2C12SRC_3)

#define LL_RCC_I2C35_CLKSOURCE_PCLK1        LL_CLKSOURCE(RCC_OFFSET_I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, RCC_I2C35CKSELR_I2C35SRC_0)
#define LL_RCC_I2C35_CLKSOURCE_PLL4R        LL_CLKSOURCE(RCC_OFFSET_I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, RCC_I2C35CKSELR_I2C35SRC_1)
#define LL_RCC_I2C35_CLKSOURCE_HSI          LL_CLKSOURCE(RCC_OFFSET_I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, RCC_I2C35CKSELR_I2C35SRC_2)
#define LL_RCC_I2C35_CLKSOURCE_CSI          LL_CLKSOURCE(RCC_OFFSET_I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, RCC_I2C35CKSELR_I2C35SRC_3)

#define LL_RCC_I2C46_CLKSOURCE_PCLK5        LL_CLKSOURCE(RCC_OFFSET_I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, RCC_I2C46CKSELR_I2C46SRC_0)
#define LL_RCC_I2C46_CLKSOURCE_PLL3Q        LL_CLKSOURCE(RCC_OFFSET_I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, RCC_I2C46CKSELR_I2C46SRC_1)
#define LL_RCC_I2C46_CLKSOURCE_HSI          LL_CLKSOURCE(RCC_OFFSET_I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, RCC_I2C46CKSELR_I2C46SRC_2)
#define LL_RCC_I2C46_CLKSOURCE_CSI          LL_CLKSOURCE(RCC_OFFSET_I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, RCC_I2C46CKSELR_I2C46SRC_3)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SAIx_CLKSOURCE  Peripheral SAI clock source selection
  * @{
  */
#define LL_RCC_SAI1_CLKSOURCE_PLL4Q         LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, RCC_SAI1CKSELR_SAI1SRC_0)
#define LL_RCC_SAI1_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, RCC_SAI1CKSELR_SAI1SRC_1)
#define LL_RCC_SAI1_CLKSOURCE_I2SCKIN       LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, RCC_SAI1CKSELR_SAI1SRC_2)
#define LL_RCC_SAI1_CLKSOURCE_PER           LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, RCC_SAI1CKSELR_SAI1SRC_3)
#define LL_RCC_SAI1_CLKSOURCE_PLL3R         LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, RCC_SAI1CKSELR_SAI1SRC_4)

#define LL_RCC_SAI2_CLKSOURCE_PLL4Q         LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_0)
#define LL_RCC_SAI2_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_1)
#define LL_RCC_SAI2_CLKSOURCE_I2SCKIN       LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_2)
#define LL_RCC_SAI2_CLKSOURCE_PER           LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_3)
#define LL_RCC_SAI2_CLKSOURCE_SPDIF         LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_4)
#define LL_RCC_SAI2_CLKSOURCE_PLL3R         LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, RCC_SAI2CKSELR_SAI2SRC_5)

#define LL_RCC_SAI3_CLKSOURCE_PLL4Q         LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, RCC_SAI3CKSELR_SAI3SRC_0)
#define LL_RCC_SAI3_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, RCC_SAI3CKSELR_SAI3SRC_1)
#define LL_RCC_SAI3_CLKSOURCE_I2SCKIN       LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, RCC_SAI3CKSELR_SAI3SRC_2)
#define LL_RCC_SAI3_CLKSOURCE_PER           LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, RCC_SAI3CKSELR_SAI3SRC_3)
#define LL_RCC_SAI3_CLKSOURCE_PLL3R         LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, RCC_SAI3CKSELR_SAI3SRC_4)

#define LL_RCC_SAI4_CLKSOURCE_PLL4Q         LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, RCC_SAI4CKSELR_SAI4SRC_0)
#define LL_RCC_SAI4_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, RCC_SAI4CKSELR_SAI4SRC_1)
#define LL_RCC_SAI4_CLKSOURCE_I2SCKIN       LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, RCC_SAI4CKSELR_SAI4SRC_2)
#define LL_RCC_SAI4_CLKSOURCE_PER           LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, RCC_SAI4CKSELR_SAI4SRC_3)
#define LL_RCC_SAI4_CLKSOURCE_PLL3R         LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, RCC_SAI4CKSELR_SAI4SRC_4)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SPIx_CLKSOURCE  Peripheral SPI/I2S clock source selection
  * @{
  */
#define LL_RCC_SPI1_CLKSOURCE_PLL4P         LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, RCC_SPI2S1CKSELR_SPI1SRC_0)
#define LL_RCC_SPI1_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, RCC_SPI2S1CKSELR_SPI1SRC_1)
#define LL_RCC_SPI1_CLKSOURCE_I2SCKIN       LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, RCC_SPI2S1CKSELR_SPI1SRC_2)
#define LL_RCC_SPI1_CLKSOURCE_PER           LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, RCC_SPI2S1CKSELR_SPI1SRC_3)
#define LL_RCC_SPI1_CLKSOURCE_PLL3R         LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, RCC_SPI2S1CKSELR_SPI1SRC_4)

#define LL_RCC_SPI23_CLKSOURCE_PLL4P        LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, RCC_SPI2S23CKSELR_SPI23SRC_0)
#define LL_RCC_SPI23_CLKSOURCE_PLL3Q        LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, RCC_SPI2S23CKSELR_SPI23SRC_1)
#define LL_RCC_SPI23_CLKSOURCE_I2SCKIN      LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, RCC_SPI2S23CKSELR_SPI23SRC_2)
#define LL_RCC_SPI23_CLKSOURCE_PER          LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, RCC_SPI2S23CKSELR_SPI23SRC_3)
#define LL_RCC_SPI23_CLKSOURCE_PLL3R        LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, RCC_SPI2S23CKSELR_SPI23SRC_4)

#define LL_RCC_SPI45_CLKSOURCE_PCLK2        LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, RCC_SPI45CKSELR_SPI45SRC_0)
#define LL_RCC_SPI45_CLKSOURCE_PLL4Q        LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, RCC_SPI45CKSELR_SPI45SRC_1)
#define LL_RCC_SPI45_CLKSOURCE_HSI          LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, RCC_SPI45CKSELR_SPI45SRC_2)
#define LL_RCC_SPI45_CLKSOURCE_CSI          LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, RCC_SPI45CKSELR_SPI45SRC_3)
#define LL_RCC_SPI45_CLKSOURCE_HSE          LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, RCC_SPI45CKSELR_SPI45SRC_4)

#define LL_RCC_SPI6_CLKSOURCE_PCLK5         LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_0)
#define LL_RCC_SPI6_CLKSOURCE_PLL4Q         LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_1)
#define LL_RCC_SPI6_CLKSOURCE_HSI           LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_2)
#define LL_RCC_SPI6_CLKSOURCE_CSI           LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_3)
#define LL_RCC_SPI6_CLKSOURCE_HSE           LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_4)
#define LL_RCC_SPI6_CLKSOURCE_PLL3Q         LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, RCC_SPI6CKSELR_SPI6SRC_5)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USARTx_CLKSOURCE  Peripheral USART clock source selection
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE_PCLK5       LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_0)
#define LL_RCC_USART1_CLKSOURCE_PLL3Q       LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_1)
#define LL_RCC_USART1_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_2)
#define LL_RCC_USART1_CLKSOURCE_CSI         LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_3)
#define LL_RCC_USART1_CLKSOURCE_PLL4Q       LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_4)
#define LL_RCC_USART1_CLKSOURCE_HSE         LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, RCC_UART1CKSELR_UART1SRC_5)

#define LL_RCC_UART24_CLKSOURCE_PCLK1       LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, RCC_UART24CKSELR_UART24SRC_0)
#define LL_RCC_UART24_CLKSOURCE_PLL4Q       LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, RCC_UART24CKSELR_UART24SRC_1)
#define LL_RCC_UART24_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, RCC_UART24CKSELR_UART24SRC_2)
#define LL_RCC_UART24_CLKSOURCE_CSI         LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, RCC_UART24CKSELR_UART24SRC_3)
#define LL_RCC_UART24_CLKSOURCE_HSE         LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, RCC_UART24CKSELR_UART24SRC_4)

#define LL_RCC_UART35_CLKSOURCE_PCLK1       LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, RCC_UART35CKSELR_UART35SRC_0)
#define LL_RCC_UART35_CLKSOURCE_PLL4Q       LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, RCC_UART35CKSELR_UART35SRC_1)
#define LL_RCC_UART35_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, RCC_UART35CKSELR_UART35SRC_2)
#define LL_RCC_UART35_CLKSOURCE_CSI         LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, RCC_UART35CKSELR_UART35SRC_3)
#define LL_RCC_UART35_CLKSOURCE_HSE         LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, RCC_UART35CKSELR_UART35SRC_4)

#define LL_RCC_USART6_CLKSOURCE_PCLK2       LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, RCC_UART6CKSELR_UART6SRC_0)
#define LL_RCC_USART6_CLKSOURCE_PLL4Q       LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, RCC_UART6CKSELR_UART6SRC_1)
#define LL_RCC_USART6_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, RCC_UART6CKSELR_UART6SRC_2)
#define LL_RCC_USART6_CLKSOURCE_CSI         LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, RCC_UART6CKSELR_UART6SRC_3)
#define LL_RCC_USART6_CLKSOURCE_HSE         LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, RCC_UART6CKSELR_UART6SRC_4)

#define LL_RCC_UART78_CLKSOURCE_PCLK1       LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, RCC_UART78CKSELR_UART78SRC_0)
#define LL_RCC_UART78_CLKSOURCE_PLL4Q       LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, RCC_UART78CKSELR_UART78SRC_1)
#define LL_RCC_UART78_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, RCC_UART78CKSELR_UART78SRC_2)
#define LL_RCC_UART78_CLKSOURCE_CSI         LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, RCC_UART78CKSELR_UART78SRC_3)
#define LL_RCC_UART78_CLKSOURCE_HSE         LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, RCC_UART78CKSELR_UART78SRC_4)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SDMMCx_CLKSOURCE  Peripheral SDMMC clock source selection
  * @{
  */
#define LL_RCC_SDMMC12_CLKSOURCE_HCLK6      LL_CLKSOURCE(RCC_OFFSET_SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, RCC_SDMMC12CKSELR_SDMMC12SRC_0)
#define LL_RCC_SDMMC12_CLKSOURCE_PLL3R      LL_CLKSOURCE(RCC_OFFSET_SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, RCC_SDMMC12CKSELR_SDMMC12SRC_1)
#define LL_RCC_SDMMC12_CLKSOURCE_PLL4P      LL_CLKSOURCE(RCC_OFFSET_SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, RCC_SDMMC12CKSELR_SDMMC12SRC_2)
#define LL_RCC_SDMMC12_CLKSOURCE_HSI        LL_CLKSOURCE(RCC_OFFSET_SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, RCC_SDMMC12CKSELR_SDMMC12SRC_3)

#define LL_RCC_SDMMC3_CLKSOURCE_HCLK2       LL_CLKSOURCE(RCC_OFFSET_SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, RCC_SDMMC3CKSELR_SDMMC3SRC_0)
#define LL_RCC_SDMMC3_CLKSOURCE_PLL3R       LL_CLKSOURCE(RCC_OFFSET_SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, RCC_SDMMC3CKSELR_SDMMC3SRC_1)
#define LL_RCC_SDMMC3_CLKSOURCE_PLL4P       LL_CLKSOURCE(RCC_OFFSET_SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, RCC_SDMMC3CKSELR_SDMMC3SRC_2)
#define LL_RCC_SDMMC3_CLKSOURCE_HSI         LL_CLKSOURCE(RCC_OFFSET_SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, RCC_SDMMC3CKSELR_SDMMC3SRC_3)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_ETH_CLKSOURCE  Peripheral ETH clock source selection
  * @{
  */
#define LL_RCC_ETH_CLKSOURCE_PLL4P          RCC_ETHCKSELR_ETHSRC_0
#define LL_RCC_ETH_CLKSOURCE_PLL3Q          RCC_ETHCKSELR_ETHSRC_1
#define LL_RCC_ETH_CLKSOURCE_OFF            RCC_ETHCKSELR_ETHSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_QSPI_CLKSOURCE  Peripheral QSPI clock source selection
  * @{
  */
#define LL_RCC_QSPI_CLKSOURCE_ACLK          RCC_QSPICKSELR_QSPISRC_0
#define LL_RCC_QSPI_CLKSOURCE_PLL3R         RCC_QSPICKSELR_QSPISRC_1
#define LL_RCC_QSPI_CLKSOURCE_PLL4P         RCC_QSPICKSELR_QSPISRC_2
#define LL_RCC_QSPI_CLKSOURCE_PER           RCC_QSPICKSELR_QSPISRC_3
/**
  * @}
  */

/** @defgroup RCC_LL_EC_FMC_CLKSOURCE  Peripheral FMC clock source selection
  * @{
  */
#define LL_RCC_FMC_CLKSOURCE_ACLK           RCC_FMCCKSELR_FMCSRC_0
#define LL_RCC_FMC_CLKSOURCE_PLL3R          RCC_FMCCKSELR_FMCSRC_1
#define LL_RCC_FMC_CLKSOURCE_PLL4P          RCC_FMCCKSELR_FMCSRC_2
#define LL_RCC_FMC_CLKSOURCE_PER            RCC_FMCCKSELR_FMCSRC_3
/**
  * @}
  */

/** @defgroup RCC_LL_EC_FDCAN_CLKSOURCE  Peripheral FDCAN clock source selection
  * @{
  */
#define LL_RCC_FDCAN_CLKSOURCE_HSE          RCC_FDCANCKSELR_FDCANSRC_0
#define LL_RCC_FDCAN_CLKSOURCE_PLL3Q        RCC_FDCANCKSELR_FDCANSRC_1
#define LL_RCC_FDCAN_CLKSOURCE_PLL4Q        RCC_FDCANCKSELR_FDCANSRC_2
#define LL_RCC_FDCAN_CLKSOURCE_PLL4R        RCC_FDCANCKSELR_FDCANSRC_3
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SPDIFRX_CLKSOURCE  Peripheral SPDIFRX clock source selection
  * @{
  */
#define LL_RCC_SPDIFRX_CLKSOURCE_PLL4P      RCC_SPDIFCKSELR_SPDIFSRC_0
#define LL_RCC_SPDIFRX_CLKSOURCE_PLL3Q      RCC_SPDIFCKSELR_SPDIFSRC_1
#define LL_RCC_SPDIFRX_CLKSOURCE_HSI        RCC_SPDIFCKSELR_SPDIFSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CEC_CLKSOURCE  Peripheral CEC clock source selection
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE_LSE            RCC_CECCKSELR_CECSRC_0
#define LL_RCC_CEC_CLKSOURCE_LSI            RCC_CECCKSELR_CECSRC_1
#define LL_RCC_CEC_CLKSOURCE_CSI122         RCC_CECCKSELR_CECSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USBPHY_CLKSOURCE  Peripheral USBPHY clock source selection
  * @{
  */
#define LL_RCC_USBPHY_CLKSOURCE_HSE         RCC_USBCKSELR_USBPHYSRC_0
#define LL_RCC_USBPHY_CLKSOURCE_PLL4R       RCC_USBCKSELR_USBPHYSRC_1
#define LL_RCC_USBPHY_CLKSOURCE_HSE2        RCC_USBCKSELR_USBPHYSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USBO_CLKSOURCE  Peripheral USBO clock source selection
  * @{
  */
#define LL_RCC_USBO_CLKSOURCE_PLL4R         RCC_USBCKSELR_USBOSRC_0
#define LL_RCC_USBO_CLKSOURCE_PHY           RCC_USBCKSELR_USBOSRC_1
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RNGx_CLKSOURCE  Peripheral RNG clock source selection
  * @{
  */
#define LL_RCC_RNG1_CLKSOURCE_CSI           LL_CLKSOURCE(RCC_OFFSET_RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, RCC_RNG1CKSELR_RNG1SRC_0)
#define LL_RCC_RNG1_CLKSOURCE_PLL4R         LL_CLKSOURCE(RCC_OFFSET_RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, RCC_RNG1CKSELR_RNG1SRC_1)
#define LL_RCC_RNG1_CLKSOURCE_LSE           LL_CLKSOURCE(RCC_OFFSET_RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, RCC_RNG1CKSELR_RNG1SRC_2)
#define LL_RCC_RNG1_CLKSOURCE_LSI           LL_CLKSOURCE(RCC_OFFSET_RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, RCC_RNG1CKSELR_RNG1SRC_3)

#define LL_RCC_RNG2_CLKSOURCE_CSI           LL_CLKSOURCE(RCC_OFFSET_RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, RCC_RNG2CKSELR_RNG2SRC_0)
#define LL_RCC_RNG2_CLKSOURCE_PLL4R         LL_CLKSOURCE(RCC_OFFSET_RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, RCC_RNG2CKSELR_RNG2SRC_1)
#define LL_RCC_RNG2_CLKSOURCE_LSE           LL_CLKSOURCE(RCC_OFFSET_RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, RCC_RNG2CKSELR_RNG2SRC_2)
#define LL_RCC_RNG2_CLKSOURCE_LSI           LL_CLKSOURCE(RCC_OFFSET_RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, RCC_RNG2CKSELR_RNG2SRC_3)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CKPER_CLKSOURCE  Peripheral CKPER clock source selection
  * @{
  */
#define LL_RCC_CKPER_CLKSOURCE_HSI          RCC_CPERCKSELR_CKPERSRC_0
#define LL_RCC_CKPER_CLKSOURCE_CSI          RCC_CPERCKSELR_CKPERSRC_1
#define LL_RCC_CKPER_CLKSOURCE_HSE          RCC_CPERCKSELR_CKPERSRC_2
#define LL_RCC_CKPER_CLKSOURCE_OFF          RCC_CPERCKSELR_CKPERSRC_3 /*Clock disabled*/
/**
  * @}
  */

/** @defgroup RCC_LL_EC_STGEN_CLKSOURCE  Peripheral STGEN clock source selection
  * @{
  */
#define LL_RCC_STGEN_CLKSOURCE_HSI          RCC_STGENCKSELR_STGENSRC_0
#define LL_RCC_STGEN_CLKSOURCE_HSE          RCC_STGENCKSELR_STGENSRC_1
#define LL_RCC_STGEN_CLKSOURCE_OFF          RCC_STGENCKSELR_STGENSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_DSI_CLKSOURCE  Peripheral  DSI clock source selection
  * @{
  */
#define LL_RCC_DSI_CLKSOURCE_PHY            RCC_DSICKSELR_DSISRC_0
#define LL_RCC_DSI_CLKSOURCE_PLL4P          RCC_DSICKSELR_DSISRC_1
/**
  * @}
  */

/** @defgroup RCC_LL_EC_ADC_CLKSOURCE  Peripheral ADC clock source selection
  * @{
  */
#define LL_RCC_ADC_CLKSOURCE_PLL4R          RCC_ADCCKSELR_ADCSRC_0
#define LL_RCC_ADC_CLKSOURCE_PER            RCC_ADCCKSELR_ADCSRC_1
#define LL_RCC_ADC_CLKSOURCE_PLL3Q          RCC_ADCCKSELR_ADCSRC_2
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIMx_CLKSOURCE  Peripheral LPTIM clock source selection
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE_PCLK1       LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_0)
#define LL_RCC_LPTIM1_CLKSOURCE_PLL4P       LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_1)
#define LL_RCC_LPTIM1_CLKSOURCE_PLL3Q       LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_2)
#define LL_RCC_LPTIM1_CLKSOURCE_LSE         LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_3)
#define LL_RCC_LPTIM1_CLKSOURCE_LSI         LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_4)
#define LL_RCC_LPTIM1_CLKSOURCE_PER         LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_5)
#define LL_RCC_LPTIM1_CLKSOURCE_OFF         LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, RCC_LPTIM1CKSELR_LPTIM1SRC_6)

#define LL_RCC_LPTIM23_CLKSOURCE_PCLK3      LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_0)
#define LL_RCC_LPTIM23_CLKSOURCE_PLL4Q      LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_1)
#define LL_RCC_LPTIM23_CLKSOURCE_PER        LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_2)
#define LL_RCC_LPTIM23_CLKSOURCE_LSE        LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_3)
#define LL_RCC_LPTIM23_CLKSOURCE_LSI        LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_4)
#define LL_RCC_LPTIM23_CLKSOURCE_OFF        LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, RCC_LPTIM23CKSELR_LPTIM23SRC_5)

#define LL_RCC_LPTIM45_CLKSOURCE_PCLK3      LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_0)
#define LL_RCC_LPTIM45_CLKSOURCE_PLL4P      LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_1)
#define LL_RCC_LPTIM45_CLKSOURCE_PLL3Q      LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_2)
#define LL_RCC_LPTIM45_CLKSOURCE_LSE        LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_3)
#define LL_RCC_LPTIM45_CLKSOURCE_LSI        LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_4)
#define LL_RCC_LPTIM45_CLKSOURCE_PER        LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_5)
#define LL_RCC_LPTIM45_CLKSOURCE_OFF        LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, RCC_LPTIM45CKSELR_LPTIM45SRC_6)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_TIMGx_Prescaler_Selection TIMG Prescaler selection
  * @{
  */
#define LL_RCC_TIMG1PRES_DEACTIVATED        LL_CLKSOURCE(RCC_OFFSET_TIMG1PRER, RCC_TIMG1PRER_TIMG1PRE, RCC_TIMG1PRER_TIMG1PRE_0)
#define LL_RCC_TIMG1PRES_ACTIVATED          LL_CLKSOURCE(RCC_OFFSET_TIMG1PRER, RCC_TIMG1PRER_TIMG1PRE, RCC_TIMG1PRER_TIMG1PRE_1)

#define LL_RCC_TIMG2PRES_DEACTIVATED        LL_CLKSOURCE(RCC_OFFSET_TIMG2PRER, RCC_TIMG2PRER_TIMG2PRE, RCC_TIMG2PRER_TIMG2PRE_0)
#define LL_RCC_TIMG2PRES_ACTIVATED          LL_CLKSOURCE(RCC_OFFSET_TIMG2PRER, RCC_TIMG2PRER_TIMG2PRE, RCC_TIMG2PRER_TIMG2PRE_1)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RTC_CLKSOURCE  RTC clock source selection
  * @{
  */
#define LL_RCC_RTC_CLKSOURCE_NONE           RCC_BDCR_RTCSRC_0  /*!< No clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSE            RCC_BDCR_RTCSRC_1  /*!< LSE oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_LSI            RCC_BDCR_RTCSRC_2  /*!< LSI oscillator clock used as RTC clock */
#define LL_RCC_RTC_CLKSOURCE_HSE_DIV        RCC_BDCR_RTCSRC_3  /*!< HSE oscillator clock divided by RTCDIV used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_I2Cx  Peripheral I2C get clock source
  * @{
  */
#define LL_RCC_I2C12_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, 0x00000000U)
#define LL_RCC_I2C35_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, 0x00000000U)
#define LL_RCC_I2C46_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SAIx  Peripheral SAI get clock source
  * @{
  */
#define LL_RCC_SAI1_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, 0x00000000U)
#define LL_RCC_SAI2_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, 0x00000000U)
#define LL_RCC_SAI3_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, 0x00000000U)
#define LL_RCC_SAI4_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_DFSDM  Peripheral DFSDM get clock source
  * @{
  */
/* @note DFSDM and SA1 share the same kernel clock Muxer */
#define LL_RCC_DFSDM_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SPIx  Peripheral SPI/I2S get clock source
  * @{
  */
#define LL_RCC_SPI1_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, 0x00000000U)
#define LL_RCC_SPI23_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, 0x00000000U)
#define LL_RCC_SPI45_CLKSOURCE        LL_CLKSOURCE(RCC_OFFSET_SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, 0x00000000U)
#define LL_RCC_SPI6_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USARTx  Peripheral USART get clock source
  * @{
  */
#define LL_RCC_USART1_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_UART1CKSELR, RCC_UART1CKSELR_UART1SRC, 0x00000000U)
#define LL_RCC_UART24_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_UART24CKSELR, RCC_UART24CKSELR_UART24SRC, 0x00000000U)
#define LL_RCC_UART35_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_UART35CKSELR, RCC_UART35CKSELR_UART35SRC, 0x00000000U)
#define LL_RCC_USART6_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_UART6CKSELR, RCC_UART6CKSELR_UART6SRC, 0x00000000U)
#define LL_RCC_UART78_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_UART78CKSELR, RCC_UART78CKSELR_UART78SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SDMMCx  Peripheral SDMMC get clock source
  * @{
  */
#define LL_RCC_SDMMC12_CLKSOURCE      LL_CLKSOURCE(RCC_OFFSET_SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, 0x00000000U)
#define LL_RCC_SDMMC3_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_ETH  Peripheral ETH get clock source
  * @{
  */
#define LL_RCC_ETH_CLKSOURCE          RCC_ETHCKSELR_ETHSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_QSPI  Peripheral QSPI get clock source
  * @{
  */
#define LL_RCC_QSPI_CLKSOURCE         RCC_QSPICKSELR_QSPISRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_FMC  Peripheral FMC get clock source
  * @{
  */
#define LL_RCC_FMC_CLKSOURCE          RCC_FMCCKSELR_FMCSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_FDCAN  Peripheral FDCAN get clock source
  * @{
  */
#define LL_RCC_FDCAN_CLKSOURCE        RCC_FDCANCKSELR_FDCANSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_SPDIFRX  Peripheral SPDIFRX get clock source
  * @{
  */
#define LL_RCC_SPDIFRX_CLKSOURCE      RCC_SPDIFCKSELR_SPDIFSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CEC Peripheral CEC get clock source
  * @{
  */
#define LL_RCC_CEC_CLKSOURCE          RCC_CECCKSELR_CECSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USBPHY Peripheral USBPHY get clock source
  * @{
  */
#define LL_RCC_USBPHY_CLKSOURCE       RCC_USBCKSELR_USBPHYSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_USBO  Peripheral USBO get clock source
  * @{
  */
#define LL_RCC_USBO_CLKSOURCE         RCC_USBCKSELR_USBOSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_RNGx Peripheral RNG get clock source
  * @{
  */
#define LL_RCC_RNG1_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, 0x00000000U)
#define LL_RCC_RNG2_CLKSOURCE         LL_CLKSOURCE(RCC_OFFSET_RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_CKPER Peripheral CKPER get clock source
  * @{
  */
#define LL_RCC_CKPER_CLKSOURCE        RCC_CPERCKSELR_CKPERSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_STGEN Peripheral STGEN get clock source
  * @{
  */
#define LL_RCC_STGEN_CLKSOURCE        RCC_STGENCKSELR_STGENSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_DSI Peripheral  DSI get clock source
  * @{
  */
#define LL_RCC_DSI_CLKSOURCE          RCC_DSICKSELR_DSISRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_ADC Peripheral ADC get clock source
  * @{
  */
#define LL_RCC_ADC_CLKSOURCE          RCC_ADCCKSELR_ADCSRC
/**
  * @}
  */

/** @defgroup RCC_LL_EC_LPTIMx Peripheral LPTIM get clock source
  * @{
  */
#define LL_RCC_LPTIM1_CLKSOURCE       LL_CLKSOURCE(RCC_OFFSET_LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, 0x00000000U)
#define LL_RCC_LPTIM23_CLKSOURCE      LL_CLKSOURCE(RCC_OFFSET_LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, 0x00000000U)
#define LL_RCC_LPTIM45_CLKSOURCE      LL_CLKSOURCE(RCC_OFFSET_LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_TIMGx_Prescaler TIMG get prescaler selection
  * @{
  */
#define LL_RCC_TIMG1PRES              LL_CLKSOURCE(RCC_OFFSET_TIMG1PRER, RCC_TIMG1PRER_TIMG1PRE, 0x00000000U)
#define LL_RCC_TIMG2PRES              LL_CLKSOURCE(RCC_OFFSET_TIMG2PRER, RCC_TIMG2PRER_TIMG2PRE, 0x00000000U)
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL12SOURCE  PLL1 and PLL2 entry clock source
  * @{
  */
#define LL_RCC_PLL12SOURCE_HSI        RCC_RCK12SELR_PLL12SRC_0  /*!< HSI clock selected as PLL12 entry clock source */
#define LL_RCC_PLL12SOURCE_HSE        RCC_RCK12SELR_PLL12SRC_1  /*!< HSE clock selected as PLL12 entry clock source */
#define LL_RCC_PLL12SOURCE_NONE       RCC_RCK12SELR_PLL12SRC_2  /*!< No clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL3SOURCE  PLL3 entry clock source
  * @{
  */
#define LL_RCC_PLL3SOURCE_HSI         RCC_RCK3SELR_PLL3SRC_0  /*!< HSI clock selected as PLL3 entry clock source */
#define LL_RCC_PLL3SOURCE_HSE         RCC_RCK3SELR_PLL3SRC_1  /*!< HSE clock selected as PLL3 entry clock source */
#define LL_RCC_PLL3SOURCE_CSI         RCC_RCK3SELR_PLL3SRC_2  /*!< CSI clock selected as PLL3 entry clock source */
#define LL_RCC_PLL3SOURCE_NONE        RCC_RCK3SELR_PLL3SRC_3  /*!< No clock */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL4SOURCE  PLL4 entry clock source
  * @{
  */
#define LL_RCC_PLL4SOURCE_HSI         RCC_RCK4SELR_PLL4SRC_0  /*!< HSI clock selected as PLL4 entry clock source */
#define LL_RCC_PLL4SOURCE_HSE         RCC_RCK4SELR_PLL4SRC_1  /*!< HSE clock selected as PLL4 entry clock source */
#define LL_RCC_PLL4SOURCE_CSI         RCC_RCK4SELR_PLL4SRC_2  /*!< CSI clock selected as PLL4 entry clock source */
#define LL_RCC_PLL4SOURCE_I2SCKIN     RCC_RCK4SELR_PLL4SRC_3  /*!< Signal I2S_CKIN selected as PLL4 entry clock source */
/**
  * @}
  */

/** @defgroup RCC_LL_PLL3_IF_Range RCC PLL3 Input Frequency Range
  * @{
  */
#define LL_RCC_PLL3IFRANGE_0          RCC_PLL3CFGR1_IFRGE_0 /*!< The PLL3 input (ref3_ck) clock range frequency
                                                                 is between 4 and 8 MHz (default after reset) */
#define LL_RCC_PLL3IFRANGE_1          RCC_PLL3CFGR1_IFRGE_1 /*!< The PLL3 input (ref3_ck) clock range frequency
                                                                 is between 8 and 16 MHz */
/**
  * @}
  */

/** @defgroup RCC_LL_PLL4_IF_Range RCC PLL4 Input Frequency Range
  * @{
  */
#define LL_RCC_PLL4IFRANGE_0          RCC_PLL4CFGR1_IFRGE_0 /*!< The PLL4 input (ref4_ck) clock range frequency
                                                                 is between 4 and 8 MHz (default after reset) */
#define LL_RCC_PLL4IFRANGE_1          RCC_PLL4CFGR1_IFRGE_1 /*!< The PLL4 input (ref4_ck) clock range frequency
                                                                 is between 8 and 16 MHz */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL1_SSCG_MODE  PLL1 Spread Spectrum Mode
  * @{
  */
#define LL_RCC_PLL1SSCG_CENTER_SPREAD          0x00000000U            /*!< Center-spread modulation selected (default after reset) */
#define LL_RCC_PLL1SSCG_DOWN_SPREAD            RCC_PLL1CSGR_SSCG_MODE /*!< Down-spread modulation selected */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL1_RPDFN_DIS  Dithering RPDF PLL1 noise control
  * @{
  */
#define LL_RCC_PLL1RPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL1RPDFN_DIS_DISABLED          RCC_PLL1CSGR_RPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL1_TPDFN_DIS  Dithering TPDFN PLL1 noise control
  * @{
  */
#define LL_RCC_PLL1TPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL1TPDFN_DIS_DISABLED          RCC_PLL1CSGR_TPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL2_SSCG_MODE  PLL2 Spread Spectrum Mode
  * @{
  */
#define LL_RCC_PLL2SSCG_CENTER_SPREAD          0x00000000U            /*!< Center-spread modulation selected (default after reset) */
#define LL_RCC_PLL2SSCG_DOWN_SPREAD            RCC_PLL2CSGR_SSCG_MODE /*!< Down-spread modulation selected */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL2_RPDFN_DIS  Dithering RPDF PLL2 noise control
  * @{
  */
#define LL_RCC_PLL2RPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL2RPDFN_DIS_DISABLED          RCC_PLL2CSGR_RPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL2_TPDFN_DIS  Dithering TPDFN PLL2 noise control
  * @{
  */
#define LL_RCC_PLL2TPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL2TPDFN_DIS_DISABLED          RCC_PLL2CSGR_TPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL3_SSCG_MODE  PLL3 Spread Spectrum Mode
  * @{
  */
#define LL_RCC_PLL3SSCG_CENTER_SPREAD          0x00000000U            /*!< Center-spread modulation selected (default after reset) */
#define LL_RCC_PLL3SSCG_DOWN_SPREAD            RCC_PLL3CSGR_SSCG_MODE /*!< Down-spread modulation selected */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL3_RPDFN_DIS  Dithering RPDF PLL3 noise control
  * @{
  */
#define LL_RCC_PLL3RPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL3RPDFN_DIS_DISABLED          RCC_PLL3CSGR_RPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL3_TPDFN_DIS  Dithering TPDFN PLL3 noise control
  * @{
  */
#define LL_RCC_PLL3TPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL3TPDFN_DIS_DISABLED          RCC_PLL3CSGR_TPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL4_SSCG_MODE  PLL4 Spread Spectrum Mode
  * @{
  */
#define LL_RCC_PLL4SSCG_CENTER_SPREAD          0x00000000U            /*!< Center-spread modulation selected (default after reset) */
#define LL_RCC_PLL4SSCG_DOWN_SPREAD            RCC_PLL4CSGR_SSCG_MODE /*!< Down-spread modulation selected */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL4_RPDFN_DIS  Dithering RPDF PLL4 noise control
  * @{
  */
#define LL_RCC_PLL4RPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL4RPDFN_DIS_DISABLED          RCC_PLL4CSGR_RPDFN_DIS /*!< Dithering noise injection disabled */
/**
  * @}
  */

/** @defgroup RCC_LL_EC_PLL4_TPDFN_DIS  Dithering TPDFN PLL4 noise control
  * @{
  */
#define LL_RCC_PLL4TPDFN_DIS_ENABLED           0x00000000U            /*!< Dithering noise injection enabled (default after reset) */
#define LL_RCC_PLL4TPDFN_DIS_DISABLED          RCC_PLL4CSGR_TPDFN_DIS /*!< Dithering noise injection disabled */
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
  * @brief  Helper macro to calculate the MPUDIV frequency
  * @param  __PLL1PINPUTCLKFREQ__ Frequency of the input of mpudiv (based on PLL1P)
  * @param  __MPUDIVPRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MPU_DIV_2
  *         @arg @ref LL_RCC_MPU_DIV_4
  *         @arg @ref LL_RCC_MPU_DIV_8
  *         @arg @ref LL_RCC_MPU_DIV_16
  * @retval MPUDIV clock frequency (in Hz)
  */
#define LL_RCC_CALC_MPUDIV_FREQ(__PLL1PINPUTCLKFREQ__, __MPUDIVPRESCALER__) ((__PLL1PINPUTCLKFREQ__) >> (__MPUDIVPRESCALER__))

/**
  * @brief  Helper macro to calculate the ACLK, HCLK5 and HCLK6 frequencies
  * @param  __ACLKINPUTCLKFREQ__ Frequency of the input of aclk_ck (based on HSI/HSE/PLL2P)
  * @param  __AXIPRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_AXI_DIV_1
  *         @arg @ref LL_RCC_AXI_DIV_2
  *         @arg @ref LL_RCC_AXI_DIV_3
  *         @arg @ref LL_RCC_AXI_DIV_4
  * @retval ACLK clock frequency (in Hz)
  */
#define LL_RCC_CALC_ACLK_FREQ(__ACLKINPUTCLKFREQ__, __AXIPRESCALER__) ((__ACLKINPUTCLKFREQ__) / (__AXIPRESCALER__ + 1U))

/**
  * @brief  Helper macro to calculate the PCLK4 frequency (APB4)
  * @param  __ACLKFREQ__ ACLK frequency
  * @param  __APB4PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB4_DIV_1
  *         @arg @ref LL_RCC_APB4_DIV_2
  *         @arg @ref LL_RCC_APB4_DIV_4
  *         @arg @ref LL_RCC_APB4_DIV_8
  *         @arg @ref LL_RCC_APB4_DIV_16
  * @retval PCLK4 clock frequency (in Hz)
  */
#define LL_RCC_CALC_PCLK4_FREQ(__ACLKFREQ__, __APB4PRESCALER__) ((__ACLKFREQ__) >> (__APB4PRESCALER__))

/**
  * @brief  Helper macro to calculate the PCLK5 frequency (APB5)
  * @param  __ACLKFREQ__ ACLK frequency
  * @param  __APB5PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB5_DIV_1
  *         @arg @ref LL_RCC_APB5_DIV_2
  *         @arg @ref LL_RCC_APB5_DIV_4
  *         @arg @ref LL_RCC_APB5_DIV_8
  *         @arg @ref LL_RCC_APB5_DIV_16
  * @retval PCLK5 clock frequency (in Hz)
  */
#define LL_RCC_CALC_PCLK5_FREQ(__ACLKFREQ__, __APB5PRESCALER__) ((__ACLKFREQ__) >> (__APB5PRESCALER__))

/**
  * @brief  Helper macro to calculate the MLHCLK, MCU, FCLK, HCLK4, HCLK3, HCLK2 and HCLK1 frequencies
  * @param  __MLHCLKINPUTCLKFREQ__ Frequency of the input of mlhclk_ck (based on HSI/HSE/CSI/PLL3P)
  * @param  __MCUPRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCU_DIV_1
  *         @arg @ref LL_RCC_MCU_DIV_2
  *         @arg @ref LL_RCC_MCU_DIV_4
  *         @arg @ref LL_RCC_MCU_DIV_8
  *         @arg @ref LL_RCC_MCU_DIV_16
  *         @arg @ref LL_RCC_MCU_DIV_32
  *         @arg @ref LL_RCC_MCU_DIV_64
  *         @arg @ref LL_RCC_MCU_DIV_128
  *         @arg @ref LL_RCC_MCU_DIV_256
  *         @arg @ref LL_RCC_MCU_DIV_512
  * @retval MLHCLK clock frequency (in Hz)
  */
#define LL_RCC_CALC_MLHCLK_FREQ(__MLHCLKINPUTCLKFREQ__, __MCUPRESCALER__) ((__MLHCLKINPUTCLKFREQ__) >> (__MCUPRESCALER__))

/**
  * @brief  Helper macro to calculate the PCLK1 frequency
  * @param  __MLHCLKFREQ__ MLHCLK frequency
  * @param  __APB1PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  * @retval PCLK1 clock frequency (in Hz)
  */
#define LL_RCC_CALC_PCLK1_FREQ(__MLHCLKFREQ__, __APB1PRESCALER__) ((__MLHCLKFREQ__) >> (__APB1PRESCALER__))

/**
  * @brief  Helper macro to calculate the PCLK2 frequency
  * @param  __MLHCLKFREQ__ MLHCLK frequency
  * @param  __APB2PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  * @retval PCLK2 clock frequency (in Hz)
  */
#define LL_RCC_CALC_PCLK2_FREQ(__MLHCLKFREQ__, __APB2PRESCALER__) ((__MLHCLKFREQ__) >> (__APB2PRESCALER__))

/**
  * @brief  Helper macro to calculate the PCLK3 frequency
  * @param  __MLHCLKFREQ__ MLHCLK frequency
  * @param  __APB3PRESCALER__ This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB3_DIV_1
  *         @arg @ref LL_RCC_APB3_DIV_2
  *         @arg @ref LL_RCC_APB3_DIV_4
  *         @arg @ref LL_RCC_APB3_DIV_8
  *         @arg @ref LL_RCC_APB3_DIV_16
  * @retval PCLK3 clock frequency (in Hz)
  */
#define LL_RCC_CALC_PCLK3_FREQ(__MLHCLKFREQ__, __APB3PRESCALER__) ((__MLHCLKFREQ__) >> (__APB3PRESCALER__))

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
  * @note Once HSE Clock Security System is enabled it cannot be changed anymore unless
  *       a reset occurs or system enter in standby mode.
  * @rmtoll OCENSETR       HSECSSON         LL_RCC_HSE_EnableCSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableCSS(void)
{
  SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSECSSON);
}


/**
  * @brief  Enable HSE external digital oscillator (HSE Digital Bypass)
  * @rmtoll OCENSETR     DIGBYP       LL_RCC_HSE_EnableDigBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableDigBypass(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_DIGBYP);
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_HSEBYP);
}

/**
  * @brief  Disable HSE external digital oscillator (HSE Digital Bypass)
  * @rmtoll OCENCLRR     DIGBYP       LL_RCC_HSE_DisableDigBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_DisableDigBypass(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_DIGBYP);
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSEBYP);
}

/**
  * @brief  Enable HSE external oscillator (HSE Bypass)
  * @rmtoll OCENSETR     HSEBYP        LL_RCC_HSE_EnableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_EnableBypass(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_HSEBYP);
}

/**
  * @brief  Disable HSE external oscillator (HSE Bypass)
  * @rmtoll OCENCLRR     HSEBYP        LL_RCC_HSE_DisableBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_DisableBypass(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSEBYP);
}

/**
  * @brief  Enable HSE crystal oscillator (HSE ON)
  * @rmtoll OCENCLRR      HSEON         LL_RCC_HSE_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_Enable(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_HSEON);
}

/**
  * @brief  Disable HSE crystal oscillator (HSE ON)
  * @rmtoll OCENCLRR           HSEON         LL_RCC_HSE_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSE_Disable(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSEON);
}

/**
  * @brief  Check if HSE oscillator is Ready
  * @rmtoll OCRDYR           HSERDY        LL_RCC_HSE_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSE_IsReady(void)
{
  return ((READ_BIT(RCC->OCRDYR, RCC_OCRDYR_HSERDY) == RCC_OCRDYR_HSERDY) ? 1UL : 0UL);
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
  * @rmtoll OCENSETR     HSIKERON      LL_RCC_HSI_EnableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_EnableInStopMode(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_HSIKERON);
}

/**
  * @brief  Disable HSI in stop mode
  * @rmtoll OCENCLRR     HSIKERON      LL_RCC_HSI_DisableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_DisableInStopMode(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSIKERON);
}

/**
  * @brief  Enable HSI oscillator
  * @rmtoll OCENSETR     HSION         LL_RCC_HSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_Enable(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_HSION);
}

/**
  * @brief  Disable HSI oscillator
  * @rmtoll OCENCLRR     HSION         LL_RCC_HSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_Disable(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSION);
}

/**
  * @brief  Check if HSI clock is ready
  * @rmtoll OCRDYR       HSIRDY        LL_RCC_HSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_IsReady(void)
{
  return ((READ_BIT(RCC->OCRDYR, RCC_OCRDYR_HSIRDY) == RCC_OCRDYR_HSIRDY) ? 1UL : 0UL);
}

/**
  * @brief  Get HSI Calibration value
  * @note When HSITRIM is written, HSICAL is updated with the sum of
  *       HSITRIM and the factory trim value
  * @rmtoll HSICFGR        HSICAL        LL_RCC_HSI_GetCalibration
  * @retval Between Min_Data = 0x00 and Max_Data = 0xFFF
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->HSICFGR, RCC_HSICFGR_HSICAL) >> RCC_HSICFGR_HSICAL_Pos);
}

/**
  * @brief  Set HSI Calibration trimming
  * @note user-programmable trimming value that is added to the HSICAL
  * @note Default value is 0
  * @rmtoll HSICFGR        HSITRIM       LL_RCC_HSI_SetCalibTrimming
  * @param  Value Between Min_Data = 0x0 and Max_Data = 0x7F
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->HSICFGR, RCC_HSICFGR_HSITRIM, Value << RCC_HSICFGR_HSITRIM_Pos);
}

/**
  * @brief  Get HSI Calibration trimming
  * @rmtoll HSICFGR        HSITRIM       LL_RCC_HSI_GetCalibTrimming
  * @retval Between Min_Data = 0 and Max_Data = 0x7F
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->HSICFGR, RCC_HSICFGR_HSITRIM) >> RCC_HSICFGR_HSITRIM_Pos);
}

/**
  * @brief  Set HSI divider
  * @rmtoll HSICFGR        HSIDIV       LL_RCC_HSI_SetDivider
  * @param  Divider This parameter can be one of the following values:
  *         @arg @ref LL_RCC_HSI_DIV_1
  *         @arg @ref LL_RCC_HSI_DIV_2
  *         @arg @ref LL_RCC_HSI_DIV_4
  *         @arg @ref LL_RCC_HSI_DIV_8
  * @retval None
  */
__STATIC_INLINE void LL_RCC_HSI_SetDivider(uint32_t Divider)
{
  MODIFY_REG(RCC->HSICFGR, RCC_HSICFGR_HSIDIV, Divider);
}

/**
  * @brief  Get HSI divider
  * @rmtoll HSICFGR        HSIDIV       LL_RCC_HSI_GetDivider
  * @retval It can be one of the following values:
  *         @arg @ref LL_RCC_HSI_DIV_1
  *         @arg @ref LL_RCC_HSI_DIV_2
  *         @arg @ref LL_RCC_HSI_DIV_4
  *         @arg @ref LL_RCC_HSI_DIV_8
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_GetDivider(void)
{
  return (READ_BIT(RCC->HSICFGR, RCC_HSICFGR_HSIDIV));
}

/**
  * @brief  Check if HSI division is Ready
  * @rmtoll OCRDYR           HSIDIVRDY        LL_RCC_HSI_IsDividerReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_HSI_IsDividerReady(void)
{
  return ((READ_BIT(RCC->OCRDYR, RCC_OCRDYR_HSIDIVRDY) == RCC_OCRDYR_HSIDIVRDY) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_CSI CSI
  * @{
  */

/**
  * @brief  Enable CSI oscillator
  * @rmtoll OCENSETR           CSION         LL_RCC_CSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_CSI_Enable(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_CSION);
}

/**
  * @brief  Disable CSI oscillator
  * @rmtoll OCENCLRR           CSION         LL_RCC_CSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_CSI_Disable(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_CSION);
}

/**
  * @brief  Check if CSI clock is ready
  * @rmtoll OCRDYR           CSIRDY        LL_RCC_CSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_CSI_IsReady(void)
{
  return ((READ_BIT(RCC->OCRDYR, RCC_OCRDYR_CSIRDY) == RCC_OCRDYR_CSIRDY) ? 1UL : 0UL);
}

/**
  * @brief  Enable CSI oscillator in Stop mode
  * @rmtoll OCENSETR           CSIKERON         LL_RCC_CSI_EnableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_CSI_EnableInStopMode(void)
{
  WRITE_REG(RCC->OCENSETR, RCC_OCENSETR_CSIKERON);
}

/**
  * @brief  Disable CSI oscillator in Stop mode
  * @rmtoll OCENCLRR           CSIKERON         LL_RCC_CSI_DisableInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_RCC_CSI_DisableInStopMode(void)
{
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_CSIKERON);
}

/**
  * @brief  Get CSI Calibration value
  * @note When CSITRIM is written, CSICAL is updated with the sum of
  *       CSITRIM and the factory trim value
  * @rmtoll CSICFGR        CSICAL        LL_RCC_CSI_GetCalibration
  * @retval A value between 0 and 255 (0xFF)
  */
__STATIC_INLINE uint32_t LL_RCC_CSI_GetCalibration(void)
{
  return (uint32_t)(READ_BIT(RCC->CSICFGR, RCC_CSICFGR_CSICAL) >> RCC_CSICFGR_CSICAL_Pos);
}

/**
  * @brief  Set CSI Calibration trimming
  * @note user-programmable trimming value that is added to the CSICAL
  * @note Default value is 16 (0x10) which, when added to the CSICAL value,
  *       should trim the CSI to 4 MHz
  * @rmtoll CSICFGR        CSITRIM       LL_RCC_CSI_SetCalibTrimming
  * @param  Value can be a value between 0 and 31
  * @retval None
  */
__STATIC_INLINE void LL_RCC_CSI_SetCalibTrimming(uint32_t Value)
{
  MODIFY_REG(RCC->CSICFGR, RCC_CSICFGR_CSITRIM, Value << RCC_CSICFGR_CSITRIM_Pos);
}

/**
  * @brief  Get CSI Calibration trimming
  * @rmtoll CSICFGR        CSITRIM       LL_RCC_CSI_GetCalibTrimming
  * @retval A value between 0 and 31
  */
__STATIC_INLINE uint32_t LL_RCC_CSI_GetCalibTrimming(void)
{
  return (uint32_t)(READ_BIT(RCC->CSICFGR, RCC_CSICFGR_CSITRIM) >> RCC_CSICFGR_CSITRIM_Pos);

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
  * @brief  Enable external digital clock source (LSE Digital Bypass).
  * @rmtoll BDCR         DIGBYP, LSEBYP    LL_RCC_LSE_EnableDigBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_EnableDigBypass(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_DIGBYP);
  SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
}

/**
  * @brief  Disable external digital clock source (LSE Digital Bypass).
  * @rmtoll BDCR         DIGBYP, LSEBYP     LL_RCC_LSE_DisableDigBypass
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSE_DisableDigBypass(void)
{
  CLEAR_BIT(RCC->BDCR, (RCC_BDCR_LSEBYP | RCC_BDCR_DIGBYP));
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
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == RCC_BDCR_LSERDY) ? 1UL : 0UL);
}

/**
  * @brief  Check if CSS on LSE failure Detection
  * @rmtoll BDCR         LSECSSD       LL_RCC_LSE_IsCSSDetected
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSE_IsCSSDetected(void)
{
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_LSECSSD) == RCC_BDCR_LSECSSD) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_LSI LSI
  * @{
  */

/**
  * @brief  Enable LSI Oscillator
  * @rmtoll RDLSICR          LSION         LL_RCC_LSI_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSI_Enable(void)
{
  SET_BIT(RCC->RDLSICR, RCC_RDLSICR_LSION);
}

/**
  * @brief  Disable LSI Oscillator
  * @rmtoll RDLSICR          LSION         LL_RCC_LSI_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_LSI_Disable(void)
{
  CLEAR_BIT(RCC->RDLSICR, RCC_RDLSICR_LSION);
}

/**
  * @brief  Check if LSI is Ready
  * @rmtoll RDLSICR          LSIRDY        LL_RCC_LSI_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_LSI_IsReady(void)
{
  return ((READ_BIT(RCC->RDLSICR, RCC_RDLSICR_LSIRDY) == RCC_RDLSICR_LSIRDY) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_System System
  * @{
  */

/**
  * @brief  Configure the mpu clock source
  * @rmtoll MPCKSELR         MPUSRC            LL_RCC_SetMPUClkSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_PLL1
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_MPUDIV
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetMPUClkSource(uint32_t Source)
{
  MODIFY_REG(RCC->MPCKSELR, RCC_MPCKSELR_MPUSRC, Source);
}

/**
  * @brief  Get the mpu clock source
  * @rmtoll MPCKSELR         MPUSRC           LL_RCC_GetMPUClkSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_PLL1
  *         @arg @ref LL_RCC_MPU_CLKSOURCE_MPUDIV
  */
__STATIC_INLINE uint32_t LL_RCC_GetMPUClkSource(void)
{
  return (uint32_t)(READ_BIT(RCC->MPCKSELR, RCC_MPCKSELR_MPUSRC));
}

/**
  * @brief  Configure the axiss clock source
  * @rmtoll ASSCKSELR         AXISSRC            LL_RCC_SetAXISSClkSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_PLL2
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAXISSClkSource(uint32_t Source)
{
  MODIFY_REG(RCC->ASSCKSELR, RCC_ASSCKSELR_AXISSRC, Source);
}

/**
  * @brief  Get the axiss clock source
  * @rmtoll ASSCKSELR         AXISSRC           LL_RCC_GetAXISSClkSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_PLL2
  *         @arg @ref LL_RCC_AXISS_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetAXISSClkSource(void)
{
  return (uint32_t)(READ_BIT(RCC->ASSCKSELR, RCC_ASSCKSELR_AXISSRC));
}

/**
  * @brief  Configure the mcuss clock source
  * @rmtoll MSSCKSELR         MCUSSRC            LL_RCC_SetMCUClkSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_PLL3
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetMCUSSClkSource(uint32_t Source)
{
  MODIFY_REG(RCC->MSSCKSELR, RCC_MSSCKSELR_MCUSSRC, Source);
}

/**
  * @brief  Get the mcuss clock source
  * @rmtoll MSSCKSELR         MCUSSRC           LL_RCC_GetMCUClkSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_MCUSS_CLKSOURCE_PLL3
  */
__STATIC_INLINE uint32_t LL_RCC_GetMCUSSClkSource(void)
{
  return (uint32_t)(READ_BIT(RCC->MSSCKSELR, RCC_MSSCKSELR_MCUSSRC));
}

/**
  * @brief  Set the MPUDIV prescaler
  * @rmtoll MPCKDIVR         MPUDIV            LL_RCC_SetMPUPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MPU_DIV_OFF
  *         @arg @ref LL_RCC_MPU_DIV_2
  *         @arg @ref LL_RCC_MPU_DIV_4
  *         @arg @ref LL_RCC_MPU_DIV_8
  *         @arg @ref LL_RCC_MPU_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetMPUPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->MPCKDIVR, RCC_MPCKDIVR_MPUDIV, Prescaler);
}

/**
  * @brief  Set the ACLK (ACLK, HCLK5 and HCLK6) prescaler
  * @rmtoll AXIDIVR         ACLK            LL_RCC_SetACLKPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_AXI_DIV_1
  *         @arg @ref LL_RCC_AXI_DIV_2
  *         @arg @ref LL_RCC_AXI_DIV_3
  *         @arg @ref LL_RCC_AXI_DIV_4
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetACLKPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->AXIDIVR, RCC_AXIDIVR_AXIDIV, Prescaler);
}

/**
  * @brief  Set the APB4 prescaler
  * @rmtoll APB4DIVR         APB4            LL_RCC_SetAPB4Prescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB4_DIV_1
  *         @arg @ref LL_RCC_APB4_DIV_2
  *         @arg @ref LL_RCC_APB4_DIV_4
  *         @arg @ref LL_RCC_APB4_DIV_8
  *         @arg @ref LL_RCC_APB4_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAPB4Prescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->APB4DIVR, RCC_APB4DIVR_APB4DIV, Prescaler);
}

/**
  * @brief  Set the APB5 prescaler
  * @rmtoll APB5DIVR         APB5            LL_RCC_SetAPB5Prescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB5_DIV_1
  *         @arg @ref LL_RCC_APB5_DIV_2
  *         @arg @ref LL_RCC_APB5_DIV_4
  *         @arg @ref LL_RCC_APB5_DIV_8
  *         @arg @ref LL_RCC_APB5_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAPB5Prescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->APB5DIVR, RCC_APB5DIVR_APB5DIV, Prescaler);
}

/**
  * @brief  Set the MLHCLK (MLHCLK, MCU, FCLK, HCLK4, HCLK3, HCLK2 and HCLK1) prescaler
  * @rmtoll MCUDIVR         MCUDIV            LL_RCC_SetMLHCLKPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCU_DIV_1
  *         @arg @ref LL_RCC_MCU_DIV_2
  *         @arg @ref LL_RCC_MCU_DIV_4
  *         @arg @ref LL_RCC_MCU_DIV_8
  *         @arg @ref LL_RCC_MCU_DIV_16
  *         @arg @ref LL_RCC_MCU_DIV_32
  *         @arg @ref LL_RCC_MCU_DIV_64
  *         @arg @ref LL_RCC_MCU_DIV_128
  *         @arg @ref LL_RCC_MCU_DIV_256
  *         @arg @ref LL_RCC_MCU_DIV_512

  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetMLHCLKPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->MCUDIVR, RCC_MCUDIVR_MCUDIV, Prescaler);
}

/**
  * @brief  Set the APB1 prescaler
  * @rmtoll APB1DIVR         APB1            LL_RCC_SetAPB1Prescaler
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
  MODIFY_REG(RCC->APB1DIVR, RCC_APB1DIVR_APB1DIV, Prescaler);
}

/**
  * @brief  Set the APB2 prescaler
  * @rmtoll APB2DIVR         APB2            LL_RCC_SetAPB2Prescaler
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
  MODIFY_REG(RCC->APB2DIVR, RCC_APB2DIVR_APB2DIV, Prescaler);
}

/**
  * @brief  Set the APB3 prescaler
  * @rmtoll APB3DIVR         APB3            LL_RCC_SetAPB3Prescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_APB3_DIV_1
  *         @arg @ref LL_RCC_APB3_DIV_2
  *         @arg @ref LL_RCC_APB3_DIV_4
  *         @arg @ref LL_RCC_APB3_DIV_8
  *         @arg @ref LL_RCC_APB3_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetAPB3Prescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->APB3DIVR, RCC_APB3DIVR_APB3DIV, Prescaler);
}

/**
  * @brief  Get the MPUDIV prescaler
  * @rmtoll MPCKDIVR         MPUDIV           LL_RCC_GetMPUPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_MPU_DIV_OFF
  *         @arg @ref LL_RCC_MPU_DIV_2
  *         @arg @ref LL_RCC_MPU_DIV_4
  *         @arg @ref LL_RCC_MPU_DIV_8
  *         @arg @ref LL_RCC_MPU_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetMPUPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->MPCKDIVR, RCC_MPCKDIVR_MPUDIV));
}

/**
  * @brief  Get the ACLK (ACLK, HCLK5 and HCLK6) prescaler
  * @rmtoll AXIDIVR         ACLK           LL_RCC_GetACLKPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_AXI_DIV_1
  *         @arg @ref LL_RCC_AXI_DIV_2
  *         @arg @ref LL_RCC_AXI_DIV_3
  *         @arg @ref LL_RCC_AXI_DIV_4
  */
__STATIC_INLINE uint32_t LL_RCC_GetACLKPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->AXIDIVR, RCC_AXIDIVR_AXIDIV));
}

/**
  * @brief  Get the APB4 prescaler
  * @rmtoll APB4DIVR         APB4           LL_RCC_GetAPB4Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB4_DIV_1
  *         @arg @ref LL_RCC_APB4_DIV_2
  *         @arg @ref LL_RCC_APB4_DIV_4
  *         @arg @ref LL_RCC_APB4_DIV_8
  *         @arg @ref LL_RCC_APB4_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB4Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->APB4DIVR, RCC_APB4DIVR_APB4DIV));
}

/**
  * @brief  Get the APB5 prescaler
  * @rmtoll APB5DIVR         APB5           LL_RCC_GetAPB5Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB5_DIV_1
  *         @arg @ref LL_RCC_APB5_DIV_2
  *         @arg @ref LL_RCC_APB5_DIV_4
  *         @arg @ref LL_RCC_APB5_DIV_8
  *         @arg @ref LL_RCC_APB5_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB5Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->APB5DIVR, RCC_APB5DIVR_APB5DIV));
}

/**
  * @brief  Get the MLHCLK (MLHCLK, MCU, FCLK, HCLK4, HCLK3, HCLK2 and HCLK1) prescaler
  * @rmtoll MCUDIVR         MCUDIV           LL_RCC_GetMLHCLKPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_MCU_DIV_1
  *         @arg @ref LL_RCC_MCU_DIV_2
  *         @arg @ref LL_RCC_MCU_DIV_4
  *         @arg @ref LL_RCC_MCU_DIV_8
  *         @arg @ref LL_RCC_MCU_DIV_16
  *         @arg @ref LL_RCC_MCU_DIV_32
  *         @arg @ref LL_RCC_MCU_DIV_64
  *         @arg @ref LL_RCC_MCU_DIV_128
  *         @arg @ref LL_RCC_MCU_DIV_256
  *         @arg @ref LL_RCC_MCU_DIV_512
  */
__STATIC_INLINE uint32_t LL_RCC_GetMLHCLKPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->MCUDIVR, RCC_MCUDIVR_MCUDIV));
}

/**
  * @brief  Get the APB1 prescaler
  * @rmtoll APB1DIVR         APB1           LL_RCC_GetAPB1Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB1_DIV_1
  *         @arg @ref LL_RCC_APB1_DIV_2
  *         @arg @ref LL_RCC_APB1_DIV_4
  *         @arg @ref LL_RCC_APB1_DIV_8
  *         @arg @ref LL_RCC_APB1_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB1Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->APB1DIVR, RCC_APB1DIVR_APB1DIV));
}

/**
  * @brief  Get the APB2 prescaler
  * @rmtoll APB2DIVR         APB2           LL_RCC_GetAPB2Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB2_DIV_1
  *         @arg @ref LL_RCC_APB2_DIV_2
  *         @arg @ref LL_RCC_APB2_DIV_4
  *         @arg @ref LL_RCC_APB2_DIV_8
  *         @arg @ref LL_RCC_APB2_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB2Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->APB2DIVR, RCC_APB2DIVR_APB2DIV));
}

/**
  * @brief  Get the APB3 prescaler
  * @rmtoll APB3DIVR         APB3           LL_RCC_GetAPB3Prescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_APB3_DIV_1
  *         @arg @ref LL_RCC_APB3_DIV_2
  *         @arg @ref LL_RCC_APB3_DIV_4
  *         @arg @ref LL_RCC_APB3_DIV_8
  *         @arg @ref LL_RCC_APB3_DIV_16
  */
__STATIC_INLINE uint32_t LL_RCC_GetAPB3Prescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->APB3DIVR, RCC_APB3DIVR_APB3DIV));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_Peripheral_Clock_Source Peripheral Clock Source
  * @{
  */

/**
  * @brief  Configure peripheral clock source
  * @rmtoll I2C46CKSELR        *     LL_RCC_SetClockSource\n
  *         SPI6CKSELR         *     LL_RCC_SetClockSource\n
  *         UART1CKSELR        *     LL_RCC_SetClockSource\n
  *         RNG1CKSELR         *     LL_RCC_SetClockSource\n
  *         MCO1CFGR           *     LL_RCC_SetClockSource\n
  *         MCO2CFGR           *     LL_RCC_SetClockSource\n
  *         TIMG1PRER          *     LL_RCC_SetClockSource\n
  *         TIMG2PRER          *     LL_RCC_SetClockSource\n
  *         I2C12CKSELR        *     LL_RCC_SetClockSource\n
  *         I2C35CKSELR        *     LL_RCC_SetClockSource\n
  *         SAI1CKSELR         *     LL_RCC_SetClockSource\n
  *         SAI2CKSELR         *     LL_RCC_SetClockSource\n
  *         SAI3CKSELR         *     LL_RCC_SetClockSource\n
  *         SAI4CKSELR         *     LL_RCC_SetClockSource\n
  *         SPI2S1CKSELR       *     LL_RCC_SetClockSource\n
  *         SPI2S23CKSELR      *     LL_RCC_SetClockSource\n
  *         SPI45CKSELR        *     LL_RCC_SetClockSource\n
  *         UART6CKSELR        *     LL_RCC_SetClockSource\n
  *         UART24CKSELR       *     LL_RCC_SetClockSource\n
  *         UART35CKSELR       *     LL_RCC_SetClockSource\n
  *         UART78CKSELR       *     LL_RCC_SetClockSource\n
  *         SDMMC12CKSELR      *     LL_RCC_SetClockSource\n
  *         SDMMC3CKSELR       *     LL_RCC_SetClockSource\n
  *         RNG2CKSELR         *     LL_RCC_SetClockSource\n
  *         LPTIM45CKSELR      *     LL_RCC_SetClockSource\n
  *         LPTIM23CKSELR      *     LL_RCC_SetClockSource\n
  *         LPTIM1CKSELR       *     LL_RCC_SetClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_SPDIF
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HCLK6
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HCLK2
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_CSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
  *         @arg @ref LL_RCC_MCO2SOURCE_MPU
  *         @arg @ref LL_RCC_MCO2SOURCE_AXI
  *         @arg @ref LL_RCC_MCO2SOURCE_MCU
  *         @arg @ref LL_RCC_MCO2SOURCE_PLL4
  *         @arg @ref LL_RCC_MCO2SOURCE_HSE
  *         @arg @ref LL_RCC_MCO2SOURCE_HSI
  *         @arg @ref LL_RCC_TIMG1PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG1PRES_ACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_ACTIVATED
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetClockSource(uint32_t ClkSource)
{
  __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)&RCC->I2C46CKSELR + LL_CLKSOURCE_REG(ClkSource));
  MODIFY_REG(*pReg, LL_CLKSOURCE_MASK(ClkSource), LL_CLKSOURCE_CONFIG(ClkSource));
}

/**
  * @brief  Configure I2Cx clock source
  * @rmtoll I2C12CKSELR        I2C12SRC     LL_RCC_SetI2CClockSource\n
  *         I2C35CKSELR        I2C35SRC     LL_RCC_SetI2CClockSource\n
  *         I2C46CKSELR        I2C46SRC     LL_RCC_SetI2CClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_CSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetI2CClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Configure SAIx clock source
  * @rmtoll SAI1CKSELR        SAI1SRC     LL_RCC_SetSAIClockSource\n
  *         SAI2CKSELR        SAI2SRC     LL_RCC_SetSAIClockSource\n
  *         SAI3CKSELR        SAI3SRC     LL_RCC_SetSAIClockSource\n
  *         SAI4CKSELR        SAI4SRC     LL_RCC_SetSAIClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_SPDIF
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3R

  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSAIClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}


/**
  * @brief  Configure SPI/I2S clock source
  * @rmtoll SPI2S1CKSELR        SPI1SRC     LL_RCC_SetSPIClockSource\n
  *         SPI2S23CKSELR       SPI23SRC    LL_RCC_SetSPIClockSource\n
  *         SPI45CKSELR         SPI45SRC    LL_RCC_SetSPIClockSource\n
  *         SPI6CKSELR          SPI6SRC     LL_RCC_SetSPIClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL3Q
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSPIClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Configure U(S)ARTx clock source
  * @rmtoll UART1CKSELR        UART1SRC      LL_RCC_SetUARTClockSource\n
  *         UART24CKSELR       UART24SRC     LL_RCC_SetUARTClockSource\n
  *         UART35CKSELR       UART35SRC     LL_RCC_SetUARTClockSource\n
  *         UART6CKSELR        UART6SRC      LL_RCC_SetUARTClockSource\n
  *         UART78CKSELR       UART78SRC     LL_RCC_SetUARTClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUARTClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Configure SDMMCx clock source
  * @rmtoll SDMMC12CKSELR      SDMMC12SRC      LL_RCC_SetSDMMCClockSource\n
  *         SDMMC3CKSELR       SDMMC3SRC       LL_RCC_SetSDMMCClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HCLK6
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HCLK2
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSDMMCClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Configure ETH clock source
  * @rmtoll ETHCKSELR      ETHSRC      LL_RCC_SetETHClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetETHClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->ETHCKSELR, RCC_ETHCKSELR_ETHSRC, ClkSource);
}

/**
  * @brief  Configure QSPI clock source
  * @rmtoll QSPICKSELR      QSPISRC      LL_RCC_SetQSPIClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_ACLK
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PER
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetQSPIClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->QSPICKSELR, RCC_QSPICKSELR_QSPISRC, ClkSource);
}

/**
  * @brief  Configure FMC clock source
  * @rmtoll FMCCKSELR      FMCSRC      LL_RCC_SetFMCClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_ACLK
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PER
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetFMCClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->FMCCKSELR, RCC_FMCCKSELR_FMCSRC, ClkSource);
}

/**
  * @brief  Configure FDCAN clock source
  * @rmtoll FDCANCKSELR      FDCANSRC      LL_RCC_SetFDCANClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL4R
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetFDCANClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->FDCANCKSELR, RCC_FDCANCKSELR_FDCANSRC, ClkSource);
}

/**
  * @brief  Configure SPDIFRX clock source
  * @rmtoll SPDIFCKSELR      SPDIFSRC      LL_RCC_SetSPDIFRXClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_HSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSPDIFRXClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->SPDIFCKSELR, RCC_SPDIFCKSELR_SPDIFSRC, ClkSource);
}

/**
  * @brief  Configure CEC clock source
  * @rmtoll CECCKSELR      CECSRC      LL_RCC_SetCECClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_CSI122
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCECClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->CECCKSELR, RCC_CECCKSELR_CECSRC, ClkSource);
}

/**
  * @brief  Configure USBPHY clock source
  * @rmtoll USBCKSELR      USBPHYSRC      LL_RCC_SetUSBPHYClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_HSE2
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSBPHYClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->USBCKSELR, RCC_USBCKSELR_USBPHYSRC, ClkSource);
}

/**
  * @brief  Configure USBO clock source
  * @rmtoll USBCKSELR      USBOSRC      LL_RCC_SetUSBOClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBO_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_USBO_CLKSOURCE_PHY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetUSBOClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->USBCKSELR, RCC_USBCKSELR_USBOSRC, ClkSource);
}

/**
  * @brief  Configure RNGx clock source
  * @rmtoll RNG1CKSELR      RNG1SRC      LL_RCC_SetRNGClockSource\n
  *         RNG2CKSELR      RNG2SRC      LL_RCC_SetRNGClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSI
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRNGClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Configure CKPER clock source
  * @rmtoll CPERCKSELR      CKPERSRC      LL_RCC_SetCKPERClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetCKPERClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->CPERCKSELR, RCC_CPERCKSELR_CKPERSRC, ClkSource);
}

/**
  * @brief  Configure STGEN clock source
  * @rmtoll STGENCKSELR      STGENSRC      LL_RCC_SetSTGENClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetSTGENClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->STGENCKSELR, RCC_STGENCKSELR_STGENSRC, ClkSource);
}

/**
  * @brief  Configure DSI clock source
  * @rmtoll DSICKSELR      DSISRC      LL_RCC_SetDSIClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PHY
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PLL4P
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetDSIClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->DSICKSELR, RCC_DSICKSELR_DSISRC, ClkSource);
}

/**
  * @brief  Configure ADC clock source
  * @rmtoll ADCCKSELR      ADCSRC      LL_RCC_SetADCClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PER
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL3Q
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetADCClockSource(uint32_t ClkSource)
{
  MODIFY_REG(RCC->ADCCKSELR, RCC_ADCCKSELR_ADCSRC, ClkSource);
}

/**
  * @brief  Configure LPTIMx clock source
  * @rmtoll LPTIM1CKSELR      LPTIM1SRC      LL_RCC_SetLPTIMClockSource\n
  *         LPTIM23CKSELR     LPTIM23SRC     LL_RCC_SetLPTIMClockSource\n
  *         LPTIM45CKSELR     LPTIM45SRC     LL_RCC_SetLPTIMClockSource
  * @param  ClkSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_OFF
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetLPTIMClockSource(uint32_t ClkSource)
{
  LL_RCC_SetClockSource(ClkSource);
}

/**
  * @brief  Get peripheral clock source
  * @rmtoll I2C46CKSELR        *     LL_RCC_GetClockSource\n
  *         SPI6CKSELR         *     LL_RCC_GetClockSource\n
  *         UART1CKSELR        *     LL_RCC_GetClockSource\n
  *         RNG1CKSELR         *     LL_RCC_GetClockSource\n
  *         TIMG1PRER          *     LL_RCC_GetClockSource\n
  *         TIMG2PRER          *     LL_RCC_GetClockSource\n
  *         I2C12CKSELR        *     LL_RCC_GetClockSource\n
  *         I2C35CKSELR        *     LL_RCC_GetClockSource\n
  *         SAI1CKSELR         *     LL_RCC_GetClockSource\n
  *         SAI2CKSELR         *     LL_RCC_GetClockSource\n
  *         SAI3CKSELR         *     LL_RCC_GetClockSource\n
  *         SAI4CKSELR         *     LL_RCC_GetClockSource\n
  *         SPI2S1CKSELR       *     LL_RCC_GetClockSource\n
  *         SPI2S23CKSELR      *     LL_RCC_GetClockSource\n
  *         SPI45CKSELR        *     LL_RCC_GetClockSource\n
  *         UART6CKSELR        *     LL_RCC_GetClockSource\n
  *         UART24CKSELR       *     LL_RCC_GetClockSource\n
  *         UART35CKSELR       *     LL_RCC_GetClockSource\n
  *         UART78CKSELR       *     LL_RCC_GetClockSource\n
  *         SDMMC12CKSELR      *     LL_RCC_GetClockSource\n
  *         SDMMC3CKSELR       *     LL_RCC_GetClockSource\n
  *         RNG2CKSELR         *     LL_RCC_GetClockSource\n
  *         LPTIM45CKSELR      *     LL_RCC_GetClockSource\n
  *         LPTIM23CKSELR      *     LL_RCC_GetClockSource\n
  *         LPTIM1CKSELR       *     LL_RCC_GetClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE
  *         @arg @ref LL_RCC_TIMG1PRES
  *         @arg @ref LL_RCC_TIMG2PRES
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_SPDIF
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HCLK6
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HCLK2
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_TIMG1PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG1PRES_ACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_ACTIVATED
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetClockSource(uint32_t Periph)
{
  __IO const uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&RCC->I2C46CKSELR) + LL_CLKSOURCE_REG(Periph)));

  return (uint32_t)(Periph | ((READ_BIT(*pReg, LL_CLKSOURCE_MASK(Periph))) << RCC_CONFIG_SHIFT));
}

/**
  * @brief  Get I2Cx clock source
  * @rmtoll I2C12CKSELR        I2C12SRC     LL_RCC_GetI2CClockSource\n
  *         I2C35CKSELR        I2C35SRC     LL_RCC_GetI2CClockSource\n
  *         I2C46CKSELR        I2C46SRC     LL_RCC_GetI2CClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE_CSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetI2CClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @brief  Get SAIx clock source
  * @rmtoll SAI1CKSELR        SAI1SRC     LL_RCC_GetSAIClockSource\n
  *         SAI2CKSELR        SAI2SRC     LL_RCC_GetSAIClockSource\n
  *         SAI3CKSELR        SAI3SRC     LL_RCC_GetSAIClockSource\n
  *         SAI4CKSELR        SAI4SRC     LL_RCC_GetSAIClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_SPDIF
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE_PLL3R
  */
__STATIC_INLINE uint32_t LL_RCC_GetSAIClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}


/**
  * @brief  Get SPI/I2S clock source
  * @rmtoll SPI2S1CKSELR        SPI1SRC     LL_RCC_GetSPIClockSource\n
  *         SPI2S23CKSELR       SPI23SRC    LL_RCC_GetSPIClockSource\n
  *         SPI45CKSELR         SPI45SRC    LL_RCC_GetSPIClockSource\n
  *         SPI6CKSELR          SPI6SRC     LL_RCC_GetSPIClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_I2SCKIN
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE_PLL3Q
  */
__STATIC_INLINE uint32_t LL_RCC_GetSPIClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @brief  Get U(S)ARTx clock source
  * @rmtoll UART1CKSELR        UART1SRC      LL_RCC_GetUARTClockSource\n
  *         UART24CKSELR       UART24SRC     LL_RCC_GetUARTClockSource\n
  *         UART35CKSELR       UART35SRC     LL_RCC_GetUARTClockSource\n
  *         UART6CKSELR        UART6SRC      LL_RCC_GetUARTClockSource\n
  *         UART78CKSELR       UART78SRC     LL_RCC_GetUARTClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PCLK5
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART1_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART24_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART35_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PCLK2
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_USART6_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_UART78_CLKSOURCE_HSE
  */
__STATIC_INLINE uint32_t LL_RCC_GetUARTClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @brief  Get SDMMCx clock source
  * @rmtoll SDMMC12CKSELR      SDMMC12SRC      LL_RCC_GetSDMMCClockSource\n
  *         SDMMC3CKSELR       SDMMC3SRC       LL_RCC_GetSDMMCClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HCLK6
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HCLK2
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE_HSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetSDMMCClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @brief  Get ETH clock source
  * @rmtoll ETHCKSELR      ETHSRC      LL_RCC_GetETHClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ETH_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_ETH_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetETHClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->ETHCKSELR, RCC_ETHCKSELR_ETHSRC));
}

/**
  * @brief  Get QSPI clock source
  * @rmtoll QSPICKSELR      QSPISRC      LL_RCC_GetQSPIClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_ACLK
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE_PER
  */
__STATIC_INLINE uint32_t LL_RCC_GetQSPIClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->QSPICKSELR, RCC_QSPICKSELR_QSPISRC));
}

/**
  * @brief  Get FMC clock source
  * @rmtoll FMCCKSELR      FMCSRC      LL_RCC_GetFMCClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FMC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_ACLK
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PLL3R
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_FMC_CLKSOURCE_PER
  */
__STATIC_INLINE uint32_t LL_RCC_GetFMCClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->FMCCKSELR, RCC_FMCCKSELR_FMCSRC));
}

/**
  * @brief  Get FDCAN clock source
  * @rmtoll FDCANCKSELR      FDCANSRC      LL_RCC_GetFDCANClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE_PLL4R
  */
__STATIC_INLINE uint32_t LL_RCC_GetFDCANClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->FDCANCKSELR, RCC_FDCANCKSELR_FDCANSRC));
}

/**
  * @brief  Get SPDIFRX clock source
  * @rmtoll SPDIFCKSELR      SPDIFSRC      LL_RCC_GetSPDIFRXClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE_HSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetSPDIFRXClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->SPDIFCKSELR, RCC_SPDIFCKSELR_SPDIFSRC));
}

/**
  * @brief  Get CEC clock source
  * @rmtoll CECCKSELR      CECSRC      LL_RCC_GetCECClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_CEC_CLKSOURCE_CSI122
  */
__STATIC_INLINE uint32_t LL_RCC_GetCECClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->CECCKSELR, RCC_CECCKSELR_CECSRC));
}

/**
  * @brief  Get USBPHY clock source
  * @rmtoll USBCKSELR      USBPHYSRC      LL_RCC_GetUSBPHYClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE_HSE2
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSBPHYClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->USBCKSELR, RCC_USBCKSELR_USBPHYSRC));
}

/**
  * @brief  Get USBO clock source
  * @rmtoll USBCKSELR      USBOSRC      LL_RCC_GetUSBOClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBO_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_USBO_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_USBO_CLKSOURCE_PHY
  */
__STATIC_INLINE uint32_t LL_RCC_GetUSBOClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->USBCKSELR, RCC_USBCKSELR_USBOSRC));
}

/**
  * @brief  Get RNGx clock source
  * @rmtoll RNG1CKSELR      RNG1SRC      LL_RCC_GetRNGClockSource\n
  *         RNG2CKSELR      RNG2SRC      LL_RCC_GetRNGClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE_LSI
  */
__STATIC_INLINE uint32_t LL_RCC_GetRNGClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @brief  Get CKPER clock source
  * @rmtoll CPERCKSELR      CKPERSRC      LL_RCC_GetCKPERClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_CSI
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetCKPERClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->CPERCKSELR, RCC_CPERCKSELR_CKPERSRC));
}

/**
  * @brief  Get STGEN clock source
  * @rmtoll STGENCKSELR      STGENSRC      LL_RCC_GetSTGENClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_HSI
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_HSE
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetSTGENClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->STGENCKSELR, RCC_STGENCKSELR_STGENSRC));
}

/**
  * @brief  Get DSI clock source
  * @rmtoll DSICKSELR      DSISRC      LL_RCC_GetDSIClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PHY
  *         @arg @ref LL_RCC_DSI_CLKSOURCE_PLL4P
  */
__STATIC_INLINE uint32_t LL_RCC_GetDSIClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->DSICKSELR, RCC_DSICKSELR_DSISRC));
}

/**
  * @brief  Get ADC clock source
  * @rmtoll ADCCKSELR      ADCSRC      LL_RCC_GetADCClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL4R
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PER
  *         @arg @ref LL_RCC_ADC_CLKSOURCE_PLL3Q
  */
__STATIC_INLINE uint32_t LL_RCC_GetADCClockSource(uint32_t Periph)
{
  return (uint32_t)(READ_BIT(RCC->ADCCKSELR, RCC_ADCCKSELR_ADCSRC));
}

/**
  * @brief  Get LPTIMx clock source
  * @rmtoll LPTIM1CKSELR      LPTIM1SRC      LL_RCC_GetLPTIMClockSource\n
  *         LPTIM23CKSELR     LPTIM23SRC     LL_RCC_GetLPTIMClockSource\n
  *         LPTIM45CKSELR     LPTIM45SRC     LL_RCC_GetLPTIMClockSource
  * @param  Periph This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PCLK1
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PLL4Q
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE_OFF
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PCLK3
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL4P
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PLL3Q
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_PER
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE_OFF
  */
__STATIC_INLINE uint32_t LL_RCC_GetLPTIMClockSource(uint32_t Periph)
{
  return LL_RCC_GetClockSource(Periph);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_RTC RTC
  * @{
  */

/**
  * @brief  Set RTC Clock Source
  * @note   Once the RTC clock source has been selected, it cannot be changed
  *         anymore unless the Backup domain is reset, or unless a failure is
  *         detected on LSE (LSECSS is set). The VSWRST bit can be used to reset
  *         them.
  * @rmtoll BDCR         RTCSRC        LL_RCC_SetRTCClockSource
  * @param  Source This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE_DIV
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTCClockSource(uint32_t Source)
{
  MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSRC, Source);
}

/**
  * @brief  Get RTC Clock Source
  * @rmtoll BDCR         RTCSRC        LL_RCC_GetRTCClockSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_NONE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSE
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_LSI
  *         @arg @ref LL_RCC_RTC_CLKSOURCE_HSE_DIV
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTCClockSource(void)
{
  return (uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_RTCSRC));
}

/**
  * @brief  Enable RTC
  * @rmtoll BDCR         RTCCKEN         LL_RCC_EnableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableRTC(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_RTCCKEN);
}

/**
  * @brief  Disable RTC
  * @rmtoll BDCR         RTCCKEN         LL_RCC_DisableRTC
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableRTC(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCCKEN);
}

/**
  * @brief  Check if RTC has been enabled or not
  * @rmtoll BDCR         RTCEN         LL_RCC_IsEnabledRTC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledRTC(void)
{
  return ((READ_BIT(RCC->BDCR, RCC_BDCR_RTCCKEN) == RCC_BDCR_RTCCKEN) ? 1UL : 0UL);
}

/**
  * @brief  Force the Backup domain reset
  * @rmtoll BDCR         VSWRST         LL_RCC_ForceBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ForceBackupDomainReset(void)
{
  SET_BIT(RCC->BDCR, RCC_BDCR_VSWRST);
}

/**
  * @brief  Release the Backup domain reset
  * @rmtoll BDCR         VSWRST         LL_RCC_ReleaseBackupDomainReset
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ReleaseBackupDomainReset(void)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_VSWRST);
}

/**
  * @brief  Set HSE Prescaler for RTC Clock
  * @rmtoll RTCDIVR         RTCDIV        LL_RCC_SetRTC_HSEPrescaler
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RTC_HSE_DIV_1
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
  *         @arg @ref LL_RCC_RTC_HSE_DIV_32
  *         @arg @ref LL_RCC_RTC_HSE_DIV_33
  *         @arg @ref LL_RCC_RTC_HSE_DIV_34
  *         @arg @ref LL_RCC_RTC_HSE_DIV_35
  *         @arg @ref LL_RCC_RTC_HSE_DIV_36
  *         @arg @ref LL_RCC_RTC_HSE_DIV_37
  *         @arg @ref LL_RCC_RTC_HSE_DIV_38
  *         @arg @ref LL_RCC_RTC_HSE_DIV_39
  *         @arg @ref LL_RCC_RTC_HSE_DIV_40
  *         @arg @ref LL_RCC_RTC_HSE_DIV_41
  *         @arg @ref LL_RCC_RTC_HSE_DIV_42
  *         @arg @ref LL_RCC_RTC_HSE_DIV_43
  *         @arg @ref LL_RCC_RTC_HSE_DIV_44
  *         @arg @ref LL_RCC_RTC_HSE_DIV_45
  *         @arg @ref LL_RCC_RTC_HSE_DIV_46
  *         @arg @ref LL_RCC_RTC_HSE_DIV_47
  *         @arg @ref LL_RCC_RTC_HSE_DIV_48
  *         @arg @ref LL_RCC_RTC_HSE_DIV_49
  *         @arg @ref LL_RCC_RTC_HSE_DIV_50
  *         @arg @ref LL_RCC_RTC_HSE_DIV_51
  *         @arg @ref LL_RCC_RTC_HSE_DIV_52
  *         @arg @ref LL_RCC_RTC_HSE_DIV_53
  *         @arg @ref LL_RCC_RTC_HSE_DIV_54
  *         @arg @ref LL_RCC_RTC_HSE_DIV_55
  *         @arg @ref LL_RCC_RTC_HSE_DIV_56
  *         @arg @ref LL_RCC_RTC_HSE_DIV_57
  *         @arg @ref LL_RCC_RTC_HSE_DIV_58
  *         @arg @ref LL_RCC_RTC_HSE_DIV_59
  *         @arg @ref LL_RCC_RTC_HSE_DIV_60
  *         @arg @ref LL_RCC_RTC_HSE_DIV_61
  *         @arg @ref LL_RCC_RTC_HSE_DIV_62
  *         @arg @ref LL_RCC_RTC_HSE_DIV_63
  *         @arg @ref LL_RCC_RTC_HSE_DIV_64
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetRTC_HSEPrescaler(uint32_t Prescaler)
{
  MODIFY_REG(RCC->RTCDIVR, RCC_RTCDIVR_RTCDIV, Prescaler);
}

/**
  * @brief  Get HSE Prescalers for RTC Clock
  * @rmtoll RTCDIVR         RTCDIV        LL_RCC_GetRTC_HSEPrescaler
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_RTC_HSE_DIV_1
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
  *         @arg @ref LL_RCC_RTC_HSE_DIV_32
  *         @arg @ref LL_RCC_RTC_HSE_DIV_33
  *         @arg @ref LL_RCC_RTC_HSE_DIV_34
  *         @arg @ref LL_RCC_RTC_HSE_DIV_35
  *         @arg @ref LL_RCC_RTC_HSE_DIV_36
  *         @arg @ref LL_RCC_RTC_HSE_DIV_37
  *         @arg @ref LL_RCC_RTC_HSE_DIV_38
  *         @arg @ref LL_RCC_RTC_HSE_DIV_39
  *         @arg @ref LL_RCC_RTC_HSE_DIV_40
  *         @arg @ref LL_RCC_RTC_HSE_DIV_41
  *         @arg @ref LL_RCC_RTC_HSE_DIV_42
  *         @arg @ref LL_RCC_RTC_HSE_DIV_43
  *         @arg @ref LL_RCC_RTC_HSE_DIV_44
  *         @arg @ref LL_RCC_RTC_HSE_DIV_45
  *         @arg @ref LL_RCC_RTC_HSE_DIV_46
  *         @arg @ref LL_RCC_RTC_HSE_DIV_47
  *         @arg @ref LL_RCC_RTC_HSE_DIV_48
  *         @arg @ref LL_RCC_RTC_HSE_DIV_49
  *         @arg @ref LL_RCC_RTC_HSE_DIV_50
  *         @arg @ref LL_RCC_RTC_HSE_DIV_51
  *         @arg @ref LL_RCC_RTC_HSE_DIV_52
  *         @arg @ref LL_RCC_RTC_HSE_DIV_53
  *         @arg @ref LL_RCC_RTC_HSE_DIV_54
  *         @arg @ref LL_RCC_RTC_HSE_DIV_55
  *         @arg @ref LL_RCC_RTC_HSE_DIV_56
  *         @arg @ref LL_RCC_RTC_HSE_DIV_57
  *         @arg @ref LL_RCC_RTC_HSE_DIV_58
  *         @arg @ref LL_RCC_RTC_HSE_DIV_59
  *         @arg @ref LL_RCC_RTC_HSE_DIV_60
  *         @arg @ref LL_RCC_RTC_HSE_DIV_61
  *         @arg @ref LL_RCC_RTC_HSE_DIV_62
  *         @arg @ref LL_RCC_RTC_HSE_DIV_63
  *         @arg @ref LL_RCC_RTC_HSE_DIV_64
  */
__STATIC_INLINE uint32_t LL_RCC_GetRTC_HSEPrescaler(void)
{
  return (uint32_t)(READ_BIT(RCC->RTCDIVR, RCC_RTCDIVR_RTCDIV));
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_TIMGx_CLOCK_PRESCALER TIMGx
  * @{
  */

/**
  * @brief  Configure TIMGx prescaler selection
  * @rmtoll TIMG1PRER      TIMG1PRE      LL_RCC_SetTIMGPrescaler\n
  *         TIMG2PRER      TIMG2PRE      LL_RCC_SetTIMGPrescaler
  * @param  PreSelection This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIMG1PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG1PRES_ACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_ACTIVATED
  * @retval None
  */
__STATIC_INLINE void LL_RCC_SetTIMGPrescaler(uint32_t PreSelection)
{
  LL_RCC_SetClockSource(PreSelection);
}

/**
  * @brief  Get TIMGx prescaler selection
  * @rmtoll TIMG1PRER      TIMG1PRE      LL_RCC_GetTIMGPrescaler\n
  *         TIMG2PRER      TIMG2PRE      LL_RCC_GetTIMGPrescaler
  * @param  TIMGroup This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIMG1PRES
  *         @arg @ref LL_RCC_TIMG2PRES
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_TIMG1PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG1PRES_ACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_DEACTIVATED
  *         @arg @ref LL_RCC_TIMG2PRES_ACTIVATED
  */
__STATIC_INLINE uint32_t LL_RCC_GetTIMGPrescaler(uint32_t TIMGroup)
{
  return LL_RCC_GetClockSource(TIMGroup);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_MCO MCO
  * @{
  */

/**
  * @brief  Configure MCOx
  * @rmtoll MCO1CFGR         MCO1SEL        LL_RCC_ConfigMCO\n
  *         MCO1CFGR         MCO1DIV        LL_RCC_ConfigMCO\n
  *         MCO2CFGR         MCO2SEL        LL_RCC_ConfigMCO\n
  *         MCO2CFGR         MCO2DIV        LL_RCC_ConfigMCO
  * @param  MCOxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1SOURCE_HSI
  *         @arg @ref LL_RCC_MCO1SOURCE_HSE
  *         @arg @ref LL_RCC_MCO1SOURCE_CSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSI
  *         @arg @ref LL_RCC_MCO1SOURCE_LSE
  *         @arg @ref LL_RCC_MCO2SOURCE_MPU
  *         @arg @ref LL_RCC_MCO2SOURCE_AXI
  *         @arg @ref LL_RCC_MCO2SOURCE_MCU
  *         @arg @ref LL_RCC_MCO2SOURCE_PLL4
  *         @arg @ref LL_RCC_MCO2SOURCE_HSE
  *         @arg @ref LL_RCC_MCO2SOURCE_HSI
  * @param  MCOxPrescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_MCO1_DIV_1
  *         @arg @ref LL_RCC_MCO1_DIV_2
  *         @arg @ref LL_RCC_MCO1_DIV_3
  *         @arg @ref LL_RCC_MCO1_DIV_4
  *         @arg @ref LL_RCC_MCO1_DIV_5
  *         @arg @ref LL_RCC_MCO1_DIV_6
  *         @arg @ref LL_RCC_MCO1_DIV_7
  *         @arg @ref LL_RCC_MCO1_DIV_8
  *         @arg @ref LL_RCC_MCO1_DIV_9
  *         @arg @ref LL_RCC_MCO1_DIV_10
  *         @arg @ref LL_RCC_MCO1_DIV_11
  *         @arg @ref LL_RCC_MCO1_DIV_12
  *         @arg @ref LL_RCC_MCO1_DIV_13
  *         @arg @ref LL_RCC_MCO1_DIV_14
  *         @arg @ref LL_RCC_MCO1_DIV_15
  *         @arg @ref LL_RCC_MCO1_DIV_16
  *         @arg @ref LL_RCC_MCO2_DIV_1
  *         @arg @ref LL_RCC_MCO2_DIV_2
  *         @arg @ref LL_RCC_MCO2_DIV_3
  *         @arg @ref LL_RCC_MCO2_DIV_4
  *         @arg @ref LL_RCC_MCO2_DIV_5
  *         @arg @ref LL_RCC_MCO2_DIV_6
  *         @arg @ref LL_RCC_MCO2_DIV_7
  *         @arg @ref LL_RCC_MCO2_DIV_8
  *         @arg @ref LL_RCC_MCO2_DIV_9
  *         @arg @ref LL_RCC_MCO2_DIV_10
  *         @arg @ref LL_RCC_MCO2_DIV_11
  *         @arg @ref LL_RCC_MCO2_DIV_12
  *         @arg @ref LL_RCC_MCO2_DIV_13
  *         @arg @ref LL_RCC_MCO2_DIV_14
  *         @arg @ref LL_RCC_MCO2_DIV_15
  *         @arg @ref LL_RCC_MCO2_DIV_16
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ConfigMCO(uint32_t MCOxSource, uint32_t MCOxPrescaler)
{
  LL_RCC_SetClockSource(MCOxSource);

  /* Set MCOx prescaler */
  __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)&RCC->I2C46CKSELR + LL_CLKSOURCE_REG(MCOxSource));
  /* RCC_MCO1CFGR_MCO1DIV and RCC_MCO2CFGR_MCO2DIV are the same value */
  MODIFY_REG(*pReg, RCC_MCO1CFGR_MCO1DIV, MCOxPrescaler);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_PLL PLL
  * @{
  */

/**
  * @brief  Set the oscillator used as PLL1 and PLL2 clock source.
  * @note   PLLSRC can be written only when all PLL1 and PLL2 are disabled.
  * @rmtoll RCK12SELR      PLL12SRC        LL_RCC_PLL12_SetSource
  * @param  PLLSource parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL12SOURCE_HSI
  *         @arg @ref LL_RCC_PLL12SOURCE_HSE
  *         @arg @ref LL_RCC_PLL12SOURCE_NONE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL12_SetSource(uint32_t PLLSource)
{
  MODIFY_REG(RCC->RCK12SELR, RCC_RCK12SELR_PLL12SRC, PLLSource);
}

/**
  * @brief  Get the oscillator used as PLL1 and PLL2 clock source.
  * @rmtoll RCK12SELR      PLL12SRC        LL_RCC_PLL12_GetSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL12SOURCE_HSI
  *         @arg @ref LL_RCC_PLL12SOURCE_HSE
  *         @arg @ref LL_RCC_PLL12SOURCE_NONE
  */
__STATIC_INLINE uint32_t LL_RCC_PLL12_GetSource(void)
{
  return (uint32_t)(READ_BIT(RCC->RCK12SELR, RCC_RCK12SELR_PLL12SRC));
}

/**
  * @brief  Enable PLL1
  * @rmtoll PLL1CR           PLLON         LL_RCC_PLL1_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1_Enable(void)
{
  SET_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON);
}

/**
  * @brief  Disable PLL1
  * @note Cannot be disabled if the PLL clock is used as a system clock.
  *       This API shall be called only when PLL1 DIVPEN, DIVQEN and
  *       DIVREN are disabled.
  * @rmtoll PLL1CR           PLLON         LL_RCC_PLL1_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1_Disable(void)
{
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON);
}

/**
  * @brief  Check if PLL1 Ready
  * @rmtoll PLL1CR           PLL1RDY        LL_RCC_PLL1_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1_IsReady(void)
{
  return ((READ_BIT(RCC->PLL1CR, RCC_PLL1CR_PLL1RDY) == RCC_PLL1CR_PLL1RDY) ? 1UL : 0UL);
}

/**
  * @brief  Enable PLL1P
  * @note   This API shall be called only when PLL1 is enabled and ready.
  * @rmtoll PLL1CR           DIVPEN         LL_RCC_PLL1P_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1P_Enable(void)
{
  SET_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
}

/**
  * @brief  Enable PLL1 FRACV
  * @rmtoll PLL1FRACR           FRACLE         LL_RCC_PLL1FRACV_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1FRACV_Enable(void)
{
  SET_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACLE);
}

/**
  * @brief  Enable PLL1 Clock Spreading Generator
  * @rmtoll PLL1CR           SSCG_CTRL         LL_RCC_PLL1CSG_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1CSG_Enable(void)
{
  SET_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL);
}

/**
  * @brief  Check if PLL1 P is enabled
  * @rmtoll PLL1CR           DIVPEN         LL_RCC_PLL1P_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1P_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN) == RCC_PLL1CR_DIVPEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL1 FRACV is enabled
  * @rmtoll PLL1FRACR           FRACLE         LL_RCC_PLL1FRACV_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1FRACV_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACLE) == RCC_PLL1FRACR_FRACLE) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL1 Clock Spreading Generator is enabled
  * @rmtoll PLL1CR           SSCG_CTRL         LL_RCC_PLL1CSG_IsEnabled
  * @retval None
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1CSG_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL) == RCC_PLL1CR_SSCG_CTRL) ? 1UL : 0UL);
}

/**
  * @brief  Disable PLL1P
  * @rmtoll PLL1CR           DIVPEN         LL_RCC_PLL1P_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1P_Disable(void)
{
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN);
}

/**
  * @brief  Disable PLL1 FRACV
  * @rmtoll PLL1FRACR           FRACLE         LL_RCC_PLL1FRACV_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1FRACV_Disable(void)
{
  CLEAR_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACLE);
}

/**
  * @brief  Disable PLL1 Clock Spreading Generator
  * @rmtoll PLL1CR           SSCG_CTRL         LL_RCC_PLL1CSG_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1CSG_Disable(void)
{
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL);
}

/**
  * @brief  Get PLL1 N Coefficient
  * @rmtoll PLL1CFGR1        DIVN          LL_RCC_PLL1_GetN
  * @retval A value between 4 and 512
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1_GetN(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1CFGR1, RCC_PLL1CFGR1_DIVN) >>  RCC_PLL1CFGR1_DIVN_Pos) + 1U);
}

/**
  * @brief  Get PLL1 M Coefficient
  * @rmtoll PLL1CFGR1       DIVM1          LL_RCC_PLL1_GetM
  * @retval A value between 1 and 64
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1_GetM(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1CFGR1, RCC_PLL1CFGR1_DIVM1) >>  RCC_PLL1CFGR1_DIVM1_Pos) + 1U);
}

/**
  * @brief  Get PLL1 P Coefficient
  * @rmtoll PLL1CFGR2        DIVP          LL_RCC_PLL1_GetP
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1_GetP(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL1CFGR2, RCC_PLL1CFGR2_DIVP) >>  RCC_PLL1CFGR2_DIVP_Pos) + 1U);
}

/**
  * @brief  Get PLL1 FRACV Coefficient
  * @rmtoll PLL1FRACR      FRACV          LL_RCC_PLL1_GetFRACV
  * @retval A value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE uint32_t LL_RCC_PLL1_GetFRACV(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACV) >>  RCC_PLL1FRACR_FRACV_Pos);
}

/**
  * @brief  Set PLL1 N Coefficient
  * @note   This API shall be called only when PLL1 is disabled.
  * @rmtoll PLL1CFGR1        DIVN          LL_RCC_PLL1_SetN
  * @param  DIVN parameter can be a value between 4 and 512
  */
__STATIC_INLINE void LL_RCC_PLL1_SetN(uint32_t DIVN)
{
  MODIFY_REG(RCC->PLL1CFGR1, RCC_PLL1CFGR1_DIVN, (DIVN - 1U) << RCC_PLL1CFGR1_DIVN_Pos);
}

/**
  * @brief  Set PLL1 M Coefficient
  * @note   This API shall be called only when PLL1 is disabled.
  * @rmtoll PLL1CFGR1       DIVM1          LL_RCC_PLL1_SetM
  * @param  DIVM1 parameter can be a value between 1 and 64
  */
__STATIC_INLINE void LL_RCC_PLL1_SetM(uint32_t DIVM1)
{
  MODIFY_REG(RCC->PLL1CFGR1, RCC_PLL1CFGR1_DIVM1, (DIVM1 - 1U) << RCC_PLL1CFGR1_DIVM1_Pos);
}

/**
  * @brief  Set PLL1 P Coefficient
  * @rmtoll PLL1CFGR2        DIVP          LL_RCC_PLL1_SetP
  * @param  DIVP parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL1_SetP(uint32_t DIVP)
{
  MODIFY_REG(RCC->PLL1CFGR2, RCC_PLL1CFGR2_DIVP, (DIVP - 1U) << RCC_PLL1CFGR2_DIVP_Pos);
}

/**
  * @brief  Set PLL1 FRACV Coefficient
  * @rmtoll PLL1FRACR        FRACV          LL_RCC_PLL1_SetFRACV
  * @param  FRACV parameter can be a value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE void LL_RCC_PLL1_SetFRACV(uint32_t FRACV)
{
  MODIFY_REG(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACV, FRACV << RCC_PLL1FRACR_FRACV_Pos);
}

/** @brief  Configure the PLL1 Clock Spreading Generator
  * @rmtoll PLL1CSGR    MOD_PER, TPDFN_DIS, RPDFN_DIS, SSCG_MODE, INC_STEP  LL_RCC_PLL1_ConfigCSG
  *
  * @param  ModPeriod: Modulation Period Adjustment for PLL1
  *         This parameter must have a value between 1 and 8191
  *
  * @param  TPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL1TPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL1TPDFN_DIS_DISABLED

  * @param  RPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL1RPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL1RPDFN_DIS_DISABLED
  *
  * @param  SSCGMode
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL1SSCG_CENTER_SPREAD
  *         @arg @ref LL_RCC_PLL1SSCG_DOWN_SPREAD
  *
  * @param  IncStep: Modulation Depth Adjustment for PLL1
  *         This parameter must have a value between 1 and 32767
  * @note   ModPeriod x IncStep shall not exceed (2^15)-1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL1_ConfigCSG(uint32_t ModPeriod, uint32_t TPDFN, uint32_t RPDFN, uint32_t SSCGMode, uint32_t IncStep)
{
  MODIFY_REG(RCC->PLL1CSGR, (RCC_PLL1CSGR_MOD_PER | RCC_PLL1CSGR_TPDFN_DIS | RCC_PLL1CSGR_RPDFN_DIS | \
                             RCC_PLL1CSGR_SSCG_MODE | RCC_PLL1CSGR_INC_STEP), \
             (ModPeriod | TPDFN | RPDFN | SSCGMode | (IncStep << RCC_PLL1CSGR_INC_STEP_Pos)));
}

/**
  * @brief  Enable PLL2
  * @rmtoll PLL2CR           PLLON         LL_RCC_PLL2_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2_Enable(void)
{
  SET_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON);
}

/**
  * @brief  Disable PLL2
  * @note Cannot be disabled if the PLL clock is used as a system clock.
  *       This API shall be called only when PLL1 DIVPEN, DIVQEN and DIVREN
  *       are disabled.
  * @rmtoll PLL2CR           PLLON         LL_RCC_PLL2_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2_Disable(void)
{
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON);
}

/**
  * @brief  Check if PLL2 Ready
  * @rmtoll PLL2CR           PLL2RDY        LL_RCC_PLL2_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_IsReady(void)
{
  return ((READ_BIT(RCC->PLL2CR, RCC_PLL2CR_PLL2RDY) == RCC_PLL2CR_PLL2RDY) ? 1UL : 0UL);
}

/**
  * @brief  Enable PLL2P
  * @note   This API shall be called only when PLL2 is enabled and ready.
  * @rmtoll PLL2CR           DIVPEN         LL_RCC_PLL2P_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2P_Enable(void)
{
  SET_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVPEN);
}

/**
  * @brief  Enable PLL2Q
  * @note   This API shall be called only when PLL2 is enabled and ready.
  * @rmtoll PLL2CR           DIVQEN         LL_RCC_PLL2Q_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2Q_Enable(void)
{
  SET_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVQEN);
}

/**
  * @brief  Enable PLL2R
  * @note   This API shall be called only when PLL2 is enabled and ready.
  * @rmtoll PLL2CR           DIVREN         LL_RCC_PLL2R_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2R_Enable(void)
{
  SET_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVREN);
}

/**
  * @brief  Enable PLL2 FRACV
  * @rmtoll PLL2FRACR           FRACLE         LL_RCC_PLL2FRACV_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2FRACV_Enable(void)
{
  SET_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACLE);
}

/**
  * @brief  Enable PLL2 Clock Spreading Generator
  * @rmtoll PLL2CR           SSCG_CTRL         LL_RCC_PLL2CSG_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2CSG_Enable(void)
{
  SET_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL);
}

/**
  * @brief  Check if PLL2 P is enabled
  * @rmtoll PLL2CR           DIVPEN         LL_RCC_PLL2P_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2P_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVPEN) == RCC_PLL2CR_DIVPEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL2 Q is enabled
  * @rmtoll PLL2CR           DIVQEN         LL_RCC_PLL2Q_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2Q_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVQEN) == RCC_PLL2CR_DIVQEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL2 R is enabled
  * @rmtoll PLL2CR           DIVREN         LL_RCC_PLL2R_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2R_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVREN) == RCC_PLL2CR_DIVREN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL2 FRACV is enabled
  * @rmtoll PLL2FRACR           FRACLE         LL_RCC_PLL2FRACV_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2FRACV_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACLE) == RCC_PLL2FRACR_FRACLE) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL2 Clock Spreading Generator is enabled
  * @rmtoll PLL2CR           SSCG_CTRL         LL_RCC_PLL2CSG_IsEnabled
  * @retval None
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2CSG_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL) == RCC_PLL2CR_SSCG_CTRL) ? 1UL : 0UL);
}

/**
  * @brief  Disable PLL2P
  * @rmtoll PLL2CR           DIVPEN         LL_RCC_PLL2P_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2P_Disable(void)
{
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVPEN);
}

/**
  * @brief  Disable PLL2Q
  * @rmtoll PLL2CR           DIVQEN         LL_RCC_PLL2Q_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2Q_Disable(void)
{
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVQEN);
}

/**
  * @brief  Disable PLL2R
  * @rmtoll PLL2CR           DIVREN         LL_RCC_PLL2R_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2R_Disable(void)
{
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVREN);
}

/**
  * @brief  Disable PLL2 FRACV
  * @rmtoll PLL2FRACR           FRACLE         LL_RCC_PLL2FRACV_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2FRACV_Disable(void)
{
  CLEAR_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACLE);
}

/**
  * @brief  Disable PLL2 Clock Spreading Generator
  * @rmtoll PLL2CR           SSCG_CTRL         LL_RCC_PLL2CSG_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2CSG_Disable(void)
{
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL);
}

/**
  * @brief  Get PLL2 N Coefficient
  * @rmtoll PLL2CFGR1        DIVN          LL_RCC_PLL2_GetN
  * @retval A value between 4 and 512
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetN(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CFGR1, RCC_PLL2CFGR1_DIVN) >>  RCC_PLL2CFGR1_DIVN_Pos) + 1U);
}

/**
  * @brief  Get PLL2 M Coefficient
  * @rmtoll PLL2CFGR1       DIVM2          LL_RCC_PLL2_GetM
  * @retval A value between 1 and 64
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetM(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CFGR1, RCC_PLL2CFGR1_DIVM2) >>  RCC_PLL2CFGR1_DIVM2_Pos) + 1U);
}

/**
  * @brief  Get PLL2 P Coefficient
  * @rmtoll PLL2CFGR2        DIVP          LL_RCC_PLL2_GetP
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetP(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVP) >>  RCC_PLL2CFGR2_DIVP_Pos) + 1U);
}

/**
  * @brief  Get PLL2 Q Coefficient
  * @rmtoll PLL2CFGR2        DIVQ          LL_RCC_PLL2_GetQ
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetQ(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVQ) >>  RCC_PLL2CFGR2_DIVQ_Pos) + 1U);
}

/**
  * @brief  Get PLL2 R Coefficient
  * @rmtoll PLL2CFGR2        DIVR          LL_RCC_PLL2_GetR
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetR(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVR) >>  RCC_PLL2CFGR2_DIVR_Pos) + 1U);
}

/**
  * @brief  Get PLL2 FRACV Coefficient
  * @rmtoll PLL2FRACR      FRACV          LL_RCC_PLL2_GetFRACV
  * @retval A value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE uint32_t LL_RCC_PLL2_GetFRACV(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACV) >>  RCC_PLL2FRACR_FRACV_Pos);
}

/**
  * @brief  Set PLL2 N Coefficient
  * @note   This API shall be called only when PLL2 is disabled.
  * @rmtoll PLL2CFGR1        DIVN          LL_RCC_PLL2_SetN
  * @param  DIVN parameter can be a value between 4 and 512
  */
__STATIC_INLINE void LL_RCC_PLL2_SetN(uint32_t DIVN)
{
  MODIFY_REG(RCC->PLL2CFGR1, RCC_PLL2CFGR1_DIVN, (DIVN - 1U) << RCC_PLL2CFGR1_DIVN_Pos);
}

/**
  * @brief  Set PLL2 M Coefficient
  * @note   This API shall be called only when PLL2 is disabled.
  * @rmtoll PLL2CFGR1       DIVM2          LL_RCC_PLL2_SetM
  * @param  DIVM2 parameter can be a value between 1 and 64
  */
__STATIC_INLINE void LL_RCC_PLL2_SetM(uint32_t DIVM2)
{
  MODIFY_REG(RCC->PLL2CFGR1, RCC_PLL2CFGR1_DIVM2, (DIVM2 - 1U) << RCC_PLL2CFGR1_DIVM2_Pos);
}

/**
  * @brief  Set PLL2 P Coefficient
  * @rmtoll PLL2CFGR2        DIVP          LL_RCC_PLL2_SetP
  * @param  DIVP parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL2_SetP(uint32_t DIVP)
{
  MODIFY_REG(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVP, (DIVP - 1U) << RCC_PLL2CFGR2_DIVP_Pos);
}

/**
  * @brief  Set PLL2 Q Coefficient
  * @rmtoll PLL2CFGR2        DIVQ          LL_RCC_PLL2_SetQ
  * @param  DIVQ parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL2_SetQ(uint32_t DIVQ)
{
  MODIFY_REG(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVQ, (DIVQ - 1U) << RCC_PLL2CFGR2_DIVQ_Pos);
}

/**
  * @brief  Set PLL2 R Coefficient
  * @rmtoll PLL2CFGR2        DIVR          LL_RCC_PLL2_SetR
  * @param  DIVR parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL2_SetR(uint32_t DIVR)
{
  MODIFY_REG(RCC->PLL2CFGR2, RCC_PLL2CFGR2_DIVR, (DIVR - 1U) << RCC_PLL2CFGR2_DIVR_Pos);
}

/**
  * @brief  Set PLL2 FRACV Coefficient
  * @rmtoll PLL2FRACR        FRACV          LL_RCC_PLL2_SetFRACV
  * @param  FRACV parameter can be a value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE void LL_RCC_PLL2_SetFRACV(uint32_t FRACV)
{
  MODIFY_REG(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACV, FRACV << RCC_PLL2FRACR_FRACV_Pos);
}

/** @brief  Configure the PLL2 Clock Spreading Generator
  * @rmtoll PLL2CSGR    MOD_PER, TPDFN_DIS, RPDFN_DIS, SSCG_MODE, INC_STEP  LL_RCC_PLL2_ConfigCSG
  *
  * @param  ModPeriod: Modulation Period Adjustment for PLL2
  *         This parameter must have a value between 1 and 8191
  *
  * @param  TPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL2TPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL2TPDFN_DIS_DISABLED

  * @param  RPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL2RPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL2RPDFN_DIS_DISABLED
  *
  * @param  SSCGMode
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL2SSCG_CENTER_SPREAD
  *         @arg @ref LL_RCC_PLL2SSCG_DOWN_SPREAD
  *
  * @param  IncStep: Modulation Depth Adjustment for PLL2
  *         This parameter must have a value between 1 and 32767
  * @note   ModPeriod x IncStep shall not exceed (2^15)-1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL2_ConfigCSG(uint32_t ModPeriod, uint32_t TPDFN, uint32_t RPDFN, uint32_t SSCGMode, uint32_t IncStep)
{
  MODIFY_REG(RCC->PLL2CSGR, (RCC_PLL2CSGR_MOD_PER | RCC_PLL2CSGR_TPDFN_DIS | RCC_PLL2CSGR_RPDFN_DIS | \
                             RCC_PLL2CSGR_SSCG_MODE | RCC_PLL2CSGR_INC_STEP), \
             (ModPeriod | TPDFN | RPDFN | SSCGMode | (IncStep << RCC_PLL2CSGR_INC_STEP_Pos)));
}

/**
  * @brief  Set the oscillator used as PLL3 clock source.
  * @note   PLLSRC can be written only when all PLL3 is disabled.
  * @rmtoll RCK3SELR      PLL3SRC        LL_RCC_PLL3_SetSource
  * @param  PLLSource parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL3SOURCE_HSI
  *         @arg @ref LL_RCC_PLL3SOURCE_HSE
  *         @arg @ref LL_RCC_PLL3SOURCE_CSI
  *         @arg @ref LL_RCC_PLL3SOURCE_NONE
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3_SetSource(uint32_t PLLSource)
{
  MODIFY_REG(RCC->RCK3SELR, RCC_RCK3SELR_PLL3SRC, PLLSource);
}

/**
  * @brief  Get the oscillator used as PLL3 clock source.
  * @rmtoll RCK3SELR      PLL3SRC        LL_RCC_PLL3_GetSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL3SOURCE_HSI
  *         @arg @ref LL_RCC_PLL3SOURCE_HSE
  *         @arg @ref LL_RCC_PLL3SOURCE_CSI
  *         @arg @ref LL_RCC_PLL3SOURCE_NONE
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetSource(void)
{
  return (uint32_t)(READ_BIT(RCC->RCK3SELR, RCC_RCK3SELR_PLL3SRC));
}

/**
  * @brief  Enable PLL3
  * @rmtoll PLL3CR           PLLON         LL_RCC_PLL3_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3_Enable(void)
{
  SET_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON);
}

/**
  * @brief  Disable PLL3
  * @note Cannot be disabled if the PLL clock is used as a system clock.
  *       This API shall be called only when PLL1 DIVPEN, DIVQEN and DIVREN are
  *       disabled.
  * @rmtoll PLL3CR           PLLON         LL_RCC_PLL3_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3_Disable(void)
{
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON);
}

/**
  * @brief  Check if PLL3 Ready
  * @rmtoll PLL3CR           PLL3RDY        LL_RCC_PLL3_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_IsReady(void)
{
  return ((READ_BIT(RCC->PLL3CR, RCC_PLL3CR_PLL3RDY) == RCC_PLL3CR_PLL3RDY) ? 1UL : 0UL);
}

/**
  * @brief  Enable PLL3P
  * @note   This API shall be called only when PLL3 is enabled and ready.
  * @rmtoll PLL3CR           DIVPEN         LL_RCC_PLL3P_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3P_Enable(void)
{
  SET_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVPEN);
}

/**
  * @brief  Enable PLL3Q
  * @note   This API shall be called only when PLL3 is enabled and ready.
  * @rmtoll PLL3CR           DIVQEN         LL_RCC_PLL3Q_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3Q_Enable(void)
{
  SET_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVQEN);
}

/**
  * @brief  Enable PLL3R
  * @note   This API shall be called only when PLL3 is enabled and ready.
  * @rmtoll PLL3CR           DIVREN         LL_RCC_PLL3R_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3R_Enable(void)
{
  SET_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVREN);
}

/**
  * @brief  Enable PLL3 FRACV
  * @rmtoll PLL3FRACR           FRACLE         LL_RCC_PLL3FRACV_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3FRACV_Enable(void)
{
  SET_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACLE);
}

/**
  * @brief  Enable PLL3 Clock Spreading Generator
  * @rmtoll PLL3CR           SSCG_CTRL         LL_RCC_PLL3CSG_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3CSG_Enable(void)
{
  SET_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL);
}

/**
  * @brief  Check if PLL3 P is enabled
  * @rmtoll PLL3CR           DIVPEN         LL_RCC_PLL3P_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3P_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVPEN) == RCC_PLL3CR_DIVPEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL3 Q is enabled
  * @rmtoll PLL3CR           DIVQEN         LL_RCC_PLL3Q_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3Q_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVQEN) == RCC_PLL3CR_DIVQEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL3 R is enabled
  * @rmtoll PLL3CR           DIVREN         LL_RCC_PLL3R_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3R_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVREN) == RCC_PLL3CR_DIVREN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL3 FRACV is enabled
  * @rmtoll PLL3FRACR           FRACLE         LL_RCC_PLL3FRACV_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3FRACV_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACLE) == RCC_PLL3FRACR_FRACLE) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL3 Clock Spreading Generator is enabled
  * @rmtoll PLL3CR           SSCG_CTRL         LL_RCC_PLL3CSG_IsEnabled
  * @retval None
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3CSG_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL) == RCC_PLL3CR_SSCG_CTRL) ? 1UL : 0UL);
}

/**
  * @brief  Disable PLL3P
  * @rmtoll PLL3CR           DIVPEN         LL_RCC_PLL3P_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3P_Disable(void)
{
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVPEN);
}

/**
  * @brief  Disable PLL3Q
  * @rmtoll PLL3CR           DIVQEN         LL_RCC_PLL3Q_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3Q_Disable(void)
{
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVQEN);
}

/**
  * @brief  Disable PLL3R
  * @rmtoll PLL3CR           DIVREN         LL_RCC_PLL3R_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3R_Disable(void)
{
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVREN);
}

/**
  * @brief  Disable PLL3 FRACV
  * @rmtoll PLL3FRACR           FRACLE         LL_RCC_PLL3FRACV_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3FRACV_Disable(void)
{
  CLEAR_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACLE);
}

/**
  * @brief  Disable PLL3 Clock Spreading Generator
  * @rmtoll PLL3CR           SSCG_CTRL         LL_RCC_PLL3CSG_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3CSG_Disable(void)
{
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL);
}

/**
  * @brief  Get PLL3 N Coefficient
  * @rmtoll PLL3CFGR1        DIVN          LL_RCC_PLL3_GetN
  * @retval A value between 4 and 512
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetN(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CFGR1, RCC_PLL3CFGR1_DIVN) >>  RCC_PLL3CFGR1_DIVN_Pos) + 1U);
}

/**
  * @brief  Get PLL3 M Coefficient
  * @rmtoll PLL3CFGR1       DIVM3          LL_RCC_PLL3_GetM
  * @retval A value between 1 and 64
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetM(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CFGR1, RCC_PLL3CFGR1_DIVM3) >>  RCC_PLL3CFGR1_DIVM3_Pos) + 1U);
}

/**
  * @brief  Get PLL3 input frequency range
  * @rmtoll PLL3CFGR1       IFRGE          LL_RCC_PLL3_GetIFRGE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL3IFRANGE_0
  *         @arg @ref LL_RCC_PLL3IFRANGE_1
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetIFRGE(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL3CFGR1, RCC_PLL3CFGR1_IFRGE));
}

/**
  * @brief  Get PLL3 P Coefficient
  * @rmtoll PLL3CFGR2        DIVP          LL_RCC_PLL3_GetP
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetP(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVP) >>  RCC_PLL3CFGR2_DIVP_Pos) + 1U);
}

/**
  * @brief  Get PLL3 Q Coefficient
  * @rmtoll PLL3CFGR2        DIVQ          LL_RCC_PLL3_GetQ
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetQ(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVQ) >>  RCC_PLL3CFGR2_DIVQ_Pos) + 1U);
}

/**
  * @brief  Get PLL3 R Coefficient
  * @rmtoll PLL3CFGR2        DIVR          LL_RCC_PLL3_GetR
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetR(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVR) >>  RCC_PLL3CFGR2_DIVR_Pos) + 1U);
}

/**
  * @brief  Get PLL3 FRACV Coefficient
  * @rmtoll PLL3FRACR      FRACV          LL_RCC_PLL3_GetFRACV
  * @retval A value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE uint32_t LL_RCC_PLL3_GetFRACV(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACV) >>  RCC_PLL3FRACR_FRACV_Pos);
}

/**
  * @brief  Set PLL3 N Coefficient
  * @note   This API shall be called only when PLL3 is disabled.
  * @rmtoll PLL3CFGR1        DIVN          LL_RCC_PLL3_SetN
  * @param  DIVN parameter can be a value between 4 and 512
  */
__STATIC_INLINE void LL_RCC_PLL3_SetN(uint32_t DIVN)
{
  MODIFY_REG(RCC->PLL3CFGR1, RCC_PLL3CFGR1_DIVN, (DIVN - 1U) << RCC_PLL3CFGR1_DIVN_Pos);
}

/**
  * @brief  Set PLL3 M Coefficient
  * @note   This API shall be called only when PLL3 is disabled.
  * @rmtoll PLL3CFGR1       DIVM3          LL_RCC_PLL3_SetM
  * @param  DIVM3 parameter can be a value between 1 and 64
  */
__STATIC_INLINE void LL_RCC_PLL3_SetM(uint32_t DIVM3)
{
  MODIFY_REG(RCC->PLL3CFGR1, RCC_PLL3CFGR1_DIVM3, (DIVM3 - 1U) << RCC_PLL3CFGR1_DIVM3_Pos);
}

/**
  * @brief  Set PLL3 input frequency range
  * @rmtoll PLL3CFGR1       IFRGE          LL_RCC_PLL3_SetIFRGE
  * @param  IFRange parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL3IFRANGE_0
  *         @arg @ref LL_RCC_PLL3IFRANGE_1
  * @note   If ref3_ck is equal to 8 MHz, it is recommended to set LL_RCC_PLL3IFRANGE_1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3_SetIFRGE(uint32_t IFRange)
{
  MODIFY_REG(RCC->PLL3CFGR1, RCC_PLL3CFGR1_IFRGE, IFRange);
}

/**
  * @brief  Set PLL3 P Coefficient
  * @rmtoll PLL3CFGR2        DIVP          LL_RCC_PLL3_SetP
  * @param  DIVP parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL3_SetP(uint32_t DIVP)
{
  MODIFY_REG(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVP, (DIVP - 1U) << RCC_PLL3CFGR2_DIVP_Pos);
}

/**
  * @brief  Set PLL3 Q Coefficient
  * @rmtoll PLL3CFGR2        DIVQ          LL_RCC_PLL3_SetQ
  * @param  DIVQ parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL3_SetQ(uint32_t DIVQ)
{
  MODIFY_REG(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVQ, (DIVQ - 1U) << RCC_PLL3CFGR2_DIVQ_Pos);
}

/**
  * @brief  Set PLL3 R Coefficient
  * @rmtoll PLL3CFGR2        DIVR          LL_RCC_PLL3_SetR
  * @param  DIVR parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL3_SetR(uint32_t DIVR)
{
  MODIFY_REG(RCC->PLL3CFGR2, RCC_PLL3CFGR2_DIVR, (DIVR - 1U) << RCC_PLL3CFGR2_DIVR_Pos);
}

/**
  * @brief  Set PLL3 FRACV Coefficient
  * @rmtoll PLL3FRACR        FRACV          LL_RCC_PLL3_SetFRACV
  * @param  FRACV parameter can be a value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE void LL_RCC_PLL3_SetFRACV(uint32_t FRACV)
{
  MODIFY_REG(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACV, FRACV << RCC_PLL3FRACR_FRACV_Pos);
}

/** @brief  Configure the PLL3 Clock Spreading Generator
  * @rmtoll PLL3CSGR    MOD_PER, TPDFN_DIS, RPDFN_DIS, SSCG_MODE, INC_STEP  LL_RCC_PLL3_ConfigCSG
  *
  * @param  ModPeriod: Modulation Period Adjustment for PLL3
  *         This parameter must have a value between 1 and 8191
  *
  * @param  TPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL3TPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL3TPDFN_DIS_DISABLED

  * @param  RPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL3RPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL3RPDFN_DIS_DISABLED
  *
  * @param  SSCGMode
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL3SSCG_CENTER_SPREAD
  *         @arg @ref LL_RCC_PLL3SSCG_DOWN_SPREAD
  *
  * @param  IncStep: Modulation Depth Adjustment for PLL3
  *         This parameter must have a value between 1 and 32767
  * @note   ModPeriod x IncStep shall not exceed (2^15)-1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL3_ConfigCSG(uint32_t ModPeriod, uint32_t TPDFN, uint32_t RPDFN, uint32_t SSCGMode, uint32_t IncStep)
{
  MODIFY_REG(RCC->PLL3CSGR, (RCC_PLL3CSGR_MOD_PER | RCC_PLL3CSGR_TPDFN_DIS | RCC_PLL3CSGR_RPDFN_DIS | \
                             RCC_PLL3CSGR_SSCG_MODE | RCC_PLL3CSGR_INC_STEP), \
             (ModPeriod | TPDFN | RPDFN | SSCGMode | (IncStep << RCC_PLL3CSGR_INC_STEP_Pos)));
}

/**
  * @brief  Set the oscillator used as PLL4 clock source.
  * @note   PLLSRC can be written only when all PLL4 is disabled.
  * @rmtoll RCK4SELR      PLL4SRC        LL_RCC_PLL4_SetSource
  * @param  PLLSource parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL4SOURCE_HSI
  *         @arg @ref LL_RCC_PLL4SOURCE_HSE
  *         @arg @ref LL_RCC_PLL4SOURCE_CSI
  *         @arg @ref LL_RCC_PLL4SOURCE_I2SCKIN
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4_SetSource(uint32_t PLLSource)
{
  MODIFY_REG(RCC->RCK4SELR, RCC_RCK4SELR_PLL4SRC, PLLSource);
}

/**
  * @brief  Get the oscillator used as PLL4 clock source.
  * @rmtoll RCK4SELR      PLL4SRC        LL_RCC_PLL4_GetSource
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL4SOURCE_HSI
  *         @arg @ref LL_RCC_PLL4SOURCE_HSE
  *         @arg @ref LL_RCC_PLL4SOURCE_CSI
  *         @arg @ref LL_RCC_PLL4SOURCE_I2SCKIN
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetSource(void)
{
  return (uint32_t)(READ_BIT(RCC->RCK4SELR, RCC_RCK4SELR_PLL4SRC));
}

/**
  * @brief  Enable PLL4
  * @rmtoll PLL4CR           PLLON         LL_RCC_PLL4_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4_Enable(void)
{
  SET_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON);
}

/**
  * @brief  Disable PLL4
  * @note Cannot be disabled if the PLL clock is used as a system clock.
  *       This API shall be called only when PLL1 DIVPEN, DIVQEN and DIVREN
  *       are disabled.
  * @rmtoll PLL4CR           PLLON         LL_RCC_PLL4_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4_Disable(void)
{
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON);
}

/**
  * @brief  Check if PLL4 Ready
  * @rmtoll PLL4CR           PLL4RDY        LL_RCC_PLL4_IsReady
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_IsReady(void)
{
  return ((READ_BIT(RCC->PLL4CR, RCC_PLL4CR_PLL4RDY) == RCC_PLL4CR_PLL4RDY) ? 1UL : 0UL);
}

/**
  * @brief  Enable PLL4P
  * @note   This API shall be called only when PLL4 is enabled and ready.
  * @rmtoll PLL4CR           DIVPEN         LL_RCC_PLL4P_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4P_Enable(void)
{
  SET_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVPEN);
}

/**
  * @brief  Enable PLL4Q
  * @note   This API shall be called only when PLL4 is enabled and ready.
  * @rmtoll PLL4CR           DIVQEN         LL_RCC_PLL4Q_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4Q_Enable(void)
{
  SET_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVQEN);
}

/**
  * @brief  Enable PLL4R
  * @note   This API shall be called only when PLL4 is enabled and ready.
  * @rmtoll PLL4CR           DIVREN         LL_RCC_PLL4R_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4R_Enable(void)
{
  SET_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVREN);
}

/**
  * @brief  Enable PLL4 FRACV
  * @rmtoll PLL4FRACR           FRACLE         LL_RCC_PLL4FRACV_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4FRACV_Enable(void)
{
  SET_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACLE);
}

/**
  * @brief  Enable PLL4 Clock Spreading Generator
  * @rmtoll PLL4CR           SSCG_CTRL         LL_RCC_PLL4CSG_Enable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4CSG_Enable(void)
{
  SET_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL);
}

/**
  * @brief  Check if PLL4 P is enabled
  * @rmtoll PLL4CR           DIVPEN         LL_RCC_PLL4P_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4P_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVPEN) == RCC_PLL4CR_DIVPEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL4 Q is enabled
  * @rmtoll PLL4CR           DIVQEN         LL_RCC_PLL4Q_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4Q_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVQEN) == RCC_PLL4CR_DIVQEN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL4 R is enabled
  * @rmtoll PLL4CR           DIVREN         LL_RCC_PLL4R_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4R_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVREN) == RCC_PLL4CR_DIVREN) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL4 FRACV is enabled
  * @rmtoll PLL4FRACR           FRACLE         LL_RCC_PLL4FRACV_IsEnabled
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4FRACV_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACLE) == RCC_PLL4FRACR_FRACLE) ? 1UL : 0UL);
}

/**
  * @brief  Check if PLL4 Clock Spreading Generator is enabled
  * @rmtoll PLL4CR           SSCG_CTRL         LL_RCC_PLL4CSG_IsEnabled
  * @retval None
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4CSG_IsEnabled(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL) == RCC_PLL4CR_SSCG_CTRL) ? 1UL : 0UL);
}

/**
  * @brief  Disable PLL4P
  * @rmtoll PLL4CR           DIVPEN         LL_RCC_PLL4P_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4P_Disable(void)
{
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVPEN);
}

/**
  * @brief  Disable PLL4Q
  * @rmtoll PLL4CR           DIVQEN         LL_RCC_PLL4Q_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4Q_Disable(void)
{
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVQEN);
}

/**
  * @brief  Disable PLL4R
  * @rmtoll PLL4CR           DIVREN         LL_RCC_PLL4R_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4R_Disable(void)
{
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVREN);
}

/**
  * @brief  Disable PLL4 FRACV
  * @rmtoll PLL4FRACR           FRACLE         LL_RCC_PLL4FRACV_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4FRACV_Disable(void)
{
  CLEAR_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACLE);
}

/**
  * @brief  Disable PLL4 Clock Spreading Generator
  * @rmtoll PLL4CR           SSCG_CTRL         LL_RCC_PLL4CSG_Disable
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4CSG_Disable(void)
{
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL);
}

/**
  * @brief  Get PLL4 N Coefficient
  * @rmtoll PLL4CFGR1        DIVN          LL_RCC_PLL4_GetN
  * @retval A value between 4 and 512
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetN(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CFGR1, RCC_PLL4CFGR1_DIVN) >>  RCC_PLL4CFGR1_DIVN_Pos) + 1U);
}

/**
  * @brief  Get PLL4 M Coefficient
  * @rmtoll PLL4CFGR1       DIVM4          LL_RCC_PLL4_GetM
  * @retval A value between 1 and 64
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetM(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CFGR1, RCC_PLL4CFGR1_DIVM4) >>  RCC_PLL4CFGR1_DIVM4_Pos) + 1U);
}

/**
  * @brief  Get PLL4 input frequency range
  * @rmtoll PLL4CFGR1       IFRGE          LL_RCC_PLL4_GetIFRGE
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_RCC_PLL4IFRANGE_0
  *         @arg @ref LL_RCC_PLL4IFRANGE_1
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetIFRGE(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL4CFGR1, RCC_PLL4CFGR1_IFRGE));
}

/**
  * @brief  Get PLL4 P Coefficient
  * @rmtoll PLL4CFGR2        DIVP          LL_RCC_PLL4_GetP
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetP(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVP) >>  RCC_PLL4CFGR2_DIVP_Pos) + 1U);
}

/**
  * @brief  Get PLL4 Q Coefficient
  * @rmtoll PLL4CFGR2        DIVQ          LL_RCC_PLL4_GetQ
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetQ(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVQ) >>  RCC_PLL4CFGR2_DIVQ_Pos) + 1U);
}

/**
  * @brief  Get PLL4 R Coefficient
  * @rmtoll PLL4CFGR2        DIVR          LL_RCC_PLL4_GetR
  * @retval A value between 1 and 128
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetR(void)
{
  return (uint32_t)((READ_BIT(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVR) >>  RCC_PLL4CFGR2_DIVR_Pos) + 1U);
}

/**
  * @brief  Get PLL4 FRACV Coefficient
  * @rmtoll PLL4FRACR      FRACV          LL_RCC_PLL4_GetFRACV
  * @retval A value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE uint32_t LL_RCC_PLL4_GetFRACV(void)
{
  return (uint32_t)(READ_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACV) >>  RCC_PLL4FRACR_FRACV_Pos);
}

/**
  * @brief  Set PLL4 N Coefficient
  * @note   This API shall be called only when PLL4 is disabled.
  * @rmtoll PLL4CFGR1        DIVN          LL_RCC_PLL4_SetN
  * @param  DIVN parameter can be a value between 4 and 512
  */
__STATIC_INLINE void LL_RCC_PLL4_SetN(uint32_t DIVN)
{
  MODIFY_REG(RCC->PLL4CFGR1, RCC_PLL4CFGR1_DIVN, (DIVN - 1U) << RCC_PLL4CFGR1_DIVN_Pos);
}

/**
  * @brief  Set PLL4 M Coefficient
  * @note   This API shall be called only when PLL4 is disabled.
  * @rmtoll PLL4CFGR1       DIVM4          LL_RCC_PLL4_SetM
  * @param  DIVM4 parameter can be a value between 1 and 64
  */
__STATIC_INLINE void LL_RCC_PLL4_SetM(uint32_t DIVM4)
{
  MODIFY_REG(RCC->PLL4CFGR1, RCC_PLL4CFGR1_DIVM4, (DIVM4 - 1U) << RCC_PLL4CFGR1_DIVM4_Pos);
}

/**
  * @brief  Set PLL4 input frequency range
  * @rmtoll PLL4CFGR1       IFRGE          LL_RCC_PLL4_SetIFRGE
  * @param  IFRange parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL4IFRANGE_0
  *         @arg @ref LL_RCC_PLL4IFRANGE_1
  * @note   If ref4_ck is equal to 8 MHz, it is recommended to set LL_RCC_PLL4IFRANGE_1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4_SetIFRGE(uint32_t IFRange)
{
  MODIFY_REG(RCC->PLL4CFGR1, RCC_PLL4CFGR1_IFRGE, IFRange);
}

/**
  * @brief  Set PLL4 P Coefficient
  * @rmtoll PLL4CFGR2        DIVP          LL_RCC_PLL4_SetP
  * @param  DIVP parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL4_SetP(uint32_t DIVP)
{
  MODIFY_REG(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVP, (DIVP - 1U) << RCC_PLL4CFGR2_DIVP_Pos);
}

/**
  * @brief  Set PLL4 Q Coefficient
  * @rmtoll PLL4CFGR2        DIVQ          LL_RCC_PLL4_SetQ
  * @param  DIVQ parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL4_SetQ(uint32_t DIVQ)
{
  MODIFY_REG(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVQ, (DIVQ - 1U) << RCC_PLL4CFGR2_DIVQ_Pos);
}

/**
  * @brief  Set PLL4 R Coefficient
  * @rmtoll PLL4CFGR2        DIVR          LL_RCC_PLL4_SetR
  * @param  DIVR parameter can be a value between 1 and 128
  */
__STATIC_INLINE void LL_RCC_PLL4_SetR(uint32_t DIVR)
{
  MODIFY_REG(RCC->PLL4CFGR2, RCC_PLL4CFGR2_DIVR, (DIVR - 1U) << RCC_PLL4CFGR2_DIVR_Pos);
}

/**
  * @brief  Set PLL4 FRACV Coefficient
  * @rmtoll PLL4FRACR        FRACV          LL_RCC_PLL4_SetFRACV
  * @param  FRACV parameter can be a value between 0 and 8191 (0x1FFF)
  */
__STATIC_INLINE void LL_RCC_PLL4_SetFRACV(uint32_t FRACV)
{
  MODIFY_REG(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACV, FRACV << RCC_PLL4FRACR_FRACV_Pos);
}

/** @brief  Configure the PLL4 Clock Spreading Generator
  * @rmtoll PLL4CSGR    MOD_PER, TPDFN_DIS, RPDFN_DIS, SSCG_MODE, INC_STEP  LL_RCC_PLL4_ConfigCSG
  *
  * @param  ModPeriod: Modulation Period Adjustment for PLL4
  *         This parameter must have a value between 1 and 8191
  *
  * @param  TPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL4TPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL4TPDFN_DIS_DISABLED

  * @param  RPDFN
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL4RPDFN_DIS_ENABLED
  *         @arg @ref LL_RCC_PLL4RPDFN_DIS_DISABLED
  *
  * @param  SSCGMode
  *         This parameter can be one of the following values:
  *         @arg @ref LL_RCC_PLL4SSCG_CENTER_SPREAD
  *         @arg @ref LL_RCC_PLL4SSCG_DOWN_SPREAD
  *
  * @param  IncStep: Modulation Depth Adjustment for PLL4
  *         This parameter must have a value between 1 and 32767
  * @note   ModPeriod x IncStep shall not exceed (2^15)-1
  * @retval None
  */
__STATIC_INLINE void LL_RCC_PLL4_ConfigCSG(uint32_t ModPeriod, uint32_t TPDFN, uint32_t RPDFN, uint32_t SSCGMode, uint32_t IncStep)
{
  MODIFY_REG(RCC->PLL4CSGR, (RCC_PLL4CSGR_MOD_PER | RCC_PLL4CSGR_TPDFN_DIS | RCC_PLL4CSGR_RPDFN_DIS | \
                             RCC_PLL4CSGR_SSCG_MODE | RCC_PLL4CSGR_INC_STEP), \
             (ModPeriod | TPDFN | RPDFN | SSCGMode | (IncStep << RCC_PLL4CSGR_INC_STEP_Pos)));
}


/**
  * @}
  */

/** @defgroup RCC_LL_EF_FLAG_Management FLAG Management
  * @{
  */

/**
  * @brief  Clear LSI ready interrupt flag
  * @rmtoll MC_CIFR      LSIRDYF      LL_RCC_ClearFlag_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSIRDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_LSIRDYF);
}

/**
  * @brief  Clear LSE ready interrupt flag
  * @rmtoll MC_CIFR      LSERDYF       LL_RCC_ClearFlag_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSERDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_LSERDYF);
}

/**
  * @brief  Clear HSI ready interrupt flag
  * @rmtoll MC_CIFR      HSIRDYF       LL_RCC_ClearFlag_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSIRDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_HSIRDYF);
}

/**
  * @brief  Clear HSE ready interrupt flag
  * @rmtoll CICR         HSERDYF       LL_RCC_ClearFlag_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_HSERDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_HSERDYF);
}

/**
  * @brief  Clear CSI ready interrupt flag
  * @rmtoll MC_CIFR      CSIRDYF       LL_RCC_ClearFlag_CSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_CSIRDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_CSIRDYF);
}

/**
  * @brief  Clear PLL1 ready interrupt flag
  * @rmtoll MC_CIFR      PLL1DYF       LL_RCC_ClearFlag_PLL1RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLL1RDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_PLL1DYF);
}

/**
  * @brief  Clear PLL2 ready interrupt flag
  * @rmtoll MC_CIFR      PLL2DYF       LL_RCC_ClearFlag_PLL2RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLL2RDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_PLL2DYF);
}

/**
  * @brief  Clear PLL3 ready interrupt flag
  * @rmtoll MC_CIFR      PLL3DYF       LL_RCC_ClearFlag_PLL3RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLL3RDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_PLL3DYF);
}

/**
  * @brief  Clear PLL4 ready interrupt flag
  * @rmtoll MC_CIFR      PLL4DYF       LL_RCC_ClearFlag_PLL4RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_PLL4RDY(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_PLL4DYF);
}

/**
  * @brief  Clear LSE Clock security system interrupt flag
  * @rmtoll MC_CIFR      LSECSSF       LL_RCC_ClearFlag_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_LSECSS(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_LSECSSF);
}

/**
  * @brief  Clear WKUP Wake up from CStop interrupt flag
  * @rmtoll MC_CIFR      WKUPF       LL_RCC_ClearFlag_WKUP
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearFlag_WKUP(void)
{
  WRITE_REG(RCC->MC_CIFR, RCC_MC_CIFR_WKUPF);
}

/**
  * @brief  Check if LSI ready interrupt occurred or not
  * @rmtoll MC_CIFR      LSIRDYF       LL_RCC_IsActiveFlag_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_LSIRDYF) == (RCC_MC_CIFR_LSIRDYF));
}

/**
  * @brief  Check if LSE ready interrupt occurred or not
  * @rmtoll MC_CIFR      LSERDYF       LL_RCC_IsActiveFlag_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSERDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_LSERDYF) == (RCC_MC_CIFR_LSERDYF));
}

/**
  * @brief  Check if HSI ready interrupt occurred or not
  * @rmtoll MC_CIFR      HSIRDYF       LL_RCC_IsActiveFlag_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_HSIRDYF) == (RCC_MC_CIFR_HSIRDYF));
}

/**
  * @brief  Check if HSE ready interrupt occurred or not
  * @rmtoll MC_CIFR      HSERDYF       LL_RCC_IsActiveFlag_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HSERDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_HSERDYF) == (RCC_MC_CIFR_HSERDYF));
}

/**
  * @brief  Check if CSI ready interrupt occurred or not
  * @rmtoll MC_CIFR      CSIRDYF       LL_RCC_IsActiveFlag_CSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_CSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_CSIRDYF) == (RCC_MC_CIFR_CSIRDYF));
}

/**
  * @brief  Check if PLL1 ready interrupt occurred or not
  * @rmtoll MC_CIFR      PLL1DYF       LL_RCC_IsActiveFlag_PLL1RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLL1RDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_PLL1DYF) == (RCC_MC_CIFR_PLL1DYF));
}

/**
  * @brief  Check if PLL2 ready interrupt occurred or not
  * @rmtoll MC_CIFR      PLL2DYF       LL_RCC_IsActiveFlag_PLL2RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLL2RDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_PLL2DYF) == (RCC_MC_CIFR_PLL2DYF));
}

/**
  * @brief  Check if PLL3 ready interrupt occurred or not
  * @rmtoll MC_CIFR      PLL3DYF       LL_RCC_IsActiveFlag_PLL3RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLL3RDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_PLL3DYF) == (RCC_MC_CIFR_PLL3DYF));
}

/**
  * @brief  Check if PLL4 ready interrupt occurred or not
  * @rmtoll MC_CIFR      PLL4DYF       LL_RCC_IsActiveFlag_PLL4RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PLL4RDY(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_PLL4DYF) == (RCC_MC_CIFR_PLL4DYF));
}

/**
  * @brief  Check if LSE Clock security system interrupt occurred or not
  * @rmtoll MC_CIFR      LSECSSF       LL_RCC_IsActiveFlag_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_LSECSS(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_LSECSSF) == (RCC_MC_CIFR_LSECSSF));
}

/**
  * @brief  Check if Wake up from CStop interrupt occurred or not
  * @rmtoll MC_CIFR      WKUPF       LL_RCC_IsActiveFlag_WKUP
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_WKUP(void)
{
  return (READ_BIT(RCC->MC_CIFR, RCC_MC_CIFR_WKUPF) == (RCC_MC_CIFR_WKUPF));
}

/**
  * @brief  Check if RCC flag Window Watchdog 1 reset is set or not.
  * @rmtoll MC_RSTSCLRR          WWDG1RSTF      LL_RCC_IsActiveFlag_WWDG1RST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_WWDG1RST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_WWDG1RSTF) == (RCC_MC_RSTSCLRR_WWDG1RSTF));
}

/**
  * @brief  Check if RCC flag Independent Watchdog 2 reset is set or not.
  * @rmtoll MC_RSTSCLRR   IWDG2RSTF      LL_RCC_IsActiveFlag_IWDG2RST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_IWDG2RST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_IWDG2RSTF) == (RCC_MC_RSTSCLRR_IWDG2RSTF));
}

/**
  * @brief  Check if RCC flag Independent Watchdog 1 reset is set or not.
  * @rmtoll MC_RSTSCLRR   IWDG1RSTF      LL_RCC_IsActiveFlag_IWDG1RST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_IWDG1RST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_IWDG1RSTF) == (RCC_MC_RSTSCLRR_IWDG1RSTF));
}

/**
  * @brief  Check if RCC flag MCU System reset is set or not.
  * @rmtoll MC_RSTSCLRR   MCSYSRSTF      LL_RCC_IsActiveFlag_MCSYSRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_MCSYSRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_MCSYSRSTF) == (RCC_MC_RSTSCLRR_MCSYSRSTF));
}

/**
  * @brief  Check if RCC flag MPU System reset is set or not.
  * @rmtoll MC_RSTSCLRR   MPSYSRSTF      LL_RCC_IsActiveFlag_MPSYSRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_MPSYSRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_MPSYSRSTF) == (RCC_MC_RSTSCLRR_MPSYSRSTF));
}

/**
  * @brief  Check if RCC flag MCU reset is set or not.
  * @rmtoll MC_RSTSCLRR   MCURSTF      LL_RCC_IsActiveFlag_MCURST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_MCURST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_MCURSTF) == (RCC_MC_RSTSCLRR_MCURSTF));
}

/**
  * @brief  Check if RCC flag VDDCORE reset is set or not.
  * @rmtoll MC_RSTSCLRR   VCORERSTF      LL_RCC_IsActiveFlag_VCORERST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_VCORERST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_VCORERSTF) == (RCC_MC_RSTSCLRR_VCORERSTF));
}

/**
  * @brief  Check if RCC flag HSE CSS reset is set or not.
  * @rmtoll MC_RSTSCLRR   HCSSRSTF      LL_RCC_IsActiveFlag_HCSSRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_HCSSRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_HCSSRSTF) == (RCC_MC_RSTSCLRR_HCSSRSTF));
}

/**
  * @brief  Check if RCC flag NRST (PAD) reset is set or not.
  * @rmtoll MC_RSTSCLRR   PADRSTF      LL_RCC_IsActiveFlag_PADRSTF
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PADRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_PADRSTF) == (RCC_MC_RSTSCLRR_PADRSTF));
}

/**
  * @brief  Check if RCC flag BOR reset is set or not.
  * @rmtoll MC_RSTSCLRR   BORRSTF      LL_RCC_IsActiveFlag_BORRSTF
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_BORRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_BORRSTF) == (RCC_MC_RSTSCLRR_BORRSTF));
}

/**
  * @brief  Check if RCC flag POR/PDR reset is set or not.
  * @rmtoll MC_RSTSCLRR   PORRSTF      LL_RCC_IsActiveFlag_PORRST
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsActiveFlag_PORRST(void)
{
  return (READ_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_PORRSTF) == (RCC_MC_RSTSCLRR_PORRSTF));
}

/**
  * @brief  Set MC_RSTSCLRR bits to clear the reset flags.
  * @rmtoll MC_RSTSCLRR      LL_RCC_MC_RSTSCLRR_ALL       LL_RCC_ClearResetFlags
  * @retval None
  */
__STATIC_INLINE void LL_RCC_ClearResetFlags(void)
{
  WRITE_REG(RCC->MC_RSTSCLRR, LL_RCC_MC_RSTSCLRR_ALL);
}

/**
  * @}
  */

/** @defgroup RCC_LL_EF_IT_Management IT Management
  * @{
  */

/**
  * @brief  Enable LSI ready interrupt
  * @rmtoll MC_CIER      LSIRDYIE      LL_RCC_EnableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSIRDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_LSIRDYIE);
}

/**
  * @brief  Enable LSE ready interrupt
  * @rmtoll MC_CIER      LSERDYIE      LL_RCC_EnableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSERDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_LSERDYIE);
}

/**
  * @brief  Enable HSI ready interrupt
  * @rmtoll MC_CIER      HSIRDYIE      LL_RCC_EnableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSIRDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_HSIRDYIE);
}

/**
  * @brief  Enable HSE ready interrupt
  * @rmtoll MC_CIER      HSERDYIE      LL_RCC_EnableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_HSERDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_HSERDYIE);
}

/**
  * @brief  Enable CSI ready interrupt
  * @rmtoll MC_CIER      CSIRDYIE      LL_RCC_EnableIT_CSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_CSIRDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_CSIRDYIE);
}

/**
  * @brief  Enable PLL1 ready interrupt
  * @rmtoll MC_CIER      PLLR1DYIE      LL_RCC_EnableIT_PLL1RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLL1RDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL1DYIE);
}

/**
  * @brief  Enable PLL2 ready interrupt
  * @rmtoll MC_CIER      PLLR2DYIE      LL_RCC_EnableIT_PLL2RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLL2RDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL2DYIE);
}

/**
  * @brief  Enable PLL3 ready interrupt
  * @rmtoll MC_CIER      PLLR3DYIE      LL_RCC_EnableIT_PLL3RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLL3RDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL3DYIE);
}

/**
  * @brief  Enable PLL4 ready interrupt
  * @rmtoll MC_CIER      PLLR3DYIE      LL_RCC_EnableIT_PLL4RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_PLL4RDY(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL4DYIE);
}

/**
  * @brief  Enable LSE clock security system interrupt
  * @rmtoll MC_CIER      LSECSSIE      LL_RCC_EnableIT_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_LSECSS(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_LSECSSIE);
}

/**
  * @brief  Enable Wake up from CStop interrupt
  * @rmtoll MC_CIER      WKUPIE      LL_RCC_EnableIT_WKUP
  * @retval None
  */
__STATIC_INLINE void LL_RCC_EnableIT_WKUP(void)
{
  SET_BIT(RCC->MC_CIER, RCC_MC_CIER_WKUPIE);
}

/**
  * @brief  Disable LSI ready interrupt
  * @rmtoll MC_CIER      LSIRDYIE      LL_RCC_DisableIT_LSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSIRDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_LSIRDYIE);
}

/**
  * @brief  Disable LSE ready interrupt
  * @rmtoll MC_CIER      LSERDYIE      LL_RCC_DisableIT_LSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSERDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_LSERDYIE);
}

/**
  * @brief  Disable HSI ready interrupt
  * @rmtoll MC_CIER      HSIRDYIE      LL_RCC_DisableIT_HSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSIRDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_HSIRDYIE);
}

/**
  * @brief  Disable HSE ready interrupt
  * @rmtoll MC_CIER      HSERDYIE      LL_RCC_DisableIT_HSERDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_HSERDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_HSERDYIE);
}

/**
  * @brief  Disable CSI ready interrupt
  * @rmtoll MC_CIER      CSIRDYIE      LL_RCC_DisableIT_CSIRDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_CSIRDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_CSIRDYIE);
}

/**
  * @brief  Disable PLL1 ready interrupt
  * @rmtoll MC_CIER      PLLR1DYIE      LL_RCC_DisableIT_PLL1RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLL1RDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL1DYIE);
}

/**
  * @brief  Disable PLL2 ready interrupt
  * @rmtoll MC_CIER      PLLR2DYIE      LL_RCC_DisableIT_PLL2RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLL2RDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL2DYIE);
}

/**
  * @brief  Disable PLL3 ready interrupt
  * @rmtoll MC_CIER      PLLR3DYIE      LL_RCC_DisableIT_PLL3RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLL3RDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL3DYIE);
}

/**
  * @brief  Disable PLL4 ready interrupt
  * @rmtoll MC_CIER      PLLR3DYIE      LL_RCC_DisableIT_PLL4RDY
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_PLL4RDY(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL4DYIE);
}

/**
  * @brief  Disable LSE clock security system interrupt
  * @rmtoll MC_CIER      LSECSSIE      LL_RCC_DisableIT_LSECSS
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_LSECSS(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_LSECSSIE);
}

/**
  * @brief  Disable Wake up from CStop interrupt
  * @rmtoll MC_CIER      WKUPIE      LL_RCC_DisableIT_WKUP
  * @retval None
  */
__STATIC_INLINE void LL_RCC_DisableIT_WKUP(void)
{
  CLEAR_BIT(RCC->MC_CIER, RCC_MC_CIER_WKUPIE);
}

/**
  * @brief  Checks if LSI ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      LSIRDYIE      LL_RCC_IsEnabledIT_LSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_LSIRDYIE) == (RCC_MC_CIER_LSIRDYIE));
}

/**
  * @brief  Checks if LSE ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      LSERDYIE      LL_RCC_IsEnabledIT_LSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSERDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_LSERDYIE) == (RCC_MC_CIER_LSERDYIE));
}

/**
  * @brief  Checks if HSI ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      HSIRDYIE      LL_RCC_IsEnabledIT_HSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_HSIRDYIE) == (RCC_MC_CIER_HSIRDYIE));
}

/**
  * @brief  Checks if HSE ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      HSERDYIE      LL_RCC_IsEnabledIT_HSERDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_HSERDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_HSERDYIE) == (RCC_MC_CIER_HSERDYIE));
}

/**
  * @brief  Checks if CSI ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      CSIRDYIE      LL_RCC_IsEnabledIT_CSIRDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_CSIRDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_CSIRDYIE) == (RCC_MC_CIER_CSIRDYIE));
}

/**
  * @brief  Checks if PLL1 ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      PLL1DYIE      LL_RCC_IsEnabledIT_PLL1RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLL1RDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL1DYIE) == (RCC_MC_CIER_PLL1DYIE));
}

/**
  * @brief  Checks if PLL2 ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      PLL2DYIE      LL_RCC_IsEnabledIT_PLL2RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLL2RDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL2DYIE) == (RCC_MC_CIER_PLL2DYIE));
}

/**
  * @brief  Checks if PLL3 ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      PLL3DYIE      LL_RCC_IsEnabledIT_PLL3RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLL3RDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL3DYIE) == (RCC_MC_CIER_PLL3DYIE));
}

/**
  * @brief  Checks if PLL4 ready interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      PLL4DYIE      LL_RCC_IsEnabledIT_PLL4RDY
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_PLL4RDY(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_PLL4DYIE) == (RCC_MC_CIER_PLL4DYIE));
}

/**
  * @brief  Checks if LSECSS interrupt source is enabled or disabled.
  * @rmtoll MC_CIER      LSECSSIE      LL_RCC_IsEnabledIT_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_LSECSS(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_LSECSSIE) == (RCC_MC_CIER_LSECSSIE));
}

/**
  * @brief  Checks if Wake up from CStop source is enabled or disabled.
  * @rmtoll MC_CIER      WKUPIE      LL_RCC_IsEnabledIT_LSECSS
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_RCC_IsEnabledIT_WKUP(void)
{
  return (READ_BIT(RCC->MC_CIER, RCC_MC_CIER_WKUPIE) == (RCC_MC_CIER_WKUPIE));
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
uint32_t    LL_RCC_CalcPLLClockFreq(uint32_t PLLInputFreq, uint32_t M, uint32_t N, uint32_t FRACV, uint32_t PQR);

void        LL_RCC_GetPLL1ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks);
void        LL_RCC_GetPLL2ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks);
void        LL_RCC_GetPLL3ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks);
void        LL_RCC_GetPLL4ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks);
void        LL_RCC_GetSystemClocksFreq(LL_RCC_ClocksTypeDef *RCC_Clocks);

uint32_t    LL_RCC_GetI2CClockFreq(uint32_t I2CxSource);
uint32_t    LL_RCC_GetSAIClockFreq(uint32_t SAIxSource);
uint32_t    LL_RCC_GetSPIClockFreq(uint32_t SPIxSource);
uint32_t    LL_RCC_GetUARTClockFreq(uint32_t UARTxSource);
uint32_t    LL_RCC_GetSDMMCClockFreq(uint32_t SDMMCxSource);
uint32_t    LL_RCC_GetETHClockFreq(uint32_t ETHxSource);
uint32_t    LL_RCC_GetQSPIClockFreq(uint32_t QSPIxSource);
uint32_t    LL_RCC_GetFMCClockFreq(uint32_t FMCxSource);
uint32_t    LL_RCC_GetFDCANClockFreq(uint32_t FDCANxSource);
uint32_t    LL_RCC_GetSPDIFRXClockFreq(uint32_t SPDIFRXxSource);
uint32_t    LL_RCC_GetCECClockFreq(uint32_t CECxSource);
uint32_t    LL_RCC_GetUSBPHYClockFreq(uint32_t USBPHYxSource);
uint32_t    LL_RCC_GetUSBOClockFreq(uint32_t USBOxSource);
uint32_t    LL_RCC_GetRNGClockFreq(uint32_t RNGxSource);
uint32_t    LL_RCC_GetCKPERClockFreq(uint32_t CKPERxSource);
uint32_t    LL_RCC_GetSTGENClockFreq(uint32_t STGENxSource);
uint32_t    LL_RCC_GetDSIClockFreq(uint32_t DSIxSource);
uint32_t    LL_RCC_GetADCClockFreq(uint32_t ADCxSource);
uint32_t    LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource);
uint32_t    LL_RCC_GetDFSDMClockFreq(uint32_t DFSDMxSource);
uint32_t    LL_RCC_GetLTDCClockFreq(void);
uint32_t    LL_RCC_GetRTCClockFreq(void);
uint32_t    LL_RCC_GetTIMGClockFreq(uint32_t TIMGxPrescaler);
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */
/* End of RCC_LL_Exported_Functions */

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

#endif /* STM32MP1xx_LL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
