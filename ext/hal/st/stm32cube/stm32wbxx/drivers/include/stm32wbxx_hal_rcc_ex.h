/**
  ******************************************************************************
  * @file    stm32wbxx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extended module.
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
#ifndef STM32WBxx_HAL_RCC_EX_H
#define STM32WBxx_HAL_RCC_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"
#include "stm32wbxx_ll_crs.h"
#include "stm32wbxx_ll_exti.h"
#include "stm32wbxx_ll_pwr.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @addtogroup RCCEx
  * @{
  */
/* Private constants ---------------------------------------------------------*/
/** @addtogroup RCC_Private_Constants
  * @{
  */
/* CRS IT Error Mask */
#define  RCC_CRS_IT_ERROR_MASK                 ((uint32_t)(RCC_CRS_IT_TRIMOVF | RCC_CRS_IT_SYNCERR | RCC_CRS_IT_SYNCMISS))

/* CRS Flag Error Mask */
#define RCC_CRS_FLAG_ERROR_MASK                ((uint32_t)(RCC_CRS_FLAG_TRIMOVF | RCC_CRS_FLAG_SYNCERR | RCC_CRS_FLAG_SYNCMISS))

/**
  * @}
  */


/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCCEx_Private_Macros
  * @{
  */
#define IS_RCC_LSCO(__LSCOX__) ( ((__LSCOX__) == RCC_LSCO1) || ((__LSCOX__) == RCC_LSCO2) || ((__LSCOX__) == RCC_LSCO3) )

#define IS_RCC_LSCOSOURCE(__SOURCE__) (((__SOURCE__) == RCC_LSCOSOURCE_LSI) || \
                                       ((__SOURCE__) == RCC_LSCOSOURCE_LSE))


#define IS_RCC_PERIPHCLOCK(__SELECTION__)  \
               ((((__SELECTION__) & RCC_PERIPHCLK_USART1)   == RCC_PERIPHCLK_USART1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPUART1) == RCC_PERIPHCLK_LPUART1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2C1)      == RCC_PERIPHCLK_I2C1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2C3)     == RCC_PERIPHCLK_I2C3)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM1)   == RCC_PERIPHCLK_LPTIM1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM2)   == RCC_PERIPHCLK_LPTIM2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_SAI1)     == RCC_PERIPHCLK_SAI1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_USB)      == RCC_PERIPHCLK_USB)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RNG)      == RCC_PERIPHCLK_RNG)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_ADC)      == RCC_PERIPHCLK_ADC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RTC)      == RCC_PERIPHCLK_RTC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RFWAKEUP) == RCC_PERIPHCLK_RFWAKEUP)|| \
                (((__SELECTION__) & RCC_PERIPHCLK_SMPS)     == RCC_PERIPHCLK_SMPS))

#define IS_RCC_USART1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_USART1CLKSOURCE_PCLK2)  || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_SYSCLK) || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_LSE)    || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_HSI))


#define IS_RCC_LPUART1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPUART1CLKSOURCE_PCLK1)  || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_SYSCLK) || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_LSE)    || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_HSI))


#define IS_RCC_I2C1CLKSOURCE(__SOURCE__)   \
               (((__SOURCE__) == RCC_I2C1CLKSOURCE_PCLK1) || \
                ((__SOURCE__) == RCC_I2C1CLKSOURCE_SYSCLK)|| \
                ((__SOURCE__) == RCC_I2C1CLKSOURCE_HSI))

#define IS_RCC_I2C3CLKSOURCE(__SOURCE__)   \
               (((__SOURCE__) == RCC_I2C3CLKSOURCE_PCLK1) || \
                ((__SOURCE__) == RCC_I2C3CLKSOURCE_SYSCLK)|| \
                ((__SOURCE__) == RCC_I2C3CLKSOURCE_HSI))


#define IS_RCC_SAI1CLK(__SOURCE__)   \
               (((__SOURCE__) == RCC_SAI1CLKSOURCE_PLLSAI1) || \
                ((__SOURCE__) == RCC_SAI1CLKSOURCE_PLL) || \
                ((__SOURCE__) == RCC_SAI1CLKSOURCE_HSI)     || \
                ((__SOURCE__) == RCC_SAI1CLKSOURCE_PIN))


#define IS_RCC_LPTIM1CLK(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPTIM1CLKSOURCE_PCLK1) || \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_LSI)  || \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_HSI)  || \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_LSE))

#define IS_RCC_LPTIM2CLK(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPTIM2CLKSOURCE_PCLK1) || \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_LSI)  || \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_HSI)  || \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_LSE))

#define IS_RCC_RNGCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_RNGCLKSOURCE_CLK48)   || \
                ((__SOURCE__) == RCC_RNGCLKSOURCE_LSI) || \
                ((__SOURCE__) == RCC_RNGCLKSOURCE_LSE))

#define IS_RCC_USBCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_USBCLKSOURCE_HSI48)   || \
                ((__SOURCE__) == RCC_USBCLKSOURCE_PLLSAI1) || \
                ((__SOURCE__) == RCC_USBCLKSOURCE_PLL)     || \
                ((__SOURCE__) == RCC_USBCLKSOURCE_MSI))


#define IS_RCC_ADCCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_ADCCLKSOURCE_NONE) || \
                ((__SOURCE__) == RCC_ADCCLKSOURCE_PLL) || \
                ((__SOURCE__) == RCC_ADCCLKSOURCE_PLLSAI1) || \
                ((__SOURCE__) == RCC_ADCCLKSOURCE_SYSCLK))

#define IS_RCC_RFWKPCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_RFWKPCLKSOURCE_NONE) || \
                ((__SOURCE__) == RCC_RFWKPCLKSOURCE_LSE) || \
                ((__SOURCE__) == RCC_RFWKPCLKSOURCE_LSI) || \
                ((__SOURCE__) == RCC_RFWKPCLKSOURCE_HSE_DIV1024))

#define IS_RCC_SMPSCLKDIV(__DIV__)  \
               (((__DIV__) == RCC_SMPSCLKDIV_RANGE0) || \
                ((__DIV__) == RCC_SMPSCLKDIV_RANGE1) || \
                ((__DIV__) == RCC_SMPSCLKDIV_RANGE2) || \
                ((__DIV__) == RCC_SMPSCLKDIV_RANGE3))

#define IS_RCC_SMPSCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_SMPSCLKSOURCE_HSI)    || \
                ((__SOURCE__) == RCC_SMPSCLKSOURCE_MSI)    || \
                ((__SOURCE__) == RCC_SMPSCLKSOURCE_HSE))

#define IS_RCC_PLLSAI1N_VALUE(__VALUE__)   ((8U <= (__VALUE__)) && ((__VALUE__) <= 86U))

#define IS_RCC_PLLSAI1P_VALUE(__VALUE__)   ((RCC_PLLP_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLP_DIV32))

#define IS_RCC_PLLSAI1Q_VALUE(__VALUE__)   ((RCC_PLLQ_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLQ_DIV8))

#define IS_RCC_PLLSAI1R_VALUE(__VALUE__)   ((RCC_PLLR_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLR_DIV8))

#define IS_RCC_TRIMOSC(__VALUE__)          ((__VALUE__) == RCC_OSCILLATORTYPE_LSI2)

#define IS_RCC_CRS_SYNC_SOURCE(__SOURCE__) (((__SOURCE__) == RCC_CRS_SYNC_SOURCE_GPIO) || \
                                            ((__SOURCE__) == RCC_CRS_SYNC_SOURCE_LSE)  || \
                                            ((__SOURCE__) == RCC_CRS_SYNC_SOURCE_USB))

#define IS_RCC_CRS_SYNC_DIV(__DIV__)       (((__DIV__) == RCC_CRS_SYNC_DIV1)  || ((__DIV__) == RCC_CRS_SYNC_DIV2)  || \
                                            ((__DIV__) == RCC_CRS_SYNC_DIV4)  || ((__DIV__) == RCC_CRS_SYNC_DIV8)  || \
                                            ((__DIV__) == RCC_CRS_SYNC_DIV16) || ((__DIV__) == RCC_CRS_SYNC_DIV32) || \
                                            ((__DIV__) == RCC_CRS_SYNC_DIV64) || ((__DIV__) == RCC_CRS_SYNC_DIV128))

