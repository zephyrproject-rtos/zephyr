/**
  ******************************************************************************
  * @file    stm32g0xx_hal_rcc.h
  * @author  MCD Application Team
   * @brief   Header file of RCC HAL module.
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
#ifndef STM32G0xx_HAL_RCC_H
#define STM32G0xx_HAL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup RCC
  * @{
  */

/* Private constants ---------------------------------------------------------*/
/** @addtogroup RCC_Private_Constants
  * @{
  */
/* Defines used for Flags */
#define CR_REG_INDEX              1U
#define BDCR_REG_INDEX            2U
#define CSR_REG_INDEX             3U

#define RCC_FLAG_MASK             0x1FU

/* Define used for IS_RCC_CLOCKTYPE() */
#define RCC_CLOCKTYPE_ALL              (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1)  /*!< All clocktype to configure */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCC_Private_Macros
  * @{
  */

#define IS_RCC_OSCILLATORTYPE(__OSCILLATOR__) (((__OSCILLATOR__) == RCC_OSCILLATORTYPE_NONE)                           || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE))

#define IS_RCC_HSE(__HSE__)  (((__HSE__) == RCC_HSE_OFF) || ((__HSE__) == RCC_HSE_ON) || \
                              ((__HSE__) == RCC_HSE_BYPASS))

#define IS_RCC_LSE(__LSE__)  (((__LSE__) == RCC_LSE_OFF) || ((__LSE__) == RCC_LSE_ON) || \
                              ((__LSE__) == RCC_LSE_BYPASS))

#define IS_RCC_HSI(__HSI__)  (((__HSI__) == RCC_HSI_OFF) || ((__HSI__) == RCC_HSI_ON))

#define IS_RCC_HSI_CALIBRATION_VALUE(__VALUE__) ((__VALUE__) <= (uint32_t)127U)

#define IS_RCC_HSIDIV(__DIV__) (((__DIV__) == RCC_HSI_DIV1)  || ((__DIV__) == RCC_HSI_DIV2) || \
                                ((__DIV__) == RCC_HSI_DIV4)  || ((__DIV__) == RCC_HSI_DIV8) || \
                                ((__DIV__) == RCC_HSI_DIV16) || ((__DIV__) == RCC_HSI_DIV32)|| \
                                ((__DIV__) == RCC_HSI_DIV64) || ((__DIV__) == RCC_HSI_DIV128))

#define IS_RCC_LSI(__LSI__)  (((__LSI__) == RCC_LSI_OFF) || ((__LSI__) == RCC_LSI_ON))

#define IS_RCC_PLL(__PLL__) (((__PLL__) == RCC_PLL_NONE) ||((__PLL__) == RCC_PLL_OFF) || \
                             ((__PLL__) == RCC_PLL_ON))

#define IS_RCC_PLLSOURCE(__SOURCE__) (((__SOURCE__) == RCC_PLLSOURCE_NONE) || \
                                      ((__SOURCE__) == RCC_PLLSOURCE_HSI)  || \
                                      ((__SOURCE__) == RCC_PLLSOURCE_HSE))

#define IS_RCC_PLLM_VALUE(__VALUE__) ((__VALUE__) <= RCC_PLLM_DIV8)

#define IS_RCC_PLLN_VALUE(__VALUE__) ((8U <= (__VALUE__)) && ((__VALUE__) <= 86U))

#define IS_RCC_PLLP_VALUE(__VALUE__) ((RCC_PLLP_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLP_DIV32))

#if defined(RCC_PLLQ_SUPPORT)
#define IS_RCC_PLLQ_VALUE(__VALUE__) ((RCC_PLLQ_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLQ_DIV8))
#endif

#define IS_RCC_PLLR_VALUE(__VALUE__) ((RCC_PLLR_DIV2 <= (__VALUE__)) && ((__VALUE__) <= RCC_PLLR_DIV8))

#define IS_RCC_CLOCKTYPE(__CLK__)  ((((__CLK__) & RCC_CLOCKTYPE_ALL) != 0x00UL) && (((__CLK__) & ~RCC_CLOCKTYPE_ALL) == 0x00UL))

#define IS_RCC_SYSCLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_SYSCLKSOURCE_HSI)  || \
                                          ((__SOURCE__) == RCC_SYSCLKSOURCE_HSE)  || \
                                          ((__SOURCE__) == RCC_SYSCLKSOURCE_LSE)  || \
                                          ((__SOURCE__) == RCC_SYSCLKSOURCE_LSI)  || \
                                          ((__SOURCE__) == RCC_SYSCLKSOURCE_PLLCLK))

#define IS_RCC_HCLK(__HCLK__) (((__HCLK__) == RCC_SYSCLK_DIV1)   || ((__HCLK__) == RCC_SYSCLK_DIV2)   || \
                               ((__HCLK__) == RCC_SYSCLK_DIV4)   || ((__HCLK__) == RCC_SYSCLK_DIV8)   || \
                               ((__HCLK__) == RCC_SYSCLK_DIV16)  || ((__HCLK__) == RCC_SYSCLK_DIV64)  || \
                               ((__HCLK__) == RCC_SYSCLK_DIV128) || ((__HCLK__) == RCC_SYSCLK_DIV256) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV512))

#define IS_RCC_PCLK(__PCLK__) (((__PCLK__) == RCC_HCLK_DIV1) || ((__PCLK__) == RCC_HCLK_DIV2) || \
                               ((__PCLK__) == RCC_HCLK_DIV4) || ((__PCLK__) == RCC_HCLK_DIV8) || \
                               ((__PCLK__) == RCC_HCLK_DIV16))

#define IS_RCC_RTCCLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_RTCCLKSOURCE_NONE) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_LSE) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_LSI) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV32))

#define IS_RCC_MCO(__MCOX__) ((__MCOX__) == RCC_MCO1)

#define IS_RCC_MCO1SOURCE(__SOURCE__) (((__SOURCE__) == RCC_MCO1SOURCE_NOCLOCK) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_SYSCLK) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_HSI) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_HSE) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_PLLCLK) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_LSI) || \
                                       ((__SOURCE__) == RCC_MCO1SOURCE_LSE))

#define IS_RCC_MCODIV(__DIV__) (((__DIV__) == RCC_MCODIV_1) || ((__DIV__) == RCC_MCODIV_2) || \
                                ((__DIV__) == RCC_MCODIV_4) || ((__DIV__) == RCC_MCODIV_8) || \
                                ((__DIV__) == RCC_MCODIV_16)|| ((__DIV__) == RCC_MCODIV_32) || \
                                ((__DIV__) == RCC_MCODIV_64)|| ((__DIV__) == RCC_MCODIV_128))

