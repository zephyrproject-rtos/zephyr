/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extension module.
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
#ifndef __STM32MP1xx_HAL_RCC_EX_H
#define __STM32MP1xx_HAL_RCC_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup RCCEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */
/**
  * @brief  RCC extended clocks structure definition
  */
typedef struct
{
  uint64_t PeriphClockSelection;   /*!< The Extended Clock to be configured.
                                        This parameter can be a value of @ref
                                        RCCEx_Periph_Clock_Selection */

  RCC_PLLInitTypeDef PLL2;         /*!< PLL2 structure parameters.
                                        This parameter will be used only when
                                        PLL2 is selected as Clock Source */

  RCC_PLLInitTypeDef PLL3;         /*!< PLL3 structure parameters.
                                        This parameter will be used only when
                                        PLL3 is selected as Clock Source */

  RCC_PLLInitTypeDef PLL4;         /*!< PLL4 structure parameters.
                                        This parameter will be used only when
                                        PLL3 is selected as Clock Source */

  uint32_t I2c12ClockSelection;    /*!< Specifies I2C12 clock source
                                        This parameter can be a value of
                                        @ref RCCEx_I2C12_Clock_Source */

  uint32_t I2c35ClockSelection;    /*!< Specifies I2C35 clock source
                                        This parameter can be a value of
                                        @ref RCCEx_I2C35_Clock_Source */

  uint32_t I2c46ClockSelection;    /*!< Specifies I2C46 clock source
                                        This parameter can be a value of
                                        @ref RCCEx_I2C46_Clock_Source */

  uint32_t Sai1ClockSelection;     /*!< Specifies SAI1 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SAI1_Clock_Source */

  uint32_t Sai2ClockSelection;     /*!< Specifies SAI2 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SAI2_Clock_Source */

  uint32_t Sai3ClockSelection;     /*!< Specifies SAI3 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SAI3_Clock_Source */

  uint32_t Sai4ClockSelection;     /*!< Specifies SAI4 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SAI4_Clock_Source */

  uint32_t Spi1ClockSelection;     /*!< Specifies SPI1 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SPI1_Clock_Source */

  uint32_t Spi23ClockSelection;    /*!< Specifies SPI23 clock source
                                         This parameter can be a value of @ref
                                         RCCEx_SPI23_Clock_Source */

  uint32_t Spi45ClockSelection;    /*!< Specifies SPI45 clock source
                                         This parameter can be a value of @ref
                                         RCCEx_SPI45_Clock_Source */

  uint32_t Spi6ClockSelection;     /*!< Specifies SPI6 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SPI6_Clock_Source */

  uint32_t Usart1ClockSelection;   /*!< Specifies USART1 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_USART1_Clock_Source */

  uint32_t Uart24ClockSelection;   /*!< Specifies UART24 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_UART24_Clock_Source */

  uint32_t Uart35ClockSelection;   /*!< Specifies UART35 clock source
                                         This parameter can be a value of @ref
                                         RCCEx_UART35_Clock_Source */

  uint32_t Usart6ClockSelection;   /*!< Specifies USART6 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_USART6_Clock_Source */

  uint32_t Uart78ClockSelection;   /*!< Specifies UART78 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_UART78_Clock_Source */

  uint32_t Sdmmc12ClockSelection; /*!< Specifies SDMMC12 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SDMMC12_Clock_Source */

  uint32_t Sdmmc3ClockSelection;  /*!< Specifies SDMMC3 clock source
                                        This parameter can be a value of @ref
                                         RCCEx_SDMMC3_Clock_Source */

  uint32_t EthClockSelection;     /*!< Specifies ETH clock source
                                         This parameter can be a value of @ref
                                         RCCEx_ETH_Clock_Source */

  uint32_t FmcClockSelection;      /*!< Specifies FMC clock source
                                        This parameter can be a value of @ref
                                        RCCEx_FMC_Clock_Source */

  uint32_t QspiClockSelection;     /*!< Specifies QSPI clock source
                                        This parameter can be a value of @ref
                                        RCCEx_QSPI_Clock_Source */

  uint32_t DsiClockSelection;      /*!< Specifies DSI clock source
                                        This parameter can be a value of @ref
                                        RCCEx_DSI_Clock_Source */

  uint32_t CkperClockSelection;    /*!< Specifies CKPER clock source
                                        This parameter can be a value of @ref
                                        RCCEx_CKPER_Clock_Source */

  uint32_t SpdifrxClockSelection;  /*!< Specifies SPDIFRX Clock clock source
                                        This parameter can be a value of @ref
                                        RCCEx_SPDIFRX_Clock_Source */

  uint32_t FdcanClockSelection;    /*!< Specifies FDCAN Clock clock source
                                        This parameter can be a value of @ref
                                        RCCEx_FDCAN_Clock_Source */

  uint32_t Rng1ClockSelection;     /*!< Specifies RNG1 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_RNG1_Clock_Source */

  uint32_t Rng2ClockSelection;     /*!< Specifies RNG2 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_RNG2_Clock_Source */

  uint32_t StgenClockSelection;    /*!< Specifies STGEN clock source
                                        This parameter can be a value of @ref
                                        RCCEx_STGEN_Clock_Source */

  uint32_t UsbphyClockSelection;   /*!< Specifies USB PHY clock source
                                        This parameter can be a value of @ref
                                        RCCEx_USBPHY_Clock_Source */

  uint32_t UsboClockSelection;     /*!< Specifies USB OTG clock source
                                        This parameter can be a value of @ref
                                        RCCEx_USBO_Clock_Source */

  uint32_t CecClockSelection;      /*!< Specifies CEC clock source
                                        This parameter can be a value of @ref
                                        RCCEx_CEC_Clock_Source */

  uint32_t Lptim1ClockSelection;   /*!< Specifies LPTIM1 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_LPTIM1_Clock_Source */

  uint32_t Lptim23ClockSelection;  /*!< Specifies LPTIM23 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_LPTIM23_Clock_Source */

  uint32_t Lptim45ClockSelection;  /*!< Specifies LPTIM45 clock source
                                        This parameter can be a value of @ref
                                        RCCEx_LPTIM45_Clock_Source */

  uint32_t AdcClockSelection;      /*!< Specifies ADC interface clock source
                                        This parameter can be a value of @ref
                                        RCCEx_ADC_Clock_Source */

  uint32_t RTCClockSelection;      /*!< Specifies RTC clock source
                                        This parameter can be a value of @ref
                                        RCC_RTC_Clock_Source */

  uint32_t TIMG1PresSelection;     /*!< Specifies TIM Group 1 Clock Prescalers
                                        Selection.
                                        This parameter can be a value of @ref
                                        RCCEx_TIMG1_Prescaler_Selection */

  uint32_t TIMG2PresSelection;     /*!< Specifies TIM Group 2 Clock Prescalers
                                        Selection.
                                        This parameter can be a value of @ref
                                        RCCEx_TIMG2_Prescaler_Selection */
} RCC_PeriphCLKInitTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCCEx_Periph_Clock_Selection RCCEx_Periph_Clock_Selection
  * @{
  */
#define RCC_PERIPHCLK_USART1            ((uint64_t)0x00000001)
#define RCC_PERIPHCLK_UART24            ((uint64_t)0x00000002)
#define RCC_PERIPHCLK_UART35            ((uint64_t)0x00000004)
#define RCC_PERIPHCLK_ADC               ((uint64_t)0x00000008)
#define RCC_PERIPHCLK_I2C12             ((uint64_t)0x00000010)
#define RCC_PERIPHCLK_I2C35             ((uint64_t)0x00000020)
#define RCC_PERIPHCLK_LPTIM1            ((uint64_t)0x00000040)
#define RCC_PERIPHCLK_SAI1              ((uint64_t)0x00000080)
#define RCC_PERIPHCLK_SAI2              ((uint64_t)0x00000100)
#define RCC_PERIPHCLK_USBPHY            ((uint64_t)0x00000200)
#define RCC_PERIPHCLK_TIMG1             ((uint64_t)0x00000400)
#define RCC_PERIPHCLK_TIMG2             ((uint64_t)0x00000800)
#define RCC_PERIPHCLK_RTC               ((uint64_t)0x00001000)
#define RCC_PERIPHCLK_CEC               ((uint64_t)0x00002000)
#define RCC_PERIPHCLK_USART6            ((uint64_t)0x00004000)
#define RCC_PERIPHCLK_UART78            ((uint64_t)0x00008000)
#define RCC_PERIPHCLK_LPTIM23           ((uint64_t)0x00010000)
#define RCC_PERIPHCLK_LPTIM45           ((uint64_t)0x00020000)
#define RCC_PERIPHCLK_SAI3              ((uint64_t)0x00040000)
#define RCC_PERIPHCLK_USBO              ((uint64_t)0x00080000)
#define RCC_PERIPHCLK_FMC               ((uint64_t)0x00100000)
#define RCC_PERIPHCLK_QSPI              ((uint64_t)0x00200000)
#define RCC_PERIPHCLK_DSI               ((uint64_t)0x00400000)
#define RCC_PERIPHCLK_CKPER             ((uint64_t)0x00800000)
#define RCC_PERIPHCLK_SPDIFRX           ((uint64_t)0x01000000)
#define RCC_PERIPHCLK_FDCAN             ((uint64_t)0x02000000)
#define RCC_PERIPHCLK_SPI1              ((uint64_t)0x04000000)
#define RCC_PERIPHCLK_SPI23             ((uint64_t)0x08000000)
#define RCC_PERIPHCLK_SPI45             ((uint64_t)0x10000000)
#define RCC_PERIPHCLK_SPI6              ((uint64_t)0x20000000)
#define RCC_PERIPHCLK_SAI4              ((uint64_t)0x40000000)
#define RCC_PERIPHCLK_SDMMC12           ((uint64_t)0x80000000)
#define RCC_PERIPHCLK_SDMMC3            ((uint64_t)0x100000000)
#define RCC_PERIPHCLK_ETH               ((uint64_t)0x200000000)
#define RCC_PERIPHCLK_RNG1              ((uint64_t)0x400000000)
#define RCC_PERIPHCLK_RNG2              ((uint64_t)0x800000000)
#define RCC_PERIPHCLK_STGEN             ((uint64_t)0x1000000000)
#define RCC_PERIPHCLK_I2C46             ((uint64_t)0x2000000000)

#define IS_RCC_PERIPHCLOCK(SELECTION) \
          ((((SELECTION) & RCC_PERIPHCLK_USART1)  == RCC_PERIPHCLK_USART1)  || \
           (((SELECTION) & RCC_PERIPHCLK_UART24)  == RCC_PERIPHCLK_UART24)  || \
           (((SELECTION) & RCC_PERIPHCLK_UART35)  == RCC_PERIPHCLK_UART35)  || \
           (((SELECTION) & RCC_PERIPHCLK_I2C12)   == RCC_PERIPHCLK_I2C12)   || \
           (((SELECTION) & RCC_PERIPHCLK_I2C35)   == RCC_PERIPHCLK_I2C35)   || \
           (((SELECTION) & RCC_PERIPHCLK_LPTIM1)  == RCC_PERIPHCLK_LPTIM1)  || \
           (((SELECTION) & RCC_PERIPHCLK_SAI1)    == RCC_PERIPHCLK_SAI1)    || \
           (((SELECTION) & RCC_PERIPHCLK_SAI2)    == RCC_PERIPHCLK_SAI2)    || \
           (((SELECTION) & RCC_PERIPHCLK_USBPHY)  == RCC_PERIPHCLK_USBPHY)  || \
           (((SELECTION) & RCC_PERIPHCLK_ADC)     == RCC_PERIPHCLK_ADC)     || \
           (((SELECTION) & RCC_PERIPHCLK_RTC)     == RCC_PERIPHCLK_RTC)     || \
           (((SELECTION) & RCC_PERIPHCLK_CEC)     == RCC_PERIPHCLK_CEC)     || \
           (((SELECTION) & RCC_PERIPHCLK_USART6)  == RCC_PERIPHCLK_USART6)  || \
           (((SELECTION) & RCC_PERIPHCLK_UART78)  == RCC_PERIPHCLK_UART78)  || \
           (((SELECTION) & RCC_PERIPHCLK_I2C46)   == RCC_PERIPHCLK_I2C46)   || \
           (((SELECTION) & RCC_PERIPHCLK_LPTIM23) == RCC_PERIPHCLK_LPTIM23) || \
           (((SELECTION) & RCC_PERIPHCLK_LPTIM45) == RCC_PERIPHCLK_LPTIM45) || \
           (((SELECTION) & RCC_PERIPHCLK_SAI3)    == RCC_PERIPHCLK_SAI3)    || \
           (((SELECTION) & RCC_PERIPHCLK_FMC)     == RCC_PERIPHCLK_FMC)     || \
           (((SELECTION) & RCC_PERIPHCLK_QSPI)    == RCC_PERIPHCLK_QSPI)    || \
           (((SELECTION) & RCC_PERIPHCLK_DSI)     == RCC_PERIPHCLK_DSI)     || \
           (((SELECTION) & RCC_PERIPHCLK_CKPER)   == RCC_PERIPHCLK_CKPER)   || \
           (((SELECTION) & RCC_PERIPHCLK_SPDIFRX) == RCC_PERIPHCLK_SPDIFRX) || \
           (((SELECTION) & RCC_PERIPHCLK_FDCAN)   == RCC_PERIPHCLK_FDCAN)   || \
           (((SELECTION) & RCC_PERIPHCLK_SPI1)    == RCC_PERIPHCLK_SPI1)    || \
           (((SELECTION) & RCC_PERIPHCLK_SPI23)   == RCC_PERIPHCLK_SPI23)   || \
           (((SELECTION) & RCC_PERIPHCLK_SPI45)   == RCC_PERIPHCLK_SPI45)   || \
           (((SELECTION) & RCC_PERIPHCLK_SPI6)    == RCC_PERIPHCLK_SPI6)    || \
           (((SELECTION) & RCC_PERIPHCLK_SAI4)    == RCC_PERIPHCLK_SAI4)    || \
           (((SELECTION) & RCC_PERIPHCLK_SDMMC12) == RCC_PERIPHCLK_SDMMC12) || \
           (((SELECTION) & RCC_PERIPHCLK_SDMMC3)  == RCC_PERIPHCLK_SDMMC3)  || \
           (((SELECTION) & RCC_PERIPHCLK_ETH)     == RCC_PERIPHCLK_ETH)     || \
           (((SELECTION) & RCC_PERIPHCLK_RNG1)    == RCC_PERIPHCLK_RNG1)    || \
           (((SELECTION) & RCC_PERIPHCLK_RNG2)    == RCC_PERIPHCLK_RNG2)    || \
           (((SELECTION) & RCC_PERIPHCLK_USBO)    == RCC_PERIPHCLK_USBO)    || \
           (((SELECTION) & RCC_PERIPHCLK_STGEN)   == RCC_PERIPHCLK_STGEN)   || \
           (((SELECTION) & RCC_PERIPHCLK_TIMG1)   == RCC_PERIPHCLK_TIMG1)   || \
           (((SELECTION) & RCC_PERIPHCLK_TIMG2)   == RCC_PERIPHCLK_TIMG2))
/**
  * @}
  */

/** @defgroup RCCEx_Periph_One_Clock RCCEx_Periph_One_Clock
  * @{
  */
#define RCC_PERIPHCLK_DAC               ((uint64_t)0x4000000000)
#define RCC_PERIPHCLK_LTDC              ((uint64_t)0x8000000000)
#define RCC_PERIPHCLK_DFSDM1            ((uint64_t)0x10000000000)
#define RCC_PERIPHCLK_TEMP              ((uint64_t)0x20000000000)
#define RCC_PERIPHCLK_IWDG1             ((uint64_t)0x40000000000)
#define RCC_PERIPHCLK_DDRPHYC           ((uint64_t)0x80000000000)
#define RCC_PERIPHCLK_IWDG2             ((uint64_t)0x100000000000)
#define RCC_PERIPHCLK_GPU               ((uint64_t)0x200000000000)
#define RCC_PERIPHCLK_WWDG              ((uint64_t)0x400000000000)
#define RCC_PERIPHCLK_TIM2              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM3              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM4              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM5              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM6              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM7              RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM12             RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM13             RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM14             RCC_PERIPHCLK_TIMG1
#define RCC_PERIPHCLK_TIM1              RCC_PERIPHCLK_TIMG2
#define RCC_PERIPHCLK_TIM8              RCC_PERIPHCLK_TIMG2
#define RCC_PERIPHCLK_TIM15             RCC_PERIPHCLK_TIMG2
#define RCC_PERIPHCLK_TIM16             RCC_PERIPHCLK_TIMG2
#define RCC_PERIPHCLK_TIM17             RCC_PERIPHCLK_TIMG2

#define IS_RCC_PERIPHONECLOCK(PERIPH) \
          ((((PERIPH) & RCC_PERIPHCLK_DAC)      == RCC_PERIPHCLK_DAC)     || \
           (((PERIPH) & RCC_PERIPHCLK_LTDC)     == RCC_PERIPHCLK_LTDC)    || \
           (((PERIPH) & RCC_PERIPHCLK_DFSDM1)   == RCC_PERIPHCLK_DFSDM1)  || \
           (((PERIPH) & RCC_PERIPHCLK_TEMP)     == RCC_PERIPHCLK_TEMP)    || \
           (((PERIPH) & RCC_PERIPHCLK_IWDG1)    == RCC_PERIPHCLK_IWDG1)   || \
           (((PERIPH) & RCC_PERIPHCLK_IWDG2)    == RCC_PERIPHCLK_IWDG2)   || \
           (((PERIPH) & RCC_PERIPHCLK_WWDG)     == RCC_PERIPHCLK_WWDG)    || \
           (((PERIPH) & RCC_PERIPHCLK_DDRPHYC)  == RCC_PERIPHCLK_DDRPHYC) || \
           (((PERIPH) & RCC_PERIPHCLK_GPU)      == RCC_PERIPHCLK_GPU)     || \
           (((PERIPH) & RCC_PERIPHCLK_TIMG1)    == RCC_PERIPHCLK_TIMG1)   || \
           (((PERIPH) & RCC_PERIPHCLK_TIMG2)    == RCC_PERIPHCLK_TIMG2))
/**
  * @}
  */

/** @defgroup RCCEx_I2C12_Clock_Source  I2C12 Clock Source
  * @{
  */
#define RCC_I2C12CLKSOURCE_BCLK         RCC_I2C12CKSELR_I2C12SRC_0
#define RCC_I2C12CLKSOURCE_PLL4         RCC_I2C12CKSELR_I2C12SRC_1
#define RCC_I2C12CLKSOURCE_HSI          RCC_I2C12CKSELR_I2C12SRC_2
#define RCC_I2C12CLKSOURCE_CSI          RCC_I2C12CKSELR_I2C12SRC_3

#define IS_RCC_I2C12CLKSOURCE(SOURCE) \
                              (((SOURCE) == RCC_I2C12CLKSOURCE_BCLK)   || \
                               ((SOURCE) == RCC_I2C12CLKSOURCE_PLL4)   || \
                               ((SOURCE) == RCC_I2C12CLKSOURCE_HSI)    || \
                               ((SOURCE) == RCC_I2C12CLKSOURCE_CSI))
/**
  * @}
  */

/** @defgroup RCCEx_I2C35_Clock_Source I2C35 Clock Source
  * @{
  */
#define RCC_I2C35CLKSOURCE_BCLK         RCC_I2C35CKSELR_I2C35SRC_0
#define RCC_I2C35CLKSOURCE_PLL4         RCC_I2C35CKSELR_I2C35SRC_1
#define RCC_I2C35CLKSOURCE_HSI          RCC_I2C35CKSELR_I2C35SRC_2
#define RCC_I2C35CLKSOURCE_CSI          RCC_I2C35CKSELR_I2C35SRC_3

#define IS_RCC_I2C35CLKSOURCE(SOURCE) \
                              (((SOURCE) == RCC_I2C35CLKSOURCE_BCLK)  || \
                               ((SOURCE) == RCC_I2C35CLKSOURCE_PLL4)  || \
                               ((SOURCE) == RCC_I2C35CLKSOURCE_HSI)   || \
                               ((SOURCE) == RCC_I2C35CLKSOURCE_CSI))
/**
  * @}
  */


/** @defgroup RCCEx_I2C46_Clock_Source I2C46 Clock Source
  * @{
  */
#define RCC_I2C46CLKSOURCE_BCLK         RCC_I2C46CKSELR_I2C46SRC_0
#define RCC_I2C46CLKSOURCE_PLL3         RCC_I2C46CKSELR_I2C46SRC_1
#define RCC_I2C46CLKSOURCE_HSI          RCC_I2C46CKSELR_I2C46SRC_2
#define RCC_I2C46CLKSOURCE_CSI          RCC_I2C46CKSELR_I2C46SRC_3

#define IS_RCC_I2C46CLKSOURCE(SOURCE) \
                              (((SOURCE) == RCC_I2C46CLKSOURCE_BCLK )  || \
                               ((SOURCE) == RCC_I2C46CLKSOURCE_PLL3)   || \
                               ((SOURCE) == RCC_I2C46CLKSOURCE_HSI)    || \
                               ((SOURCE) == RCC_I2C46CLKSOURCE_CSI))
/**
  * @}
  */

/** @defgroup RCCEx_SAI1_Clock_Source SAI1 Clock Source
  * @{
  */
#define RCC_SAI1CLKSOURCE_PLL4         RCC_SAI1CKSELR_SAI1SRC_0
#define RCC_SAI1CLKSOURCE_PLL3_Q       RCC_SAI1CKSELR_SAI1SRC_1
#define RCC_SAI1CLKSOURCE_I2SCKIN      RCC_SAI1CKSELR_SAI1SRC_2
#define RCC_SAI1CLKSOURCE_PER          RCC_SAI1CKSELR_SAI1SRC_3
#define RCC_SAI1CLKSOURCE_PLL3_R       RCC_SAI1CKSELR_SAI1SRC_4

#define IS_RCC_SAI1CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SAI1CLKSOURCE_PLL4)    || \
                             ((__SOURCE__) == RCC_SAI1CLKSOURCE_PLL3_Q)   || \
                             ((__SOURCE__) == RCC_SAI1CLKSOURCE_I2SCKIN)  || \
                             ((__SOURCE__) == RCC_SAI1CLKSOURCE_PER)      || \
                             ((__SOURCE__) == RCC_SAI1CLKSOURCE_PLL3_R))