#define IS_RCC_CRS_SYNC_POLARITY(__POLARITY__) (((__POLARITY__) == RCC_CRS_SYNC_POLARITY_RISING) || \
                                                ((__POLARITY__) == RCC_CRS_SYNC_POLARITY_FALLING))

#define IS_RCC_CRS_RELOADVALUE(__VALUE__)  (((__VALUE__) <= 0xFFFFU))

#define IS_RCC_CRS_ERRORLIMIT(__VALUE__)   (((__VALUE__) <= 0xFFU))

#define IS_RCC_CRS_HSI48CALIBRATION(__VALUE__) (((__VALUE__) <= 0x3FU))

#define IS_RCC_CRS_FREQERRORDIR(__DIR__)   (((__DIR__) == RCC_CRS_FREQERRORDIR_UP) || \
                                            ((__DIR__) == RCC_CRS_FREQERRORDIR_DOWN))

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */

/**
  * @brief  PLLSAI1 Clock structure definition
  */
typedef struct
{

  uint32_t PLLN;             /*!< PLLN: specifies the multiplication factor for PLLSAI1 VCO output clock.
                                  This parameter must be a number between Min_Data=8 and Max_Data=86. */

  uint32_t PLLP;             /*!< PLLP: specifies the division factor for SAI clock.
                                  This parameter must be a value of @ref RCC_PLLP_Clock_Divider */

  uint32_t PLLQ;             /*!< PLLQ: specifies the division factor for USB/RNG clock.
                                  This parameter must be a value of @ref RCC_PLLQ_Clock_Divider */

  uint32_t PLLR;             /*!< PLLR: specifies the division factor for ADC clock.
                                  This parameter must be a value of @ref RCC_PLLR_Clock_Divider */

  uint32_t PLLSAI1ClockOut;  /*!< PLLSAI1ClockOut: specifies PLLSAI1 output clock to be enabled.
                                  This parameter must be a value of @ref RCC_PLLSAI1_Clock_Output */
} RCC_PLLSAI1InitTypeDef;


/**
  * @brief  RCC extended clocks structure definition
  */
typedef struct
{
  uint32_t PeriphClockSelection;   /*!< The Extended Clock to be configured.
                                        This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */


  RCC_PLLSAI1InitTypeDef PLLSAI1;  /*!< PLLSAI1 structure parameters.
                                        This parameter will be used only when PLLSAI1 is selected as Clock
                                        Source for SAI, USB/RNG or ADC */

  uint32_t Usart1ClockSelection;   /*!< Specifies USART1 clock source.
                                        This parameter can be a value of @ref RCCEx_USART1_Clock_Source */


  uint32_t Lpuart1ClockSelection;  /*!< Specifies LPUART1 clock source.
                                        This parameter can be a value of @ref RCCEx_LPUART1_Clock_Source */

  uint32_t I2c1ClockSelection;     /*!< Specifies I2C1 clock source.
                                        This parameter can be a value of @ref RCCEx_I2C1_Clock_Source */

  uint32_t I2c3ClockSelection;     /*!< Specifies I2C3 clock source.
                                        This parameter can be a value of @ref RCCEx_I2C3_Clock_Source */

  uint32_t Lptim1ClockSelection;   /*!< Specifies LPTIM1 clock source.
                                        This parameter can be a value of @ref RCCEx_LPTIM1_Clock_Source */

  uint32_t Lptim2ClockSelection;   /*!< Specifies LPTIM2 clock source.
                                        This parameter can be a value of @ref RCCEx_LPTIM2_Clock_Source */

  uint32_t Sai1ClockSelection;     /*!< Specifies SAI1 clock source.
                                        This parameter can be a value of @ref RCCEx_SAI1_Clock_Source */


  uint32_t UsbClockSelection;      /*!< Specifies USB clock source (warning: same source for RNG).
                                        This parameter can be a value of @ref RCCEx_USB_Clock_Source */

  uint32_t RngClockSelection;      /*!< Specifies RNG clock source (warning: same source for USB).
                                        This parameter can be a value of @ref RCCEx_RNG_Clock_Source */


  uint32_t AdcClockSelection;      /*!< Specifies ADC interface clock source.
                                        This parameter can be a value of @ref RCCEx_ADC_Clock_Source */

  uint32_t RTCClockSelection;      /*!< Specifies RTC clock source (also used for LCD).
                                        This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint32_t RFWakeUpClockSelection; /*!< Specifies RF Wake-up clock source.
                                        This parameter can be a value of @ref RCCEx_RFWKP_Clock_Source */

  uint32_t SmpsClockSelection;     /*!< Specifies SMPS clock source.
                                        This parameter can be a value of @ref RCCEx_SMPS_Clock_Source */

  uint32_t SmpsDivSelection;       /*!< Specifies SMPS clock division factor.
                                        This parameter can be a value of @ref RCCEx_SMPS_Clock_Divider */

} RCC_PeriphCLKInitTypeDef;


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
                                       It can be calculated in using macro __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)
                                       This parameter must be a number between Min_Data=0 and Max_Data=0xFFFF or a value of @ref RCCEx_CRS_ReloadValueDefault .*/

  uint32_t ErrorLimitValue;       /*!< Specifies the value to be used to evaluate the captured frequency error value.
                                       This parameter must be a number between Min_Data=0 and Max_Data=0xFF or a value of @ref RCCEx_CRS_ErrorLimitDefault */

  uint32_t HSI48CalibrationValue; /*!< Specifies a user-programmable trimming value to the HSI48 oscillator.
                                       This parameter must be a number between Min_Data=0 and Max_Data=0x3F or a value of @ref RCCEx_CRS_HSI48CalibrationDefault */

} RCC_CRSInitTypeDef;

/**
  * @brief RCC_CRS Synchronization structure definition
  */
typedef struct
{
  uint32_t ReloadValue;           /*!< Specifies the value loaded in the Counter reload value.
                                       This parameter must be a number between Min_Data=0 and Max_Data=0xFFFF */

  uint32_t HSI48CalibrationValue; /*!< Specifies value loaded in HSI48 oscillator smooth trimming.
                                       This parameter must be a number between Min_Data=0 and Max_Data=0x3F */

  uint32_t FreqErrorCapture;      /*!< Specifies the value loaded in the .FECAP, the frequency error counter
                                       value latched in the time of the last SYNC event.
                                       This parameter must be a number between Min_Data=0 and Max_Data=0xFFFF */

  uint32_t FreqErrorDirection;    /*!< Specifies the value loaded in the .FEDIR, the counting direction of the
                                       frequency error counter latched in the time of the last SYNC event.
                                       It shows whether the actual frequency is below or above the target.
                                       This parameter must be a value of @ref RCCEx_CRS_FreqErrorDirection*/

} RCC_CRSSynchroInfoTypeDef;