#define IS_RCC_LSE_DRIVE(__DRIVE__) (((__DRIVE__) == RCC_LSEDRIVE_LOW)        || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMLOW)  || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMHIGH) || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_HIGH))

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup RCC_Exported_Types RCC Exported Types
  * @{
  */

/**
  * @brief  RCC PLL configuration structure definition
  */
typedef struct
{
  uint32_t PLLState;   /*!< The new state of the PLL.
                            This parameter can be a value of @ref RCC_PLL_Config                      */

  uint32_t PLLSource;  /*!< RCC_PLLSource: PLL entry clock source.
                            This parameter must be a value of @ref RCC_PLL_Clock_Source               */

  uint32_t PLLM;       /*!< PLLM: Division factor for PLL VCO input clock.
                            This parameter must be a value of @ref RCC_PLLM_Clock_Divider                  */

  uint32_t PLLN;       /*!< PLLN: Multiplication factor for PLL VCO output clock.
                            This parameter must be a number between Min_Data = 8 and Max_Data = 86    */

  uint32_t PLLP;       /*!< PLLP: PLL Division factor.
                            User have to set the PLLQ parameter correctly to not exceed max frequency 64MHZ.
                            This parameter must be a value of @ref RCC_PLLP_Clock_Divider             */

#if defined(RCC_PLLQ_SUPPORT)
  uint32_t PLLQ;       /*!< PLLQ: PLL Division factor.
                            User have to set the PLLQ parameter correctly to not exceed max frequency 64MHZ.
                            This parameter must be a value of @ref RCC_PLLQ_Clock_Divider             */
#endif

  uint32_t PLLR;       /*!< PLLR: PLL Division for the main system clock.
                            User have to set the PLLR parameter correctly to not exceed max frequency 64MHZ.
                            This parameter must be a value of @ref RCC_PLLR_Clock_Divider             */

} RCC_PLLInitTypeDef;

/**
  * @brief  RCC Internal/External Oscillator (HSE, HSI, LSE and LSI) configuration structure definition
  */
typedef struct
{
  uint32_t OscillatorType;       /*!< The oscillators to be configured.
                                      This parameter can be a value of @ref RCC_Oscillator_Type                   */

  uint32_t HSEState;             /*!< The new state of the HSE.
                                      This parameter can be a value of @ref RCC_HSE_Config                        */

  uint32_t LSEState;             /*!< The new state of the LSE.
                                      This parameter can be a value of @ref RCC_LSE_Config                        */

  uint32_t HSIState;             /*!< The new state of the HSI.
                                      This parameter can be a value of @ref RCC_HSI_Config                        */

  uint32_t HSIDiv;               /*!< The division factor of the HSI16.
                                      This parameter can be a value of @ref RCC_HSI_Div                           */

  uint32_t HSICalibrationValue;  /*!< The calibration trimming value (default is RCC_HSICALIBRATION_DEFAULT).
                                      This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x7F */

  uint32_t LSIState;             /*!< The new state of the LSI.
                                      This parameter can be a value of @ref RCC_LSI_Config                        */

  RCC_PLLInitTypeDef PLL;        /*!< Main PLL structure parameters                                               */

} RCC_OscInitTypeDef;

/**
  * @brief  RCC System, AHB and APB busses clock configuration structure definition
  */
typedef struct
{
  uint32_t ClockType;             /*!< The clock to be configured.
                                       This parameter can be a combination of @ref RCC_System_Clock_Type      */

  uint32_t SYSCLKSource;          /*!< The clock source used as system clock (SYSCLK).
                                       This parameter can be a value of @ref RCC_System_Clock_Source    */

  uint32_t AHBCLKDivider;         /*!< The AHB clock (HCLK) divider. This clock is derived from the system clock (SYSCLK).
                                       This parameter can be a value of @ref RCC_AHB_Clock_Source       */

  uint32_t APB1CLKDivider;        /*!< The APB1 clock (PCLK1) divider. This clock is derived from the AHB clock (HCLK).
                                       This parameter can be a value of @ref RCC_APB1_Clock_Source */


} RCC_ClkInitTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCC_Exported_Constants RCC Exported Constants
  * @{
  */

/** @defgroup RCC_Timeout_Value Timeout Values
  * @{
  */
#define RCC_DBP_TIMEOUT_VALUE          2U                   /* 2 ms (minimum Tick + 1)  */
#define RCC_LSE_TIMEOUT_VALUE          LSE_STARTUP_TIMEOUT  /* LSE timeout in ms        */
/**
  * @}
  */

/** @defgroup RCC_Oscillator_Type Oscillator Type
  * @{
  */
#define RCC_OSCILLATORTYPE_NONE        0x00000000U   /*!< Oscillator configuration unchanged */
#define RCC_OSCILLATORTYPE_HSE         0x00000001U   /*!< HSE to configure */
#define RCC_OSCILLATORTYPE_HSI         0x00000002U   /*!< HSI to configure */
#define RCC_OSCILLATORTYPE_LSE         0x00000004U   /*!< LSE to configure */
#define RCC_OSCILLATORTYPE_LSI         0x00000008U   /*!< LSI to configure */
/**
  * @}
  */

/** @defgroup RCC_HSE_Config HSE Config
  * @{
  */
#define RCC_HSE_OFF                    0x00000000U                                /*!< HSE clock deactivation */
#define RCC_HSE_ON                     RCC_CR_HSEON                               /*!< HSE clock activation */
#define RCC_HSE_BYPASS                 ((uint32_t)(RCC_CR_HSEBYP | RCC_CR_HSEON)) /*!< External clock source for HSE clock */
/**
  * @}
  */

/** @defgroup RCC_LSE_Config LSE Config
  * @{
  */
#define RCC_LSE_OFF                    0x00000000U                                    /*!< LSE clock deactivation */
#define RCC_LSE_ON                     RCC_BDCR_LSEON                                 /*!< LSE clock activation */
#define RCC_LSE_BYPASS                 ((uint32_t)(RCC_BDCR_LSEBYP | RCC_BDCR_LSEON)) /*!< External clock source for LSE clock */
/**
  * @}
  */

/** @defgroup RCC_HSI_Config HSI Config
  * @{
  */
#define RCC_HSI_OFF                    0x00000000U            /*!< HSI clock deactivation */
#define RCC_HSI_ON                     RCC_CR_HSION           /*!< HSI clock activation */

#define RCC_HSICALIBRATION_DEFAULT     64U                    /*!< Default HSI calibration trimming value */
/**
  * @}
  */

/** @defgroup RCC_HSI_Div HSI Div
  * @{
  */
#define RCC_HSI_DIV1                   0x00000000U                                        /*!< HSI clock is not divided */
#define RCC_HSI_DIV2                   RCC_CR_HSIDIV_0                                    /*!< HSI clock is divided by 2 */
#define RCC_HSI_DIV4                   RCC_CR_HSIDIV_1                                    /*!< HSI clock is divided by 4 */
#define RCC_HSI_DIV8                   (RCC_CR_HSIDIV_1|RCC_CR_HSIDIV_0)                  /*!< HSI clock is divided by 8 */
#define RCC_HSI_DIV16                  RCC_CR_HSIDIV_2                                    /*!< HSI clock is divided by 16 */
#define RCC_HSI_DIV32                  (RCC_CR_HSIDIV_2|RCC_CR_HSIDIV_0)                  /*!< HSI clock is divided by 32 */
#define RCC_HSI_DIV64                  (RCC_CR_HSIDIV_2|RCC_CR_HSIDIV_1)                  /*!< HSI clock is divided by 64 */
#define RCC_HSI_DIV128                 (RCC_CR_HSIDIV_2|RCC_CR_HSIDIV_1|RCC_CR_HSIDIV_0)  /*!< HSI clock is divided by 128 */
/**
  * @}
  */

/** @defgroup RCC_LSI_Config LSI Config
  * @{
  */
#define RCC_LSI_OFF                    0x00000000U            /*!< LSI clock deactivation */
#define RCC_LSI_ON                     RCC_CSR_LSION          /*!< LSI clock activation */
/**
  * @}
  */

/** @defgroup RCC_PLL_Config PLL Config
  * @{
  */
#define RCC_PLL_NONE                   0x00000000U /*!< PLL configuration unchanged */
#define RCC_PLL_OFF                    0x00000001U /*!< PLL deactivation */
#define RCC_PLL_ON                     0x00000002U /*!< PLL activation */
/**
  * @}
  */

/** @defgroup RCC_PLLM_Clock_Divider PLLM Clock Divider
  * @{
  */
#define RCC_PLLM_DIV1                  0x00000000U                                                    /*!< PLLM division factor = 8  */
#define RCC_PLLM_DIV2                  RCC_PLLCFGR_PLLM_0                                             /*!< PLLM division factor = 2  */
#define RCC_PLLM_DIV3                  RCC_PLLCFGR_PLLM_1                                             /*!< PLLM division factor = 3  */
#define RCC_PLLM_DIV4                  (RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0)                      /*!< PLLM division factor = 4  */
#define RCC_PLLM_DIV5                  RCC_PLLCFGR_PLLM_2                                             /*!< PLLM division factor = 5  */
#define RCC_PLLM_DIV6                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0)                      /*!< PLLM division factor = 6  */
#define RCC_PLLM_DIV7                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1)                      /*!< PLLM division factor = 7  */
#define RCC_PLLM_DIV8                  (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_1| RCC_PLLCFGR_PLLM_0)  /*!< PLLM division factor = 8  */
/**
  * @}
  */

/** @defgroup RCC_PLLP_Clock_Divider PLLP Clock Divider
  * @{
  */
#define RCC_PLLP_DIV2                  RCC_PLLCFGR_PLLP_0                                                                                       /*!< PLLP division factor = 2  */
#define RCC_PLLP_DIV3                  RCC_PLLCFGR_PLLP_1                                                                                       /*!< PLLP division factor = 3  */
#define RCC_PLLP_DIV4                  (RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                                                                /*!< PLLP division factor = 4  */
#define RCC_PLLP_DIV5                  RCC_PLLCFGR_PLLP_2                                                                                       /*!< PLLP division factor = 5  */
#define RCC_PLLP_DIV6                  (RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_0)                                                                /*!< PLLP division factor = 6  */
#define RCC_PLLP_DIV7                  (RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1)                                                                /*!< PLLP division factor = 7  */
#define RCC_PLLP_DIV8                  (RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 8  */
#define RCC_PLLP_DIV9                  RCC_PLLCFGR_PLLP_3                                                                                       /*!< PLLP division factor = 9  */
#define RCC_PLLP_DIV10                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_0)                                                                /*!< PLLP division factor = 10 */
#define RCC_PLLP_DIV11                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_1)                                                                /*!< PLLP division factor = 11 */
#define RCC_PLLP_DIV12                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 12 */
#define RCC_PLLP_DIV13                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2)                                                                /*!< PLLP division factor = 13 */
#define RCC_PLLP_DIV14                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 14 */
#define RCC_PLLP_DIV15                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1)                                           /*!< PLLP division factor = 15 */
#define RCC_PLLP_DIV16                 (RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                      /*!< PLLP division factor = 16 */
#define RCC_PLLP_DIV17                 RCC_PLLCFGR_PLLP_4                                                                                       /*!< PLLP division factor = 17 */
#define RCC_PLLP_DIV18                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_0)                                                                /*!< PLLP division factor = 18 */
#define RCC_PLLP_DIV19                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_1)                                                                /*!< PLLP division factor = 19 */
#define RCC_PLLP_DIV20                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 20 */
#define RCC_PLLP_DIV21                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_2)                                                                /*!< PLLP division factor = 21 */
#define RCC_PLLP_DIV22                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 22 */
#define RCC_PLLP_DIV23                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1)                                           /*!< PLLP division factor = 23 */
#define RCC_PLLP_DIV24                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                      /*!< PLLP division factor = 24 */
#define RCC_PLLP_DIV25                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3)                                                                /*!< PLLP division factor = 25 */
#define RCC_PLLP_DIV26                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_0)                                           /*!< PLLP division factor = 26 */
#define RCC_PLLP_DIV27                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_1)                                           /*!< PLLP division factor = 27 */
#define RCC_PLLP_DIV28                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0)                      /*!< PLLP division factor = 28 */
#define RCC_PLLP_DIV29                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2)                                           /*!< PLLP division factor = 29 */
#define RCC_PLLP_DIV30                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_0)                      /*!< PLLP division factor = 30 */
#define RCC_PLLP_DIV31                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1)                      /*!< PLLP division factor = 31 */
#define RCC_PLLP_DIV32                 (RCC_PLLCFGR_PLLP_4 | RCC_PLLCFGR_PLLP_3 | RCC_PLLCFGR_PLLP_2 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLP_0) /*!< PLLP division factor = 32 */
/**
  * @}
  */

#if defined(RCC_PLLQ_SUPPORT)
/** @defgroup RCC_PLLQ_Clock_Divider PLLQ Clock Divider
  * @{
  */
#define RCC_PLLQ_DIV2                  RCC_PLLCFGR_PLLQ_0                                             /*!< PLLQ division factor = 2 */
#define RCC_PLLQ_DIV3                  RCC_PLLCFGR_PLLQ_1                                             /*!< PLLQ division factor = 3 */
#define RCC_PLLQ_DIV4                  (RCC_PLLCFGR_PLLQ_1 | RCC_PLLCFGR_PLLQ_0)                      /*!< PLLQ division factor = 4 */
#define RCC_PLLQ_DIV5                  RCC_PLLCFGR_PLLQ_2                                             /*!< PLLQ division factor = 5 */
#define RCC_PLLQ_DIV6                  (RCC_PLLCFGR_PLLQ_2 | RCC_PLLCFGR_PLLQ_0)                      /*!< PLLQ division factor = 6 */
#define RCC_PLLQ_DIV7                  (RCC_PLLCFGR_PLLQ_2 | RCC_PLLCFGR_PLLQ_1)                      /*!< PLLQ division factor = 7 */
#define RCC_PLLQ_DIV8                  (RCC_PLLCFGR_PLLQ_2 |RCC_PLLCFGR_PLLQ_1 | RCC_PLLCFGR_PLLQ_0)  /*!< PLLQ division factor = 8 */
/**  * @}
  */
#endif

/** @defgroup RCC_PLLR_Clock_Divider PLLR Clock Divider
  * @{
  */
#define RCC_PLLR_DIV2                  RCC_PLLCFGR_PLLR_0                                             /*!< PLLR division factor = 2 */
#define RCC_PLLR_DIV3                  RCC_PLLCFGR_PLLR_1                                             /*!< PLLR division factor = 3 */
#define RCC_PLLR_DIV4                  (RCC_PLLCFGR_PLLR_1 | RCC_PLLCFGR_PLLR_0)                      /*!< PLLR division factor = 4 */
#define RCC_PLLR_DIV5                  RCC_PLLCFGR_PLLR_2                                             /*!< PLLR division factor = 5 */
#define RCC_PLLR_DIV6                  (RCC_PLLCFGR_PLLR_2 | RCC_PLLCFGR_PLLR_0)                      /*!< PLLR division factor = 6 */
#define RCC_PLLR_DIV7                  (RCC_PLLCFGR_PLLR_2 | RCC_PLLCFGR_PLLR_1)                      /*!< PLLR division factor = 7 */
#define RCC_PLLR_DIV8                  (RCC_PLLCFGR_PLLR_2 | RCC_PLLCFGR_PLLR_1 | RCC_PLLCFGR_PLLR_0) /*!< PLLR division factor = 8 */
/**  * @}
  */

/** @defgroup RCC_PLL_Clock_Source PLL Clock Source
  * @{
  */
#define RCC_PLLSOURCE_NONE             0x00000000U             /*!< No clock selected as PLL entry clock source  */
#define RCC_PLLSOURCE_HSI              RCC_PLLCFGR_PLLSRC_HSI  /*!< HSI clock selected as PLL entry clock source */
#define RCC_PLLSOURCE_HSE              RCC_PLLCFGR_PLLSRC_HSE  /*!< HSE clock selected as PLL entry clock source */
/**
  * @}
  */

/** @defgroup RCC_PLL_Clock_Output PLL Clock Output
  * @{
  */
#define RCC_PLLPCLK                 RCC_PLLCFGR_PLLPEN      /*!< PLLPCLK selection from main PLL */
#if defined(RCC_PLLQ_SUPPORT)
#define RCC_PLLQCLK                 RCC_PLLCFGR_PLLQEN      /*!< PLLQCLK selection from main PLL */
#endif
#define RCC_PLLRCLK                 RCC_PLLCFGR_PLLREN      /*!< PLLRCLK selection from main PLL */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Type System Clock Type
  * @{
  */
#define RCC_CLOCKTYPE_SYSCLK           0x00000001U  /*!< SYSCLK to configure */
#define RCC_CLOCKTYPE_HCLK             0x00000002U  /*!< HCLK to configure */
#define RCC_CLOCKTYPE_PCLK1            0x00000004U  /*!< PCLK1 to configure */
/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source System Clock Source
  * @{
  */
#define RCC_SYSCLKSOURCE_HSI           0x00000000U                       /*!< HSI selection as system clock */
#define RCC_SYSCLKSOURCE_HSE           RCC_CFGR_SW_0                     /*!< HSE selection as system clock */
#define RCC_SYSCLKSOURCE_PLLCLK        RCC_CFGR_SW_1                     /*!< PLL selection as system clock */
#define RCC_SYSCLKSOURCE_LSI           (RCC_CFGR_SW_1 | RCC_CFGR_SW_0)   /*!< LSI selection as system clock */
#define RCC_SYSCLKSOURCE_LSE           RCC_CFGR_SW_2                     /*!< LSE selection as system clock */
/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source_Status System Clock Source Status
  * @{
  */
#define RCC_SYSCLKSOURCE_STATUS_HSI    0x00000000U                       /*!< HSI used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_HSE    RCC_CFGR_SWS_0                    /*!< HSE used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_PLLCLK RCC_CFGR_SWS_1                    /*!< PLL used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_LSI    (RCC_CFGR_SWS_1 | RCC_CFGR_SWS_0) /*!< LSI used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_LSE    RCC_CFGR_SWS_2                    /*!< LSE used as system clock */
/**
  * @}
  */

/** @defgroup RCC_AHB_Clock_Source AHB Clock Source
  * @{
  */
#define RCC_SYSCLK_DIV1                0x00000000U                                                             /*!< SYSCLK not divided */
#define RCC_SYSCLK_DIV2                RCC_CFGR_HPRE_3                                                         /*!< SYSCLK divided by 2 */
#define RCC_SYSCLK_DIV4                (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_0)                                     /*!< SYSCLK divided by 4 */
#define RCC_SYSCLK_DIV8                (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1)                                     /*!< SYSCLK divided by 8 */
#define RCC_SYSCLK_DIV16               (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0)                   /*!< SYSCLK divided by 16 */
#define RCC_SYSCLK_DIV64               (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2)                                     /*!< SYSCLK divided by 64 */
#define RCC_SYSCLK_DIV128              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_0)                   /*!< SYSCLK divided by 128 */
#define RCC_SYSCLK_DIV256              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1)                   /*!< SYSCLK divided by 256 */
#define RCC_SYSCLK_DIV512              (RCC_CFGR_HPRE_3 | RCC_CFGR_HPRE_2 | RCC_CFGR_HPRE_1 | RCC_CFGR_HPRE_0) /*!< SYSCLK divided by 512 */
/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Source APB Clock Source
  * @{
  */
#define RCC_HCLK_DIV1                  0x00000000U                                           /*!< HCLK not divided */
#define RCC_HCLK_DIV2                  RCC_CFGR_PPRE_2                                       /*!< HCLK divided by 2 */
#define RCC_HCLK_DIV4                  (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_0)                   /*!< HCLK divided by 4 */
#define RCC_HCLK_DIV8                  (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1)                   /*!< HCLK divided by 8 */
#define RCC_HCLK_DIV16                 (RCC_CFGR_PPRE_2 | RCC_CFGR_PPRE_1 | RCC_CFGR_PPRE_0) /*!< HCLK divided by 16 */
/**
  * @}
  */

/** @defgroup RCC_RTC_Clock_Source RTC Clock Source
  * @{
  */
#define RCC_RTCCLKSOURCE_NONE          0x00000000U            /*!< No clock configured for RTC */
#define RCC_RTCCLKSOURCE_LSE           RCC_BDCR_RTCSEL_0      /*!< LSE oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_LSI           RCC_BDCR_RTCSEL_1      /*!< LSI oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV32     RCC_BDCR_RTCSEL        /*!< HSE oscillator clock divided by 32 used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_MCO_Index MCO Index
  * @{
  */
#define RCC_MCO1                       0x00000000U
#define RCC_MCO                        RCC_MCO1               /*!< MCO1 to be compliant with other families with 2 MCOs*/
/**
  * @}
  */

/** @defgroup RCC_MCO1_Clock_Source MCO1 Clock Source
  * @{
  */
#define RCC_MCO1SOURCE_NOCLOCK         0x00000000U                            /*!< MCO1 output disabled, no clock on MCO1 */
#define RCC_MCO1SOURCE_SYSCLK          RCC_CFGR_MCOSEL_0                      /*!< SYSCLK selection as MCO1 source */
#define RCC_MCO1SOURCE_HSI             (RCC_CFGR_MCOSEL_0| RCC_CFGR_MCOSEL_1) /*!< HSI selection as MCO1 source */
#define RCC_MCO1SOURCE_HSE             RCC_CFGR_MCOSEL_2                      /*!< HSE selection as MCO1 source */
#define RCC_MCO1SOURCE_PLLCLK          (RCC_CFGR_MCOSEL_0|RCC_CFGR_MCOSEL_2)  /*!< PLLCLK selection as MCO1 source */
#define RCC_MCO1SOURCE_LSI             (RCC_CFGR_MCOSEL_1|RCC_CFGR_MCOSEL_2)  /*!< LSI selection as MCO1 source */
#define RCC_MCO1SOURCE_LSE             (RCC_CFGR_MCOSEL_0|RCC_CFGR_MCOSEL_1|RCC_CFGR_MCOSEL_2) /*!< LSE selection as MCO1 source */
/**
  * @}
  */

/** @defgroup RCC_MCOx_Clock_Prescaler MCO1 Clock Prescaler
  * @{
  */
#define RCC_MCODIV_1                   0x00000000U                                                 /*!< MCO not divided */
#define RCC_MCODIV_2                   RCC_CFGR_MCOPRE_0                                           /*!< MCO divided by 2 */
#define RCC_MCODIV_4                   RCC_CFGR_MCOPRE_1                                           /*!< MCO divided by 4 */
#define RCC_MCODIV_8                   (RCC_CFGR_MCOPRE_1 | RCC_CFGR_MCOPRE_0)                     /*!< MCO divided by 8 */
#define RCC_MCODIV_16                  RCC_CFGR_MCOPRE_2                                           /*!< MCO divided by 16 */
#define RCC_MCODIV_32                  (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_0)                     /*!< MCO divided by 32 */
#define RCC_MCODIV_64                  (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_1)                     /*!< MCO divided by 64 */
#define RCC_MCODIV_128                 (RCC_CFGR_MCOPRE_2 | RCC_CFGR_MCOPRE_1 | RCC_CFGR_MCOPRE_0) /*!< MCO divided by 128 */
/**
  * @}
  */

/** @defgroup RCC_Interrupt Interrupts
  * @{
  */
#define RCC_IT_LSIRDY                  RCC_CIFR_LSIRDYF            /*!< LSI Ready Interrupt flag */
#define RCC_IT_LSERDY                  RCC_CIFR_LSERDYF            /*!< LSE Ready Interrupt flag */
#define RCC_IT_HSIRDY                  RCC_CIFR_HSIRDYF            /*!< HSI Ready Interrupt flag */
#define RCC_IT_HSERDY                  RCC_CIFR_HSERDYF            /*!< HSE Ready Interrupt flag */
#define RCC_IT_PLLRDY                  RCC_CIFR_PLLRDYF            /*!< PLL Ready Interrupt flag */
#define RCC_IT_CSS                     RCC_CIFR_CSSF               /*!< HSE Clock Security System Interrupt flag */
#define RCC_IT_LSECSS                  RCC_CIFR_LSECSSF            /*!< LSE Clock Security System Interrupt flag */
/**
  * @}
  */

/** @defgroup RCC_Flag Flags
  *        Elements values convention: XXXYYYYYb
  *           - YYYYY  : Flag position in the register
  *           - XXX  : Register index
  *                 - 001: CR register
  *                 - 010: BDCR register
  *                 - 011: CSR register
  * @{
  */
/* Flags in the CR register */
#define RCC_FLAG_HSIRDY                ((CR_REG_INDEX << 5U) | RCC_CR_HSIRDY_Pos) /*!< HSI Ready flag */
#define RCC_FLAG_HSERDY                ((CR_REG_INDEX << 5U) | RCC_CR_HSERDY_Pos) /*!< HSE Ready flag */
#define RCC_FLAG_PLLRDY                ((CR_REG_INDEX << 5U) | RCC_CR_PLLRDY_Pos) /*!< PLL Ready flag */

/* Flags in the BDCR register */
#define RCC_FLAG_LSERDY                ((BDCR_REG_INDEX << 5U) | RCC_BDCR_LSERDY_Pos)  /*!< LSE Ready flag */
#define RCC_FLAG_LSECSSD               ((BDCR_REG_INDEX << 5U) | RCC_BDCR_LSECSSD_Pos) /*!< LSE Clock Security System Interrupt flag */

/* Flags in the CSR register */
#define RCC_FLAG_LSIRDY                ((CSR_REG_INDEX << 5U) | RCC_CSR_LSIRDY_Pos)    /*!< LSI Ready flag */
#define RCC_FLAG_OBLRST                ((CSR_REG_INDEX << 5U) | RCC_CSR_OBLRSTF_Pos)   /*!< Option Byte Loader reset flag */
#define RCC_FLAG_PINRST                ((CSR_REG_INDEX << 5U) | RCC_CSR_PINRSTF_Pos)   /*!< PIN reset flag */
#define RCC_FLAG_PWRRST                ((CSR_REG_INDEX << 5U) | RCC_CSR_PWRRSTF_Pos)   /*!< BOR or POR/PDR reset flag */
#define RCC_FLAG_SFTRST                ((CSR_REG_INDEX << 5U) | RCC_CSR_SFTRSTF_Pos)   /*!< Software Reset flag */
#define RCC_FLAG_IWDGRST               ((CSR_REG_INDEX << 5U) | RCC_CSR_IWDGRSTF_Pos)  /*!< Independent Watchdog reset flag */
#define RCC_FLAG_WWDGRST               ((CSR_REG_INDEX << 5U) | RCC_CSR_WWDGRSTF_Pos)  /*!< Window watchdog reset flag */
#define RCC_FLAG_LPWRRST               ((CSR_REG_INDEX << 5U) | RCC_CSR_LPWRRSTF_Pos)  /*!< Low-Power reset flag */

/**
  * @}
  */

/** @defgroup RCC_LSEDrive_Config LSE Drive Configuration
  * @{
  */
#define RCC_LSEDRIVE_LOW                 0x00000000U            /*!< LSE low drive capability */
#define RCC_LSEDRIVE_MEDIUMLOW           RCC_BDCR_LSEDRV_0      /*!< LSE medium low drive capability */
#define RCC_LSEDRIVE_MEDIUMHIGH          RCC_BDCR_LSEDRV_1      /*!< LSE medium high drive capability */
#define RCC_LSEDRIVE_HIGH                RCC_BDCR_LSEDRV        /*!< LSE high drive capability */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/** @defgroup RCC_Exported_Macros RCC Exported Macros
  * @{
  */

/** @defgroup RCC_AHB_Peripheral_Clock_Enable_Disable AHB Peripheral Clock Enable Disable
  * @brief  Enable or disable the AHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_DMA1_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_FLASH_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->AHBENR, RCC_AHBENR_FLASHEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FLASHEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_CRC_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(RNG)
#define __HAL_RCC_RNG_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif /* RNG */

#if defined(AES)
#define __HAL_RCC_AES_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->AHBENR, RCC_AHBENR_AESEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_AESEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif /* AES */

#define __HAL_RCC_DMA1_CLK_DISABLE()           CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN)
#define __HAL_RCC_FLASH_CLK_DISABLE()          CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_FLASHEN)
#define __HAL_RCC_CRC_CLK_DISABLE()            CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN)
#if defined(RNG)
#define __HAL_RCC_RNG_CLK_DISABLE()            CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_CLK_DISABLE()            CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_AESEN)
#endif /* AES */
/**
  * @}
  */

/** @defgroup RCC_IOPORT_Clock_Enable_Disable IOPORT Clock Enable Disable
  * @brief  Enable or disable the IO Ports clock.
  * @note   After reset, the IO ports clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_GPIOA_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOAEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOAEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_GPIOB_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_GPIOC_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOCEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOCEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_GPIOD_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#define __HAL_RCC_GPIOF_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_GPIOA_CLK_DISABLE()          CLEAR_BIT(RCC->IOPENR, RCC_IOPENR_GPIOAEN)
#define __HAL_RCC_GPIOB_CLK_DISABLE()          CLEAR_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN)
#define __HAL_RCC_GPIOC_CLK_DISABLE()          CLEAR_BIT(RCC->IOPENR, RCC_IOPENR_GPIOCEN)
#define __HAL_RCC_GPIOD_CLK_DISABLE()          CLEAR_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN)
#define __HAL_RCC_GPIOF_CLK_DISABLE()          CLEAR_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN)

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Enable_Disable APB1 Peripheral Clock Enable Disable
  * @brief  Enable or disable the APB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#if defined(TIM2)
#define __HAL_RCC_TIM2_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_TIM2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#define __HAL_RCC_TIM3_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_TIM3EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM3EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_TIM6_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_TIM6EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM6EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_TIM7_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_TIM7EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM7EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_RTCAPB_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_RTCAPBEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_RTCAPBEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_WWDG_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_WWDGEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_WWDGEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_SPI2_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_SPI2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_SPI2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_USART2_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_USART3_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_USART3EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_USART3EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_USART4_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(LPUART1)
#define __HAL_RCC_LPUART1_CLK_ENABLE()         do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_LPUART1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_LPUART1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#define __HAL_RCC_I2C1_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_I2C1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_I2C2_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_I2C2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(CEC)
#define __HAL_RCC_CEC_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_CECEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_CECEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#if defined(UCPD1)
#define __HAL_RCC_UCPD1_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_UCPD1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#if defined(UCPD2)
#define __HAL_RCC_UCPD2_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_UCPD2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#define __HAL_RCC_DBGMCU_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_DBGEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_DBGEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_PWR_CLK_ENABLE()             do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_PWREN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_PWREN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(DAC1)
#define __HAL_RCC_DAC1_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_DAC1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_DAC1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM2EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM2EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif

/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Enable_Disable APB2 Peripheral Clock Enable Disable
  * @brief  Enable or disable the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_SYSCFG_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_TIM1_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_TIM1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_SPI1_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_SPI1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_SPI1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_USART1_CLK_ENABLE()          do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_TIM14_CLK_ENABLE()            do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_TIM14EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM14EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(TIM15)
#define __HAL_RCC_TIM15_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_TIM15EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM15EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)
#endif /* TIM15 */

#define __HAL_RCC_TIM16_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_TIM16EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM16EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_TIM17_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_TIM17EN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM17EN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#define __HAL_RCC_ADC_CLK_ENABLE()           do { \
                                                 __IO uint32_t tmpreg; \
                                                 SET_BIT(RCC->APBENR2, RCC_APBENR2_ADCEN); \
                                                 /* Delay after an RCC peripheral clock enabling */ \
                                                 tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_ADCEN); \
                                                 UNUSED(tmpreg); \
                                               } while(0U)

#if defined(TIM2)
#define __HAL_RCC_TIM2_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_TIM2EN)
#endif
#define __HAL_RCC_TIM3_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_TIM3EN)
#if defined(TIM6)
#define __HAL_RCC_TIM6_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_TIM6EN)
#endif /* TIM6 */
#if defined(TIM7)
#define __HAL_RCC_TIM7_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_TIM7EN)
#endif /* TIM7 */
#define __HAL_RCC_RTCAPB_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_RTCAPBEN)
#define __HAL_RCC_SPI2_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_SPI2EN)
#define __HAL_RCC_USART2_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN)
#if defined(USART3)
#define __HAL_RCC_USART3_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_USART3EN)
#endif /* USART3 */
#if defined(USART4)
#define __HAL_RCC_USART4_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN)
#endif /* USART4 */
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_CLK_DISABLE()        CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_LPUART1EN)
#endif /* LPUART1 */
#define __HAL_RCC_I2C1_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_I2C1EN)
#define __HAL_RCC_I2C2_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_I2C2EN)
#if defined(CEC)
#define __HAL_RCC_CEC_CLK_DISABLE()            CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_CECEN)
#endif /* CEC */
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_UCPD1EN)
#endif /* UCPD1 */
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_UCPD2EN)
#endif /* UCPD2 */
#define __HAL_RCC_DBGMCU_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_DBGEN)
#define __HAL_RCC_PWR_CLK_DISABLE()            CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_PWREN)
#if defined(DAC1)
#define __HAL_RCC_DAC1_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_DAC1EN)
#endif /* DAC1 */
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM1EN)
#endif /* LPTIM1 */
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM2EN)
#endif /* LPTIM2 */
#define __HAL_RCC_SYSCFG_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN)
#define __HAL_RCC_TIM1_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_TIM1EN)
#define __HAL_RCC_SPI1_CLK_DISABLE()           CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_SPI1EN)
#define __HAL_RCC_USART1_CLK_DISABLE()         CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN)
#define __HAL_RCC_TIM14_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_TIM14EN)
#if defined(TIM15)
#define __HAL_RCC_TIM15_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_TIM15EN)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_TIM16EN)
#define __HAL_RCC_TIM17_CLK_DISABLE()          CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_TIM17EN)
#define __HAL_RCC_ADC_CLK_DISABLE()            CLEAR_BIT(RCC->APBENR2, RCC_APBENR2_ADCEN)

/**
  * @}
  */

/** @defgroup RCC_AHB_Peripheral_Clock_Enabled_Disabled_Status AHB Peripheral Clock Enabled or Disabled Status
  * @brief  Check whether the AHB peripheral clock is enabled or not.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_DMA1_IS_CLK_ENABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN)  != RESET)
#define __HAL_RCC_FLASH_IS_CLK_ENABLED()       (READ_BIT(RCC->AHBENR, RCC_AHBENR_FLASHEN) != RESET)
#define __HAL_RCC_CRC_IS_CLK_ENABLED()         (READ_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN)   != RESET)
#if defined(RNG)
#define __HAL_RCC_RNG_IS_CLK_ENABLED()         (READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN)   != RESET)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_IS_CLK_ENABLED()         (READ_BIT(RCC->AHBENR, RCC_AHBENR_AESEN)   != RESET)
#endif /* AES */

#define __HAL_RCC_DMA1_IS_CLK_DISABLED()       (READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN)  == RESET)
#define __HAL_RCC_FLASH_IS_CLK_DISABLED()      (READ_BIT(RCC->AHBENR, RCC_AHBENR_FLASHEN) == RESET)
#define __HAL_RCC_CRC_IS_CLK_DISABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN)   == RESET)
#if defined(RNG)
#define __HAL_RCC_RNG_IS_CLK_DISABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN)   == RESET)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_IS_CLK_DISABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_AESEN)   == RESET)
#endif /* AES */
/**
  * @}
  */

/** @defgroup RCC_IOPORT_Clock_Enabled_Disabled_Status IOPORT Clock Enabled or Disabled Status
  * @brief  Check whether the IO Port clock is enabled or not.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_GPIOA_IS_CLK_ENABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOAEN) != RESET)
#define __HAL_RCC_GPIOB_IS_CLK_ENABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN) != RESET)
#define __HAL_RCC_GPIOC_IS_CLK_ENABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOCEN) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_ENABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN) != RESET)
#define __HAL_RCC_GPIOF_IS_CLK_ENABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN) != RESET)


#define __HAL_RCC_GPIOA_IS_CLK_DISABLED()      (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOAEN) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_DISABLED()      (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_DISABLED()      (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOCEN) == RESET)
#define __HAL_RCC_GPIOD_IS_CLK_DISABLED()      (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN) == RESET)
#define __HAL_RCC_GPIOF_IS_CLK_DISABLED()      (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOFEN) == RESET)

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Enabled_Disabled_Status APB1 Peripheral Clock Enabled or Disabled Status
  * @brief  Check whether the APB1 peripheral clock is enabled or not.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#if defined(TIM2)
#define __HAL_RCC_TIM2_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM2EN)   != 0U)
#endif
#define __HAL_RCC_TIM3_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM3EN)   != 0U)
#define __HAL_RCC_TIM6_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM6EN)   != 0U)
#define __HAL_RCC_TIM7_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM7EN)   != 0U)
#define __HAL_RCC_RTCAPB_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_RTCAPBEN) != 0U)
#define __HAL_RCC_WWDG_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_WWDGEN)   != 0U)
#define __HAL_RCC_SPI2_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_SPI2EN)   != 0U)
#define __HAL_RCC_USART2_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN) != 0U)
#define __HAL_RCC_USART3_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART3EN) != 0U)
#define __HAL_RCC_USART4_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN) != 0U)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_IS_CLK_ENABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPUART1EN)!= 0U)
#endif
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C1EN)   != 0U)
#define __HAL_RCC_I2C2_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C2EN)   != 0U)
#if defined(CEC)
#define __HAL_RCC_CEC_IS_CLK_ENABLED()         (READ_BIT(RCC->APBENR1, RCC_APBENR1_CECEN)    != 0U)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD1EN)  != 0U)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD2EN)  != 0U)
#endif
#define __HAL_RCC_DBGMCU_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_DBGEN)    != 0U)
#define __HAL_RCC_PWR_IS_CLK_ENABLED()         (READ_BIT(RCC->APBENR1, RCC_APBENR1_PWREN)    != 0U)
#if defined(DAC1)
#define __HAL_RCC_DAC1_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_DAC1EN)   != 0U)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM2EN) != 0U)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM1EN) != 0U)
#endif
#if defined(TIM2)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM2EN)   == 0U)
#endif
#define __HAL_RCC_TIM3_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM3EN)   == 0U)
#define __HAL_RCC_TIM6_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM6EN)   == 0U)
#define __HAL_RCC_TIM7_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_TIM7EN)   == 0U)
#define __HAL_RCC_RTCAPB_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_RTCAPBEN) == 0U)
#define __HAL_RCC_WWDG_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_WWDGEN)   == 0U)
#define __HAL_RCC_SPI2_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_SPI2EN)   == 0U)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART2EN) == 0U)
#define __HAL_RCC_USART3_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART3EN) == 0U)
#define __HAL_RCC_USART4_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_USART4EN) == 0U)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_IS_CLK_DISABLED()    (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPUART1EN)== 0U)
#endif
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C1EN)   == 0U)
#define __HAL_RCC_I2C2_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_I2C2EN)   == 0U)
#if defined(CEC)
#define __HAL_RCC_CEC_IS_CLK_DISABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_CECEN)    == 0U)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD1EN)  == 0U)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR1, RCC_APBENR1_UCPD2EN)  == 0U)
#endif
#define __HAL_RCC_DBGMCU_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_DBGEN)    == 0U)
#define __HAL_RCC_PWR_IS_CLK_DISABLED()        (READ_BIT(RCC->APBENR1, RCC_APBENR1_PWREN)    == 0U)
#if defined(DAC1)
#define __HAL_RCC_DAC1_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR1, RCC_APBENR1_DAC1EN)   == 0U)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM2EN) == 0U)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR1, RCC_APBENR1_LPTIM1EN) == 0U)
#endif

/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Enabled_Disabled_Status APB2 Peripheral Clock Enabled or Disabled Status
  * @brief  Check whether the APB2 peripheral clock is enabled or not.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_SYSCFG_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN) != 0U)
#define __HAL_RCC_TIM1_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM1EN)   != 0U)
#define __HAL_RCC_SPI1_IS_CLK_ENABLED()        (READ_BIT(RCC->APBENR2, RCC_APBENR2_SPI1EN)   != 0U)
#define __HAL_RCC_USART1_IS_CLK_ENABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN) != 0U)
#define __HAL_RCC_TIM14_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM14EN)  != 0U)
#if defined(TIM15)
#define __HAL_RCC_TIM15_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM15EN)  != 0U)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM16EN)  != 0U)
#define __HAL_RCC_TIM17_IS_CLK_ENABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM17EN)  != 0U)
#define __HAL_RCC_ADC_IS_CLK_ENABLED()         (READ_BIT(RCC->APBENR2, RCC_APBENR2_ADCEN)    != 0U)

#define __HAL_RCC_SYSCFG_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR2, RCC_APBENR2_SYSCFGEN) == 0U)
#define __HAL_RCC_TIM1_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM1EN)   == 0U)
#define __HAL_RCC_SPI1_IS_CLK_DISABLED()       (READ_BIT(RCC->APBENR2, RCC_APBENR2_SPI1EN)   == 0U)
#define __HAL_RCC_USART1_IS_CLK_DISABLED()     (READ_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN) == 0U)
#define __HAL_RCC_TIM14_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM14EN)  == 0U)
#if defined(TIM15)
#define __HAL_RCC_TIM15_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM15EN)  == 0U)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM16EN)  == 0U)
#define __HAL_RCC_TIM17_IS_CLK_DISABLED()      (READ_BIT(RCC->APBENR2, RCC_APBENR2_TIM17EN)  == 0U)
#define __HAL_RCC_ADC_IS_CLK_DISABLED()        (READ_BIT(RCC->APBENR2, RCC_APBENR2_ADCEN)    == 0U)

/**
  * @}
  */

/** @defgroup RCC_AHB_Force_Release_Reset AHB Peripheral Force Release Reset
  * @brief  Force or release AHB1 peripheral reset.
  * @{
  */
#define __HAL_RCC_AHB_FORCE_RESET()            WRITE_REG(RCC->AHBRSTR, 0xFFFFFFFFU)
#define __HAL_RCC_DMA1_FORCE_RESET()           SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_DMA1RST)
#define __HAL_RCC_FLASH_FORCE_RESET()          SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_FLASHRST)
#define __HAL_RCC_CRC_FORCE_RESET()            SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST)
#if defined(RNG)
#define __HAL_RCC_RNG_FORCE_RESET()            SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_RNGRST)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_FORCE_RESET()            SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_AESRST)
#endif /* AES */

#define __HAL_RCC_AHB_RELEASE_RESET()          WRITE_REG(RCC->AHBRSTR, 0x00000000U)
#define __HAL_RCC_DMA1_RELEASE_RESET()         CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_DMA1RST)
#define __HAL_RCC_FLASH_RELEASE_RESET()        CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_FLASHRST)
#define __HAL_RCC_CRC_RELEASE_RESET()          CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST)
#if defined(RNG)
#define __HAL_RCC_RNG_RELEASE_RESET()          CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_RNGRST)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_RELEASE_RESET()          CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_AESRST)
#endif /* AES */
/**
  * @}
  */

/** @defgroup RCC_IOPORT_Force_Release_Reset IOPORT Force Release Reset
  * @brief  Force or release IO Port reset.
  * @{
  */
#define __HAL_RCC_IOP_FORCE_RESET()            WRITE_REG(RCC->IOPRSTR, 0xFFFFFFFFU)
#define __HAL_RCC_GPIOA_FORCE_RESET()          SET_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOARST)
#define __HAL_RCC_GPIOB_FORCE_RESET()          SET_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOBRST)
#define __HAL_RCC_GPIOC_FORCE_RESET()          SET_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOCRST)
#define __HAL_RCC_GPIOD_FORCE_RESET()          SET_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIODRST)
#define __HAL_RCC_GPIOF_FORCE_RESET()          SET_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOFRST)

#define __HAL_RCC_IOP_RELEASE_RESET()          WRITE_REG(RCC->IOPRSTR, 0x00000000U)
#define __HAL_RCC_GPIOA_RELEASE_RESET()        CLEAR_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOARST)
#define __HAL_RCC_GPIOB_RELEASE_RESET()        CLEAR_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOBRST)
#define __HAL_RCC_GPIOC_RELEASE_RESET()        CLEAR_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOCRST)
#define __HAL_RCC_GPIOD_RELEASE_RESET()        CLEAR_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIODRST)
#define __HAL_RCC_GPIOF_RELEASE_RESET()        CLEAR_BIT(RCC->IOPRSTR, RCC_IOPRSTR_GPIOFRST)

/**
  * @}
  */

/** @defgroup RCC_APB1_Force_Release_Reset APB1 Peripheral Force Release Reset
  * @brief  Force or release APB1 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB1_FORCE_RESET()           WRITE_REG(RCC->APBRSTR1, 0xFFFFFFFFU)

#if defined(TIM2)
#define __HAL_RCC_TIM2_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM2RST)
#endif
#define __HAL_RCC_TIM3_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM3RST)
#define __HAL_RCC_TIM6_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM6RST)
#define __HAL_RCC_TIM7_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM7RST)
#define __HAL_RCC_SPI2_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_SPI2RST)
#define __HAL_RCC_USART2_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART2RST)
#define __HAL_RCC_USART3_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART3RST)
#define __HAL_RCC_USART4_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART4RST)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_FORCE_RESET()        SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPUART1RST)
#endif
#define __HAL_RCC_I2C1_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_I2C1RST)
#define __HAL_RCC_I2C2_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_I2C2RST)
#if defined(CEC)
#define __HAL_RCC_CEC_FORCE_RESET()            SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_CECRST)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_FORCE_RESET()          SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_UCPD1RST)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_FORCE_RESET()          SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_UCPD2RST)
#endif
#define __HAL_RCC_DBGMCU_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_DBGRST)
#define __HAL_RCC_PWR_FORCE_RESET()            SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_PWRRST)
#if defined(DAC1)
#define __HAL_RCC_DAC1_FORCE_RESET()           SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_DAC1RST)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPTIM2RST)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_FORCE_RESET()         SET_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPTIM1RST)
#endif
#define __HAL_RCC_APB1_RELEASE_RESET()         WRITE_REG(RCC->APBRSTR1, 0x00000000U)
#if defined(TIM2)
#define __HAL_RCC_TIM2_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM2RST)
#endif
#define __HAL_RCC_TIM3_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM3RST)
#define __HAL_RCC_TIM6_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM6RST)
#define __HAL_RCC_TIM7_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_TIM7RST)
#define __HAL_RCC_SPI2_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_SPI2RST)
#define __HAL_RCC_USART2_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART2RST)
#define __HAL_RCC_USART3_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART3RST)
#define __HAL_RCC_USART4_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_USART4RST)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_RELEASE_RESET()      CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPUART1RST)
#endif
#define __HAL_RCC_I2C1_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_I2C1RST)
#define __HAL_RCC_I2C2_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_I2C2RST)
#if defined(CEC)
#define __HAL_RCC_CEC_RELEASE_RESET()          CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_CECRST)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_UCPD1RST)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_UCPD2RST)
#endif
#define __HAL_RCC_DBGMCU_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_DBGRST)
#define __HAL_RCC_PWR_RELEASE_RESET()          CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_PWRRST)
#if defined(DAC1)
#define __HAL_RCC_DAC1_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_DAC1RST)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPTIM2RST)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR1, RCC_APBRSTR1_LPTIM1RST)
#endif
/**
  * @}
  */

/** @defgroup RCC_APB2_Force_Release_Reset APB2 Peripheral Force Release Reset
  * @brief  Force or release APB2 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB2_FORCE_RESET()           WRITE_REG(RCC->APBRSTR2, 0xFFFFFFFFU)
#define __HAL_RCC_SYSCFG_FORCE_RESET()         SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_SYSCFGRST)
#define __HAL_RCC_TIM1_FORCE_RESET()           SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM1RST)
#define __HAL_RCC_SPI1_FORCE_RESET()           SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_SPI1RST)
#define __HAL_RCC_USART1_FORCE_RESET()         SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_USART1RST)
#define __HAL_RCC_TIM14_FORCE_RESET()          SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM14RST)
#if defined(TIM15)
#define __HAL_RCC_TIM15_FORCE_RESET()          SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM15RST)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_FORCE_RESET()          SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM16RST)
#define __HAL_RCC_TIM17_FORCE_RESET()          SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM17RST)
#define __HAL_RCC_ADC_FORCE_RESET()            SET_BIT(RCC->APBRSTR2, RCC_APBRSTR2_ADCRST)

#define __HAL_RCC_APB2_RELEASE_RESET()         WRITE_REG(RCC->APBRSTR2, 0x00U)
#define __HAL_RCC_SYSCFG_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_SYSCFGRST)
#define __HAL_RCC_TIM1_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM1RST)
#define __HAL_RCC_SPI1_RELEASE_RESET()         CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_SPI1RST)
#define __HAL_RCC_USART1_RELEASE_RESET()       CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_USART1RST)
#define __HAL_RCC_TIM14_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM14RST)
#if defined(TIM15)
#define __HAL_RCC_TIM15_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM15RST)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM16RST)
#define __HAL_RCC_TIM17_RELEASE_RESET()        CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_TIM17RST)
#define __HAL_RCC_ADC_RELEASE_RESET()          CLEAR_BIT(RCC->APBRSTR2, RCC_APBRSTR2_ADCRST)

/**
  * @}
  */

/** @defgroup RCC_AHB_Clock_Sleep_Enable_Disable AHB Peripherals Clock Sleep Enable Disable
  * @brief  Enable or disable the AHB peripherals clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_DMA1_CLK_SLEEP_ENABLE()      SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_DMA1SMEN)
#define __HAL_RCC_FLASH_CLK_SLEEP_ENABLE()     SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_FLASHSMEN)
#define __HAL_RCC_SRAM_CLK_SLEEP_ENABLE()      SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_SRAMSMEN)
#define __HAL_RCC_CRC_CLK_SLEEP_ENABLE()       SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_CRCSMEN)
#if defined(RNG)
#define __HAL_RCC_RNG_CLK_SLEEP_ENABLE()       SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_CLK_SLEEP_ENABLE()       SET_BIT(RCC->AHBSMENR, RCC_AHBSMENR_AESSMEN)
#endif /* AES */
#define __HAL_RCC_DMA1_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_DMA1SMEN)
#define __HAL_RCC_FLASH_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_FLASHSMEN)
#define __HAL_RCC_SRAM_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_SRAMSMEN)
#define __HAL_RCC_CRC_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_CRCSMEN)
#if defined(RNG)
#define __HAL_RCC_RNG_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->AHBSMENR, RCC_AHBSMENR_AESSMEN)
#endif /* AES */

/**
  * @}
  */

/** @defgroup RCC_IOPORT_Clock_Sleep_Enable_Disable IOPORT Clock Sleep Enable Disable
  * @brief  Enable or disable the IOPORT clock during Low Power (Sleep) mode.
  * @note   IOPORT clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_GPIOA_CLK_SLEEP_ENABLE()     SET_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOASMEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_ENABLE()     SET_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOBSMEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE()     SET_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOCSMEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()     SET_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_ENABLE()     SET_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOFSMEN)

#define __HAL_RCC_GPIOA_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOASMEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOBSMEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOCSMEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOFSMEN)
/**
  *   @}
  */

/** @defgroup RCC_APB1_Clock_Sleep_Enable_Disable APB1 Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the APB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(TIM2)
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM2SMEN)
#endif
#define __HAL_RCC_TIM3_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM3SMEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM6SMEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM7SMEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_RTCAPBSMEN)
#define __HAL_RCC_WWDG_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_WWDGSMEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_SPI2SMEN)
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART2SMEN)
#define __HAL_RCC_USART3_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART3SMEN)
#define __HAL_RCC_USART4_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART4SMEN)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_CLK_SLEEP_ENABLE()   SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPUART1SMEN)
#endif
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C1SMEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C2SMEN)
#if defined(CEC)
#define __HAL_RCC_CEC_CLK_SLEEP_ENABLE()       SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_CECSMEN)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD1SMEN)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD2SMEN)
#endif
#define __HAL_RCC_DBGMCU_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DBGSMEN)
#define __HAL_RCC_PWR_CLK_SLEEP_ENABLE()       SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_PWRSMEN)
#if defined(DAC1)
#define __HAL_RCC_DAC1_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DAC1SMEN)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM2SMEN)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM1SMEN)
#endif
#if defined(TIM2)
#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM2SMEN)
#endif
#define __HAL_RCC_TIM3_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM3SMEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM6SMEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM7SMEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_RTCAPBSMEN)
#define __HAL_RCC_WWDG_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_WWDGSMEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_SPI2SMEN)
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART2SMEN)
#define __HAL_RCC_USART3_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART3SMEN)
#define __HAL_RCC_USART4_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART4SMEN)
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPUART1SMEN)
#endif
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C1SMEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C2SMEN)
#if defined(CEC)
#define __HAL_RCC_CEC_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_CECSMEN)
#endif
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD1SMEN)
#endif
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD2SMEN)
#endif
#define __HAL_RCC_DBGMCU_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DBGSMEN)
#define __HAL_RCC_PWR_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_PWRSMEN)
#if defined(DAC1)
#define __HAL_RCC_DAC1_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DAC1SMEN)
#endif
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM2SMEN)
#endif
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM1SMEN)
#endif
/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Sleep_Enable_Disable APB2 Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the APB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SYSCFGSMEN)
#define __HAL_RCC_TIM1_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM1SMEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SPI1SMEN)
#define __HAL_RCC_USART1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_USART1SMEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM14SMEN)
#if defined(TIM15)
#define __HAL_RCC_TIM15_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM15SMEN)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM16SMEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM17SMEN)
#define __HAL_RCC_ADC_CLK_SLEEP_ENABLE()       SET_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_ADCSMEN)

#define __HAL_RCC_SYSCFG_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SYSCFGSMEN)
#define __HAL_RCC_TIM1_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM1SMEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SPI1SMEN)
#define __HAL_RCC_USART1_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_USART1SMEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM14SMEN)
#if defined(TIM15)
#define __HAL_RCC_TIM15_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM15SMEN)
#endif /* TIM15 */
#define __HAL_RCC_TIM16_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM16SMEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM17SMEN)
#define __HAL_RCC_ADC_CLK_SLEEP_DISABLE()      CLEAR_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_ADCSMEN)

/**
  * @}
  */

/** @defgroup RCC_AHB_Clock_Sleep_Enabled_Disabled_Status AHB Peripheral Clock Sleep Enabled or Disabled Status
  * @brief  Check whether the AHB peripheral clock during Low Power (Sleep) mode is enabled or not.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_DMA1_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_DMA1SMEN) != RESET)
#define __HAL_RCC_FLASH_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_FLASHSMEN)!= RESET)
#define __HAL_RCC_SRAM_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_SRAMSMEN) != RESET)
#define __HAL_RCC_CRC_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_CRCSMEN)  != RESET)
#if defined(RNG)
#define __HAL_RCC_RNG_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN)  != RESET)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_AESSMEN)  != RESET)
#endif /* AES */
#define __HAL_RCC_DMA1_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_DMA1SMEN)  == RESET)
#define __HAL_RCC_FLASH_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_FLASHSMEN) == RESET)
#define __HAL_RCC_SRAM_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_SRAMSMEN)  == RESET)
#define __HAL_RCC_CRC_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_CRCSMEN)   == RESET)
#if defined(RNG)
#define __HAL_RCC_RNG_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN)   == RESET)
#endif /* RNG */
#if defined(AES)
#define __HAL_RCC_AES_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_AESSMEN)   == RESET)
#endif /* AES */

/**
  * @}
  */

/** @defgroup RCC_IOPORT_Clock_Sleep_Enabled_Disabled_Status IOPORT Clock Sleep Enabled or Disabled Status
  * @brief  Check whether the IOPORT clock during Low Power (Sleep) mode is enabled or not.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */


#define __HAL_RCC_GPIOA_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOASMEN)!= RESET)
#define __HAL_RCC_GPIOB_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOBSMEN)!= RESET)
#define __HAL_RCC_GPIOC_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOCSMEN)!= RESET)
#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN)!= RESET)
#define __HAL_RCC_GPIOF_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOFSMEN)!= RESET)



#define __HAL_RCC_GPIOA_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOASMEN) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOBSMEN) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOCSMEN) == RESET)
#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN) == RESET)
#define __HAL_RCC_GPIOF_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOFSMEN) == RESET)

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Sleep_Enabled_Disabled_Status APB1 Peripheral Clock Sleep Enabled or Disabled Status
  * @brief  Check whether the APB1 peripheral clock during Low Power (Sleep) mode is enabled or not.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(TIM2)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM2SMEN)   != RESET)