/**
  * @}
  */


/** @defgroup RCCEx_SAI2_Clock_Source SAI2 Clock Source
  * @{
  */
#define RCC_SAI2CLKSOURCE_PLL4          RCC_SAI2CKSELR_SAI2SRC_0
#define RCC_SAI2CLKSOURCE_PLL3_Q        RCC_SAI2CKSELR_SAI2SRC_1
#define RCC_SAI2CLKSOURCE_I2SCKIN       RCC_SAI2CKSELR_SAI2SRC_2
#define RCC_SAI2CLKSOURCE_PER           RCC_SAI2CKSELR_SAI2SRC_3
#define RCC_SAI2CLKSOURCE_SPDIF         RCC_SAI2CKSELR_SAI2SRC_4
#define RCC_SAI2CLKSOURCE_PLL3_R        RCC_SAI2CKSELR_SAI2SRC_5


#define IS_RCC_SAI2CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SAI2CLKSOURCE_PLL4)    || \
                              ((__SOURCE__) == RCC_SAI2CLKSOURCE_PLL3_Q)  || \
                              ((__SOURCE__) == RCC_SAI2CLKSOURCE_I2SCKIN) || \
                              ((__SOURCE__) == RCC_SAI2CLKSOURCE_PER)     || \
                              ((__SOURCE__) == RCC_SAI2CLKSOURCE_SPDIF)   || \
                              ((__SOURCE__) == RCC_SAI2CLKSOURCE_PLL3_R))
/**
  * @}
  */

/** @defgroup RCCEx_SAI3_Clock_Source SAI3 Clock Source
  * @{
  */
#define RCC_SAI3CLKSOURCE_PLL4          RCC_SAI3CKSELR_SAI3SRC_0
#define RCC_SAI3CLKSOURCE_PLL3_Q        RCC_SAI3CKSELR_SAI3SRC_1
#define RCC_SAI3CLKSOURCE_I2SCKIN       RCC_SAI3CKSELR_SAI3SRC_2
#define RCC_SAI3CLKSOURCE_PER           RCC_SAI3CKSELR_SAI3SRC_3
#define RCC_SAI3CLKSOURCE_PLL3_R        RCC_SAI3CKSELR_SAI3SRC_4

#define IS_RCC_SAI3CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SAI3CLKSOURCE_PLL4)    || \
                              ((__SOURCE__) == RCC_SAI3CLKSOURCE_PLL3_Q)  || \
                              ((__SOURCE__) == RCC_SAI3CLKSOURCE_I2SCKIN) || \
                              ((__SOURCE__) == RCC_SAI3CLKSOURCE_PER)     || \
                              ((__SOURCE__) == RCC_SAI3CLKSOURCE_PLL3_R))
/**
  * @}
  */


/** @defgroup RCCEx_SAI4_Clock_Source SAI4 Clock Source
  * @{
  */
#define RCC_SAI4CLKSOURCE_PLL4          RCC_SAI4CKSELR_SAI4SRC_0
#define RCC_SAI4CLKSOURCE_PLL3_Q        RCC_SAI4CKSELR_SAI4SRC_1
#define RCC_SAI4CLKSOURCE_I2SCKIN       RCC_SAI4CKSELR_SAI4SRC_2
#define RCC_SAI4CLKSOURCE_PER           RCC_SAI4CKSELR_SAI4SRC_3
#define RCC_SAI4CLKSOURCE_PLL3_R        RCC_SAI4CKSELR_SAI4SRC_4

#define IS_RCC_SAI4CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SAI4CLKSOURCE_PLL4)    || \
                             ((__SOURCE__) == RCC_SAI4CLKSOURCE_PLL3_Q)   || \
                             ((__SOURCE__) == RCC_SAI4CLKSOURCE_I2SCKIN)  || \
                             ((__SOURCE__) == RCC_SAI4CLKSOURCE_PER)      || \
                             ((__SOURCE__) == RCC_SAI4CLKSOURCE_PLL3_R))
/**
  * @}
  */


/** @defgroup RCCEx_SPI1_Clock_Source SPI/I2S1 Clock Source
  * @{
  */
#define RCC_SPI1CLKSOURCE_PLL4        RCC_SPI2S1CKSELR_SPI1SRC_0
#define RCC_SPI1CLKSOURCE_PLL3_Q      RCC_SPI2S1CKSELR_SPI1SRC_1
#define RCC_SPI1CLKSOURCE_I2SCKIN     RCC_SPI2S1CKSELR_SPI1SRC_2
#define RCC_SPI1CLKSOURCE_PER         RCC_SPI2S1CKSELR_SPI1SRC_3
#define RCC_SPI1CLKSOURCE_PLL3_R      RCC_SPI2S1CKSELR_SPI1SRC_4

#define IS_RCC_SPI1CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SPI1CLKSOURCE_PLL4)    || \
                              ((__SOURCE__) == RCC_SPI1CLKSOURCE_PLL3_Q)  || \
                              ((__SOURCE__) == RCC_SPI1CLKSOURCE_I2SCKIN) || \
                              ((__SOURCE__) == RCC_SPI1CLKSOURCE_PER)     || \
                              ((__SOURCE__) == RCC_SPI1CLKSOURCE_PLL3_R))
/**
  * @}
  */

/** @defgroup RCCEx_SPI23_Clock_Source SPI/I2S2,3 Clock Source
  * @{
  */
#define RCC_SPI23CLKSOURCE_PLL4       RCC_SPI2S23CKSELR_SPI23SRC_0
#define RCC_SPI23CLKSOURCE_PLL3_Q     RCC_SPI2S23CKSELR_SPI23SRC_1
#define RCC_SPI23CLKSOURCE_I2SCKIN    RCC_SPI2S23CKSELR_SPI23SRC_2
#define RCC_SPI23CLKSOURCE_PER        RCC_SPI2S23CKSELR_SPI23SRC_3
#define RCC_SPI23CLKSOURCE_PLL3_R     RCC_SPI2S23CKSELR_SPI23SRC_4

#define IS_RCC_SPI23CLKSOURCE(__SOURCE__) \
                              (((__SOURCE__) == RCC_SPI23CLKSOURCE_PLL4)    || \
                               ((__SOURCE__) == RCC_SPI23CLKSOURCE_PLL3_Q)  || \
                               ((__SOURCE__) == RCC_SPI23CLKSOURCE_I2SCKIN) || \
                               ((__SOURCE__) == RCC_SPI23CLKSOURCE_PER)     || \
                               ((__SOURCE__) == RCC_SPI23CLKSOURCE_PLL3_R))