/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCC_LSCO_Index LSCO Index
  * @{
  */
#define RCC_LSCO1                       0x00000000U          /*!< LSCO1 index */
#define RCC_LSCO2                       0x00000001U          /*!< LSCO2 index */
#define RCC_LSCO3                       0x00000002U          /*!< LSCO3 index */
/**
  * @}
  */


/** @defgroup RCCEx_LSCO_Clock_Source Low Speed Clock Source
  * @{
  */
#define RCC_LSCOSOURCE_LSI             LL_RCC_LSCO_CLKSOURCE_LSI      /*!< LSI selection for low speed clock output */
#define RCC_LSCOSOURCE_LSE             LL_RCC_LSCO_CLKSOURCE_LSE      /*!< LSE selection for low speed clock output */
/**
  * @}
  */

/** @defgroup RCCEx_Periph_Clock_Selection Periph Clock Selection
  * @{
  */
#define RCC_PERIPHCLK_USART1           0x00000001U                    /*!< USART1 Peripheral Clock Selection      */
#define RCC_PERIPHCLK_LPUART1          0x00000002U                    /*!< LPUART1 Peripheral Clock Selection     */
#define RCC_PERIPHCLK_I2C1             0x00000004U                    /*!< I2C1 Peripheral Clock Selection        */
#define RCC_PERIPHCLK_I2C3             0x00000008U                    /*!< I2C3 Peripheral Clock Selection        */
#define RCC_PERIPHCLK_LPTIM1           0x00000010U                    /*!< LPTIM1 Peripheral Clock Selection      */
#define RCC_PERIPHCLK_LPTIM2           0x00000020U                    /*!< LPTIM2 Peripheral Clock Selection      */
#define RCC_PERIPHCLK_SAI1             0x00000040U                    /*!< SAI1 Peripheral Clock Selection        */
#define RCC_PERIPHCLK_USB              0x00000100U                    /*!< USB Peripheral Clock Selection         */
#define RCC_PERIPHCLK_RNG              0x00000200U                    /*!< RNG Peripheral Clock Selection         */
#define RCC_PERIPHCLK_ADC              0x00000400U                    /*!< ADC Peripheral Clock Selection         */
#define RCC_PERIPHCLK_RTC              0x00000800U                    /*!< RTC Peripheral Clock Selection         */
#define RCC_PERIPHCLK_RFWAKEUP         0x00001000U                    /*!< RF Wakeup Peripheral Clock Selection   */
#define RCC_PERIPHCLK_SMPS             0x00002000U                    /*!< SMPSS Peripheral Clock Selection       */
/**
  * @}
  */




/** @defgroup RCCEx_USART1_Clock_Source USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_PCLK2      LL_RCC_USART1_CLKSOURCE_PCLK2  /*!< APB2 clock selected as USART 1 clock*/
#define RCC_USART1CLKSOURCE_SYSCLK     LL_RCC_USART1_CLKSOURCE_SYSCLK /*!< SYSCLK clock selected as USART 1 clock*/
#define RCC_USART1CLKSOURCE_HSI        LL_RCC_USART1_CLKSOURCE_HSI    /*!< HSI clock selected as USART 1 clock*/
#define RCC_USART1CLKSOURCE_LSE        LL_RCC_USART1_CLKSOURCE_LSE    /*!< LSE clock selected as USART 1 clock*/
/**
  * @}
  */


/** @defgroup RCCEx_LPUART1_Clock_Source LPUART1 Clock Source
  * @{
  */
#define RCC_LPUART1CLKSOURCE_PCLK1     LL_RCC_LPUART1_CLKSOURCE_PCLK1  /*!< APB1 clock selected as LPUART 1 clock*/
#define RCC_LPUART1CLKSOURCE_SYSCLK    LL_RCC_LPUART1_CLKSOURCE_SYSCLK /*!< SYSCLK clock selected as LPUART 1 clock*/
#define RCC_LPUART1CLKSOURCE_HSI       LL_RCC_LPUART1_CLKSOURCE_HSI    /*!< HSI clock selected as LPUART 1 clock*/
#define RCC_LPUART1CLKSOURCE_LSE       LL_RCC_LPUART1_CLKSOURCE_LSE    /*!< LSE clock selected as LPUART 1 clock*/
/**
  * @}
  */

/** @defgroup RCCEx_I2C1_Clock_Source I2C1 Clock Source
  * @{
  */
#define RCC_I2C1CLKSOURCE_PCLK1        LL_RCC_I2C1_CLKSOURCE_PCLK1    /*!< APB1 clock selected as I2C1 clock */
#define RCC_I2C1CLKSOURCE_SYSCLK       LL_RCC_I2C1_CLKSOURCE_SYSCLK   /*!< SYSCLK clock selected as I2C1 clock */
#define RCC_I2C1CLKSOURCE_HSI          LL_RCC_I2C1_CLKSOURCE_HSI      /*!< HSI clock selected as I2C1 clock */
/**
  * @}
  */


/** @defgroup RCCEx_I2C3_Clock_Source I2C3 Clock Source
  * @{
  */
#define RCC_I2C3CLKSOURCE_PCLK1        LL_RCC_I2C3_CLKSOURCE_PCLK1   /*!< APB1 clock selected as I2C3 clock */
#define RCC_I2C3CLKSOURCE_SYSCLK       LL_RCC_I2C3_CLKSOURCE_SYSCLK  /*!< SYSCLK clock selected as I2C3 clock */
#define RCC_I2C3CLKSOURCE_HSI          LL_RCC_I2C3_CLKSOURCE_HSI     /*!< HSI clock selected as I2C3 clock */
/**
  * @}
  */


/** @defgroup RCCEx_SAI1_Clock_Source SAI1 Clock Source
  * @{
  */
#define RCC_SAI1CLKSOURCE_PLLSAI1      LL_RCC_SAI1_CLKSOURCE_PLLSAI1  /*!< PLLSAI "P" clock selected as SAI1 clock  */
#define RCC_SAI1CLKSOURCE_PLL          LL_RCC_SAI1_CLKSOURCE_PLL      /*!< PLL "P" clock selected as SAI1 clock     */
#define RCC_SAI1CLKSOURCE_HSI          LL_RCC_SAI1_CLKSOURCE_HSI      /*!< HSI clock selected as SAI1 clock         */
#define RCC_SAI1CLKSOURCE_PIN          LL_RCC_SAI1_CLKSOURCE_PIN      /*!< External PIN clock selected as SAI1 clock */
/**
  * @}
  */

/** @defgroup RCCEx_LPTIM1_Clock_Source LPTIM1 Clock Source
  * @{
  */
#define RCC_LPTIM1CLKSOURCE_PCLK1      LL_RCC_LPTIM1_CLKSOURCE_PCLK1  /*!< APB1 clock selected as LPTIM1 clock */
#define RCC_LPTIM1CLKSOURCE_LSI        LL_RCC_LPTIM1_CLKSOURCE_LSI    /*!< LSI clock selected as LPTIM1 clock  */
#define RCC_LPTIM1CLKSOURCE_HSI        LL_RCC_LPTIM1_CLKSOURCE_HSI    /*!< HSI clock selected as LPTIM1 clock  */
#define RCC_LPTIM1CLKSOURCE_LSE        LL_RCC_LPTIM1_CLKSOURCE_LSE    /*!< LSE clock selected as LPTIM1 clock  */
/**
  * @}
  */

/** @defgroup RCCEx_LPTIM2_Clock_Source LPTIM2 Clock Source
  * @{
  */
#define RCC_LPTIM2CLKSOURCE_PCLK1      LL_RCC_LPTIM2_CLKSOURCE_PCLK1  /*!< APB1 clock selected as LPTIM2 clock */
#define RCC_LPTIM2CLKSOURCE_LSI        LL_RCC_LPTIM2_CLKSOURCE_LSI    /*!< LSI clock selected as LPTIM1 clock  */
#define RCC_LPTIM2CLKSOURCE_HSI        LL_RCC_LPTIM2_CLKSOURCE_HSI    /*!< HSI clock selected as LPTIM1 clock  */
#define RCC_LPTIM2CLKSOURCE_LSE        LL_RCC_LPTIM2_CLKSOURCE_LSE    /*!< LSE clock selected as LPTIM1 clock  */
/**
  * @}
  */

/** @defgroup RCCEx_RNG_Clock_Source RNG Clock Source
  * @{
  */
#define RCC_RNGCLKSOURCE_CLK48         LL_RCC_RNG_CLKSOURCE_CLK48     /*!< CLK48 divided by 3 selected as RNG Clock */
#define RCC_RNGCLKSOURCE_LSI           LL_RCC_RNG_CLKSOURCE_LSI       /*!< LSI selected as ADC clock*/
#define RCC_RNGCLKSOURCE_LSE           LL_RCC_RNG_CLKSOURCE_LSE       /*!< LSE selected as ADC clock*/
/**
  * @}
  */

/** @defgroup RCCEx_USB_Clock_Source USB Clock Source
  * @{
  */

#define RCC_USBCLKSOURCE_HSI48         LL_RCC_USB_CLKSOURCE_HSI48     /*!< HSI48 clock selected as USB clock       */
#define RCC_USBCLKSOURCE_PLLSAI1       LL_RCC_USB_CLKSOURCE_PLLSAI1   /*!< PLLSAI1 "Q" clock selected as USB clock */
#define RCC_USBCLKSOURCE_PLL           LL_RCC_USB_CLKSOURCE_PLL       /*!< PLL "Q" clock selected as USB clock     */
#define RCC_USBCLKSOURCE_MSI           LL_RCC_USB_CLKSOURCE_MSI       /*!< MSI clock selected as USB clock         */

/**
  * @}
  */

/** @defgroup RCCEx_ADC_Clock_Source ADC Clock Source
  * @{
  */

#define RCC_ADCCLKSOURCE_NONE          LL_RCC_ADC_CLKSOURCE_NONE      /*!< None clock selected as ADC clock        */
#define RCC_ADCCLKSOURCE_PLLSAI1       LL_RCC_ADC_CLKSOURCE_PLLSAI1   /*!< PLLSAI1 "R" clock selected as ADC clock */
#define RCC_ADCCLKSOURCE_PLL           LL_RCC_ADC_CLKSOURCE_PLL       /*!< PLL "P" clock selected as ADC clock     */
#define RCC_ADCCLKSOURCE_SYSCLK        LL_RCC_ADC_CLKSOURCE_SYSCLK    /*!< SYSCLK clock selected as ADC clock      */

/**
  * @}
  */

/** @defgroup RCCEx_HCLK5_Clock_Source HCLK RF Clock Source
  * @{
  */

#define RCC_HCLK5SOURCE_HSI            0x00000001U  /*!< HSI clock not divided selected as Radio Domain clock */
#define RCC_HCLK5SOURCE_HSE            0x00000002U  /*!< HSE clock divided by 2 selected as Radio Domain clock */

/**
  * @}
  */

/** @defgroup RCCEx_RFWKP_Clock_Source RF WKP Clock Source
  * @{
  */

#define RCC_RFWKPCLKSOURCE_NONE          LL_RCC_RFWKP_CLKSOURCE_NONE  /*!< None clock selected as RF system wakeup clock                      */
#define RCC_RFWKPCLKSOURCE_LSE           LL_RCC_RFWKP_CLKSOURCE_LSE   /*!< LSE clock selected as RF system wakeup clock                       */
#define RCC_RFWKPCLKSOURCE_LSI           LL_RCC_RFWKP_CLKSOURCE_LSI   /*!< LSI clock selected as RF system wakeup clock                       */
#define RCC_RFWKPCLKSOURCE_HSE_DIV1024   LL_RCC_RFWKP_CLKSOURCE_HSE_DIV1024 /*!< HSE clock divided by 1024 selected as RF system wakeup clock */

/**
  * @}
  */



/** @defgroup RCCEx_SMPS_Clock_Source SMPS Clock Source
  * @{
  */

#define RCC_SMPSCLKSOURCE_HSI            LL_RCC_SMPS_CLKSOURCE_HSI     /*!< HSI selection as smps clock */
#define RCC_SMPSCLKSOURCE_MSI            LL_RCC_SMPS_CLKSOURCE_MSI     /*!< MSI selection as smps clock */
#define RCC_SMPSCLKSOURCE_HSE            LL_RCC_SMPS_CLKSOURCE_HSE     /*!< HSE selection as smps clock */

/**
  * @}
  */


/** @defgroup RCC_System_Clock_Source_Status System Clock Source Status
  * @{
  */
#define RCC_SMPSCLKSOURCE_STATUS_HSI    LL_RCC_SMPS_CLKSOURCE_STATUS_HSI     /*!< HSI selection as smps clock */
#define RCC_SMPSCLKSOURCE_STATUS_MSI    LL_RCC_SMPS_CLKSOURCE_STATUS_MSI     /*!< MSI selection as smps clock */
#define RCC_SMPSCLKSOURCE_STATUS_HSE    LL_RCC_SMPS_CLKSOURCE_STATUS_HSE     /*!< HSE selection as smps clock */


/**
  * @}
  */


/** @defgroup RCCEx_SMPS_Clock_Divider SMPS Clock Division Factor
  * @{
  */

#define RCC_SMPSCLKDIV_RANGE0           LL_RCC_SMPS_DIV_0            /*!< PLLM division factor = 0 */
#define RCC_SMPSCLKDIV_RANGE1           LL_RCC_SMPS_DIV_1            /*!< PLLM division factor = 1 */
#define RCC_SMPSCLKDIV_RANGE2           LL_RCC_SMPS_DIV_2            /*!< PLLM division factor = 2 */
#define RCC_SMPSCLKDIV_RANGE3           LL_RCC_SMPS_DIV_3            /*!< PLLM division factor = 3 */

/**
  * @}
  */

/** @defgroup RCCEx_EXTI_LINE_LSECSS  RCC LSE CSS external interrupt line
  * @{
  */
#define  RCC_EXTI_LINE_LSECSS          EXTI_IMR1_IM18       /*!< External interrupt line 18 connected to the LSE CSS EXTI Line */

/**
  * @}
  */


/** @defgroup RCCEx_CRS_Status RCCEx CRS Status
  * @{
  */
#define RCC_CRS_NONE                   0x00000000U            /*!< CRS status none                           */
#define RCC_CRS_TIMEOUT                0x00000001U            /*!< CRS status timeout                        */
#define RCC_CRS_SYNCOK                 0x00000002U            /*!< CRS status synchronization success        */
#define RCC_CRS_SYNCWARN               0x00000004U            /*!< CRS status synchronization warning        */
#define RCC_CRS_SYNCERR                0x00000008U            /*!< CRS status synchronization error          */
#define RCC_CRS_SYNCMISS               0x00000010U            /*!< CRS status synchronization missed         */
#define RCC_CRS_TRIMOVF                0x00000020U            /*!< CRS status trimming overflow or underflow */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroSource RCCEx CRS SynchroSource
  * @{
  */
#define RCC_CRS_SYNC_SOURCE_GPIO       LL_CRS_SYNC_SOURCE_GPIO             /*!< Synchro Signal source GPIO */
#define RCC_CRS_SYNC_SOURCE_LSE        LL_CRS_SYNC_SOURCE_LSE              /*!< Synchro Signal source LSE */
#define RCC_CRS_SYNC_SOURCE_USB        LL_CRS_SYNC_SOURCE_USB              /*!< Synchro Signal source USB SOF (default)*/
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroDivider RCCEx CRS SynchroDivider
  * @{
  */
#define RCC_CRS_SYNC_DIV1        LL_CRS_SYNC_DIV_1                        /*!< Synchro Signal not divided (default) */
#define RCC_CRS_SYNC_DIV2        LL_CRS_SYNC_DIV_2                        /*!< Synchro Signal divided by 2 */
#define RCC_CRS_SYNC_DIV4        LL_CRS_SYNC_DIV_4                        /*!< Synchro Signal divided by 4 */
#define RCC_CRS_SYNC_DIV8        LL_CRS_SYNC_DIV_8                        /*!< Synchro Signal divided by 8 */
#define RCC_CRS_SYNC_DIV16       LL_CRS_SYNC_DIV_16                       /*!< Synchro Signal divided by 16 */
#define RCC_CRS_SYNC_DIV32       LL_CRS_SYNC_DIV_32                       /*!< Synchro Signal divided by 32 */
#define RCC_CRS_SYNC_DIV64       LL_CRS_SYNC_DIV_64                       /*!< Synchro Signal divided by 64 */
#define RCC_CRS_SYNC_DIV128      LL_CRS_SYNC_DIV_128                      /*!< Synchro Signal divided by 128 */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroPolarity RCCEx CRS SynchroPolarity
  * @{
  */
#define RCC_CRS_SYNC_POLARITY_RISING   LL_CRS_SYNC_POLARITY_RISING        /*!< Synchro Active on rising edge (default) */
#define RCC_CRS_SYNC_POLARITY_FALLING  LL_CRS_SYNC_POLARITY_FALLING       /*!< Synchro Active on falling edge */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_ReloadValueDefault RCCEx CRS ReloadValueDefault
  * @{
  */
#define RCC_CRS_RELOADVALUE_DEFAULT    LL_CRS_RELOADVALUE_DEFAULT /*!< The reset value of the RELOAD field corresponds
                                                                       to a target frequency of 48 MHz and a synchronization signal frequency of 1 kHz (SOF signal from USB). */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_ErrorLimitDefault RCCEx CRS ErrorLimitDefault
  * @{
  */
#define RCC_CRS_ERRORLIMIT_DEFAULT     LL_CRS_ERRORLIMIT_DEFAULT /*!< Default Frequency error limit */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_HSI48CalibrationDefault RCCEx CRS HSI48CalibrationDefault
  * @{
  */
#define RCC_CRS_HSI48CALIBRATION_DEFAULT LL_CRS_HSI48CALIBRATION_DEFAULT /*!< The default value is 32, which corresponds to the middle of the trimming interval.
                                                                              The trimming step is around 67 kHz between two consecutive TRIM steps. A higher TRIM value
                                                                              corresponds to a higher output frequency */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_FreqErrorDirection RCCEx CRS FreqErrorDirection
  * @{
  */
#define RCC_CRS_FREQERRORDIR_UP        LL_CRS_FREQ_ERROR_DIR_UP   /*!< Upcounting direction, the actual frequency is above the target */
#define RCC_CRS_FREQERRORDIR_DOWN      LL_CRS_FREQ_ERROR_DIR_DOWN /*!< Downcounting direction, the actual frequency is below the target */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_Interrupt_Sources RCCEx CRS Interrupt Sources
  * @{
  */
#define RCC_CRS_IT_SYNCOK              LL_CRS_CR_SYNCOKIE       /*!< SYNC event OK */
#define RCC_CRS_IT_SYNCWARN            LL_CRS_CR_SYNCWARNIE     /*!< SYNC warning */
#define RCC_CRS_IT_ERR                 LL_CRS_CR_ERRIE          /*!< Error */
#define RCC_CRS_IT_ESYNC               LL_CRS_CR_ESYNCIE        /*!< Expected SYNC */
#define RCC_CRS_IT_SYNCERR             LL_CRS_CR_ERRIE          /*!< SYNC error */
#define RCC_CRS_IT_SYNCMISS            LL_CRS_CR_ERRIE          /*!< SYNC missed */
#define RCC_CRS_IT_TRIMOVF             LL_CRS_CR_ERRIE          /*!< Trimming overflow or underflow */

/**
  * @}
  */

/** @defgroup RCCEx_CRS_Flags RCCEx CRS Flags
  * @{
  */
#define RCC_CRS_FLAG_SYNCOK            LL_CRS_ISR_SYNCOKF       /*!< SYNC event OK flag     */
#define RCC_CRS_FLAG_SYNCWARN          LL_CRS_ISR_SYNCWARNF     /*!< SYNC warning flag      */
#define RCC_CRS_FLAG_ERR               LL_CRS_ISR_ERRF          /*!< Error flag        */
#define RCC_CRS_FLAG_ESYNC             LL_CRS_ISR_ESYNCF        /*!< Expected SYNC flag     */
#define RCC_CRS_FLAG_SYNCERR           LL_CRS_ISR_SYNCERR       /*!< SYNC error */
#define RCC_CRS_FLAG_SYNCMISS          LL_CRS_ISR_SYNCMISS      /*!< SYNC missed*/
#define RCC_CRS_FLAG_TRIMOVF           LL_CRS_ISR_TRIMOVF       /*!< Trimming overflow or underflow */

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

/*================================================================================================================*/

/**
  * @brief  Macro to configure the PLLSAI1 clock multiplication and division factors.
  *
  * @note   This function must be used only when the PLLSAI1 is disabled.
  * @note   PLLSAI1 clock source is common with the main PLL (configured through
  *         @ref __HAL_RCC_PLL_CONFIG() macro)
  *
  * @param  __PLLN__ specifies the multiplication factor for PLLSAI1 VCO output clock.
  *         This parameter must be a number between 4 and 86.
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 64 and 344 MHz.
  *         PLLSAI1 clock frequency = f(PLLSAI1) multiplied by PLLN
  *
  * @param  __PLLP__ specifies the division factor for SAI clock.
  *         This parameter must be a number in the range (RCC_PLLP_DIV2 to RCC_PLLP_DIV32).
  *         SAI clock frequency = f(PLLSAI1) / PLLP
  *
  * @param  __PLLQ__ specifies the division factor for USB/RNG clock.
  *         This parameter must be in the range (RCC_PLLQ_DIV2 to RCC_PLLQ_DIV8).
  *         USB/RNG clock frequency = f(PLLSAI1) / PLLQ
  *
  * @param  __PLLR__ specifies the division factor for SAR ADC clock.
  *         This parameter must be in the range (RCC_PLLR_DIV2 to RCC_PLLR_DIV8).
  *         ADC clock frequency = f(PLLSAI1) / PLLR
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_CONFIG(__PLLN__, __PLLP__, __PLLQ__, __PLLR__) \
                  MODIFY_REG(RCC->PLLSAI1CFGR, \
                             (RCC_PLLSAI1CFGR_PLLN | RCC_PLLSAI1CFGR_PLLP | RCC_PLLSAI1CFGR_PLLQ | RCC_PLLSAI1CFGR_PLLR), \
                             (((__PLLN__) << RCC_PLLCFGR_PLLN_Pos) | (__PLLP__) | (__PLLQ__) | (__PLLR__)))

/**
  * @brief  Macro to configure the PLLSAI1 clock multiplication factor N.
  *
  * @note   This function must be used only when the PLLSAI1 is disabled.
  * @note   PLLSAI1 clock source is common with the main PLL (configured through
  *         @ref __HAL_RCC_PLL_CONFIG() macro)
  *
  * @param  __PLLN__ specifies the multiplication factor for PLLSAI1 VCO output clock.
  *          This parameter must be a number between Min_Data=4 and Max_Data=86.
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 64 and 344 MHz.
  *         Use to set PLLSAI1 clock frequency = f(PLLSAI1) multiplied by PLLN
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_MULN_CONFIG(__PLLN__) \
                  MODIFY_REG(RCC->PLLSAI1CFGR, RCC_PLLSAI1CFGR_PLLN, (__PLLN__) << RCC_PLLSAI1CFGR_PLLN_Pos)


/** @brief  Macro to configure the PLLSAI1 clock division factor P.
  *
  * @note   This function must be used only when the PLLSAI1 is disabled.
  * @note   PLLSAI1 clock source is common with the main PLL (configured through
  *         @ref __HAL_RCC_PLL_CONFIG() macro)
  *
  * @param  __PLLP__  specifies the division factor for SAI clock.
  *                   This parameter must be a number in range (RCC_PLLP_DIV2 to RCC_PLLP_DIV32).
  *                   Use to set SAI clock frequency = f(PLLSAI1) / PLLP
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_DIVP_CONFIG(__PLLP__) \
                  MODIFY_REG(RCC->PLLSAI1CFGR, RCC_PLLSAI1CFGR_PLLP, (__PLLP__))


/** @brief  Macro to configure the PLLSAI1 clock division factor Q.
  *
  * @note   This function must be used only when the PLLSAI1 is disabled.
  * @note   PLLSAI1 clock source is common with the main PLL (configured through
  *         @ref __HAL_RCC_PLL_CONFIG() macro)
  *
  * @param  __PLLQ__  specifies the division factor for USB clock.
  *                   This parameter must be in the range (RCC_PLLQ_DIV2 to RCC_PLLQ_DIV8).
  *                   Use to set USB clock frequency = f(PLLSAI1) / PLLQ
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_DIVQ_CONFIG(__PLLQ__) \
                  MODIFY_REG(RCC->PLLSAI1CFGR, RCC_PLLSAI1CFGR_PLLQ, (__PLLQ__))

/** @brief  Macro to configure the PLLSAI1 clock division factor R.
  *
  * @note   This function must be used only when the PLLSAI1 is disabled.
  * @note   PLLSAI1 clock source is common with the main PLL (configured through
  *         @ref __HAL_RCC_PLL_CONFIG() macro)
  *
  * @param  __PLLR__  specifies the division factor for ADC clock.
  *                   This parameter must be in the range (RCC_PLLR_DIV2 to RCC_PLLR_DIV8).
  *                   Use to set ADC clock frequency = f(PLLSAI1) / PLLR
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_DIVR_CONFIG(__PLLR__) \
                  MODIFY_REG(RCC->PLLSAI1CFGR, RCC_PLLSAI1CFGR_PLLR, (__PLLR__))

/**
  * @brief  Macros to enable the PLLSAI1.
  * @note   The PLLSAI1 is disabled by hardware when entering STOP and STANDBY modes.
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_ENABLE()  LL_RCC_PLLSAI1_Enable()

/**
  * @brief  Macros to disable the PLLSAI1.
  * @note   The PLLSAI1 is disabled by hardware when entering STOP and STANDBY modes.
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_DISABLE() LL_RCC_PLLSAI1_Disable()

/**
  * @brief  Macros to enable each clock output (RCC_PLLSAI1_SAI1CLK, RCC_PLLSAI1_USBCLK and RCC_PLLSAI1_ADCCLK).
  * @note   Enabling and disabling those clocks can be done without the need to stop the PLL.
  *         This is mainly used to save Power.
  * @param  __PLLSAI1_CLOCKOUT__ specifies the PLLSAI1 clock to be output.
  *         This parameter can be one or a combination of the following values:
  *            @arg @ref RCC_PLLSAI1_SAI1CLK  This clock is used to generate an accurate clock to achieve high-quality audio performance on SAI interface
  *            @arg @ref RCC_PLLSAI1_ADCCLK  Clock used to clock ADC peripheral
  *            @arg @ref RCC_PLLSAI1_USBCLK  This clock is used to generate the clock for the USB Device (48 MHz)
  *            @arg @ref RCC_PLLSAI1_RNGCLK same as RCC_PLLSAI1_USBCLK
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1CLKOUT_ENABLE(__PLLSAI1_CLOCKOUT__)   SET_BIT(RCC->PLLSAI1CFGR, (__PLLSAI1_CLOCKOUT__))

/**
  * @brief  Macros to disable each clock output (RCC_PLLSAI1_SAI1CLK, RCC_PLLSAI1_USBCLK and RCC_PLLSAI1_ADCCLK).
  * @note   Enabling and disabling those clocks can be done without the need to stop the PLL.
  *         This is mainly used to save Power.
  * @param  __PLLSAI1_CLOCKOUT__ specifies the PLLSAI1 clock to be output.
  *         This parameter can be one or a combination of the following values:
  *            @arg @ref RCC_PLLSAI1_SAI1CLK  This clock is used to generate an accurate clock to achieve high-quality audio performance on SAI interface
  *            @arg @ref RCC_PLLSAI1_ADCCLK  Clock used to clock ADC peripheral
  *            @arg @ref RCC_PLLSAI1_USBCLK  This clock is used to generate the clock for the USB Device (48 MHz)
  *            @arg @ref RCC_PLLSAI1_RNGCLK  same as RCC_PLLSAI1_USBCLK
  *
  * @retval None
  */
#define __HAL_RCC_PLLSAI1CLKOUT_DISABLE(__PLLSAI1_CLOCKOUT__)  CLEAR_BIT(RCC->PLLSAI1CFGR, (__PLLSAI1_CLOCKOUT__))

/**
  * @brief  Macro to get clock output enable status (RCC_PLLSAI1_SAI1CLK, RCC_PLLSAI1_USBCLK and RCC_PLLSAI1_ADCCLK).
  * @param  __PLLSAI1_CLOCKOUT__ specifies the PLLSAI1 clock to be output.
  *         This parameter can be one or a combination of the following values:
  *            @arg @ref RCC_PLLSAI1_SAI1CLK  This clock is used to generate an accurate clock to achieve high-quality audio performance on SAI interface
  *            @arg @ref RCC_PLLSAI1_ADCCLK  Clock used to clock ADC peripheral
  *            @arg @ref RCC_PLLSAI1_USBCLK  This clock is used to generate the clock for the USB Device (48 MHz)
  *            @arg @ref RCC_PLLSAI1_RNGCLK  same as RCC_PLLSAI1_USBCLK
  * @retval SET / RESET
  */
#define __HAL_RCC_GET_PLLSAI1CLKOUT_CONFIG(__PLLSAI1_CLOCKOUT__)  READ_BIT(RCC->PLLSAI1CFGR, (__PLLSAI1_CLOCKOUT__))


/**
  * @brief  Macro to configure the SAI1 clock source.
  * @param  __SAI1_CLKSOURCE__ defines the SAI1 clock source. This clock is derived
  *         from the PLLSAI1, system PLL, HSI or external clock (through a dedicated pin).
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_SAI1CLKSOURCE_PLLSAI1    SAI1 clock = PLLSAI1 "P" clock
  *             @arg @ref RCC_SAI1CLKSOURCE_PLL        SAI1 clock  = PLL "P" clock
  *             @arg @ref RCC_SAI1CLKSOURCE_HSI        SAI1 clock  = HSI clock
  *             @arg @ref RCC_SAI1CLKSOURCE_PIN        SAI1 clock = External Clock (SAI1_EXTCLK)
  *
  * @retval None
  */
#define __HAL_RCC_SAI1_CONFIG(__SAI1_CLKSOURCE__)  LL_RCC_SetSAIClockSource(__SAI1_CLKSOURCE__)


/** @brief  Macro to get the SAI1 clock source.
  * @retval The clock source can be one of the following values:
  *             @arg @ref RCC_SAI1CLKSOURCE_PLLSAI1     SAI1 clock = PLLSAI1 "P" clock
  *             @arg @ref RCC_SAI1CLKSOURCE_PLL         SAI1 clock  = PLL "P" clock
  *             @arg @ref RCC_SAI1CLKSOURCE_HSI         SAI1 clock  = HSI clock
  *             @arg @ref RCC_SAI1CLKSOURCE_PIN         SAI1 clock = External Clock (SAI1_EXTCLK)
  *
  * @retval None
  */
#define __HAL_RCC_GET_SAI1_SOURCE()  LL_RCC_GetSAIClockSource(LL_RCC_SAI1_CLKSOURCE)

/** @brief  Macro to configure the I2C1 clock (I2C1CLK).
  *
  * @param  __I2C1_CLKSOURCE__ specifies the I2C1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_PCLK1   PCLK1 selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI     HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK  System Clock selected as I2C1 clock
  * @retval None
  */
#define __HAL_RCC_I2C1_CONFIG(__I2C1_CLKSOURCE__) LL_RCC_SetI2CClockSource(__I2C1_CLKSOURCE__)

/** @brief  Macro to get the I2C1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_PCLK1   PCLK1 selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI     HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK  System Clock selected as I2C1 clock
  */
#define __HAL_RCC_GET_I2C1_SOURCE() LL_RCC_GetI2CClockSource(LL_RCC_I2C1_CLKSOURCE)

/** @brief  Macro to configure the I2C3 clock (I2C3CLK).
  *
  * @param  __I2C3_CLKSOURCE__ specifies the I2C3 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2C3CLKSOURCE_PCLK1   PCLK1 selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_HSI     HSI selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_SYSCLK  System Clock selected as I2C3 clock
  * @retval None
  */
#define __HAL_RCC_I2C3_CONFIG(__I2C3_CLKSOURCE__) LL_RCC_SetI2CClockSource(__I2C3_CLKSOURCE__)

/** @brief  Macro to get the I2C3 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C3CLKSOURCE_PCLK1   PCLK1 selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_HSI     HSI selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_SYSCLK  System Clock selected as I2C3 clock
  */
#define __HAL_RCC_GET_I2C3_SOURCE() LL_RCC_GetI2CClockSource(LL_RCC_I2C3_CLKSOURCE)

/** @brief  Macro to configure the USART1 clock (USART1CLK).
  *
  * @param  __USART1_CLKSOURCE__ specifies the USART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK2   PCLK2 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI     HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK  System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE     LSE selected as USART1 clock
  * @retval None
  */
#define __HAL_RCC_USART1_CONFIG(__USART1_CLKSOURCE__) LL_RCC_SetUSARTClockSource(__USART1_CLKSOURCE__)

/** @brief  Macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK2   PCLK2 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI     HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK  System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE     LSE selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() LL_RCC_GetUSARTClockSource(LL_RCC_USART1_CLKSOURCE)

/** @brief  Macro to configure the LPUART clock (LPUARTCLK).
  *
  * @param  __LPUART_CLKSOURCE__ specifies the LPUART clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1   PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI     HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK  System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE     LSE selected as LPUART1 clock
  * @retval None
  */
#define __HAL_RCC_LPUART1_CONFIG(__LPUART_CLKSOURCE__) LL_RCC_SetLPUARTClockSource(__LPUART_CLKSOURCE__)

/** @brief  Macro to get the LPUART clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1   PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI     HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK  System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE     LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPUART1_SOURCE() LL_RCC_GetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE)

/** @brief  Macro to configure the LPTIM1 clock (LPTIM1CLK).
  *
  * @param  __LPTIM1_CLKSOURCE__ specifies the LPTIM1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK1  PCLK selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI   HSI selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI   LSI selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE   LSE selected as LPTIM1 clock
  * @retval None
  */
#define __HAL_RCC_LPTIM1_CONFIG(__LPTIM1_CLKSOURCE__) LL_RCC_SetLPTIMClockSource(__LPTIM1_CLKSOURCE__)

/** @brief  Macro to get the LPTIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK1  PCLK selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI   HSI selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI   System Clock selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE   LSE selected as LPTIM1 clock
  */
#define __HAL_RCC_GET_LPTIM1_SOURCE() LL_RCC_GetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE)

/** @brief  Macro to configure the LPTIM2 clock (LPTIM2CLK).
  *
  * @param  __LPTIM2_CLKSOURCE__ specifies the LPTIM2 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPTIM2CLKSOURCE_PCLK1  PCLK selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSI   HSI selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_HSI   LSI selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSE   LSE selected as LPTIM2 clock
  * @retval None
  */
#define __HAL_RCC_LPTIM2_CONFIG(__LPTIM2_CLKSOURCE__) LL_RCC_SetLPTIMClockSource(__LPTIM2_CLKSOURCE__)

/** @brief  Macro to get the LPTIM2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPTIM2CLKSOURCE_PCLK1  PCLK selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSI   HSI selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_HSI   System Clock selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSE   LSE selected as LPTIM2 clock
  */
#define __HAL_RCC_GET_LPTIM2_SOURCE() LL_RCC_GetLPTIMClockSource(LL_RCC_LPTIM2_CLKSOURCE)


/** @brief  Macro to configure the RNG clock.
  *
  * @note  USB and RNG peripherals share the same 48MHz clock source.
  *
  * @param  __RNG_CLKSOURCE__ specifies the RNG clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_CLK48    CLK48 divided by 3 selected as RNG Clock
  *            @arg @ref RCC_RNGCLKSOURCE_LSI      LSI selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_LSE      LSE selected as RNG clock
  * @retval None
  */
#define __HAL_RCC_RNG_CONFIG(__RNG_CLKSOURCE__)  LL_RCC_SetRNGClockSource(__RNG_CLKSOURCE__)

/** @brief  Macro to get the RNG clock.  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_CLK48    CLK48 divided by 3 selected as RNG Clock
  *            @arg @ref RCC_RNGCLKSOURCE_LSI      LSI selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_LSE      LSE selected as RNG clock
  */
#define __HAL_RCC_GET_RNG_SOURCE() LL_RCC_GetRNGClockSource(LL_RCC_RNG_CLKSOURCE)
/** @brief  Macro to configure the USB clock (USBCLK).
  *
  * @note  USB and RNG peripherals share the same 48MHz clock source.
  *
  * @param  __USB_CLKSOURCE__ specifies the USB clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_USBCLKSOURCE_HSI48    HSI48 selected as 48MHz clock for devices with HSI48
  *            @arg @ref RCC_USBCLKSOURCE_MSI      MSI selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLLSAI1  PLLSAI1 "Q" clock (PLL48M2CLK) selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLL      PLL "Q" clock (PLL48M1CLK) selected as USB clock
  * @retval None
  */
#define __HAL_RCC_USB_CONFIG(__USB_CLKSOURCE__)  LL_RCC_SetUSBClockSource(__USB_CLKSOURCE__)

/** @brief  Macro to get the USB clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USBCLKSOURCE_HSI48    HSI48 selected as 48MHz clock for devices with HSI48
  *            @arg @ref RCC_USBCLKSOURCE_MSI      MSI selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLLSAI1  PLLSAI1 "Q" clock (PLL48M2CLK) selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLL      PLL "Q" clock (PLL48M1CLK) selected as USB clock
  */
#define __HAL_RCC_GET_USB_SOURCE()  LL_RCC_GetUSBClockSource(LL_RCC_USB_CLKSOURCE)

/** @brief  Macro to configure the ADC interface clock.
  * @param  __ADC_CLKSOURCE__ specifies the ADC digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_ADCCLKSOURCE_NONE    No clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_PLLSAI1  PLLSAI1 Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_PLL  PLL Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_SYSCLK  System Clock selected as ADC clock
  * @retval None
  */
#define __HAL_RCC_ADC_CONFIG(__ADC_CLKSOURCE__)  LL_RCC_SetADCClockSource(__ADC_CLKSOURCE__)

/** @brief  Macro to get the ADC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_ADCCLKSOURCE_NONE    No clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_PLLSAI1  PLLSAI1 Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_PLL  PLL Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_SYSCLK  System Clock selected as ADC clock
  */
#define __HAL_RCC_GET_ADC_SOURCE() LL_RCC_GetADCClockSource(LL_RCC_ADC_CLKSOURCE)

/** @brief  Macro to configure the RFWKP interface clock.
  * @param  __RFWKP_CLKSOURCE__ specifies the RFWKP digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RFWKPCLKSOURCE_NONE         No clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_LSE          LSE Clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_LSI          LSI Clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_HSE_DIV1024  HSE div1024 Clock selected as RFWKP clock
  * @retval None
  */
#define __HAL_RCC_RFWAKEUP_CONFIG(__RFWKP_CLKSOURCE__)  LL_RCC_SetRFWKPClockSource(__RFWKP_CLKSOURCE__)

/** @brief  Macro to get the RFWKP clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RFWKPCLKSOURCE_NONE         No clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_LSE          LSE Clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_LSI          LSI Clock selected as RFWKP clock
  *            @arg @ref RCC_RFWKPCLKSOURCE_HSE_DIV1024  HSE div1024 Clock selected as RFWKP clock
  */
#define __HAL_RCC_GET_RFWAKEUP_SOURCE()  LL_RCC_GetRFWKPClockSource()

/** @brief  Macro to configure the SMPS clock division factor.
  *
  * @param  __SMPSCLKDIV__ specifies the division factor for SMPS clock.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_SMPSCLKDIV_RANGE0           1st divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE1           2nd divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE2           3th divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE3           4th divider factor value
  *
  * @note divider value predefined by HW depending of SMPS clock source
  *
  * @retval None
  */
#define __HAL_RCC_SMPS_DIV_CONFIG(__SMPSCLKDIV__) LL_RCC_SetSMPSPrescaler(__SMPSCLKDIV__)

/** @brief  Macro to get the SMPS clock division factor.
  *
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_SMPSCLKDIV_RANGE0           1st divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE1           2nd divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE2           3th divider factor value
  *            @arg @ref RCC_SMPSCLKDIV_RANGE3           4th divider factor value
  *
  */
#define __HAL_RCC_GET_SMPS_DIV() LL_RCC_GetSMPSPrescaler()

/** @brief  Macro to configure the SMPS interface clock.
  * @param  __SMPS_CLKSOURCE__ specifies the SMPS digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_SMPSCLKSOURCE_HSI          HSI clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_MSI          MSI Clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_HSE          HSE Clock selected as SMPS clock
  * @retval None
  */

#define __HAL_RCC_SMPS_CONFIG(__SMPS_CLKSOURCE__) LL_RCC_SetSMPSClockSource(__SMPS_CLKSOURCE__)

/** @brief  Macro to get the SMPS clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_SMPSCLKSOURCE_HSI          HSI clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_MSI          MSI Clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_HSE          HSE Clock selected as SMPS clock
  */
#define __HAL_RCC_GET_SMPS_SOURCE() LL_RCC_GetSMPSClockSelection()

/** @brief  Macro to get the SMPS clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_SMPSCLKSOURCE_STATUS_HSI   HSI clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_STATUS_MSI   MSI Clock selected as SMPS clock
  *            @arg @ref RCC_SMPSCLKSOURCE_STATUS_HSE   HSE Clock selected as SMPS clock
  */
#define __HAL_RCC_GET_SMPS_SOURCE_STATUS() LL_RCC_GetSMPSClockSource()

/** @defgroup RCCEx_Flags_Interrupts_Management Flags Interrupts Management
  * @brief macros to manage the specified RCC Flags and interrupts.
  * @{
  */

/** @brief Enable PLLSAI1RDY interrupt.
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_ENABLE_IT() LL_RCC_EnableIT_PLLSAI1RDY()

/** @brief Disable PLLSAI1RDY interrupt.
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_DISABLE_IT() LL_RCC_DisableIT_PLLSAI1RDY()

/** @brief Clear the PLLSAI1RDY interrupt pending bit.
  * @retval None
  */
#define __HAL_RCC_PLLSAI1_CLEAR_IT() LL_RCC_ClearFlag_PLLSAI1RDY()

/** @brief Check whether PLLSAI1RDY interrupt has occurred or not.
  * @retval TRUE or FALSE.
  */
#define __HAL_RCC_PLLSAI1_GET_IT_SOURCE() LL_RCC_IsActiveFlag_PLLSAI1RDY()

/** @brief  Check whether the PLLSAI1RDY flag is set or not.
  * @retval TRUE or FALSE.
  */
#define __HAL_RCC_PLLSAI1_GET_FLAG()  LL_RCC_PLLSAI1_IsReady()

/**
  * @brief Enable the RCC LSE CSS Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_IT()            LL_EXTI_EnableIT_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Enable the RCC LSE CSS Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_C2_RCC_LSECSS_EXTI_ENABLE_IT()          LL_C2_EXTI_EnableIT_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt C1 Line.
  * @retval None
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_IT()           LL_EXTI_DisableIT_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt C2 Line.
  * @retval None
  */
#define __HAL_C2_RCC_LSECSS_EXTI_DISABLE_IT()        LL_C2_EXTI_DisableIT_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Enable the RCC LSE CSS Event C1 Line.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_EVENT()         LL_EXTI_EnableEvent_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Enable the RCC LSE CSS Event C2 Line.
  * @retval None.
  */
#define __HAL_C2_RCC_LSECSS_EXTI_ENABLE_EVENT()      LL_C2_EXTI_EnableEvent_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Event C1 Line.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_EVENT()        LL_EXTI_DisableEvent_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Event C2 Line.
  * @retval None.
  */
#define __HAL_C2_RCC_LSECSS_EXTI_DISABLE_EVENT()     LL_C2_EXTI_DisableEvent_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief  Enable the RCC LSE CSS Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_FALLING_EDGE()  LL_EXTI_EnableFallingTrig_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableFallingTrig_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief  Enable the RCC LSE CSS Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_EDGE()   LL_EXTI_EnableRisingTrig_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_EDGE()  LL_EXTI_DisableRisingTrig_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Enable the RCC LSE CSS Extended Interrupt Rising & Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                      \
    __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_RCC_LSECSS_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Rising & Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                       \
    __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_RCC_LSECSS_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0)

/**
  * @brief Check whether the specified RCC LSE CSS EXTI interrupt flag is set or not.
  * @retval EXTI RCC LSE CSS Line Status.
  */
#define __HAL_RCC_LSECSS_EXTI_GET_FLAG()       LL_EXTI_IsActiveFlag_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Clear the RCC LSE CSS EXTI flag.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_CLEAR_FLAG()    LL_EXTI_ClearFlag_0_31(RCC_EXTI_LINE_LSECSS)

/**
  * @brief Generate a Software interrupt on the RCC LSE CSS EXTI line.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_GENERATE_SWIT()  LL_EXTI_GenerateSWI_0_31(RCC_EXTI_LINE_LSECSS)

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
                                               } while(0)

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
                                                 if(((__FLAG__) & RCC_CRS_FLAG_ERROR_MASK) != 0U) \
                                                 { \
                                                   WRITE_REG(CRS->ICR, CRS_ICR_ERRC | ((__FLAG__) & ~RCC_CRS_FLAG_ERROR_MASK)); \
                                                 } \
                                                 else \
                                                 { \
                                                   WRITE_REG(CRS->ICR, (__FLAG__)); \
                                                 } \
                                               } while(0)

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
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_ENABLE()  LL_CRS_EnableFreqErrorCounter()

/**
  * @brief  Disable the oscillator clock for frequency error counter.
  * @retval None
  */
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_DISABLE() LL_CRS_DisableFreqErrorCounter()

/**
  * @brief  Enable the automatic hardware adjustement of TRIM bits.
  * @note   When the AUTOTRIMEN bit is set the CRS_CFGR register becomes write-protected.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_ENABLE()     LL_CRS_EnableAutoTrimming()

/**
  * @brief  Enable or disable the automatic hardware adjustement of TRIM bits.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_DISABLE()    LL_CRS_DisableAutoTrimming()

/**
  * @brief  Macro to calculate reload value to be set in CRS register according to target and sync frequencies
  * @note   The RELOAD value should be selected according to the ratio between the target frequency and the frequency
  *             of the synchronization source after prescaling. It is then decreased by one in order to
  *             reach the expected synchronization on the zero value. The formula is the following:
  *             RELOAD = (fTARGET / fSYNC) -1
  * @param  __FTARGET__ Target frequency (value in Hz)
  * @param  __FSYNC__ Synchronization signal frequency (value in Hz)
  * @retval None
  */
#define __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)  __LL_CRS_CALC_CALCULATE_RELOADVALUE((__FTARGET__),(__FSYNC__))

/**
  * @}
  */

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

HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void              HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
uint32_t          HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk);

/**
  * @}
  */

/** @addtogroup RCCEx_Exported_Functions_Group2
  * @{
  */

HAL_StatusTypeDef HAL_RCCEx_EnablePLLSAI1(RCC_PLLSAI1InitTypeDef  *PLLSAI1Init);
HAL_StatusTypeDef HAL_RCCEx_DisablePLLSAI1(void);

void              HAL_RCCEx_WakeUpStopCLKConfig(uint32_t WakeUpClk);

void              HAL_RCCEx_EnableLSECSS(void);
void              HAL_RCCEx_DisableLSECSS(void);
void              HAL_RCCEx_EnableLSECSS_IT(void);
void              HAL_RCCEx_LSECSS_IRQHandler(void);
void              HAL_RCCEx_LSECSS_Callback(void);

void              HAL_RCCEx_LSCOConfig(uint32_t RCC_LSCOx, uint32_t RCC_LSCOSource);
void              HAL_RCCEx_EnableLSCO(uint32_t LSCOSource);
void              HAL_RCCEx_DisableLSCO(void);

void              HAL_RCCEx_EnableMSIPLLMode(void);
void              HAL_RCCEx_DisableMSIPLLMode(void);

HAL_StatusTypeDef   HAL_RCCEx_TrimOsc(uint32_t OscillatorType);

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

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