#endif /* TIM2 */
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM3SMEN)   != RESET)
#if defined(TIM6)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM6SMEN)   != RESET)
#endif /* TIM6 */
#if defined(TIM7)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM7SMEN)   != RESET)
#endif /* TIM7 */
#define __HAL_RCC_RTCAPB_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_RTCAPBSMEN) != RESET)
#define __HAL_RCC_WWDG_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_WWDGSMEN)   != RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_SPI2SMEN)   != RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART2SMEN) != RESET)
#if defined(USART3)
#define __HAL_RCC_USART3_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART3SMEN) != RESET)
#endif /* USART3 */
#if defined(USART4)
#define __HAL_RCC_USART4_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART4SMEN) != RESET)
#endif /* USART4 */
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPUART1SMEN)!= RESET)
#endif /* LPUART1 */
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C1SMEN)   != RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C2SMEN)   != RESET)
#if defined(CEC)
#define __HAL_RCC_CEC_IS_CLK_SLEEP_ENABLED()       (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_CECSMEN)    != RESET)
#endif /* CEC */
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD1SMEN)  != RESET)
#endif /* UCPD1 */
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD2SMEN)  != RESET)
#endif /* UCPD2 */
#define __HAL_RCC_DBGMCU_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DBGSMEN)    != RESET)
#define __HAL_RCC_PWR_IS_CLK_SLEEP_ENABLED()       (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_PWRSMEN)    != RESET)
#if defined(DAC1)
#define __HAL_RCC_DAC1_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DAC1SMEN)   != RESET)
#endif /* DAC1 */
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM2SMEN) != RESET)
#endif /* LPTIM2 */
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM1SMEN) != RESET)
#endif /* LPTIM1 */
#if defined(TIM2)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM2SMEN)   == RESET)
#endif /* TIM2 */
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM3SMEN)   == RESET)
#if defined(TIM6)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM6SMEN)   == RESET)
#endif /* TIM6 */
#if defined(TIM7)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_TIM7SMEN)   == RESET)
#endif /* TIM6 */
#define __HAL_RCC_RTCAPB_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_RTCAPBSMEN) == RESET)
#define __HAL_RCC_WWDG_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_WWDGSMEN)   == RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_SPI2SMEN)   == RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART2SMEN) == RESET)
#if defined(USART3)
#define __HAL_RCC_USART3_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART3SMEN) == RESET)
#endif /* USART3 */
#if defined(USART4)
#define __HAL_RCC_USART4_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_USART4SMEN) == RESET)
#endif /* USART4 */
#if defined(LPUART1)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPUART1SMEN)== RESET)
#endif /* LPUART1 */
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C1SMEN)   == RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_I2C2SMEN)   == RESET)
#if defined(CEC)
#define __HAL_RCC_CEC_IS_CLK_SLEEP_DISABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_CECSMEN)    == RESET)
#endif /* CEC */
#if defined(UCPD1)
#define __HAL_RCC_UCPD1_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD1SMEN)  == RESET)
#endif /* UCPD1 */
#if defined(UCPD2)
#define __HAL_RCC_UCPD2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_UCPD2SMEN)  == RESET)
#endif /* UCPD2 */
#define __HAL_RCC_DBGMCU_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DBGSMEN)    == RESET)
#define __HAL_RCC_PWR_IS_CLK_SLEEP_DISABLED()      (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_PWRSMEN)    == RESET)
#if defined(DAC1)
#define __HAL_RCC_DAC1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_DAC1SMEN)   == RESET)
#endif /* DAC1 */
#if defined(LPTIM2)
#define __HAL_RCC_LPTIM2_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM2SMEN) == RESET)
#endif /* LPTIM2 */
#if defined(LPTIM1)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR1, RCC_APBSMENR1_LPTIM1SMEN) == RESET)
#endif /* LPTIM1 */