/**
  * @}
  */

/** @defgroup RCCEx_SPI45_Clock_Source SPI45 Clock Source
  * @{
  */
#define RCC_SPI45CLKSOURCE_BCLK         RCC_SPI45CKSELR_SPI45SRC_0
#define RCC_SPI45CLKSOURCE_PLL4         RCC_SPI45CKSELR_SPI45SRC_1
#define RCC_SPI45CLKSOURCE_HSI          RCC_SPI45CKSELR_SPI45SRC_2
#define RCC_SPI45CLKSOURCE_CSI          RCC_SPI45CKSELR_SPI45SRC_3
#define RCC_SPI45CLKSOURCE_HSE          RCC_SPI45CKSELR_SPI45SRC_4

#define IS_RCC_SPI45CLKSOURCE(__SOURCE__) \
                              (((__SOURCE__) == RCC_SPI45CLKSOURCE_BCLK) || \
                               ((__SOURCE__) == RCC_SPI45CLKSOURCE_PLL4) || \
                               ((__SOURCE__) == RCC_SPI45CLKSOURCE_HSI)  || \
                               ((__SOURCE__) == RCC_SPI45CLKSOURCE_CSI)  || \
                               ((__SOURCE__) == RCC_SPI45CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_SPI6_Clock_Source SPI6 Clock Source
  * @{
  */
#define RCC_SPI6CLKSOURCE_BCLK          RCC_SPI6CKSELR_SPI6SRC_0
#define RCC_SPI6CLKSOURCE_PLL4          RCC_SPI6CKSELR_SPI6SRC_1
#define RCC_SPI6CLKSOURCE_HSI           RCC_SPI6CKSELR_SPI6SRC_2
#define RCC_SPI6CLKSOURCE_CSI           RCC_SPI6CKSELR_SPI6SRC_3
#define RCC_SPI6CLKSOURCE_HSE           RCC_SPI6CKSELR_SPI6SRC_4
#define RCC_SPI6CLKSOURCE_PLL3          RCC_SPI6CKSELR_SPI6SRC_5

#define IS_RCC_SPI6CLKSOURCE(__SOURCE__) \
                             (((__SOURCE__) == RCC_SPI6CLKSOURCE_BCLK)  || \
                              ((__SOURCE__) == RCC_SPI6CLKSOURCE_PLL4)  || \
                              ((__SOURCE__) == RCC_SPI6CLKSOURCE_HSI)   || \
                              ((__SOURCE__) == RCC_SPI6CLKSOURCE_CSI)   || \
                              ((__SOURCE__) == RCC_SPI6CLKSOURCE_HSE)   || \
                              ((__SOURCE__) == RCC_SPI6CLKSOURCE_PLL3))
/**
  * @}
  */


/** @defgroup RCCEx_USART1_Clock_Source USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_BCLK        RCC_UART1CKSELR_UART1SRC_0
#define RCC_USART1CLKSOURCE_PLL3        RCC_UART1CKSELR_UART1SRC_1
#define RCC_USART1CLKSOURCE_HSI         RCC_UART1CKSELR_UART1SRC_2
#define RCC_USART1CLKSOURCE_CSI         RCC_UART1CKSELR_UART1SRC_3
#define RCC_USART1CLKSOURCE_PLL4        RCC_UART1CKSELR_UART1SRC_4
#define RCC_USART1CLKSOURCE_HSE         RCC_UART1CKSELR_UART1SRC_5

#define IS_RCC_USART1CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_USART1CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_USART1CLKSOURCE_PLL3)  || \
                                ((SOURCE) == RCC_USART1CLKSOURCE_HSI)   || \
                                ((SOURCE) == RCC_USART1CLKSOURCE_CSI)   || \
                                ((SOURCE) == RCC_USART1CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_USART1CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_UART24_Clock_Source UART24 Clock Source
  * @{
  */
#define RCC_UART24CLKSOURCE_BCLK         RCC_UART24CKSELR_UART24SRC_0
#define RCC_UART24CLKSOURCE_PLL4         RCC_UART24CKSELR_UART24SRC_1
#define RCC_UART24CLKSOURCE_HSI          RCC_UART24CKSELR_UART24SRC_2
#define RCC_UART24CLKSOURCE_CSI          RCC_UART24CKSELR_UART24SRC_3
#define RCC_UART24CLKSOURCE_HSE          RCC_UART24CKSELR_UART24SRC_4

#define IS_RCC_UART24CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_UART24CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_UART24CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_UART24CLKSOURCE_HSI)   || \
                                ((SOURCE) == RCC_UART24CLKSOURCE_CSI)   || \
                                ((SOURCE) == RCC_UART24CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_UART35_Clock_Source UART35 Clock Source
  * @{
  */
#define RCC_UART35CLKSOURCE_BCLK         RCC_UART35CKSELR_UART35SRC_0
#define RCC_UART35CLKSOURCE_PLL4         RCC_UART35CKSELR_UART35SRC_1
#define RCC_UART35CLKSOURCE_HSI          RCC_UART35CKSELR_UART35SRC_2
#define RCC_UART35CLKSOURCE_CSI          RCC_UART35CKSELR_UART35SRC_3
#define RCC_UART35CLKSOURCE_HSE          RCC_UART35CKSELR_UART35SRC_4

#define IS_RCC_UART35CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_UART35CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_UART35CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_UART35CLKSOURCE_HSI)   || \
                                ((SOURCE) == RCC_UART35CLKSOURCE_CSI)   || \
                                ((SOURCE) == RCC_UART35CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_USART6_Clock_Source USART6 Clock Source
  * @{
  */
#define RCC_USART6CLKSOURCE_BCLK        RCC_UART6CKSELR_UART6SRC_0
#define RCC_USART6CLKSOURCE_PLL4        RCC_UART6CKSELR_UART6SRC_1
#define RCC_USART6CLKSOURCE_HSI         RCC_UART6CKSELR_UART6SRC_2
#define RCC_USART6CLKSOURCE_CSI         RCC_UART6CKSELR_UART6SRC_3
#define RCC_USART6CLKSOURCE_HSE         RCC_UART6CKSELR_UART6SRC_4

#define IS_RCC_USART6CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_USART6CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_USART6CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_USART6CLKSOURCE_HSI)   || \
                                ((SOURCE) == RCC_USART6CLKSOURCE_CSI)   || \
                                ((SOURCE) == RCC_USART6CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_UART78_Clock_Source UART78 Clock Source
  * @{
  */
#define RCC_UART78CLKSOURCE_BCLK         RCC_UART78CKSELR_UART78SRC_0
#define RCC_UART78CLKSOURCE_PLL4         RCC_UART78CKSELR_UART78SRC_1
#define RCC_UART78CLKSOURCE_HSI          RCC_UART78CKSELR_UART78SRC_2
#define RCC_UART78CLKSOURCE_CSI          RCC_UART78CKSELR_UART78SRC_3
#define RCC_UART78CLKSOURCE_HSE          RCC_UART78CKSELR_UART78SRC_4

#define IS_RCC_UART78CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_UART78CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_UART78CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_UART78CLKSOURCE_HSI)   || \
                                ((SOURCE) == RCC_UART78CLKSOURCE_CSI)   || \
                                ((SOURCE) == RCC_UART78CLKSOURCE_HSE))
/**
  * @}
  */

/** @defgroup RCCEx_SDMMC12_Clock_Source SDMMC12 Clock Source
  * @{
  */
#define RCC_SDMMC12CLKSOURCE_BCLK        RCC_SDMMC12CKSELR_SDMMC12SRC_0
#define RCC_SDMMC12CLKSOURCE_PLL3        RCC_SDMMC12CKSELR_SDMMC12SRC_1
#define RCC_SDMMC12CLKSOURCE_PLL4        RCC_SDMMC12CKSELR_SDMMC12SRC_2
#define RCC_SDMMC12CLKSOURCE_HSI         RCC_SDMMC12CKSELR_SDMMC12SRC_3

#define IS_RCC_SDMMC12CLKSOURCE(SOURCE) \
                                (((SOURCE) == RCC_SDMMC12CLKSOURCE_BCLK)  || \
                                 ((SOURCE) == RCC_SDMMC12CLKSOURCE_PLL3)  || \
                                 ((SOURCE) == RCC_SDMMC12CLKSOURCE_PLL4)  || \
                                 ((SOURCE) == RCC_SDMMC12CLKSOURCE_HSI))
/**
  * @}
  */

/** @defgroup RCCEx_SDMMC3_Clock_Source SDMMC3 Clock Source
  * @{
  */
#define RCC_SDMMC3CLKSOURCE_BCLK       RCC_SDMMC3CKSELR_SDMMC3SRC_0
#define RCC_SDMMC3CLKSOURCE_PLL3       RCC_SDMMC3CKSELR_SDMMC3SRC_1
#define RCC_SDMMC3CLKSOURCE_PLL4       RCC_SDMMC3CKSELR_SDMMC3SRC_2
#define RCC_SDMMC3CLKSOURCE_HSI        RCC_SDMMC3CKSELR_SDMMC3SRC_3

#define IS_RCC_SDMMC3CLKSOURCE(SOURCE)  \
                               (((SOURCE) == RCC_SDMMC3CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_SDMMC3CLKSOURCE_PLL3)  || \
                                ((SOURCE) == RCC_SDMMC3CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_SDMMC3CLKSOURCE_HSI))
/**
  * @}
  */

/** @defgroup RCCEx_ETH_Clock_Source ETH Clock Source
  * @{
  */
#define RCC_ETHCLKSOURCE_PLL4       RCC_ETHCKSELR_ETHSRC_0
#define RCC_ETHCLKSOURCE_PLL3       RCC_ETHCKSELR_ETHSRC_1
#define RCC_ETHCLKSOURCE_OFF        RCC_ETHCKSELR_ETHSRC_2


#define IS_RCC_ETHCLKSOURCE(SOURCE) (((SOURCE) == RCC_ETHCLKSOURCE_PLL4)  || \
                                     ((SOURCE) == RCC_ETHCLKSOURCE_PLL3)  || \
                                     ((SOURCE) == RCC_ETHCLKSOURCE_OFF))
/**
  * @}
  */


/** @defgroup RCCEx_ETH_PrecisionTimeProtocol_Divider ETH PrecisionTimeProtocol Divider
  * @{
  */
#define RCC_ETHPTPDIV_1   RCC_ETHCKSELR_ETHPTPDIV_0   /*Bypass (default after reset*/
#define RCC_ETHPTPDIV_2   RCC_ETHCKSELR_ETHPTPDIV_1   /*Division by 2*/
#define RCC_ETHPTPDIV_3   RCC_ETHCKSELR_ETHPTPDIV_2   /*Division by 3*/
#define RCC_ETHPTPDIV_4   RCC_ETHCKSELR_ETHPTPDIV_3   /*Division by 4*/
#define RCC_ETHPTPDIV_5   RCC_ETHCKSELR_ETHPTPDIV_4   /*Division by 5*/
#define RCC_ETHPTPDIV_6   RCC_ETHCKSELR_ETHPTPDIV_5   /*Division by 6*/
#define RCC_ETHPTPDIV_7   RCC_ETHCKSELR_ETHPTPDIV_6   /*Division by 7*/
#define RCC_ETHPTPDIV_8   RCC_ETHCKSELR_ETHPTPDIV_7   /*Division by 8*/
#define RCC_ETHPTPDIV_9   RCC_ETHCKSELR_ETHPTPDIV_8   /*Division by 9*/
#define RCC_ETHPTPDIV_10  RCC_ETHCKSELR_ETHPTPDIV_9   /*Division by 10*/
#define RCC_ETHPTPDIV_11  RCC_ETHCKSELR_ETHPTPDIV_10  /*Division by 11*/
#define RCC_ETHPTPDIV_12  RCC_ETHCKSELR_ETHPTPDIV_11  /*Division by 12*/
#define RCC_ETHPTPDIV_13  RCC_ETHCKSELR_ETHPTPDIV_12  /*Division by 13*/
#define RCC_ETHPTPDIV_14  RCC_ETHCKSELR_ETHPTPDIV_13  /*Division by 14*/
#define RCC_ETHPTPDIV_15  RCC_ETHCKSELR_ETHPTPDIV_14  /*Division by 15*/
#define RCC_ETHPTPDIV_16  RCC_ETHCKSELR_ETHPTPDIV_15  /*Division by 16*/


#define IS_RCC_ETHPTPDIV(SOURCE)        (((SOURCE) == RCC_ETHPTPDIV_1)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_2)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_3)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_4)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_5)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_6)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_7)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_8)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_9)  || \
                                         ((SOURCE) == RCC_ETHPTPDIV_10) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_11) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_12) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_13) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_14) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_15) || \
                                         ((SOURCE) == RCC_ETHPTPDIV_16))
/**
  * @}
  */


/** @defgroup RCCEx_QSPI_Clock_Source QSPI Clock Source
  * @{
  */
#define RCC_QSPICLKSOURCE_BCLK  RCC_QSPICKSELR_QSPISRC_0
#define RCC_QSPICLKSOURCE_PLL3  RCC_QSPICKSELR_QSPISRC_1
#define RCC_QSPICLKSOURCE_PLL4  RCC_QSPICKSELR_QSPISRC_2
#define RCC_QSPICLKSOURCE_PER   RCC_QSPICKSELR_QSPISRC_3

#define IS_RCC_QSPICLKSOURCE(SOURCE) \
                             (((SOURCE) == RCC_QSPICLKSOURCE_BCLK) || \
                              ((SOURCE) == RCC_QSPICLKSOURCE_PLL3) || \
                              ((SOURCE) == RCC_QSPICLKSOURCE_PLL4) || \
                              ((SOURCE) == RCC_QSPICLKSOURCE_PER))
/**
  * @}
  */

/** @defgroup RCCEx_FMC_Clock_Source FMC Clock Source
  * @{
  */
#define RCC_FMCCLKSOURCE_BCLK       RCC_FMCCKSELR_FMCSRC_0
#define RCC_FMCCLKSOURCE_PLL3       RCC_FMCCKSELR_FMCSRC_1
#define RCC_FMCCLKSOURCE_PLL4       RCC_FMCCKSELR_FMCSRC_2
#define RCC_FMCCLKSOURCE_PER        RCC_FMCCKSELR_FMCSRC_3

#define IS_RCC_FMCCLKSOURCE(SOURCE) (((SOURCE) == RCC_FMCCLKSOURCE_BCLK)  || \
                                     ((SOURCE) == RCC_FMCCLKSOURCE_PLL3)  || \
                                     ((SOURCE) == RCC_FMCCLKSOURCE_PLL4)  || \
                                     ((SOURCE) == RCC_FMCCLKSOURCE_PER))