/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Sleep_Enabled_Disabled_Status APB2 Peripheral Clock Sleep Enabled or Disabled Status
  * @brief  Check whether the APB2 peripheral clock during Low Power (Sleep) mode is enabled or not.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_SYSCFG_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SYSCFGSMEN) != RESET)
#define __HAL_RCC_TIM1_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM1SMEN)   != RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SPI1SMEN)   != RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_USART1SMEN) != RESET)
#define __HAL_RCC_TIM14_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM14SMEN)  != RESET)
#if defined(TIM15)
#define __HAL_RCC_TIM15_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM15SMEN)  != RESET)
#endif
#define __HAL_RCC_TIM16_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM16SMEN)  != RESET)
#define __HAL_RCC_TIM17_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM17SMEN)  != RESET)
#define __HAL_RCC_ADC_IS_CLK_SLEEP_ENABLED()       (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_ADCSMEN)    != RESET)


#define __HAL_RCC_SYSCFG_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SYSCFGSMEN) == RESET)
#define __HAL_RCC_TIM1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM1SMEN)   == RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_SPI1SMEN)   == RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_USART1SMEN) == RESET)
#define __HAL_RCC_TIM14_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM14SMEN)  == RESET)
#if defined(TIM15)
#define __HAL_RCC_TIM15_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM15SMEN)  == RESET)
#endif
#define __HAL_RCC_TIM16_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM16SMEN)  == RESET)
#define __HAL_RCC_TIM17_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_TIM17SMEN)  == RESET)
#define __HAL_RCC_ADC_IS_CLK_SLEEP_DISABLED()      (READ_BIT(RCC->APBSMENR2 , RCC_APBSMENR2_ADCSMEN)    == RESET)


/**
  * @}
  */

/** @defgroup RCC_Backup_Domain_Reset RCC Backup Domain Reset
  * @{
  */

/** @brief  Macros to force or release the Backup domain reset.
  * @note   This function resets the RTC peripheral (including the backup registers)
  *         and the RTC clock source selection in RCC_CSR register.
  * @note   The BKPSRAM is not affected by this reset.
  * @retval None
  */
#define __HAL_RCC_BACKUPRESET_FORCE()   SET_BIT(RCC->BDCR, RCC_BDCR_BDRST)

#define __HAL_RCC_BACKUPRESET_RELEASE() CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST)

/**
  * @}
  */

/** @defgroup RCC_RTC_Clock_Configuration RCC RTC Clock Configuration
  * @{
  */

/** @brief  Macros to enable or disable the RTC clock.
  * @note   As the RTC is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function before to configure the RTC
  *         (to be done once after reset).
  * @note   These macros must be used after the RTC clock source was selected.
  * @retval None
  */
#define __HAL_RCC_RTC_ENABLE()         SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN)

#define __HAL_RCC_RTC_DISABLE()        CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN)

/**
  * @}
  */

/** @defgroup RCC_Clock_Configuration RCC Clock Configuration
  * @{
  */

/** @brief  Macros to enable the Internal High Speed oscillator (HSI).
  * @note   The HSI is stopped by hardware when entering STOP and STANDBY modes.
  *         It is used (enabled by hardware) as system clock source after startup
  *         from Reset, wakeup from STOP and STANDBY mode, or in case of failure
  *         of the HSE used directly or indirectly as system clock (if the Clock
  *         Security System CSS is enabled).
  * @note   After enabling the HSI, the application software should wait on HSIRDY
  *         flag to be set indicating that HSI clock is stable and can be used as
  *         system clock source.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
#define __HAL_RCC_HSI_ENABLE()  SET_BIT(RCC->CR, RCC_CR_HSION)

/** @brief  Macros to disable the Internal High Speed oscillator (HSI).
  * @note   HSI can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI.
  * @note   When the HSI is stopped, HSIRDY flag goes low after 6 HSI oscillator
  *         clock cycles.
  * @retval None
  */