/**
  * @}
  */

#if defined(FDCAN1)
/** @defgroup RCCEx_FDCAN_Clock_Source FDCAN Clock Source
  * @{
  */
#define RCC_FDCANCLKSOURCE_HSE          RCC_FDCANCKSELR_FDCANSRC_0
#define RCC_FDCANCLKSOURCE_PLL3         RCC_FDCANCKSELR_FDCANSRC_1
#define RCC_FDCANCLKSOURCE_PLL4_Q       RCC_FDCANCKSELR_FDCANSRC_2
#define RCC_FDCANCLKSOURCE_PLL4_R       RCC_FDCANCKSELR_FDCANSRC_3



#define IS_RCC_FDCANCLKSOURCE(SOURCE) \
                              (((SOURCE) == RCC_FDCANCLKSOURCE_HSE)     || \
                               ((SOURCE) == RCC_FDCANCLKSOURCE_PLL3)    || \
                               ((SOURCE) == RCC_FDCANCLKSOURCE_PLL4_Q)  || \
                               ((SOURCE) == RCC_FDCANCLKSOURCE_PLL4_R))
/**
  * @}
  */
#endif /*FDCAN1*/

/** @defgroup RCCEx_SPDIFRX_Clock_Source SPDIFRX Clock Source
  * @{
  */
#define RCC_SPDIFRXCLKSOURCE_PLL4         RCC_SPDIFCKSELR_SPDIFSRC_0
#define RCC_SPDIFRXCLKSOURCE_PLL3         RCC_SPDIFCKSELR_SPDIFSRC_1
#define RCC_SPDIFRXCLKSOURCE_HSI          RCC_SPDIFCKSELR_SPDIFSRC_2

#define IS_RCC_SPDIFRXCLKSOURCE(SOURCE) \
                                (((SOURCE) == RCC_SPDIFRXCLKSOURCE_PLL4)  || \
                                 ((SOURCE) == RCC_SPDIFRXCLKSOURCE_PLL3)  || \
                                 ((SOURCE) == RCC_SPDIFRXCLKSOURCE_HSI))
/**
  * @}
  */

/** @defgroup RCCEx_CEC_Clock_Source CEC Clock Source
  * @{
  */
#define RCC_CECCLKSOURCE_LSE            RCC_CECCKSELR_CECSRC_0
#define RCC_CECCLKSOURCE_LSI            RCC_CECCKSELR_CECSRC_1
#define RCC_CECCLKSOURCE_CSI122         RCC_CECCKSELR_CECSRC_2

#define IS_RCC_CECCLKSOURCE(SOURCE)     (((SOURCE) == RCC_CECCLKSOURCE_LSE) || \
                                         ((SOURCE) == RCC_CECCLKSOURCE_LSI) || \
                                         ((SOURCE) == RCC_CECCLKSOURCE_CSI122))
/**
  * @}
  */

/** @defgroup RCCEx_USBPHY_Clock_Source USBPHY Clock Source
  * @{
  */
#define RCC_USBPHYCLKSOURCE_HSE         RCC_USBCKSELR_USBPHYSRC_0
#define RCC_USBPHYCLKSOURCE_PLL4        RCC_USBCKSELR_USBPHYSRC_1
#define RCC_USBPHYCLKSOURCE_HSE2        RCC_USBCKSELR_USBPHYSRC_2

#define IS_RCC_USBPHYCLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_USBPHYCLKSOURCE_HSE) || \
                                ((SOURCE) ==RCC_USBPHYCLKSOURCE_PLL4) || \
                                ((SOURCE) ==RCC_USBPHYCLKSOURCE_HSE2))
/**
  * @}
  */

/** @defgroup RCCEx_USBO_Clock_Source USBO Clock Source
  * @{
  */
#define RCC_USBOCLKSOURCE_PLL4            RCC_USBCKSELR_USBOSRC_0
#define RCC_USBOCLKSOURCE_PHY             RCC_USBCKSELR_USBOSRC_1

#define IS_RCC_USBOCLKSOURCE(SOURCE)  (((SOURCE) == RCC_USBOCLKSOURCE_PLL4) || \
                                       ((SOURCE) == RCC_USBOCLKSOURCE_PHY))
/**
  * @}
  */


/** @defgroup RCCEx_RNG1_Clock_Source RNG1 Clock Source
  * @{
  */
#define RCC_RNG1CLKSOURCE_CSI         RCC_RNG1CKSELR_RNG1SRC_0
#define RCC_RNG1CLKSOURCE_PLL4        RCC_RNG1CKSELR_RNG1SRC_1
#define RCC_RNG1CLKSOURCE_LSE         RCC_RNG1CKSELR_RNG1SRC_2
#define RCC_RNG1CLKSOURCE_LSI         RCC_RNG1CKSELR_RNG1SRC_3

#define IS_RCC_RNG1CLKSOURCE(SOURCE)  (((SOURCE) == RCC_RNG1CLKSOURCE_CSI)  || \
                                       ((SOURCE) == RCC_RNG1CLKSOURCE_PLL4) || \
                                       ((SOURCE) == RCC_RNG1CLKSOURCE_LSE)  || \
                                       ((SOURCE) == RCC_RNG1CLKSOURCE_LSI))

/**
  * @}
  */


/** @defgroup RCCEx_RNG2_Clock_Source RNG2 Clock Source
  * @{
  */
#define RCC_RNG2CLKSOURCE_CSI         RCC_RNG2CKSELR_RNG2SRC_0
#define RCC_RNG2CLKSOURCE_PLL4        RCC_RNG2CKSELR_RNG2SRC_1
#define RCC_RNG2CLKSOURCE_LSE         RCC_RNG2CKSELR_RNG2SRC_2
#define RCC_RNG2CLKSOURCE_LSI         RCC_RNG2CKSELR_RNG2SRC_3

#define IS_RCC_RNG2CLKSOURCE(SOURCE)  (((SOURCE) == RCC_RNG2CLKSOURCE_CSI)  || \
                                       ((SOURCE) == RCC_RNG2CLKSOURCE_PLL4) || \
                                       ((SOURCE) == RCC_RNG2CLKSOURCE_LSE)  || \
                                       ((SOURCE) == RCC_RNG2CLKSOURCE_LSI))

/**
  * @}
  */


/** @defgroup RCCEx_CKPER_Clock_Source CKPER Clock Source
  * @{
  */
#define RCC_CKPERCLKSOURCE_HSI          RCC_CPERCKSELR_CKPERSRC_0
#define RCC_CKPERCLKSOURCE_CSI          RCC_CPERCKSELR_CKPERSRC_1
#define RCC_CKPERCLKSOURCE_HSE          RCC_CPERCKSELR_CKPERSRC_2
#define RCC_CKPERCLKSOURCE_OFF          RCC_CPERCKSELR_CKPERSRC_3 /*Clock disabled*/

#define IS_RCC_CKPERCLKSOURCE(SOURCE) (((SOURCE) == RCC_CKPERCLKSOURCE_HSI) || \
                                       ((SOURCE) == RCC_CKPERCLKSOURCE_CSI) || \
                                       ((SOURCE) == RCC_CKPERCLKSOURCE_HSE) || \
                                       ((SOURCE) == RCC_CKPERCLKSOURCE_OFF))
/**
  * @}
  */


/** @defgroup RCCEx_STGEN_Clock_Source STGEN Clock Source
  * @{
  */
#define RCC_STGENCLKSOURCE_HSI          RCC_STGENCKSELR_STGENSRC_0
#define RCC_STGENCLKSOURCE_HSE          RCC_STGENCKSELR_STGENSRC_1
#define RCC_STGENCLKSOURCE_OFF          RCC_STGENCKSELR_STGENSRC_2

#define IS_RCC_STGENCLKSOURCE(SOURCE) \
                              (((SOURCE) == RCC_STGENCLKSOURCE_HSI) || \
                               ((SOURCE) == RCC_STGENCLKSOURCE_HSE) || \
                               ((SOURCE) == RCC_STGENCLKSOURCE_OFF))
/**
  * @}
  */

#if defined(DSI)
/** @defgroup RCCEx_DSI_Clock_Source  DSI Clock Source
  * @{
  */
#define RCC_DSICLKSOURCE_PHY            RCC_DSICKSELR_DSISRC_0
#define RCC_DSICLKSOURCE_PLL4           RCC_DSICKSELR_DSISRC_1

#define IS_RCC_DSICLKSOURCE(__SOURCE__) \
                            (((__SOURCE__) == RCC_DSICLKSOURCE_PHY)  || \
                             ((__SOURCE__) == RCC_DSICLKSOURCE_PLL4))
/**
  * @}
  */
#endif /*DSI*/

/** @defgroup RCCEx_ADC_Clock_Source ADC Clock Source
  * @{
  */
#define RCC_ADCCLKSOURCE_PLL4           RCC_ADCCKSELR_ADCSRC_0
#define RCC_ADCCLKSOURCE_PER            RCC_ADCCKSELR_ADCSRC_1
#define RCC_ADCCLKSOURCE_PLL3           RCC_ADCCKSELR_ADCSRC_2

#define IS_RCC_ADCCLKSOURCE(SOURCE) (((SOURCE) == RCC_ADCCLKSOURCE_PLL4)  || \
                                     ((SOURCE) == RCC_ADCCLKSOURCE_PER)   || \
                                     ((SOURCE) == RCC_ADCCLKSOURCE_PLL3))
/**
  * @}
  */


/** @defgroup RCCEx_LPTIM1_Clock_Source LPTIM1 Clock Source
  * @{
  */
#define RCC_LPTIM1CLKSOURCE_BCLK        RCC_LPTIM1CKSELR_LPTIM1SRC_0
#define RCC_LPTIM1CLKSOURCE_PLL4        RCC_LPTIM1CKSELR_LPTIM1SRC_1
#define RCC_LPTIM1CLKSOURCE_PLL3        RCC_LPTIM1CKSELR_LPTIM1SRC_2
#define RCC_LPTIM1CLKSOURCE_LSE         RCC_LPTIM1CKSELR_LPTIM1SRC_3
#define RCC_LPTIM1CLKSOURCE_LSI         RCC_LPTIM1CKSELR_LPTIM1SRC_4
#define RCC_LPTIM1CLKSOURCE_PER         RCC_LPTIM1CKSELR_LPTIM1SRC_5
#define RCC_LPTIM1CLKSOURCE_OFF         RCC_LPTIM1CKSELR_LPTIM1SRC_6

#define IS_RCC_LPTIM1CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_LPTIM1CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_PLL3)  || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_LSE)   || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_LSI)   || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_PER)   || \
                                ((SOURCE) == RCC_LPTIM1CLKSOURCE_OFF))
/**
  * @}
  */

/** @defgroup RCCEx_LPTIM23_Clock_Source LPTIM23 Clock Source
  * @{
  */
#define RCC_LPTIM23CLKSOURCE_BCLK        RCC_LPTIM23CKSELR_LPTIM23SRC_0
#define RCC_LPTIM23CLKSOURCE_PLL4        RCC_LPTIM23CKSELR_LPTIM23SRC_1
#define RCC_LPTIM23CLKSOURCE_PER         RCC_LPTIM23CKSELR_LPTIM23SRC_2
#define RCC_LPTIM23CLKSOURCE_LSE         RCC_LPTIM23CKSELR_LPTIM23SRC_3
#define RCC_LPTIM23CLKSOURCE_LSI         RCC_LPTIM23CKSELR_LPTIM23SRC_4
#define RCC_LPTIM23CLKSOURCE_OFF         RCC_LPTIM23CKSELR_LPTIM23SRC_5


#define IS_RCC_LPTIM23CLKSOURCE(SOURCE) \
                               (((SOURCE) == RCC_LPTIM23CLKSOURCE_BCLK)  || \
                                ((SOURCE) == RCC_LPTIM23CLKSOURCE_PLL4)  || \
                                ((SOURCE) == RCC_LPTIM23CLKSOURCE_PER)   || \
                                ((SOURCE) == RCC_LPTIM23CLKSOURCE_LSE)   || \
                                ((SOURCE) == RCC_LPTIM23CLKSOURCE_LSI)   || \
                                ((SOURCE) == RCC_LPTIM23CLKSOURCE_OFF))
/**
  * @}
  */

/** @defgroup RCCEx_LPTIM45_Clock_Source LPTIM45 Clock Source
  * @{
  */
#define RCC_LPTIM45CLKSOURCE_BCLK       RCC_LPTIM45CKSELR_LPTIM45SRC_0
#define RCC_LPTIM45CLKSOURCE_PLL4       RCC_LPTIM45CKSELR_LPTIM45SRC_1
#define RCC_LPTIM45CLKSOURCE_PLL3       RCC_LPTIM45CKSELR_LPTIM45SRC_2
#define RCC_LPTIM45CLKSOURCE_LSE        RCC_LPTIM45CKSELR_LPTIM45SRC_3
#define RCC_LPTIM45CLKSOURCE_LSI        RCC_LPTIM45CKSELR_LPTIM45SRC_4
#define RCC_LPTIM45CLKSOURCE_PER        RCC_LPTIM45CKSELR_LPTIM45SRC_5
#define RCC_LPTIM45CLKSOURCE_OFF        RCC_LPTIM45CKSELR_LPTIM45SRC_6



#define IS_RCC_LPTIM45CLKSOURCE(SOURCE) \
                                (((SOURCE) == RCC_LPTIM45CLKSOURCE_BCLK)  || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_PLL4)  || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_PLL3)  || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_LSE)   || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_LSI)   || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_PER)   || \
                                 ((SOURCE) == RCC_LPTIM45CLKSOURCE_OFF))
/**
  * @}
  */


/** @defgroup RCCEx_TIMG1_Prescaler_Selection TIMG1 Prescaler Selection
  * @{
  */
#define RCC_TIMG1PRES_DEACTIVATED                RCC_TIMG1PRER_TIMG1PRE_0
#define RCC_TIMG1PRES_ACTIVATED                  RCC_TIMG1PRER_TIMG1PRE_1