#define __HAL_RCC_HSI_DISABLE() CLEAR_BIT(RCC->CR, RCC_CR_HSION)

/** @brief  Macro to adjust the Internal High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI RC.
  * @param  __HSICALIBRATIONVALUE__ specifies the calibration trimming value
  *         (default is RCC_HSICALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 127.
  * @retval None
  */
#define __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(__HSICALIBRATIONVALUE__) \
                  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, (uint32_t)(__HSICALIBRATIONVALUE__) << RCC_ICSCR_HSITRIM_Pos)

/**
  * @brief    Macros to enable or disable the force of the Internal High Speed oscillator (HSI)
  *           in STOP mode to be quickly available as kernel clock for USARTs and I2Cs.
  * @note     Keeping the HSI ON in STOP mode allows to avoid slowing down the communication
  *           speed because of the HSI startup time.
  * @note     The enable of this function has not effect on the HSION bit.
  *           This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
#define __HAL_RCC_HSISTOP_ENABLE()     SET_BIT(RCC->CR, RCC_CR_HSIKERON)

#define __HAL_RCC_HSISTOP_DISABLE()    CLEAR_BIT(RCC->CR, RCC_CR_HSIKERON)

/** @brief  Macro to configure the HSISYS clock.
  * @param  __HSIDIV__ specifies the HSI16 division factor.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_HSI_DIV1   HSI clock source is divided by 1
  *            @arg @ref RCC_HSI_DIV2   HSI clock source is divided by 2
  *            @arg @ref RCC_HSI_DIV4   HSI clock source is divided by 4
  *            @arg @ref RCC_HSI_DIV8   HSI clock source is divided by 8
  *            @arg @ref RCC_HSI_DIV16  HSI clock source is divided by 16
  *            @arg @ref RCC_HSI_DIV32  HSI clock source is divided by 32
  *            @arg @ref RCC_HSI_DIV64  HSI clock source is divided by 64
  *            @arg @ref RCC_HSI_DIV128 HSI clock source is divided by 128
  */
#define __HAL_RCC_HSI_CONFIG(__HSIDIV__) \
                 MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, (__HSIDIV__))

/** @brief  Macros to enable or disable the Internal Low Speed oscillator (LSI).
  * @note   After enabling the LSI, the application software should wait on
  *         LSIRDY flag to be set indicating that LSI clock is stable and can
  *         be used to clock the IWDG and/or the RTC.
  * @note   LSI can not be disabled if the IWDG is running.
  * @note   When the LSI is stopped, LSIRDY flag goes low after 6 LSI oscillator
  *         clock cycles.
  * @retval None
  */
#define __HAL_RCC_LSI_ENABLE()         SET_BIT(RCC->CSR, RCC_CSR_LSION)

#define __HAL_RCC_LSI_DISABLE()        CLEAR_BIT(RCC->CSR, RCC_CSR_LSION)

/**
  * @brief  Macro to configure the External High Speed oscillator (HSE).
  * @note   Transition HSE Bypass to HSE On and HSE On to HSE Bypass are not
  *         supported by this macro. User should request a transition to HSE Off
  *         first and then HSE On or HSE Bypass.
  * @note   After enabling the HSE (RCC_HSE_ON or RCC_HSE_Bypass), the application
  *         software should wait on HSERDY flag to be set indicating that HSE clock
  *         is stable and can be used to clock the PLL and/or system clock.
  * @note   HSE state can not be changed if it is used directly or through the
  *         PLL as system clock. In this case, you have to select another source
  *         of the system clock then change the HSE state (ex. disable it).
  * @note   The HSE is stopped by hardware when entering STOP and STANDBY modes.
  * @note   This function reset the CSSON bit, so if the clock security system(CSS)
  *         was previously enabled you have to enable it again after calling this
  *         function.
  * @param  __STATE__  specifies the new state of the HSE.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_HSE_OFF  Turn OFF the HSE oscillator, HSERDY flag goes low after
  *                              6 HSE oscillator clock cycles.
  *            @arg @ref RCC_HSE_ON  Turn ON the HSE oscillator.
  *            @arg @ref RCC_HSE_BYPASS  HSE oscillator bypassed with external clock.
  * @retval None
  */
#define __HAL_RCC_HSE_CONFIG(__STATE__)                      \
                    do {                                     \
                      if((__STATE__) == RCC_HSE_ON)          \
                      {                                      \
                        SET_BIT(RCC->CR, RCC_CR_HSEON);      \
                      }                                      \
                      else if((__STATE__) == RCC_HSE_BYPASS) \
                      {                                      \
                        SET_BIT(RCC->CR, RCC_CR_HSEBYP);     \
                        SET_BIT(RCC->CR, RCC_CR_HSEON);      \
                      }                                      \
                      else                                   \
                      {                                      \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEON);    \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);   \
                      }                                      \
                    } while(0U)

/**
  * @brief  Macro to configure the External Low Speed oscillator (LSE).
  * @note   Transitions LSE Bypass to LSE On and LSE On to LSE Bypass are not
  *         supported by this macro. User should request a transition to LSE Off
  *         first and then LSE On or LSE Bypass.
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function before to configure the LSE
  *         (to be done once after reset).
  * @note   After enabling the LSE (RCC_LSE_ON or RCC_LSE_BYPASS), the application
  *         software should wait on LSERDY flag to be set indicating that LSE clock
  *         is stable and can be used to clock the RTC.
  * @param  __STATE__  specifies the new state of the LSE.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_LSE_OFF  Turn OFF the LSE oscillator, LSERDY flag goes low after
  *                              6 LSE oscillator clock cycles.
  *            @arg @ref RCC_LSE_ON  Turn ON the LSE oscillator.
  *            @arg @ref RCC_LSE_BYPASS  LSE oscillator bypassed with external clock.
  * @retval None
  */
#define __HAL_RCC_LSE_CONFIG(__STATE__)                        \
                    do {                                       \
                      if((__STATE__) == RCC_LSE_ON)            \
                      {                                        \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);    \
                      }                                        \
                      else if((__STATE__) == RCC_LSE_BYPASS)   \
                      {                                        \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);   \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);    \
                      }                                        \
                      else                                     \
                      {                                        \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);  \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEBYP); \
                      }                                        \
                    } while(0U)
/**
  * @}
  */

/** @addtogroup RCC_RTC_Clock_Configuration
  * @{
  */

/** @brief  Macros to configure the RTC clock (RTCCLK).
  * @note   As the RTC clock configuration bits are in the Backup domain and write
  *         access is denied to this domain after reset, you have to enable write
  *         access using the Power Backup Access macro before to configure
  *         the RTC clock source (to be done once after reset).
  * @note   Once the RTC clock is configured it cannot be changed unless the
  *         Backup domain is reset using __HAL_RCC_BACKUPRESET_FORCE() macro, or by
  *         a Power On Reset (POR).
  *
  * @param  __RTC_CLKSOURCE__  specifies the RTC clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RTCCLKSOURCE_NONE No clock selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_LSE  LSE selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_LSI  LSI selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_HSE_DIV32  HSE clock divided by 32 selected
  *
  * @note   If the LSE or LSI is used as RTC clock source, the RTC continues to
  *         work in STOP and STANDBY modes, and can be used as wakeup source.
  *         However, when the HSE clock is used as RTC clock source, the RTC
  *         cannot be used in STOP and STANDBY modes.
  * @note   The maximum input clock frequency for RTC is 1MHz (when using HSE as
  *         RTC clock source).
  * @retval None
  */
#define __HAL_RCC_RTC_CONFIG(__RTC_CLKSOURCE__)  \
                  MODIFY_REG( RCC->BDCR, RCC_BDCR_RTCSEL, (__RTC_CLKSOURCE__))


/** @brief Macro to get the RTC clock source.
  * @retval The returned value can be one of the following:
  *            @arg @ref RCC_RTCCLKSOURCE_NONE No clock selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_LSE  LSE selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_LSI  LSI selected as RTC clock.
  *            @arg @ref RCC_RTCCLKSOURCE_HSE_DIV32  HSE clock divided by 32 selected
  */
#define  __HAL_RCC_GET_RTC_SOURCE() ((uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL)))

/** @brief  Macros to enable or disable the main PLL.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The main PLL can not be disabled if it is used as system clock source
  * @note   The main PLL is disabled by hardware when entering STOP and STANDBY modes.
  * @retval None
  */

/**
  * @}
  */

/** @addtogroup RCC_Clock_Configuration
  * @{
  */

#define __HAL_RCC_PLL_ENABLE()         SET_BIT(RCC->CR, RCC_CR_PLLON)

#define __HAL_RCC_PLL_DISABLE()        CLEAR_BIT(RCC->CR, RCC_CR_PLLON)

/** @brief  Macro to configure the PLL clock source.
  * @note   This function must be used only when the main PLL is disabled.
  * @param  __PLLSOURCE__  specifies the PLL entry clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PLLSOURCE_NONE  No clock selected as PLL clock entry
  *            @arg @ref RCC_PLLSOURCE_HSI  HSI oscillator clock selected as PLL clock entry
  *            @arg @ref RCC_PLLSOURCE_HSE  HSE oscillator clock selected as PLL clock entry
  * @retval None
  *
  */
#define __HAL_RCC_PLL_PLLSOURCE_CONFIG(__PLLSOURCE__) \
                  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC, (__PLLSOURCE__))

/** @brief  Macro to configure the PLL multiplication factor.
  * @note   This function must be used only when the main PLL is disabled.
  * @param  __PLLM__  specifies the division factor for PLL VCO input clock
  *         This parameter must be a value of RCC_PLLM_Clock_Divider.
  * @note   You have to set the PLLM parameter correctly to ensure that the VCO input
  *         frequency ranges from 4 to 16 MHz. It is recommended to select a frequency
  *         of 16 MHz to limit PLL jitter.
  * @retval None
  *
  */
#define __HAL_RCC_PLL_PLLM_CONFIG(__PLLM__) \
                  MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLM, (__PLLM__))

/**
  * @brief  Macro to configure the main PLL clock source, multiplication and division factors.
  * @note   This function must be used only when the main PLL is disabled.
  *
  * @param  __PLLSOURCE__  specifies the PLL entry clock source.
  *         This parameter can be one of the following values:
  *           @arg @ref RCC_PLLSOURCE_NONE  No clock selected as PLL clock entry
  *           @arg @ref RCC_PLLSOURCE_HSI  HSI oscillator clock selected as PLL clock entry
  *           @arg @ref RCC_PLLSOURCE_HSE  HSE oscillator clock selected as PLL clock entry
  *
  * @param  __PLLM__  specifies the division factor for PLL VCO input clock.
  *         This parameter must be a value of RCC_PLLM_Clock_Divider.
  * @note   You have to set the PLLM parameter correctly to ensure that the VCO input
  *         frequency ranges from 4 to 16 MHz. It is recommended to select a frequency
  *         of 16 MHz to limit PLL jitter.
  *
  * @param  __PLLN__  specifies the multiplication factor for PLL VCO output clock.
  *         This parameter must be a number between 8 and 86.
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 64 and 344 MHz.
  *
  * @param  __PLLP__  specifies the division factor for ADC clock.
  *         This parameter must be a value of @ref RCC_PLLP_Clock_Divider.
  *
  * @param  __PLLQ__  specifies the division factor for RBG & HS Timers clocks.(1)
  *         This parameter must be a value of @ref RCC_PLLQ_Clock_Divider
  * @note   (1)__PLLQ__ parameter availability depends on devices
  * @note   If the USB FS is used in your application, you have to set the
  *         PLLQ parameter correctly to have 48 MHz clock for the USB. However,
  *         the RNG needs a frequency lower than or equal to 48 MHz to work
  *         correctly.
  *
  * @param  __PLLR__  specifies the division factor for the main system clock.
  *         This parameter must be a value of RCC_PLLR_Clock_Divider
  * @note   You have to set the PLL parameters correctly to not exceed 64MHZ.
  * @retval None
  */
#if defined(RCC_PLLQ_SUPPORT)
#define __HAL_RCC_PLL_CONFIG(__PLLSOURCE__, __PLLM__, __PLLN__, __PLLP__, __PLLQ__,__PLLR__ ) \
                  (RCC->PLLCFGR = ((uint32_t) (__PLLR__)                           | \
                                   (uint32_t) (__PLLQ__)                           | \
                                   (uint32_t) (__PLLP__)                           | \
                                   (uint32_t) ((__PLLN__) << RCC_PLLCFGR_PLLN_Pos) | \
                                   (uint32_t) (__PLLM__)                           | \
                                   (uint32_t) (__PLLSOURCE__)))
#else
#define __HAL_RCC_PLL_CONFIG(__PLLSOURCE__, __PLLM__, __PLLN__, __PLLP__, __PLLR__ ) \
                  (RCC->PLLCFGR = ((uint32_t) (__PLLR__)                           | \
                                   (uint32_t) (__PLLP__)                           | \
                                   (uint32_t) ((__PLLN__) << RCC_PLLCFGR_PLLN_Pos) | \
                                   (uint32_t) (__PLLM__)                           | \
                                   (uint32_t) (__PLLSOURCE__)))