#define IS_RCC_TIMG1PRES(PRES)  (((PRES) == RCC_TIMG1PRES_DEACTIVATED)    || \
                                ((PRES) == RCC_TIMG1PRES_ACTIVATED))
/**
  * @}
  */


/** @defgroup RCCEx_TIMG2_Prescaler_Selection TIMG2 Prescaler Selection
  * @{
  */
#define RCC_TIMG2PRES_DEACTIVATED                RCC_TIMG2PRER_TIMG2PRE_0
#define RCC_TIMG2PRES_ACTIVATED                  RCC_TIMG2PRER_TIMG2PRE_1

#define IS_RCC_TIMG2PRES(PRES)  (((PRES) == RCC_TIMG2PRES_DEACTIVATED)    || \
                                ((PRES) == RCC_TIMG2PRES_ACTIVATED))
/**
  * @}
  */


/** @defgroup RCCEx_RCC_BootCx RCC BootCx
  * @{
  */
#define RCC_BOOT_C1                     RCC_MP_BOOTCR_MPU_BEN
#define RCC_BOOT_C2                     RCC_MP_BOOTCR_MCU_BEN

#define IS_RCC_BOOT_CORE(CORE)          (((CORE) == RCC_BOOT_C1)  || \
                                         ((CORE) == RCC_BOOT_C2)  || \
                                         ((CORE) == (RCC_BOOT_C1 ||RCC_BOOT_C2) ))
/**
  * @}
  */