#endif
/** @brief  Macro to get the oscillator used as PLL clock source.
  * @retval The oscillator used as PLL clock source. The returned value can be one
  *         of the following:
  *              @arg @ref RCC_PLLSOURCE_NONE No oscillator is used as PLL clock source.
  *              @arg @ref RCC_PLLSOURCE_HSI HSI oscillator is used as PLL clock source.
  *              @arg @ref RCC_PLLSOURCE_HSE HSE oscillator is used as PLL clock source.
  */
#define __HAL_RCC_GET_PLL_OSCSOURCE() ((uint32_t)(RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC))

/**
  * @brief  Enable each clock output (RCC_PLLRCLK, RCC_PLLQCLK(*), RCC_PLLPCLK)
  * @note   Enabling clock outputs RCC_PLLPCLK and RCC_PLLQCLK(*) can be done at anytime
  *         without the need to stop the PLL in order to save power. But RCC_PLLRCLK cannot
  *         be stopped if used as System Clock.
  * @note   (*) RCC_PLLQCLK availability depends on devices
  * @param  __PLLCLOCKOUT__ specifies the PLL clock to be output.
  *          This parameter can be one or a combination of the following values:
  *            @arg @ref RCC_PLLPCLK  This clock is used to generate the clock for the ADC.
  * @if defined(STM32G081xx)
  *            @arg @ref RCC_PLLQCLK  This Clock is used to generate the clock for the High Speed Timers,
  *                                  and the random analog generator (<=48 MHz).
  * @endif
  *            @arg @ref RCC_PLLRCLK  This Clock is used to generate the high speed system clock (up to 64MHz)
  * @retval None
  */
#define __HAL_RCC_PLLCLKOUT_ENABLE(__PLLCLOCKOUT__)   SET_BIT(RCC->PLLCFGR, (__PLLCLOCKOUT__))

/**
  * @brief  Disable each clock output (RCC_PLLRCLK, RCC_PLLQCLK(*), RCC_PLLPCLK)
  * @note   Disabling clock outputs RCC_PLLPCLK and RCC_PLLQCLK(*) can be done at anytime
  *         without the need to stop the PLL in order to save power. But RCC_PLLRCLK cannot
  *         be stopped if used as System Clock.
  * @note   (*) RCC_PLLQCLK availability depends on devices
  * @param  __PLLCLOCKOUT__  specifies the PLL clock to be output.
  *          This parameter can be one or a combination of the following values:
  *            @arg @ref RCC_PLLPCLK  This clock may be used to generate the clock for the ADC, I2S1.
  * @if defined(STM32G081xx)
  *            @arg @ref RCC_PLLQCLK This Clock may be used to generate the clock for the High Speed Timers,
  *             and RNG (<=48 MHz).
  * @endif
  *            @arg @ref RCC_PLLRCLK  This Clock is used to generate the high speed system clock (up to 64MHz)
  * @retval None
  */
#define __HAL_RCC_PLLCLKOUT_DISABLE(__PLLCLOCKOUT__)  CLEAR_BIT(RCC->PLLCFGR, (__PLLCLOCKOUT__))

/**
  * @brief  Get clock output enable status (RCC_PLLRCLK, RCC_PLLQCLK(*), RCC_PLLPCLK)
  * @param  __PLLCLOCKOUT__ specifies the output PLL clock to be checked.
  *          This parameter can be one of the following values:
  *            @arg RCC_PLLPCLK This clock may be used to generate the clock for ADC, I2S1.
  * @if defined(STM32G081xx)
  *            @arg RCC_PLLQCLK This Clock may be used to generate the clock for the HS Timers,
  *                                   the RNG (<=48 MHz).
  * @endif
  *            @arg @ref RCC_PLLRCLK  This Clock is used to generate the high speed system clock (up to 64MHz)
  * @retval SET / RESET
  * @note   (*) RCC_PLLQCLK availability depends on devices
  */
#define __HAL_RCC_GET_PLLCLKOUT_CONFIG(__PLLCLOCKOUT__)  READ_BIT(RCC->PLLCFGR, (__PLLCLOCKOUT__))

/**
  * @brief  Macro to configure the system clock source.
  * @param  __SYSCLKSOURCE__ specifies the system clock source.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_SYSCLKSOURCE_HSI HSI oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_HSE HSE oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_PLLCLK PLL output is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_LSI LSI oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_LSE LSE oscillator is used as system clock source.
  * @retval None
  */
#define __HAL_RCC_SYSCLK_CONFIG(__SYSCLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, (__SYSCLKSOURCE__))

/** @brief  Macro to get the clock source used as system clock.
  * @retval The clock source used as system clock. The returned value can be one
  *         of the following:
  *              @arg @ref RCC_SYSCLKSOURCE_STATUS_HSI HSI used as system clock.
  *              @arg @ref RCC_SYSCLKSOURCE_STATUS_HSE HSE used as system clock.
  *              @arg @ref RCC_SYSCLKSOURCE_STATUS_PLLCLK PLL used as system clock.
  *              @arg @ref RCC_SYSCLKSOURCE_STATUS_LSI LSI used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_STATUS_LSE LSE used as system clock source.
  */
#define __HAL_RCC_GET_SYSCLK_SOURCE()         (RCC->CFGR & RCC_CFGR_SWS)

/**
  * @brief  Macro to configure the External Low Speed oscillator (LSE) drive capability.
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function before to configure the LSE
  *         (to be done once after reset).
  * @param  __LSEDRIVE__ specifies the new state of the LSE drive capability.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LSEDRIVE_LOW LSE oscillator low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMLOW LSE oscillator medium low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMHIGH LSE oscillator medium high drive capability.
  *            @arg @ref RCC_LSEDRIVE_HIGH LSE oscillator high drive capability.
  * @retval None
  */
#define __HAL_RCC_LSEDRIVE_CONFIG(__LSEDRIVE__) \
                  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSEDRV, (uint32_t)(__LSEDRIVE__))

/** @brief  Macro to configure the MCO clock.
  * @param  __MCOCLKSOURCE__ specifies the MCO clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK  MCO output disabled
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK System  clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_HSI HSI clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_HSE HSE clock selected as MCO sourcee
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK  Main PLL clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_LSI LSI clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_LSE LSE clock selected as MCO source
  * @param  __MCODIV__ specifies the MCO clock prescaler.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCODIV_1   MCO clock source is divided by 1
  *            @arg @ref RCC_MCODIV_2   MCO clock source is divided by 2
  *            @arg @ref RCC_MCODIV_4   MCO clock source is divided by 4
  *            @arg @ref RCC_MCODIV_8   MCO clock source is divided by 8
  *            @arg @ref RCC_MCODIV_16  MCO clock source is divided by 16
  */
#define __HAL_RCC_MCO1_CONFIG(__MCOCLKSOURCE__, __MCODIV__) \
                 MODIFY_REG(RCC->CFGR, (RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE), ((__MCOCLKSOURCE__) | (__MCODIV__)))

/**
  * @}
  */

/** @defgroup RCC_Flags_Interrupts_Management Flags Interrupts Management
  * @brief macros to manage the specified RCC Flags and interrupts.
  * @{
  */

/** @brief  Enable RCC interrupt.
  * @param  __INTERRUPT__ specifies the RCC interrupt sources to be enabled.
  *         This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY  Main PLL ready interrupt
  * @retval None
  */
#define __HAL_RCC_ENABLE_IT(__INTERRUPT__) SET_BIT(RCC->CIER, (__INTERRUPT__))

/** @brief Disable RCC interrupt.
  * @param  __INTERRUPT__ specifies the RCC interrupt sources to be disabled.
  *         This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY  Main PLL ready interrupt
  * @retval None
  */
#define __HAL_RCC_DISABLE_IT(__INTERRUPT__) CLEAR_BIT(RCC->CIER, (__INTERRUPT__))

/** @brief  Clear RCC interrupt pending bits.
  * @param  __INTERRUPT__ specifies the interrupt pending bit to clear.
  *         This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY  Main PLL ready interrupt
  *            @arg @ref RCC_IT_CSS     HSE Clock security system interrupt
  *            @arg @ref RCC_IT_LSECSS  LSE Clock security system interrupt
  * @retval None
  */
#define __HAL_RCC_CLEAR_IT(__INTERRUPT__) (RCC->CICR = (__INTERRUPT__))

/** @brief  Check whether the RCC interrupt has occurred or not.
  * @param  __INTERRUPT__ specifies the RCC interrupt source to check.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY  Main PLL ready interrupt
  *            @arg @ref RCC_IT_CSS     HSE Clock security system interrupt
  *            @arg @ref RCC_IT_LSECSS  LSE Clock security system interrupt
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_IT(__INTERRUPT__) ((RCC->CIFR & (__INTERRUPT__)) == (__INTERRUPT__))

/** @brief Set RMVF bit to clear the reset flags.
  *        The reset flags are: RCC_FLAG_OBLRST, RCC_FLAG_PINRST, RCC_FLAG_PWRRST,
  *        RCC_FLAG_SFTRST, RCC_FLAG_IWDGRST, RCC_FLAG_WWDGRST and RCC_FLAG_LPWRRST.
  * @retval None
  */
#define __HAL_RCC_CLEAR_RESET_FLAGS() (RCC->CSR |= RCC_CSR_RMVF)

/** @brief  Check whether the selected RCC flag is set or not.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_FLAG_HSIRDY HSI oscillator clock ready
  *            @arg @ref RCC_FLAG_HSERDY HSE oscillator clock ready
  *            @arg @ref RCC_FLAG_PLLRDY  Main PLL clock ready
  *            @arg @ref RCC_FLAG_LSERDY LSE oscillator clock ready
  *            @arg @ref RCC_FLAG_LSECSSD Clock security system failure on LSE oscillator detection
  *            @arg @ref RCC_FLAG_LSIRDY LSI oscillator clock ready
  *            @arg @ref RCC_FLAG_PWRRST BOR or POR/PDR reset
  *            @arg @ref RCC_FLAG_OBLRST OBLRST reset
  *            @arg @ref RCC_FLAG_PINRST Pin reset
  *            @arg @ref RCC_FLAG_SFTRST Software reset
  *            @arg @ref RCC_FLAG_IWDGRST Independent Watchdog reset
  *            @arg @ref RCC_FLAG_WWDGRST Window Watchdog reset
  *            @arg @ref RCC_FLAG_LPWRRST Low Power reset
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_FLAG(__FLAG__) (((((((__FLAG__) >> 5U) == CR_REG_INDEX) ? RCC->CR :                  \
                                        ((((__FLAG__) >> 5U) == BDCR_REG_INDEX) ? RCC->BDCR :              \
                                        ((((__FLAG__) >> 5U) == CSR_REG_INDEX) ? RCC->CSR : RCC->CIFR))) & \
                                          (1U << ((__FLAG__) & RCC_FLAG_MASK))) != RESET) \
                                            ? 1U : 0U)

/**
  * @}
  */

/**
  * @}
  */

/* Include RCC HAL Extended module */
#include "stm32g0xx_hal_rcc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCC_Exported_Functions
  * @{
  */


/** @addtogroup RCC_Exported_Functions_Group1
  * @{
  */

/* Initialization and de-initialization functions  ******************************/
HAL_StatusTypeDef HAL_RCC_DeInit(void);
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency);

/**
  * @}
  */

/** @addtogroup RCC_Exported_Functions_Group2
  * @{
  */

/* Peripheral Control functions  ************************************************/
void              HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv);
void              HAL_RCC_EnableCSS(void);
void              HAL_RCC_EnableLSECSS(void);
void              HAL_RCC_DisableLSECSS(void);
uint32_t          HAL_RCC_GetSysClockFreq(void);
uint32_t          HAL_RCC_GetHCLKFreq(void);
uint32_t          HAL_RCC_GetPCLK1Freq(void);
void              HAL_RCC_GetOscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
void              HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t *pFLatency);
/* LSE & HSE CSS NMI IRQ handler */
void              HAL_RCC_NMI_IRQHandler(void);
/* User Callbacks in non blocking mode (IT mode) */
void              HAL_RCC_CSSCallback(void);
void              HAL_RCC_LSECSSCallback(void);

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

#endif /* STM32G0xx_HAL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