/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCCEx Exported Macros
 * @{
 */
/** @brief macro to configure the I2C12 clock (I2C12CLK).
  *
  * @param  __I2C12CLKSource__: specifies the I2C12 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_I2C12CLKSOURCE_BCLK:   PCLK1 selected as I2C12 clock (default after reset)
  *            @arg RCC_I2C12CLKSOURCE_PLL4:   PLL4_R selected as I2C12 clock
  *            @arg RCC_I2C12CLKSOURCE_HSI:    HSI selected as I2C12 clock
  *            @arg RCC_I2C12CLKSOURCE_CSI:    CSI selected as I2C12 clock
  * @retval None
  */
#define __HAL_RCC_I2C12_CONFIG(__I2C12CLKSource__) \
                  MODIFY_REG(RCC->I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC, (uint32_t)(__I2C12CLKSource__))

/** @brief  macro to get the I2C12 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_I2C12CLKSOURCE_BCLK:   PCLK1 selected as I2C12 clock
  *            @arg RCC_I2C12CLKSOURCE_PLL4:   PLL4_R selected as I2C12 clock
  *            @arg RCC_I2C12CLKSOURCE_HSI:    HSI selected as I2C12 clock
  *            @arg RCC_I2C12CLKSOURCE_CSI:    CSI selected as I2C12 clock
  */
#define __HAL_RCC_GET_I2C12_SOURCE() ((uint32_t)(READ_BIT(RCC->I2C12CKSELR, RCC_I2C12CKSELR_I2C12SRC)))

/** @brief macro to configure the I2C35 clock (I2C35CLK).
  *
  * @param  __I2C35CLKSource__: specifies the I2C35 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_I2C35CLKSOURCE_BCLK:   PCLK1 selected as I2C35 clock (default after reset)
  *            @arg RCC_I2C35CLKSOURCE_PLL4:   PLL4_R selected as I2C35 clock
  *            @arg RCC_I2C35CLKSOURCE_HSI:    HSI selected as I2C35 clock
  *            @arg RCC_I2C35CLKSOURCE_CSI:    CSI selected as I2C35 clock
  * @retval None
  */
#define __HAL_RCC_I2C35_CONFIG(__I2C35CLKSource__) \
                  MODIFY_REG(RCC->I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC, (uint32_t)(__I2C35CLKSource__))

/** @brief  macro to get the I2C35 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_I2C35CLKSOURCE_BCLK:   PCLK1 selected as I2C35 clock
  *            @arg RCC_I2C35CLKSOURCE_PLL4:   PLL4_R selected as I2C35 clock
  *            @arg RCC_I2C35CLKSOURCE_HSI:    HSI selected as I2C35 clock
  *            @arg RCC_I2C35CLKSOURCE_CSI:    CSI selected as I2C35 clock
  */
#define __HAL_RCC_GET_I2C35_SOURCE() ((uint32_t)(READ_BIT(RCC->I2C35CKSELR, RCC_I2C35CKSELR_I2C35SRC)))

/** @brief macro to configure the I2C46 clock (I2C46CLK).
  *
  * @param  __I2C46CLKSource__: specifies the I2C46 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_I2C46CLKSOURCE_BCLK:   PCLK5 selected as I2C46 clock (default after reset)
  *            @arg RCC_I2C46CLKSOURCE_PLL3:   PLL3_Q selected as I2C46 clock
  *            @arg RCC_I2C46CLKSOURCE_HSI:    HSI selected as I2C46 clock
  *            @arg RCC_I2C46CLKSOURCE_CSI:    CSI selected as I2C46 clock
  * @retval None
  */
#define __HAL_RCC_I2C46_CONFIG(__I2C46CLKSource__) \
                  MODIFY_REG(RCC->I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC, (uint32_t)(__I2C46CLKSource__))

/** @brief  macro to get the I2C46 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_I2C46CLKSOURCE_BCLK:   PCLK5 selected as I2C46 clock
  *            @arg RCC_I2C46CLKSOURCE_PLL3:   PLL3_Q selected as I2C46 clock
  *            @arg RCC_I2C46CLKSOURCE_HSI:    HSI selected as I2C46 clock
  *            @arg RCC_I2C46CLKSOURCE_CSI:    CSI selected as I2C46 clock
  */
#define __HAL_RCC_GET_I2C46_SOURCE() ((uint32_t)(READ_BIT(RCC->I2C46CKSELR, RCC_I2C46CKSELR_I2C46SRC)))

/**
  * @brief  Macro to Configure the SAI1 clock source.
  * @param  __RCC_SAI1CLKSource__: defines the SAI1 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SAI1CLKSOURCE_PLL4:    SAI1 clock = PLL4Q
  *             @arg RCC_SAI1CLKSOURCE_PLL3_Q:  SAI1 clock = PLL3Q
  *             @arg RCC_SAI1CLKSOURCE_I2SCKIN: SAI1 clock = I2SCKIN
  *             @arg RCC_SAI1CLKSOURCE_PER:     SAI1 clock = PER
  *             @arg RCC_SAI1CLKSOURCE_PLL3_R:  SAI1 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SAI1_CONFIG(__RCC_SAI1CLKSource__ ) \
                  MODIFY_REG(RCC->SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC, (uint32_t)(__RCC_SAI1CLKSource__))

/** @brief  Macro to get the SAI1 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SAI1CLKSOURCE_PLL4:    SAI1 clock = PLL4Q
  *             @arg RCC_SAI1CLKSOURCE_PLL3_Q:  SAI1 clock = PLL3Q
  *             @arg RCC_SAI1CLKSOURCE_I2SCKIN: SAI1 clock = I2SCKIN
  *             @arg RCC_SAI1CLKSOURCE_PER:     SAI1 clock = PER
  *             @arg RCC_SAI1CLKSOURCE_PLL3_R:  SAI1 clock = PLL3R
  */
#define __HAL_RCC_GET_SAI1_SOURCE() ((uint32_t)(READ_BIT(RCC->SAI1CKSELR, RCC_SAI1CKSELR_SAI1SRC)))


/**
  * @brief  Macro to Configure the SAI2 clock source.
  * @param  __RCC_SAI2CLKSource__: defines the SAI2 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SAI2CLKSOURCE_PLL4:    SAI2 clock = PLL4Q
  *             @arg RCC_SAI2CLKSOURCE_PLL3_Q:  SAI2 clock = PLL3Q
  *             @arg RCC_SAI2CLKSOURCE_I2SCKIN: SAI2 clock = I2SCKIN
  *             @arg RCC_SAI2CLKSOURCE_PER:     SAI2 clock = PER
  *             @arg RCC_SAI2CLKSOURCE_SPDIF:   SAI2 clock = SPDIF_CK_SYMB
  *             @arg RCC_SAI2CLKSOURCE_PLL3_R:  SAI2 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SAI2_CONFIG(__RCC_SAI2CLKSource__ ) \
                  MODIFY_REG(RCC->SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC, (uint32_t)(__RCC_SAI2CLKSource__))

/** @brief  Macro to get the SAI2 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SAI2CLKSOURCE_PLL4:    SAI2 clock = PLL4Q
  *             @arg RCC_SAI2CLKSOURCE_PLL3_Q:  SAI2 clock = PLL3Q
  *             @arg RCC_SAI2CLKSOURCE_I2SCKIN: SAI2 clock = I2SCKIN
  *             @arg RCC_SAI2CLKSOURCE_PER:     SAI2 clock = PER
  *             @arg RCC_SAI2CLKSOURCE_SPDIF:   SAI2 clock = SPDIF_CK_SYMB
  *             @arg RCC_SAI2CLKSOURCE_PLL3_R:  SAI2 clock = PLL3R
  */
#define __HAL_RCC_GET_SAI2_SOURCE() ((uint32_t)(READ_BIT(RCC->SAI2CKSELR, RCC_SAI2CKSELR_SAI2SRC)))


/**
  * @brief  Macro to Configure the SAI3 clock source.
  * @param  __RCC_SAI3CLKSource__: defines the SAI3 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SAI3CLKSOURCE_PLL4:    SAI3 clock = PLL4Q
  *             @arg RCC_SAI3CLKSOURCE_PLL3_Q:  SAI3 clock = PLL3Q
  *             @arg RCC_SAI3CLKSOURCE_I2SCKIN: SAI3 clock = I2SCKIN
  *             @arg RCC_SAI3CLKSOURCE_PER:     SAI3 clock = PER
  *             @arg RCC_SAI3CLKSOURCE_PLL3_R:  SAI3 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SAI3_CONFIG(__RCC_SAI3CLKSource__ ) \
                  MODIFY_REG(RCC->SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC, (uint32_t)(__RCC_SAI3CLKSource__))

/** @brief  Macro to get the SAI3 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SAI3CLKSOURCE_PLL4:    SAI3 clock = PLL4Q
  *             @arg RCC_SAI3CLKSOURCE_PLL3_Q:  SAI3 clock = PLL3Q
  *             @arg RCC_SAI3CLKSOURCE_I2SCKIN: SAI3 clock = I2SCKIN
  *             @arg RCC_SAI3CLKSOURCE_PER:     SAI3 clock = PER
  *             @arg RCC_SAI3CLKSOURCE_PLL3_R:  SAI3 clock = PLL3R
  */
#define __HAL_RCC_GET_SAI3_SOURCE() ((uint32_t)(READ_BIT(RCC->SAI3CKSELR, RCC_SAI3CKSELR_SAI3SRC)))


/**
  * @brief  Macro to Configure the SAI4 clock source.
  * @param  __RCC_SAI4CLKSource__: defines the SAI4 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SAI4CLKSOURCE_PLL4:    SAI4 clock = PLL4Q
  *             @arg RCC_SAI4CLKSOURCE_PLL3_Q:  SAI4 clock = PLL3Q
  *             @arg RCC_SAI4CLKSOURCE_I2SCKIN: SAI4 clock = I2SCKIN
  *             @arg RCC_SAI4CLKSOURCE_PER:     SAI4 clock = PER
  *             @arg RCC_SAI4CLKSOURCE_PLL3_R:  SAI4 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SAI4_CONFIG(__RCC_SAI4CLKSource__ ) \
                  MODIFY_REG(RCC->SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC, (uint32_t)(__RCC_SAI4CLKSource__))

/** @brief  Macro to get the SAI4 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SAI4CLKSOURCE_PLL4:    SAI4 clock = PLL4Q
  *             @arg RCC_SAI4CLKSOURCE_PLL3_Q:  SAI4 clock = PLL3Q
  *             @arg RCC_SAI4CLKSOURCE_I2SCKIN: SAI4 clock = I2SCKIN
  *             @arg RCC_SAI4CLKSOURCE_PER:     SAI4 clock = PER
  *             @arg RCC_SAI4CLKSOURCE_PLL3_R:  SAI4 clock = PLL3R
  */
#define __HAL_RCC_GET_SAI4_SOURCE() ((uint32_t)(READ_BIT(RCC->SAI4CKSELR, RCC_SAI4CKSELR_SAI4SRC)))


/**
  * @brief  Macro to Configure the SPI/I2S1 clock source.
  * @param  __RCC_SPI1CLKSource__: defines the SPI/I2S1 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SPI1CLKSOURCE_PLL4:    SPI1 clock = PLL4P
  *             @arg RCC_SPI1CLKSOURCE_PLL3_Q:  SPI1 clock = PLL3Q
  *             @arg RCC_SPI1CLKSOURCE_I2SCKIN: SPI1 clock = I2SCKIN
  *             @arg RCC_SPI1CLKSOURCE_PER:     SPI1 clock = PER
  *             @arg RCC_SPI1CLKSOURCE_PLL3_R:  SPI1 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SPI1_CONFIG(__RCC_SPI1CLKSource__) \
                  MODIFY_REG(RCC->SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC, (uint32_t)(__RCC_SPI1CLKSource__))

/** @brief  Macro to get the SPI/I2S1 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SPI1CLKSOURCE_PLL4:    SPI1 clock = PLL4P
  *             @arg RCC_SPI1CLKSOURCE_PLL3_Q:  SPI1 clock = PLL3Q
  *             @arg RCC_SPI1CLKSOURCE_I2SCKIN: SPI1 clock = I2SCKIN
  *             @arg RCC_SPI1CLKSOURCE_PER:     SPI1 clock = PER
  *             @arg RCC_SPI1CLKSOURCE_PLL3_R:  SPI1 clock = PLL3R
  */
#define __HAL_RCC_GET_SPI1_SOURCE() ((uint32_t)(READ_BIT(RCC->SPI2S1CKSELR, RCC_SPI2S1CKSELR_SPI1SRC)))


/**
  * @brief  Macro to Configure the SPI/I2S2,3 clock source.
  * @param  __RCC_SPI23CLKSource__: defines the SPI/I2S2,3 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SPI23CLKSOURCE_PLL4:    SPI23 clock = PLL4P
  *             @arg RCC_SPI23CLKSOURCE_PLL3_Q:  SPI23 clock = PLL3Q
  *             @arg RCC_SPI23CLKSOURCE_I2SCKIN: SPI23 clock = I2SCKIN
  *             @arg RCC_SPI23CLKSOURCE_PER:     SPI23 clock = PER
  *             @arg RCC_SPI23CLKSOURCE_PLL3_R:  SPI23 clock = PLL3R
  * @retval None
  */
#define __HAL_RCC_SPI23_CONFIG(__RCC_SPI23CLKSource__ ) \
                  MODIFY_REG(RCC->SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC, (uint32_t)(__RCC_SPI23CLKSource__))

/** @brief  Macro to get the SPI/I2S2,3 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SPI23CLKSOURCE_PLL4:    SPI23 clock = PLL4P
  *             @arg RCC_SPI23CLKSOURCE_PLL3_Q:  SPI23 clock = PLL3Q
  *             @arg RCC_SPI23CLKSOURCE_I2SCKIN: SPI23 clock = I2SCKIN
  *             @arg RCC_SPI23CLKSOURCE_PER:     SPI23 clock = PER
  *             @arg RCC_SPI23CLKSOURCE_PLL3_R:  SPI23 clock = PLL3R
  */
#define __HAL_RCC_GET_SPI23_SOURCE() ((uint32_t)(READ_BIT(RCC->SPI2S23CKSELR, RCC_SPI2S23CKSELR_SPI23SRC)))

/**
  * @brief  Macro to Configure the SPI45 clock source.
  * @param  __RCC_SPI45CLKSource__: defines the SPI45 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SPI45CLKSOURCE_BCLK:    SPI45 clock = PCLK2
  *             @arg RCC_SPI45CLKSOURCE_PLL4:    SPI45 clock = PLL4Q
  *             @arg RCC_SPI45CLKSOURCE_HSI:     SPI45 clock = HSI
  *             @arg RCC_SPI45CLKSOURCE_CSI:     SPI45 clock = CSI
  *             @arg RCC_SPI45CLKSOURCE_HSE:     SPI45 clock = HSE
  * @retval None
  */
#define __HAL_RCC_SPI45_CONFIG(__RCC_SPI45CLKSource__ ) \
                MODIFY_REG(RCC->SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC, (uint32_t)(__RCC_SPI45CLKSource__))

/** @brief  Macro to get the SPI45 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SPI45CLKSOURCE_BCLK:    SPI45 clock = PCLK2
  *             @arg RCC_SPI45CLKSOURCE_PLL4:    SPI45 clock = PLL4Q
  *             @arg RCC_SPI45CLKSOURCE_HSI:     SPI45 clock = HSI
  *             @arg RCC_SPI45CLKSOURCE_CSI:     SPI45 clock = CSI
  *             @arg RCC_SPI45CLKSOURCE_HSE:     SPI45 clock = HSE
  */
#define __HAL_RCC_GET_SPI45_SOURCE() ((uint32_t)(READ_BIT(RCC->SPI45CKSELR, RCC_SPI45CKSELR_SPI45SRC)))

/**
  * @brief  Macro to Configure the SPI6 clock source.
  * @param  __RCC_SPI6CLKSource__: defines the SPI6 clock source.
  *          This parameter can be one of the following values:
  *             @arg RCC_SPI6CLKSOURCE_BCLK:   SPI6 clock = PCLK5
  *             @arg RCC_SPI6CLKSOURCE_PLL4:   SPI6 clock = PLL4Q
  *             @arg RCC_SPI6CLKSOURCE_HSI:    SPI6 clock = HSI
  *             @arg RCC_SPI6CLKSOURCE_CSI:    SPI6 clock = CSI
  *             @arg RCC_SPI6CLKSOURCE_HSE:    SPI6 clock = HSE
  *             @arg RCC_SPI6CLKSOURCE_PLL3:   SPI6 clock = PLL3Q
  * @retval None
  */
#define __HAL_RCC_SPI6_CONFIG(__RCC_SPI6CLKSource__ ) \
                  MODIFY_REG(RCC->SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC, (uint32_t)(__RCC_SPI6CLKSource__))

/** @brief  Macro to get the SPI6 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg RCC_SPI6CLKSOURCE_BCLK:   SPI6 clock = PCLK5
  *             @arg RCC_SPI6CLKSOURCE_PLL4:   SPI6 clock = PLL4Q
  *             @arg RCC_SPI6CLKSOURCE_HSI:    SPI6 clock = HSI
  *             @arg RCC_SPI6CLKSOURCE_CSI:    SPI6 clock = CSI
  *             @arg RCC_SPI6CLKSOURCE_HSE:    SPI6 clock = HSE
  *             @arg RCC_SPI6CLKSOURCE_PLL3:   SPI6 clock = PLL3Q
  */
#define __HAL_RCC_GET_SPI6_SOURCE() ((uint32_t)(READ_BIT(RCC->SPI6CKSELR, RCC_SPI6CKSELR_SPI6SRC)))


/** @brief macro to configure the USART1 clock (USART1CLK).
  *
  * @param  __USART1CLKSource__: specifies the USART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USART1CLKSOURCE_BCLK:   PCLK5 Clock selected as USART1 clock (default after reset)
  *            @arg RCC_USART1CLKSOURCE_PLL3:   PLL3_Q Clock selected as USART1 clock USART1
  *            @arg RCC_USART1CLKSOURCE_HSI:    HSI Clock selected as USART1 clock USART1
  *            @arg RCC_USART1CLKSOURCE_CSI:    CSI Clock selected as USART1 clock USART1
  *            @arg RCC_USART1CLKSOURCE_PLL4:   PLL4_Q Clock selected as USART1 clock USART1
  *            @arg RCC_USART1CLKSOURCE_HSE:    HSE Clock selected as USART1 clock USART1
  * @retval None
  */
#define __HAL_RCC_USART1_CONFIG(__USART1CLKSource__) \
                  MODIFY_REG(RCC->UART1CKSELR, RCC_UART1CKSELR_UART1SRC, (uint32_t)(__USART1CLKSource__))

/** @brief  macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USART1CLKSOURCE_BCLK:   PCLK5 Clock selected as USART1 clock (default after reset)
  *            @arg RCC_USART1CLKSOURCE_PLL3:   PLL3_Q Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_HSI:    HSI Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_CSI:    CSI Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_PLL4:   PLL4_Q Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_HSE:    HSE Clock selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() ((uint32_t)(READ_BIT(RCC->UART1CKSELR, RCC_UART1CKSELR_UART1SRC)))

/** @brief macro to configure the UART24 clock (UART24CLK).
  *
  * @param  __UART24CLKSource__: specifies the UART24 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_UART24CLKSOURCE_BCLK: PCLK1 Clock selected as UART24
  *                                           clock (default after reset)
  *            @arg RCC_UART24CLKSOURCE_PLL4: PLL4_Q Clock selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_HSI:  HSI selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_CSI:  CSI Clock selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_HSE:  HSE selected as UART24 clock
  * @retval None
  */
#define __HAL_RCC_UART24_CONFIG(__UART24CLKSource__) \
                  MODIFY_REG(RCC->UART24CKSELR, RCC_UART24CKSELR_UART24SRC, (uint32_t)(__UART24CLKSource__))

/** @brief  macro to get the UART24 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_UART24CLKSOURCE_BCLK: PCLK1 Clock selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_PLL4: PLL4_Q Clock selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_HSI:  HSI selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_CSI:  CSI Clock selected as UART24 clock
  *            @arg RCC_UART24CLKSOURCE_HSE:  HSE selected as UART24 clock
  */
#define __HAL_RCC_GET_UART24_SOURCE() ((uint32_t)(READ_BIT(RCC->UART24CKSELR, RCC_UART24CKSELR_UART24SRC)))


/** @brief macro to configure the UART35 clock (UART35CLK).
  *
  * @param  __UART35CLKSource__: specifies the UART35 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_UART35CLKSOURCE_BCLK: PCLK1 Clock selected as UART35
  *                                           clock (default after reset)
  *            @arg RCC_UART35CLKSOURCE_PLL4: PLL4_Q Clock selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_HSI:  HSI selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_CSI:  CSI Clock selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_HSE:  HSE selected as UART35 clock
  * @retval None
  */
#define __HAL_RCC_UART35_CONFIG(__UART35CLKSource__) \
                  MODIFY_REG(RCC->UART35CKSELR, RCC_UART35CKSELR_UART35SRC, (uint32_t)(__UART35CLKSource__))

/** @brief  macro to get the UART35 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_UART35CLKSOURCE_BCLK:  PCLK1 Clock selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_PLL4:  PLL4_Q Clock selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_HSI:   HSI selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_CSI:   CSI Clock selected as UART35 clock
  *            @arg RCC_UART35CLKSOURCE_HSE:   HSE selected as UART35 clock
  */
#define __HAL_RCC_GET_UART35_SOURCE() ((uint32_t)(READ_BIT(RCC->UART35CKSELR, RCC_UART35CKSELR_UART35SRC)))


/** @brief macro to configure the USART6 clock (USART6CLK).
  *
  * @param  __USART6CLKSource__: specifies the USART6 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USART6CLKSOURCE_BCLK:  PCLK2 Clock selected as USART6 clock (default after reset)
  *            @arg RCC_USART6CLKSOURCE_PLL4:  PLL4_Q Clock selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_HSI:   HSI selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_CSI:   CSI Clock selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_HSE:   HSE selected as USART6 clock
  * @retval None
  */
#define __HAL_RCC_USART6_CONFIG(__USART6CLKSource__) \
                  MODIFY_REG(RCC->UART6CKSELR, RCC_UART6CKSELR_UART6SRC, (uint32_t)(__USART6CLKSource__))

/** @brief  macro to get the USART6 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USART6CLKSOURCE_BCLK:  PCLK2 Clock selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_PLL4:  PLL4_Q Clock selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_HSI:   HSI selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_CSI:   CSI Clock selected as USART6 clock
  *            @arg RCC_USART6CLKSOURCE_HSE:   HSE selected as USART6 clock
  */
#define __HAL_RCC_GET_USART6_SOURCE() ((uint32_t)(READ_BIT(RCC->UART6CKSELR, RCC_UART6CKSELR_UART6SRC)))

/** @brief macro to configure the UART78clock (UART78CLK).
  *
  * @param  __UART78CLKSource__: specifies the UART78 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_UART78CLKSOURCE_BCLK:  PCLK1 Clock selected as UART78 clock (default after reset)
  *            @arg RCC_UART78CLKSOURCE_PLL4:  PLL4_Q Clock selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_HSI:   HSI selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_CSI:   CSI Clock selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_HSE:   HSE selected as UART78 clock
  * @retval None
  */
#define __HAL_RCC_UART78_CONFIG(__UART78CLKSource__) \
                  MODIFY_REG(RCC->UART78CKSELR, RCC_UART78CKSELR_UART78SRC, (uint32_t)(__UART78CLKSource__))

/** @brief  macro to get the UART78 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_UART78CLKSOURCE_BCLK:  PCLK1 Clock selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_PLL4:  PLL4_Q Clock selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_HSI:   HSI selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_CSI:   CSI Clock selected as UART78 clock
  *            @arg RCC_UART78CLKSOURCE_HSE:   HSE selected as UART78 clock
  */
#define __HAL_RCC_GET_UART78_SOURCE() ((uint32_t)(READ_BIT(RCC->UART78CKSELR, RCC_UART78CKSELR_UART78SRC)))

/** @brief macro to configure the SDMMC12 clock (SDMMC12CLK).
  *
  * @param  __SDMMC12CLKSource__: specifies the SDMMC12 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_SDMMC12CLKSOURCE_BCLK:  HCLK6 Clock selected as SDMMC12 clock (default after reset)
  *            @arg RCC_SDMMC12CLKSOURCE_PLL3:  PLL3_R Clock selected as SDMMC12 clock
  *            @arg RCC_SDMMC12CLKSOURCE_PLL4:  PLL4_P selected as SDMMC12 clock
  *            @arg RCC_SDMMC12CLKSOURCE_HSI:   HSI selected as SDMMC12 clock
  * @retval None
  */
#define __HAL_RCC_SDMMC12_CONFIG(__SDMMC12CLKSource__) \
                  MODIFY_REG(RCC->SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC, (uint32_t)(__SDMMC12CLKSource__))

/** @brief  macro to get the SDMMC12 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_SDMMC12CLKSOURCE_BCLK:  HCLK6 Clock selected as SDMMC12 clock (default after reset)
  *            @arg RCC_SDMMC12CLKSOURCE_PLL3:  PLL3_R Clock selected as SDMMC12 clock
  *            @arg RCC_SDMMC12CLKSOURCE_PLL4:  PLL4_P selected as SDMMC12 clock
  *            @arg RCC_SDMMC12CLKSOURCE_HSI:   HSI selected as SDMMC12 clock
  */
#define __HAL_RCC_GET_SDMMC12_SOURCE() ((uint32_t)(READ_BIT(RCC->SDMMC12CKSELR, RCC_SDMMC12CKSELR_SDMMC12SRC)))


/** @brief macro to configure the SDMMC3 clock (SDMMC3CLK).
  *
  * @param  __SDMMC3CLKSource__: specifies the SDMMC3 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_SDMMC3CLKSOURCE_BCLK:  HCLK2 Clock selected as SDMMC3 clock (default after reset)
  *            @arg RCC_SDMMC3CLKSOURCE_PLL3:  PLL3_R Clock selected as SDMMC3 clock
  *            @arg RCC_SDMMC3CLKSOURCE_PLL4:  PLL4_P selected as SDMMC3 clock
  *            @arg RCC_SDMMC3CLKSOURCE_HSI:   HSI selected as SDMMC3 clock
  *
  * @retval None
  */
#define __HAL_RCC_SDMMC3_CONFIG(__SDMMC3CLKSource__) \
                  MODIFY_REG(RCC->SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC, (uint32_t)(__SDMMC3CLKSource__))

/** @brief  macro to get the SDMMC3 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_SDMMC3CLKSOURCE_BCLK:  HCLK2 Clock selected as SDMMC3 clock (default after reset)
  *            @arg RCC_SDMMC3CLKSOURCE_PLL3:  PLL3_R Clock selected as SDMMC3 clock
  *            @arg RCC_SDMMC3CLKSOURCE_PLL4:  PLL4_P selected as SDMMC3 clock
  *            @arg RCC_SDMMC3CLKSOURCE_HSI:   HSI selected as SDMMC3 clock
  */
#define __HAL_RCC_GET_SDMMC3_SOURCE() ((uint32_t)(READ_BIT(RCC->SDMMC3CKSELR, RCC_SDMMC3CKSELR_SDMMC3SRC)))


/** @brief macro to configure the ETH clock (ETHCLK).
  *
  * @param  __ETHCLKSource__: specifies the ETH clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_ETHCLKSOURCE_PLL4:  PLL4_P selected as ETH clock (default after reset)
  *            @arg RCC_ETHCLKSOURCE_PLL3:  PLL3_Q Clock selected as ETH clock
  *            @arg RCC_ETHCLKSOURCE_OFF:   the kernel clock is disabled
  * @retval None
  */
#define __HAL_RCC_ETH_CONFIG(__ETHCLKSource__) \
                  MODIFY_REG(RCC->ETHCKSELR, RCC_ETHCKSELR_ETHSRC, (uint32_t)(__ETHCLKSource__))

/** @brief  macro to get the ETH clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_ETHCLKSOURCE_PLL4: PLL4_P selected as ETH clock (default after reset)
  *            @arg RCC_ETHCLKSOURCE_PLL3: PLL3_Q Clock selected as ETH clock
  *            @arg RCC_ETHCLKSOURCE_OFF:  the kernel clock is disabled
  */
#define __HAL_RCC_GET_ETH_SOURCE() ((uint32_t)(READ_BIT(RCC->ETHCKSELR, RCC_ETHCKSELR_ETHSRC)))


/** @brief macro to configure the QSPI clock (QSPICLK).
  *
  * @param  __QSPICLKSource__: specifies the QSPI clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_QSPICLKSOURCE_BCLK:  ACLK Clock selected as QSPI clock (default after reset)
  *            @arg RCC_QSPICLKSOURCE_PLL3:  PLL3_R Clock selected as QSPI clock
  *            @arg RCC_QSPICLKSOURCE_PLL4:  PLL4_P selected as QSPI clock
  *            @arg RCC_QSPICLKSOURCE_PER:   PER selected as QSPI clock
  * @retval None
  */
#define __HAL_RCC_QSPI_CONFIG(__QSPICLKSource__) \
                  MODIFY_REG(RCC->QSPICKSELR, RCC_QSPICKSELR_QSPISRC, (uint32_t)(__QSPICLKSource__))

/** @brief  macro to get the QSPI clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_QSPICLKSOURCE_BCLK:  ACLK Clock selected as QSPI clock (default after reset)
  *            @arg RCC_QSPICLKSOURCE_PLL3:  PLL3_R Clock selected as QSPI clock
  *            @arg RCC_QSPICLKSOURCE_PLL4:  PLL4_P selected as QSPI clock
  *            @arg RCC_QSPICLKSOURCE_PER:   PER selected as QSPI clock
  */
#define __HAL_RCC_GET_QSPI_SOURCE() ((uint32_t)(READ_BIT(RCC->QSPICKSELR, RCC_QSPICKSELR_QSPISRC)))


/** @brief macro to configure the FMC clock (FMCCLK).
  *
  * @param  __FMCCLKSource__: specifies the FMC clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_FMCCLKSOURCE_BCLK:  ACLK Clock selected as FMC clock (default after reset)
  *            @arg RCC_FMCCLKSOURCE_PLL3:  PLL3_R Clock selected as FMC clock
  *            @arg RCC_FMCCLKSOURCE_PLL4:  PLL4_P selected as FMC clock
  *            @arg RCC_FMCCLKSOURCE_PER:   PER selected as FMC clock
  * @retval None
  */
#define __HAL_RCC_FMC_CONFIG(__FMCCLKSource__) \
                  MODIFY_REG(RCC->FMCCKSELR, RCC_FMCCKSELR_FMCSRC, (uint32_t)(__FMCCLKSource__))

/** @brief  macro to get the FMC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_FMCCLKSOURCE_BCLK:  ACLK Clock selected as FMC clock (default after reset)
  *            @arg RCC_FMCCLKSOURCE_PLL3:  PLL3_R Clock selected as FMC clock
  *            @arg RCC_FMCCLKSOURCE_PLL4:  PLL4_P selected as FMC clock
  *            @arg RCC_FMCCLKSOURCE_PER:   PER selected as FMC clock
  */
#define __HAL_RCC_GET_FMC_SOURCE() ((uint32_t)(READ_BIT(RCC->FMCCKSELR, RCC_FMCCKSELR_FMCSRC)))

#if defined(FDCAN1)
/** @brief macro to configure the FDCAN clock (FDCANCLK).
  *
  * @param  __FDCANCLKSource__: specifies the FDCAN clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_FDCANCLKSOURCE_HSE:    HSE Clock selected as FDCAN clock (default after reset)
  *            @arg RCC_FDCANCLKSOURCE_PLL3:   PLL3_Q Clock selected as FDCAN clock
  *            @arg RCC_FDCANCLKSOURCE_PLL4_Q: PLL4_Q selected as FDCAN clock
  *            @arg RCC_FDCANCLKSOURCE_PLL4_R: PLL4_R selected as FDCAN clock
  * @retval None
  */
#define __HAL_RCC_FDCAN_CONFIG(__FDCANCLKSource__) \
                  MODIFY_REG(RCC->FDCANCKSELR, RCC_FDCANCKSELR_FDCANSRC, (uint32_t)(__FDCANCLKSource__))

/** @brief  macro to get the FDCAN clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_FDCANCLKSOURCE_HSE:   HSE Clock selected as FDCAN clock (default after reset)
  *            @arg RCC_FDCANCLKSOURCE_PLL3:  PLL3_Q Clock selected as FDCAN clock
  *            @arg RCC_FDCANCLKSOURCE_PLL4_Q: PLL4_Q selected as FDCAN clock
  *            @arg RCC_FDCANCLKSOURCE_PLL4_R: PLL4_R selected as FDCAN clock
  */
#define __HAL_RCC_GET_FDCAN_SOURCE() ((uint32_t)(READ_BIT(RCC->FDCANCKSELR, RCC_FDCANCKSELR_FDCANSRC)))
#endif /*FDCAN1*/

/** @brief macro to configure the SPDIFRX clock (SPDIFCLK).
  *
  * @param  __SPDIFCLKSource__: specifies the SPDIF clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_SPDIFRXCLKSOURCE_PLL4:  PLL4_P Clock selected as SPDIF clock (default after reset)
  *            @arg RCC_SPDIFRXCLKSOURCE_PLL3:  PLL3_Q Clock selected as SPDIF clock
  *            @arg RCC_SPDIFRXCLKSOURCE_HSI:   HSI selected as SPDIF clock
  * @retval None
  */
#define __HAL_RCC_SPDIFRX_CONFIG(__SPDIFCLKSource__) \
                  MODIFY_REG(RCC->SPDIFCKSELR, RCC_SPDIFCKSELR_SPDIFSRC, (uint32_t)(__SPDIFCLKSource__))

/** @brief  macro to get the SPDIFRX clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_SPDIFRXCLKSOURCE_PLL4:  PLL4_P Clock selected as SPDIF clock (default after reset)
  *            @arg RCC_SPDIFRXCLKSOURCE_PLL3:  PLL3_Q Clock selected as SPDIF clock
  *            @arg RCC_SPDIFRXCLKSOURCE_HSI:   HSI selected as SPDIF clock
  */
#define __HAL_RCC_GET_SPDIFRX_SOURCE() ((uint32_t)(READ_BIT(RCC->SPDIFCKSELR, RCC_SPDIFCKSELR_SPDIFSRC)))


/** @brief macro to configure the CEC clock (CECCLK).
  *
  * @param  __CECCLKSource__: specifies the CEC clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_CECCLKSOURCE_LSE:    LSE Clock selected as CEC clock (default after reset)
  *            @arg RCC_CECCLKSOURCE_LSI:    LSI Clock selected as CEC clock
  *            @arg RCC_CECCLKSOURCE_CSI122: CSI/122 Clock selected as CEC clock
  * @retval None
  */
#define __HAL_RCC_CEC_CONFIG(__CECCLKSource__) \
                  MODIFY_REG(RCC->CECCKSELR, RCC_CECCKSELR_CECSRC, (uint32_t)(__CECCLKSource__))

/** @brief  macro to get the CEC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_CECCLKSOURCE_LSE:    LSE Clock selected as CEC clock (default after reset)
  *            @arg RCC_CECCLKSOURCE_LSI:    LSI Clock selected as CEC clock
  *            @arg RCC_CECCLKSOURCE_CSI122: CSI/122 Clock selected as CEC clock
  */
#define __HAL_RCC_GET_CEC_SOURCE() ((uint32_t)(READ_BIT(RCC->CECCKSELR, RCC_CECCKSELR_CECSRC)))


/** @brief macro to configure the USB PHY clock (USBPHYCLK).
  *
  * @param  __USBPHYCLKSource__: specifies the USB PHY clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USBPHYCLKSOURCE_HSE:   HSE_KER   Clock selected as USB PHY clock (default after reset)
  *            @arg RCC_USBPHYCLKSOURCE_PLL4:  PLL4_R    Clock selected as USB PHY clock
  *            @arg RCC_USBPHYCLKSOURCE_HSE2:  HSE_KER/2 Clock selected as USB PHY clock
  * @retval None
  */
#define __HAL_RCC_USBPHY_CONFIG(__USBPHYCLKSource__) \
                  MODIFY_REG(RCC->USBCKSELR, RCC_USBCKSELR_USBPHYSRC, (uint32_t)(__USBPHYCLKSource__))

/** @brief  macro to get the USB PHY PLL clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USBPHYCLKSOURCE_HSE:   HSE_KER   Clock selected as USB PHY clock (default after reset)
  *            @arg RCC_USBPHYCLKSOURCE_PLL4:  PLL4_R    Clock selected as USB PHY clock
  *            @arg RCC_USBPHYCLKSOURCE_HSE2:  HSE_KER/2 Clock selected as USB PHY clock
  */
#define __HAL_RCC_GET_USBPHY_SOURCE() ((uint32_t)(READ_BIT(RCC->USBCKSELR, RCC_USBCKSELR_USBPHYSRC)))


/** @brief macro to configure the USB OTG clock (USBOCLK).
  *
  * @param  __USBOCLKSource__: specifies the USB OTG clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USBOCLKSOURCE_PLL4: PLL4_R Clock selected as USB OTG clock (default after reset)
  *            @arg RCC_USBOCLKSOURCE_PHY:  USB PHY Clock selected as USB OTG clock
  * @retval None
  */
#define __HAL_RCC_USBO_CONFIG(__USBOCLKSource__) \
                  MODIFY_REG(RCC->USBCKSELR, RCC_USBCKSELR_USBOSRC, (uint32_t)(__USBOCLKSource__))

/** @brief  macro to get the USB OTG PLL clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USBOCLKSOURCE_PLL4: PLL4_R Clock selected as USB OTG clock (default after reset)
  *            @arg RCC_USBOCLKSOURCE_PHY:  USB PHY Clock selected as USB OTG clock
  */
#define __HAL_RCC_GET_USBO_SOURCE() ((uint32_t)(READ_BIT(RCC->USBCKSELR, RCC_USBCKSELR_USBOSRC)))


/** @brief macro to configure the RNG1 clock (RNG1CLK).
  *
  * @param  __RNG1CLKSource__: specifies the RNG1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_RNG1CLKSOURCE_CSI:   CSI Clock selected as RNG1 clock (default after reset)
  *            @arg RCC_RNG1CLKSOURCE_PLL4:  PLL4_R Clock selected as RNG1 clock
  *            @arg RCC_RNG1CLKSOURCE_LSE:   LSE Clock selected as RNG1 clock
  *            @arg RCC_RNG1CLKSOURCE_LSI:   LSI Clock selected as RNG1 clock
  * @retval None
  */
#define __HAL_RCC_RNG1_CONFIG(__RNG1CLKSource__) \
                  MODIFY_REG(RCC->RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC, (uint32_t)(__RNG1CLKSource__))

/** @brief  macro to get the RNG1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_RNG1CLKSOURCE_CSI:   CSI Clock selected as RNG1 clock (default after reset)
  *            @arg RCC_RNG1CLKSOURCE_PLL4:  PLL4_R Clock selected as RNG1 clock
  *            @arg RCC_RNG1CLKSOURCE_LSE:   LSE Clock selected as RNG1 clock
  *            @arg RCC_RNG1CLKSOURCE_LSI:   LSI Clock selected as RNG1 clock
  */
#define __HAL_RCC_GET_RNG1_SOURCE() ((uint32_t)(READ_BIT(RCC->RNG1CKSELR, RCC_RNG1CKSELR_RNG1SRC)))


/** @brief macro to configure the RNG2 clock (RNG2CLK).
  *
  * @param  __RNG2CLKSource__: specifies the RNG2 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_RNG2CLKSOURCE_CSI:   CSI Clock selected as RNG2 clock (default after reset)
  *            @arg RCC_RNG2CLKSOURCE_PLL4:  PLL4_R Clock selected as RNG2 clock
  *            @arg RCC_RNG2CLKSOURCE_LSE:   LSE Clock selected as RNG2 clock
  *            @arg RCC_RNG2CLKSOURCE_LSI:   LSI Clock selected as RNG2 clock
  * @retval None
  */
#define __HAL_RCC_RNG2_CONFIG(__RNG2CLKSource__) \
                  MODIFY_REG(RCC->RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC, (uint32_t)(__RNG2CLKSource__))

/** @brief  macro to get the RNG2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_RNG2CLKSOURCE_CSI:   CSI Clock selected as RNG2 clock (default after reset)
  *            @arg RCC_RNG2CLKSOURCE_PLL4:  PLL4_R Clock selected as RNG2 clock
  *            @arg RCC_RNG2CLKSOURCE_LSE:   LSE Clock selected as RNG2 clock
  *            @arg RCC_RNG2CLKSOURCE_LSI:   LSI Clock selected as RNG2 clock
  */
#define __HAL_RCC_GET_RNG2_SOURCE() ((uint32_t)(READ_BIT(RCC->RNG2CKSELR, RCC_RNG2CKSELR_RNG2SRC)))


/** @brief macro to configure the CK_PER clock (CK_PERCLK).
  *
  * @param  __CKPERCLKSource__: specifies the CPER clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_CKPERCLKSOURCE_HSI:   HSI Clock selected as CK_PER clock (default after reset)
  *            @arg RCC_CKPERCLKSOURCE_CSI:   CSI Clock selected as CK_PER clock
  *            @arg RCC_CKPERCLKSOURCE_HSE:   HSE Clock selected as CK_PER clock
  *            @arg RCC_CKPERCLKSOURCE_OFF:   Clock disabled for CK_PER
  * @retval None
  */
#define __HAL_RCC_CKPER_CONFIG(__CKPERCLKSource__) \
                  MODIFY_REG(RCC->CPERCKSELR, RCC_CPERCKSELR_CKPERSRC, (uint32_t)(__CKPERCLKSource__))

/** @brief  macro to get the CPER clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_CKPERCLKSOURCE_HSI:   HSI Clock selected as CK_PER clock (default after reset)
  *            @arg RCC_CKPERCLKSOURCE_CSI:   CSI Clock selected as CK_PER clock
  *            @arg RCC_CKPERCLKSOURCE_HSE:   HSE Clock selected as CK_PER clock
  *            @arg RCC_CKPERCLKSOURCE_OFF:   Clock disabled for CK_PER
  */
#define __HAL_RCC_GET_CKPER_SOURCE() ((uint32_t)(READ_BIT(RCC->CPERCKSELR, RCC_CPERCKSELR_CKPERSRC)))


/** @brief macro to configure the STGEN clock (STGENCLK).
  *
  * @param  __STGENCLKSource__: specifies the STGEN clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_STGENCLKSOURCE_HSI:   HSI Clock selected as STGEN clock (default after reset)
  *            @arg RCC_STGENCLKSOURCE_HSE:   HSE Clock selected as STGEN clock
  *            @arg RCC_STGENCLKSOURCE_OFF:   Clock disabled
  *
  * @retval None
  */
#define __HAL_RCC_STGEN_CONFIG(__STGENCLKSource__) \
                  MODIFY_REG(RCC->STGENCKSELR, RCC_STGENCKSELR_STGENSRC, (uint32_t)(__STGENCLKSource__))

/** @brief  macro to get the STGEN clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_STGENCLKSOURCE_HSI:   HSI Clock selected as STGEN clock (default after reset)
  *            @arg RCC_STGENCLKSOURCE_HSE:   HSE Clock selected as STGEN clock
  *            @arg RCC_STGENCLKSOURCE_OFF:   Clock disabled
  */
#define __HAL_RCC_GET_STGEN_SOURCE() ((uint32_t)(READ_BIT(RCC->STGENCKSELR, RCC_STGENCKSELR_STGENSRC)))

#if defined(DSI)
/** @brief macro to configure the DSI clock (DSICLK).
  *
  * @param  __DSICLKSource__: specifies the DSI clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_DSICLKSOURCE_PHY:     DSIHOST clock from PHY is selected as DSI byte lane clock (default after reset)
  *            @arg RCC_DSICLKSOURCE_PLL4:    PLL4_P Clock selected as DSI byte lane clock
  * @retval None
  */
#define __HAL_RCC_DSI_CONFIG(__DSICLKSource__) \
                  MODIFY_REG(RCC->DSICKSELR, RCC_DSICKSELR_DSISRC, (uint32_t)(__DSICLKSource__))

/** @brief  macro to get the DSI clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_DSICLKSOURCE_PHY:     DSIHOST clock from PHY is selected as DSI byte lane clock (default after reset)
  *            @arg RCC_DSICLKSOURCE_PLL4:    PLL4_P Clock selected as DSI byte lane clock
  */
#define __HAL_RCC_GET_DSI_SOURCE() ((uint32_t)(READ_BIT(RCC->DSICKSELR, RCC_DSICKSELR_DSISRC)))
#endif /*DSI*/

/** @brief macro to configure the ADC clock (ADCCLK).
  *
  * @param  __ADCCLKSource__: specifies the ADC clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_ADCCLKSOURCE_PLL4:    PLL4_R Clock selected as ADC clock (default after reset)
  *            @arg RCC_ADCCLKSOURCE_PER:     PER Clock selected as ADC clock
  *            @arg RCC_ADCCLKSOURCE_PLL3:    PLL3_Q Clock selected as ADC clock
  * @retval None
  */
#define __HAL_RCC_ADC_CONFIG(__ADCCLKSource__) \
                  MODIFY_REG(RCC->ADCCKSELR, RCC_ADCCKSELR_ADCSRC, (uint32_t)(__ADCCLKSource__))

/** @brief  macro to get the ADC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_ADCCLKSOURCE_PLL4:    PLL4_R Clock selected as ADC clock (default after reset)
  *            @arg RCC_ADCCLKSOURCE_PER:     PER Clock selected as ADC clock
  *            @arg RCC_ADCCLKSOURCE_PLL3:    PLL3_Q Clock selected as ADC clock
  */
#define __HAL_RCC_GET_ADC_SOURCE() ((uint32_t)(READ_BIT(RCC->ADCCKSELR, RCC_ADCCKSELR_ADCSRC)))


/** @brief macro to configure the LPTIM1 clock (LPTIM1CLK).
  *
  * @param  __LPTIM1CLKSource__: specifies the LPTIM1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_LPTIM1CLKSOURCE_BCLK:    PCLK1 Clock selected as LPTIM1 clock (default after reset)
  *            @arg RCC_LPTIM1CLKSOURCE_PLL4:    PLL4_P Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_PLL3:    PLL3_Q Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSE:     LSE Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSI:     LSI Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_PER:     PER Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_OFF:     The kernel clock is disabled
  *
  * @retval None
  */
#define __HAL_RCC_LPTIM1_CONFIG(__LPTIM1CLKSource__) \
                  MODIFY_REG(RCC->LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC, (uint32_t)(__LPTIM1CLKSource__))

/** @brief  macro to get the LPTIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_LPTIM1CLKSOURCE_BCLK:    PCLK1 Clock selected as LPTIM1 clock (default after reset)
  *            @arg RCC_LPTIM1CLKSOURCE_PLL4:    PLL4_P Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_PLL3:    PLL3_Q Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSE:     LSE Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSI:     LSI Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_PER:     PER Clock selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_OFF:     The kernel clock is disabled
  */
#define __HAL_RCC_GET_LPTIM1_SOURCE() ((uint32_t)(READ_BIT(RCC->LPTIM1CKSELR, RCC_LPTIM1CKSELR_LPTIM1SRC)))

/** @brief macro to configure the LPTIM23 clock (LPTIM23CLK).
  *
  * @param  __LPTIM23CLKSource__: specifies the LPTIM23 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_LPTIM23CLKSOURCE_BCLK:    PCLK3 Clock selected as LPTIM23 clock (default after reset)
  *            @arg RCC_LPTIM23CLKSOURCE_PLL4:    PLL4_Q Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_PER:     PER Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_LSE:     LSE Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_LSI:     LSI Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_OFF:     The kernel clock is disabled
  * @retval None
  */
#define __HAL_RCC_LPTIM23_CONFIG(__LPTIM23CLKSource__) \
                  MODIFY_REG(RCC->LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC, (uint32_t)(__LPTIM23CLKSource__))

/** @brief  macro to get the LPTIM23 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_LPTIM23CLKSOURCE_BCLK:    PCLK3 Clock selected as LPTIM23 clock (default after reset)
  *            @arg RCC_LPTIM23CLKSOURCE_PLL4:    PLL4_Q Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_PER:     PER Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_LSE:     LSE Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_LSI:     LSI Clock selected as LPTIM23 clock
  *            @arg RCC_LPTIM23CLKSOURCE_OFF:     The kernel clock is disabled
  */
#define __HAL_RCC_GET_LPTIM23_SOURCE() ((uint32_t)(READ_BIT(RCC->LPTIM23CKSELR, RCC_LPTIM23CKSELR_LPTIM23SRC)))

/** @brief macro to configure the LPTIM45 clock (LPTIM45CLK).
  *
  * @param  __LPTIM45CLKSource__: specifies the LPTIM45 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_LPTIM45CLKSOURCE_BCLK:    PCLK3 Clock selected as LPTIM45 clock (default after reset)
  *            @arg RCC_LPTIM45CLKSOURCE_PLL4:    PLL4_P Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_PLL3:    PLL3_Q Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_LSE:     LSE Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_LSI:     LSI Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_PER:     PER Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_OFF:     The kernel clock is disabled
  * @retval None
  */
#define __HAL_RCC_LPTIM45_CONFIG(__LPTIM45CLKSource__) \
                  MODIFY_REG(RCC->LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC, (uint32_t)(__LPTIM45CLKSource__))

/** @brief  macro to get the LPTIM45 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_LPTIM45CLKSOURCE_BCLK:    PCLK3 Clock selected as LPTIM45 clock (default after reset)
  *            @arg RCC_LPTIM45CLKSOURCE_PLL4:    PLL4_P Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_PLL3:    PLL3_Q Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_LSE:     LSE Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_LSI:     LSI Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_PER:     PER Clock selected as LPTIM45 clock
  *            @arg RCC_LPTIM45CLKSOURCE_OFF:     The kernel clock is disabled
  */
#define __HAL_RCC_GET_LPTIM45_SOURCE() ((uint32_t)(READ_BIT(RCC->LPTIM45CKSELR, RCC_LPTIM45CKSELR_LPTIM45SRC)))



/**
  * @brief  Macro to set the APB1 timer clock prescaler
  * @note   Set and reset by software to control the clock frequency of all the timers connected to APB1 domain.
  *         It concerns TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM12, TIM13 and TIM14.
  * @param  __RCC_TIMG1PRES__: specifies the Timers clocks prescaler selection
  *          This parameter can be one of the following values:
  *              @arg RCC_TIMG1PRES_DEACTIVATED:  The Timers kernel clock is equal to ck_hclk if APB1DIV is corresponding
  *                                               to a division by 1 or 2, else it is equal to 2 x Fck_pclk1 (default after reset)
  *              @arg RCC_TIMG1PRES_ACTIVATED:    The Timers kernel clock is equal to ck_hclk if APB1DIV is corresponding
  *                                               to division by 1, 2 or 4, else it is equal to 4 x Fck_pclk1
  */
#define __HAL_RCC_TIMG1PRES(__RCC_TIMG1PRES__) \
                 do{  MODIFY_REG( RCC->TIMG1PRER, RCC_TIMG1PRER_TIMG1PRE , __RCC_TIMG1PRES__ );\
                 } while(0)

/** @brief  Macro to get the APB1 timer clock prescaler.
  * @retval The APB1 timer clock prescaler. The returned value can be one
  *         of the following:
  *             - RCC_TIMG1PRES_DEACTIVATED: The Timers kernel clock is equal to ck_hclk if APB1DIV is corresponding
  *                                           to a division by 1 or 2, else it is equal to 2 x Fck_pclk1 (default after reset)
  *             - RCC_TIMG1PRES_ACTIVATED:   The Timers kernel clock is equal to ck_hclk if APB1DIV is corresponding
  *                                          to division by 1, 2 or 4, else it is equal to 4 x Fck_pclk1
  */
#define __HAL_RCC_GET_TIMG1PRES() ((uint32_t)(RCC->TIMG1PRER & RCC_TIMG1PRER_TIMG1PRE))

/**
  * @brief  Macro to set the APB2 timers clocks prescaler
  * @note   Set and reset by software to control the clock frequency of all the timers connected to APB2 domain.
  *         It concerns TIM1, TIM8, TIM15, TIM16, and TIM17.
  * @param  __RCC_TIMG2PRES__: specifies the timers clocks prescaler selection
  *          This parameter can be one of the following values:
  *              @arg RCC_TIMG2PRES_DEACTIVATED:  The Timers kernel clock is equal to ck_hclk if APB2DIV is corresponding
  *                                               to a division by 1 or 2, else it is equal to 2 x Fck_pclk2 (default after reset)
  *              @arg RCC_TIMG2PRES_ACTIVATED:    The Timers kernel clock is equal to ck_hclk if APB2DIV is corresponding
  *                                               to division by 1, 2 or 4, else it is equal to 4 x Fck_pclk1
  */
#define __HAL_RCC_TIMG2PRES(__RCC_TIMG2PRES__) \
                 do{  MODIFY_REG( RCC->TIMG2PRER, RCC_TIMG2PRER_TIMG2PRE , __RCC_TIMG2PRES__ );\
                 } while(0)

/** @brief  Macro to get the APB2 timer clock prescaler.
  * @retval The APB2 timer clock prescaler. The returned value can be one
  *         of the following:
  *             - RCC_TIMG2PRES_DEACTIVATED: The Timers kernel clock is equal to ck_hclk if APB2DIV is corresponding
  *                                           to a division by 1 or 2, else it is equal to 2 x Fck_pclk1 (default after reset)
  *             - RCC_TIMG2PRES_ACTIVATED:   The Timers kernel clock is equal to ck_hclk if APB2DIV is corresponding
  *                                          to division by 1, 2 or 4, else it is equal to 4 x Fck_pclk1
  */
#define __HAL_RCC_GET_TIMG2PRES() ((uint32_t)(RCC->TIMG2PRER & RCC_TIMG2PRER_TIMG2PRE))



#define USB_PHY_VALUE    ((uint32_t)48000000) /*!< Value of the USB_PHY_VALUE signal in Hz
                                                   It is equal to rcc_hsphy_CLK_48M which is
                                                   a constant value */
/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Functions RCCEx Exported Functions
  * @{
  */

/** @addtogroup RCCEx_Exported_Functions_Group1
  * @{
  */

uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint64_t PeriphClk);
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
HAL_StatusTypeDef RCCEx_PLL2_Config(RCC_PLLInitTypeDef *pll2);
HAL_StatusTypeDef RCCEx_PLL3_Config(RCC_PLLInitTypeDef *pll3);
HAL_StatusTypeDef RCCEx_PLL4_Config(RCC_PLLInitTypeDef *pll4);

/**
  * @}
  */

/** @addtogroup RCCEx_Exported_Functions_Group2
  * @{
  */

void HAL_RCCEx_EnableLSECSS(void);
void HAL_RCCEx_DisableLSECSS(void);

#ifdef CORE_CA7
void HAL_RCCEx_EnableBootCore(uint32_t RCC_BootCx);
void HAL_RCCEx_DisableBootCore(uint32_t RCC_BootCx);
void HAL_RCCEx_HoldBootMCU(void);
void HAL_RCCEx_BootMCU(void);
#endif /* CORE_CA7 */

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

#endif /* __STM32MP1xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
