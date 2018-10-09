/**
  ******************************************************************************
  * @file    stm32f0xx_hal_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL module.
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
#ifndef __STM32F0xx_HAL_RCC_H
#define __STM32F0xx_HAL_RCC_H

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

/** @addtogroup RCC_Private_Constants
  * @{
  */

/** @defgroup RCC_Timeout RCC Timeout
  * @{
  */

/* Disable Backup domain write protection state change timeout */
#define RCC_DBP_TIMEOUT_VALUE      (100U)       /* 100 ms */
/* LSE state change timeout */
#define RCC_LSE_TIMEOUT_VALUE      LSE_STARTUP_TIMEOUT
#define CLOCKSWITCH_TIMEOUT_VALUE  (5000U)  /* 5 s    */
#define HSE_TIMEOUT_VALUE          HSE_STARTUP_TIMEOUT
#define HSI_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define LSI_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define PLL_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define HSI14_TIMEOUT_VALUE        (2U)      /* 2 ms (minimum Tick + 1U) */
#if defined(RCC_HSI48_SUPPORT)
#define HSI48_TIMEOUT_VALUE        (2U)      /* 2 ms (minimum Tick + 1U) */
#endif /* RCC_HSI48_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_Register_Offset Register offsets
  * @{
  */
#define RCC_OFFSET                (RCC_BASE - PERIPH_BASE)
#define RCC_CR_OFFSET             0x00
#define RCC_CFGR_OFFSET           0x04
#define RCC_CIR_OFFSET            0x08
#define RCC_BDCR_OFFSET           0x20
#define RCC_CSR_OFFSET            0x24

/**
  * @}
  */


/* CR register byte 2 (Bits[23:16]) base address */
#define RCC_CR_BYTE2_ADDRESS          ((uint32_t)(RCC_BASE + RCC_CR_OFFSET + 0x02U))

/* CIR register byte 1 (Bits[15:8]) base address */
#define RCC_CIR_BYTE1_ADDRESS     ((uint32_t)(RCC_BASE + RCC_CIR_OFFSET + 0x01U))

/* CIR register byte 2 (Bits[23:16]) base address */
#define RCC_CIR_BYTE2_ADDRESS     ((uint32_t)(RCC_BASE + RCC_CIR_OFFSET + 0x02U))

/* Defines used for Flags */
#define CR_REG_INDEX                     ((uint8_t)1U)
#define CR2_REG_INDEX                    ((uint8_t)2U)
#define BDCR_REG_INDEX                   ((uint8_t)3U)
#define CSR_REG_INDEX                    ((uint8_t)4U)

/* Bits position in  in the CFGR register */
#define RCC_CFGR_PLLMUL_BITNUMBER         18U
#define RCC_CFGR_HPRE_BITNUMBER           4U
#define RCC_CFGR_PPRE_BITNUMBER           8U
/* Flags in the CFGR2 register */
#define RCC_CFGR2_PREDIV_BITNUMBER        0
/* Flags in the CR register */
#define RCC_CR_HSIRDY_BitNumber           1
#define RCC_CR_HSERDY_BitNumber           17
#define RCC_CR_PLLRDY_BitNumber           25
/* Flags in the CR2 register */
#define RCC_CR2_HSI14RDY_BitNumber        1
#define RCC_CR2_HSI48RDY_BitNumber       16
/* Flags in the BDCR register */
#define RCC_BDCR_LSERDY_BitNumber         1
/* Flags in the CSR register */
#define RCC_CSR_LSIRDY_BitNumber          1
#define RCC_CSR_V18PWRRSTF_BitNumber      23
#define RCC_CSR_RMVF_BitNumber            24
#define RCC_CSR_OBLRSTF_BitNumber         25
#define RCC_CSR_PINRSTF_BitNumber         26
#define RCC_CSR_PORRSTF_BitNumber         27
#define RCC_CSR_SFTRSTF_BitNumber         28
#define RCC_CSR_IWDGRSTF_BitNumber        29
#define RCC_CSR_WWDGRSTF_BitNumber        30
#define RCC_CSR_LPWRRSTF_BitNumber        31
/* Flags in the HSITRIM register */
#define RCC_CR_HSITRIM_BitNumber          3
#define RCC_HSI14TRIM_BIT_NUMBER          3
#define RCC_FLAG_MASK                    ((uint8_t)0x1FU)

/**
  * @}
  */

/** @addtogroup RCC_Private_Macros
  * @{
  */
#define IS_RCC_HSE(__HSE__) (((__HSE__) == RCC_HSE_OFF) || ((__HSE__) == RCC_HSE_ON) || \
                             ((__HSE__) == RCC_HSE_BYPASS))
#define IS_RCC_LSE(__LSE__) (((__LSE__) == RCC_LSE_OFF) || ((__LSE__) == RCC_LSE_ON) || \
                             ((__LSE__) == RCC_LSE_BYPASS))
#define IS_RCC_HSI(__HSI__) (((__HSI__) == RCC_HSI_OFF) || ((__HSI__) == RCC_HSI_ON))
#define IS_RCC_HSI14(__HSI14__) (((__HSI14__) == RCC_HSI14_OFF) || ((__HSI14__) == RCC_HSI14_ON) || ((__HSI14__) == RCC_HSI14_ADC_CONTROL))
#define IS_RCC_CALIBRATION_VALUE(__VALUE__) ((__VALUE__) <= 0x1FU)
#define IS_RCC_LSI(__LSI__) (((__LSI__) == RCC_LSI_OFF) || ((__LSI__) == RCC_LSI_ON))
#define IS_RCC_PLL(__PLL__) (((__PLL__) == RCC_PLL_NONE) || ((__PLL__) == RCC_PLL_OFF) || \
                             ((__PLL__) == RCC_PLL_ON))
#define IS_RCC_PREDIV(__PREDIV__) (((__PREDIV__) == RCC_PREDIV_DIV1)  || ((__PREDIV__) == RCC_PREDIV_DIV2)   || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV3)  || ((__PREDIV__) == RCC_PREDIV_DIV4)   || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV5)  || ((__PREDIV__) == RCC_PREDIV_DIV6)   || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV7)  || ((__PREDIV__) == RCC_PREDIV_DIV8)   || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV9)  || ((__PREDIV__) == RCC_PREDIV_DIV10)  || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV11) || ((__PREDIV__) == RCC_PREDIV_DIV12)  || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV13) || ((__PREDIV__) == RCC_PREDIV_DIV14)  || \
                                  ((__PREDIV__) == RCC_PREDIV_DIV15) || ((__PREDIV__) == RCC_PREDIV_DIV16))

#define IS_RCC_PLL_MUL(__MUL__) (((__MUL__) == RCC_PLL_MUL2)  || ((__MUL__) == RCC_PLL_MUL3)   || \
                                 ((__MUL__) == RCC_PLL_MUL4)  || ((__MUL__) == RCC_PLL_MUL5)   || \
                                 ((__MUL__) == RCC_PLL_MUL6)  || ((__MUL__) == RCC_PLL_MUL7)   || \
                                 ((__MUL__) == RCC_PLL_MUL8)  || ((__MUL__) == RCC_PLL_MUL9)   || \
                                 ((__MUL__) == RCC_PLL_MUL10) || ((__MUL__) == RCC_PLL_MUL11)  || \
                                 ((__MUL__) == RCC_PLL_MUL12) || ((__MUL__) == RCC_PLL_MUL13)  || \
                                 ((__MUL__) == RCC_PLL_MUL14) || ((__MUL__) == RCC_PLL_MUL15)  || \
                                 ((__MUL__) == RCC_PLL_MUL16))
#define IS_RCC_CLOCKTYPE(__CLK__) ((((__CLK__) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK) || \
                                   (((__CLK__) & RCC_CLOCKTYPE_HCLK)   == RCC_CLOCKTYPE_HCLK)   || \
                                   (((__CLK__) & RCC_CLOCKTYPE_PCLK1)  == RCC_CLOCKTYPE_PCLK1))
#define IS_RCC_HCLK(__HCLK__) (((__HCLK__) == RCC_SYSCLK_DIV1) || ((__HCLK__) == RCC_SYSCLK_DIV2) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV4) || ((__HCLK__) == RCC_SYSCLK_DIV8) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV16) || ((__HCLK__) == RCC_SYSCLK_DIV64) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV128) || ((__HCLK__) == RCC_SYSCLK_DIV256) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV512))
#define IS_RCC_PCLK(__PCLK__) (((__PCLK__) == RCC_HCLK_DIV1) || ((__PCLK__) == RCC_HCLK_DIV2) || \
                               ((__PCLK__) == RCC_HCLK_DIV4) || ((__PCLK__) == RCC_HCLK_DIV8) || \
                               ((__PCLK__) == RCC_HCLK_DIV16))
#define IS_RCC_MCO(__MCO__)  ((__MCO__) == RCC_MCO)
#define IS_RCC_RTCCLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_RTCCLKSOURCE_NO_CLK) || \
                                          ((__SOURCE__) == RCC_RTCCLKSOURCE_LSE)  || \
                                          ((__SOURCE__) == RCC_RTCCLKSOURCE_LSI)  || \
                                          ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV32))
#define IS_RCC_USART1CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USART1CLKSOURCE_PCLK1)  || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_SYSCLK) || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_LSE)    || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_HSI))
#define IS_RCC_I2C1CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_I2C1CLKSOURCE_HSI) || \
                                           ((__SOURCE__) == RCC_I2C1CLKSOURCE_SYSCLK))

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
  uint32_t PLLState;      /*!< PLLState: The new state of the PLL.
                              This parameter can be a value of @ref RCC_PLL_Config */

  uint32_t PLLSource;     /*!< PLLSource: PLL entry clock source.
                              This parameter must be a value of @ref RCC_PLL_Clock_Source */

  uint32_t PLLMUL;        /*!< PLLMUL: Multiplication factor for PLL VCO input clock
                              This parameter must be a value of @ref RCC_PLL_Multiplication_Factor*/

  uint32_t PREDIV;        /*!< PREDIV: Predivision factor for PLL VCO input clock
                              This parameter must be a value of @ref RCC_PLL_Prediv_Factor */

} RCC_PLLInitTypeDef;

/**
  * @brief  RCC Internal/External Oscillator (HSE, HSI, LSE and LSI) configuration structure definition
  */
typedef struct
{
  uint32_t OscillatorType;        /*!< The oscillators to be configured.
                                       This parameter can be a value of @ref RCC_Oscillator_Type */

  uint32_t HSEState;              /*!< The new state of the HSE.
                                       This parameter can be a value of @ref RCC_HSE_Config */

  uint32_t LSEState;              /*!< The new state of the LSE.
                                       This parameter can be a value of @ref RCC_LSE_Config */

  uint32_t HSIState;              /*!< The new state of the HSI.
                                       This parameter can be a value of @ref RCC_HSI_Config */

  uint32_t HSICalibrationValue;   /*!< The HSI calibration trimming value (default is RCC_HSICALIBRATION_DEFAULT).
                                       This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x1FU */

  uint32_t HSI14State;             /*!< The new state of the HSI14.
                                        This parameter can be a value of @ref RCC_HSI14_Config */

  uint32_t HSI14CalibrationValue;  /*!< The HSI14 calibration trimming value (default is RCC_HSI14CALIBRATION_DEFAULT).
                                        This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x1FU */

  uint32_t LSIState;              /*!< The new state of the LSI.
                                       This parameter can be a value of @ref RCC_LSI_Config */

#if defined(RCC_HSI48_SUPPORT)
  uint32_t HSI48State;            /*!< The new state of the HSI48.
                                       This parameter can be a value of @ref RCC_HSI48_Config */

#endif /* RCC_HSI48_SUPPORT */
  RCC_PLLInitTypeDef PLL;         /*!< PLL structure parameters */

} RCC_OscInitTypeDef;

/**
  * @brief  RCC System, AHB and APB busses clock configuration structure definition
  */
typedef struct
{
  uint32_t ClockType;             /*!< The clock to be configured.
                                       This parameter can be a value of @ref RCC_System_Clock_Type */

  uint32_t SYSCLKSource;          /*!< The clock source (SYSCLKS) used as system clock.
                                       This parameter can be a value of @ref RCC_System_Clock_Source */

  uint32_t AHBCLKDivider;         /*!< The AHB clock (HCLK) divider. This clock is derived from the system clock (SYSCLK).
                                       This parameter can be a value of @ref RCC_AHB_Clock_Source */

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

/** @defgroup RCC_PLL_Clock_Source PLL Clock Source
  * @{
  */

#define RCC_PLLSOURCE_HSE           RCC_CFGR_PLLSRC_HSE_PREDIV /*!< HSE clock selected as PLL entry clock source */

/**
  * @}
  */

/** @defgroup RCC_Oscillator_Type Oscillator Type
  * @{
  */
#define RCC_OSCILLATORTYPE_NONE            (0x00000000U)
#define RCC_OSCILLATORTYPE_HSE             (0x00000001U)
#define RCC_OSCILLATORTYPE_HSI             (0x00000002U)
#define RCC_OSCILLATORTYPE_LSE             (0x00000004U)
#define RCC_OSCILLATORTYPE_LSI             (0x00000008U)
#define RCC_OSCILLATORTYPE_HSI14           (0x00000010U)
#if defined(RCC_HSI48_SUPPORT)
#define RCC_OSCILLATORTYPE_HSI48           (0x00000020U)
#endif /* RCC_HSI48_SUPPORT */
/**
  * @}
  */

/** @defgroup RCC_HSE_Config HSE Config
  * @{
  */
#define RCC_HSE_OFF                      (0x00000000U)                     /*!< HSE clock deactivation */
#define RCC_HSE_ON                       (0x00000001U)                     /*!< HSE clock activation */
#define RCC_HSE_BYPASS                   (0x00000005U)                     /*!< External clock source for HSE clock */
/**
  * @}
  */

/** @defgroup RCC_LSE_Config LSE Config
  * @{
  */
#define RCC_LSE_OFF                      (0x00000000U)                       /*!< LSE clock deactivation */
#define RCC_LSE_ON                       (0x00000001U)                       /*!< LSE clock activation */
#define RCC_LSE_BYPASS                   (0x00000005U)                       /*!< External clock source for LSE clock */

/**
  * @}
  */

/** @defgroup RCC_HSI_Config HSI Config
  * @{
  */
#define RCC_HSI_OFF                      (0x00000000U)           /*!< HSI clock deactivation */
#define RCC_HSI_ON                       RCC_CR_HSION                     /*!< HSI clock activation */

#define RCC_HSICALIBRATION_DEFAULT       (0x10U)         /* Default HSI calibration trimming value */

/**
  * @}
  */

/** @defgroup RCC_HSI14_Config RCC HSI14 Config
  * @{
  */
#define RCC_HSI14_OFF                    (0x00000000U)
#define RCC_HSI14_ON                     RCC_CR2_HSI14ON
#define RCC_HSI14_ADC_CONTROL            (~RCC_CR2_HSI14DIS)

#define RCC_HSI14CALIBRATION_DEFAULT     (0x10U)   /* Default HSI14 calibration trimming value */
/**
  * @}
  */

/** @defgroup RCC_LSI_Config LSI Config
  * @{
  */
#define RCC_LSI_OFF                      (0x00000000U)   /*!< LSI clock deactivation */
#define RCC_LSI_ON                       RCC_CSR_LSION            /*!< LSI clock activation */

/**
  * @}
  */

#if defined(RCC_HSI48_SUPPORT)
/** @defgroup RCC_HSI48_Config HSI48 Config
  * @{
  */
#define RCC_HSI48_OFF               ((uint8_t)0x00U)
#define RCC_HSI48_ON                ((uint8_t)0x01U)

/**
  * @}
  */
#endif /* RCC_HSI48_SUPPORT */

/** @defgroup RCC_PLL_Config PLL Config
  * @{
  */
#define RCC_PLL_NONE                      (0x00000000U)  /*!< PLL is not configured */
#define RCC_PLL_OFF                       (0x00000001U)  /*!< PLL deactivation */
#define RCC_PLL_ON                        (0x00000002U)  /*!< PLL activation */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Type System Clock Type
  * @{
  */
#define RCC_CLOCKTYPE_SYSCLK             (0x00000001U) /*!< SYSCLK to configure */
#define RCC_CLOCKTYPE_HCLK               (0x00000002U) /*!< HCLK to configure */
#define RCC_CLOCKTYPE_PCLK1              (0x00000004U) /*!< PCLK1 to configure */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source System Clock Source
  * @{
  */
#define RCC_SYSCLKSOURCE_HSI             RCC_CFGR_SW_HSI /*!< HSI selected as system clock */
#define RCC_SYSCLKSOURCE_HSE             RCC_CFGR_SW_HSE /*!< HSE selected as system clock */
#define RCC_SYSCLKSOURCE_PLLCLK          RCC_CFGR_SW_PLL /*!< PLL selected as system clock */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source_Status System Clock Source Status
  * @{
  */
#define RCC_SYSCLKSOURCE_STATUS_HSI      RCC_CFGR_SWS_HSI            /*!< HSI used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_HSE      RCC_CFGR_SWS_HSE            /*!< HSE used as system clock */
#define RCC_SYSCLKSOURCE_STATUS_PLLCLK   RCC_CFGR_SWS_PLL            /*!< PLL used as system clock */

/**
  * @}
  */

/** @defgroup RCC_AHB_Clock_Source AHB Clock Source
  * @{
  */
#define RCC_SYSCLK_DIV1                  RCC_CFGR_HPRE_DIV1   /*!< SYSCLK not divided */
#define RCC_SYSCLK_DIV2                  RCC_CFGR_HPRE_DIV2   /*!< SYSCLK divided by 2 */
#define RCC_SYSCLK_DIV4                  RCC_CFGR_HPRE_DIV4   /*!< SYSCLK divided by 4 */
#define RCC_SYSCLK_DIV8                  RCC_CFGR_HPRE_DIV8   /*!< SYSCLK divided by 8 */
#define RCC_SYSCLK_DIV16                 RCC_CFGR_HPRE_DIV16  /*!< SYSCLK divided by 16 */
#define RCC_SYSCLK_DIV64                 RCC_CFGR_HPRE_DIV64  /*!< SYSCLK divided by 64 */
#define RCC_SYSCLK_DIV128                RCC_CFGR_HPRE_DIV128 /*!< SYSCLK divided by 128 */
#define RCC_SYSCLK_DIV256                RCC_CFGR_HPRE_DIV256 /*!< SYSCLK divided by 256 */
#define RCC_SYSCLK_DIV512                RCC_CFGR_HPRE_DIV512 /*!< SYSCLK divided by 512 */

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Source RCC APB1 Clock Source
  * @{
  */
#define RCC_HCLK_DIV1                    RCC_CFGR_PPRE_DIV1  /*!< HCLK not divided */
#define RCC_HCLK_DIV2                    RCC_CFGR_PPRE_DIV2  /*!< HCLK divided by 2 */
#define RCC_HCLK_DIV4                    RCC_CFGR_PPRE_DIV4  /*!< HCLK divided by 4 */
#define RCC_HCLK_DIV8                    RCC_CFGR_PPRE_DIV8  /*!< HCLK divided by 8 */
#define RCC_HCLK_DIV16                   RCC_CFGR_PPRE_DIV16 /*!< HCLK divided by 16 */

/**
  * @}
  */

/** @defgroup RCC_RTC_Clock_Source RTC Clock Source
  * @{
  */
#define RCC_RTCCLKSOURCE_NO_CLK          (0x00000000U)                 /*!< No clock */
#define RCC_RTCCLKSOURCE_LSE             RCC_BDCR_RTCSEL_LSE                  /*!< LSE oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_LSI             RCC_BDCR_RTCSEL_LSI                  /*!< LSI oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV32       RCC_BDCR_RTCSEL_HSE                    /*!< HSE oscillator clock divided by 32 used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_PLL_Multiplication_Factor RCC PLL Multiplication Factor
  * @{
  */
#define RCC_PLL_MUL2                     RCC_CFGR_PLLMUL2
#define RCC_PLL_MUL3                     RCC_CFGR_PLLMUL3
#define RCC_PLL_MUL4                     RCC_CFGR_PLLMUL4
#define RCC_PLL_MUL5                     RCC_CFGR_PLLMUL5
#define RCC_PLL_MUL6                     RCC_CFGR_PLLMUL6
#define RCC_PLL_MUL7                     RCC_CFGR_PLLMUL7
#define RCC_PLL_MUL8                     RCC_CFGR_PLLMUL8
#define RCC_PLL_MUL9                     RCC_CFGR_PLLMUL9
#define RCC_PLL_MUL10                    RCC_CFGR_PLLMUL10
#define RCC_PLL_MUL11                    RCC_CFGR_PLLMUL11
#define RCC_PLL_MUL12                    RCC_CFGR_PLLMUL12
#define RCC_PLL_MUL13                    RCC_CFGR_PLLMUL13
#define RCC_PLL_MUL14                    RCC_CFGR_PLLMUL14
#define RCC_PLL_MUL15                    RCC_CFGR_PLLMUL15
#define RCC_PLL_MUL16                    RCC_CFGR_PLLMUL16

/**
  * @}
  */

/** @defgroup RCC_PLL_Prediv_Factor RCC PLL Prediv Factor
  * @{
  */

#define RCC_PREDIV_DIV1                  RCC_CFGR2_PREDIV_DIV1
#define RCC_PREDIV_DIV2                  RCC_CFGR2_PREDIV_DIV2
#define RCC_PREDIV_DIV3                  RCC_CFGR2_PREDIV_DIV3
#define RCC_PREDIV_DIV4                  RCC_CFGR2_PREDIV_DIV4
#define RCC_PREDIV_DIV5                  RCC_CFGR2_PREDIV_DIV5
#define RCC_PREDIV_DIV6                  RCC_CFGR2_PREDIV_DIV6
#define RCC_PREDIV_DIV7                  RCC_CFGR2_PREDIV_DIV7
#define RCC_PREDIV_DIV8                  RCC_CFGR2_PREDIV_DIV8
#define RCC_PREDIV_DIV9                  RCC_CFGR2_PREDIV_DIV9
#define RCC_PREDIV_DIV10                 RCC_CFGR2_PREDIV_DIV10
#define RCC_PREDIV_DIV11                 RCC_CFGR2_PREDIV_DIV11
#define RCC_PREDIV_DIV12                 RCC_CFGR2_PREDIV_DIV12
#define RCC_PREDIV_DIV13                 RCC_CFGR2_PREDIV_DIV13
#define RCC_PREDIV_DIV14                 RCC_CFGR2_PREDIV_DIV14
#define RCC_PREDIV_DIV15                 RCC_CFGR2_PREDIV_DIV15
#define RCC_PREDIV_DIV16                 RCC_CFGR2_PREDIV_DIV16

/**
  * @}
  */


/** @defgroup RCC_USART1_Clock_Source RCC USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_PCLK1        RCC_CFGR3_USART1SW_PCLK
#define RCC_USART1CLKSOURCE_SYSCLK       RCC_CFGR3_USART1SW_SYSCLK
#define RCC_USART1CLKSOURCE_LSE          RCC_CFGR3_USART1SW_LSE
#define RCC_USART1CLKSOURCE_HSI          RCC_CFGR3_USART1SW_HSI

/**
  * @}
  */

/** @defgroup RCC_I2C1_Clock_Source RCC I2C1 Clock Source
  * @{
  */
#define RCC_I2C1CLKSOURCE_HSI            RCC_CFGR3_I2C1SW_HSI
#define RCC_I2C1CLKSOURCE_SYSCLK         RCC_CFGR3_I2C1SW_SYSCLK

/**
  * @}
  */
/** @defgroup RCC_MCO_Index MCO Index
  * @{
  */
#define RCC_MCO1                         (0x00000000U)
#define RCC_MCO                          RCC_MCO1               /*!< MCO1 to be compliant with other families with 2 MCOs*/

/**
  * @}
  */

/** @defgroup RCC_MCO_Clock_Source RCC MCO Clock Source
  * @{
  */
#define RCC_MCO1SOURCE_NOCLOCK            RCC_CFGR_MCO_NOCLOCK
#define RCC_MCO1SOURCE_LSI                RCC_CFGR_MCO_LSI
#define RCC_MCO1SOURCE_LSE                RCC_CFGR_MCO_LSE
#define RCC_MCO1SOURCE_SYSCLK             RCC_CFGR_MCO_SYSCLK
#define RCC_MCO1SOURCE_HSI                RCC_CFGR_MCO_HSI
#define RCC_MCO1SOURCE_HSE                RCC_CFGR_MCO_HSE
#define RCC_MCO1SOURCE_PLLCLK_DIV2        RCC_CFGR_MCO_PLL
#define RCC_MCO1SOURCE_HSI14              RCC_CFGR_MCO_HSI14

/**
  * @}
  */

/** @defgroup RCC_Interrupt Interrupts
  * @{
  */
#define RCC_IT_LSIRDY                    ((uint8_t)RCC_CIR_LSIRDYF)   /*!< LSI Ready Interrupt flag */
#define RCC_IT_LSERDY                    ((uint8_t)RCC_CIR_LSERDYF)   /*!< LSE Ready Interrupt flag */
#define RCC_IT_HSIRDY                    ((uint8_t)RCC_CIR_HSIRDYF)   /*!< HSI Ready Interrupt flag */
#define RCC_IT_HSERDY                    ((uint8_t)RCC_CIR_HSERDYF)   /*!< HSE Ready Interrupt flag */
#define RCC_IT_PLLRDY                    ((uint8_t)RCC_CIR_PLLRDYF)   /*!< PLL Ready Interrupt flag */
#define RCC_IT_HSI14RDY                  ((uint8_t)RCC_CIR_HSI14RDYF) /*!< HSI14 Ready Interrupt flag */
#if defined(RCC_CIR_HSI48RDYF)
#define RCC_IT_HSI48RDY                  ((uint8_t)RCC_CIR_HSI48RDYF) /*!< HSI48 Ready Interrupt flag */
#endif
#define RCC_IT_CSS                       ((uint8_t)RCC_CIR_CSSF)      /*!< Clock Security System Interrupt flag */
/**
  * @}
  */

/** @defgroup RCC_Flag Flags
  *        Elements values convention: XXXYYYYYb
  *           - YYYYY  : Flag position in the register
  *           - XXX  : Register index
  *                 - 001: CR register
  *                 - 010: CR2 register
  *                 - 011: BDCR register
  *                 - 0100: CSR register
  * @{
  */
/* Flags in the CR register */
#define RCC_FLAG_HSIRDY                  ((uint8_t)((CR_REG_INDEX << 5U) | RCC_CR_HSIRDY_BitNumber))
#define RCC_FLAG_HSERDY                  ((uint8_t)((CR_REG_INDEX << 5U) | RCC_CR_HSERDY_BitNumber))
#define RCC_FLAG_PLLRDY                  ((uint8_t)((CR_REG_INDEX << 5U) | RCC_CR_PLLRDY_BitNumber))
/* Flags in the CR2 register */
#define RCC_FLAG_HSI14RDY                ((uint8_t)((CR2_REG_INDEX << 5U) | RCC_CR2_HSI14RDY_BitNumber))

/* Flags in the CSR register */
#define RCC_FLAG_LSIRDY                  ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_LSIRDY_BitNumber))
#if   defined(RCC_CSR_V18PWRRSTF)
#define RCC_FLAG_V18PWRRST               ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_V18PWRRSTF_BitNumber))
#endif
#define RCC_FLAG_OBLRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_OBLRSTF_BitNumber))
#define RCC_FLAG_PINRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_PINRSTF_BitNumber))      /*!< PIN reset flag */
#define RCC_FLAG_PORRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_PORRSTF_BitNumber))      /*!< POR/PDR reset flag */
#define RCC_FLAG_SFTRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_SFTRSTF_BitNumber))      /*!< Software Reset flag */
#define RCC_FLAG_IWDGRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_IWDGRSTF_BitNumber))     /*!< Independent Watchdog reset flag */
#define RCC_FLAG_WWDGRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_WWDGRSTF_BitNumber))     /*!< Window watchdog reset flag */
#define RCC_FLAG_LPWRRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | RCC_CSR_LPWRRSTF_BitNumber))     /*!< Low-Power reset flag */

/* Flags in the BDCR register */
#define RCC_FLAG_LSERDY                  ((uint8_t)((BDCR_REG_INDEX << 5U) | RCC_BDCR_LSERDY_BitNumber))     /*!< External Low Speed oscillator Ready */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/** @defgroup RCC_Exported_Macros RCC Exported Macros
  * @{
  */

/** @defgroup RCC_AHB_Clock_Enable_Disable RCC AHB Clock Enable Disable
  * @brief  Enable or disable the AHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_GPIOA_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOB_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOBEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOBEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOCEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOF_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOFEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOFEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_CRC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_DMA1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SRAM_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_SRAMEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_SRAMEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_FLITF_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_FLITFEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FLITFEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_GPIOA_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIOAEN))
#define __HAL_RCC_GPIOB_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIOBEN))
#define __HAL_RCC_GPIOC_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIOCEN))
#define __HAL_RCC_GPIOF_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_GPIOFEN))
#define __HAL_RCC_CRC_CLK_DISABLE()          (RCC->AHBENR &= ~(RCC_AHBENR_CRCEN))
#define __HAL_RCC_DMA1_CLK_DISABLE()         (RCC->AHBENR &= ~(RCC_AHBENR_DMA1EN))
#define __HAL_RCC_SRAM_CLK_DISABLE()         (RCC->AHBENR &= ~(RCC_AHBENR_SRAMEN))
#define __HAL_RCC_FLITF_CLK_DISABLE()        (RCC->AHBENR &= ~(RCC_AHBENR_FLITFEN))
/**
  * @}
  */

/** @defgroup RCC_AHB_Peripheral_Clock_Enable_Disable_Status AHB Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the AHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_GPIOA_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIOAEN)) != RESET)
#define __HAL_RCC_GPIOB_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIOBEN)) != RESET)
#define __HAL_RCC_GPIOC_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIOCEN)) != RESET)
#define __HAL_RCC_GPIOF_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_GPIOFEN)) != RESET)
#define __HAL_RCC_CRC_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_CRCEN))   != RESET)
#define __HAL_RCC_DMA1_IS_CLK_ENABLED()      ((RCC->AHBENR & (RCC_AHBENR_DMA1EN))  != RESET)
#define __HAL_RCC_SRAM_IS_CLK_ENABLED()      ((RCC->AHBENR & (RCC_AHBENR_SRAMEN))  != RESET)
#define __HAL_RCC_FLITF_IS_CLK_ENABLED()     ((RCC->AHBENR & (RCC_AHBENR_FLITFEN)) != RESET)
#define __HAL_RCC_GPIOA_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIOAEN)) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIOBEN)) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIOCEN)) == RESET)
#define __HAL_RCC_GPIOF_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_GPIOFEN)) == RESET)
#define __HAL_RCC_CRC_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_CRCEN))   == RESET)
#define __HAL_RCC_DMA1_IS_CLK_DISABLED()     ((RCC->AHBENR & (RCC_AHBENR_DMA1EN))  == RESET)
#define __HAL_RCC_SRAM_IS_CLK_DISABLED()     ((RCC->AHBENR & (RCC_AHBENR_SRAMEN))  == RESET)
#define __HAL_RCC_FLITF_IS_CLK_DISABLED()    ((RCC->AHBENR & (RCC_AHBENR_FLITFEN)) == RESET)
/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Enable_Disable RCC APB1 Clock Enable Disable
  * @brief  Enable or disable the Low Speed APB (APB1) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_TIM3_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM14_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM14EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM14EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_WWDG_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_WWDGEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_WWDGEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_I2C1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_PWR_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM3_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM3EN))
#define __HAL_RCC_TIM14_CLK_DISABLE()  (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM14EN))
#define __HAL_RCC_WWDG_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_WWDGEN))
#define __HAL_RCC_I2C1_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_PWR_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN))
/**
  * @}
  */

/** @defgroup RCC_APB1_Peripheral_Clock_Enable_Disable_Status APB1 Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the APB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_TIM3_IS_CLK_ENABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_TIM3EN))  != RESET)
#define __HAL_RCC_TIM14_IS_CLK_ENABLED()  ((RCC->APB1ENR & (RCC_APB1ENR_TIM14EN)) != RESET)
#define __HAL_RCC_WWDG_IS_CLK_ENABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_WWDGEN))  != RESET)
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN))  != RESET)
#define __HAL_RCC_PWR_IS_CLK_ENABLED()    ((RCC->APB1ENR & (RCC_APB1ENR_PWREN))   != RESET)
#define __HAL_RCC_TIM3_IS_CLK_DISABLED()  ((RCC->APB1ENR & (RCC_APB1ENR_TIM3EN))  == RESET)
#define __HAL_RCC_TIM14_IS_CLK_DISABLED() ((RCC->APB1ENR & (RCC_APB1ENR_TIM14EN)) == RESET)
#define __HAL_RCC_WWDG_IS_CLK_DISABLED()  ((RCC->APB1ENR & (RCC_APB1ENR_WWDGEN))  == RESET)
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()  ((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN))  == RESET)
#define __HAL_RCC_PWR_IS_CLK_DISABLED()   ((RCC->APB1ENR & (RCC_APB1ENR_PWREN))   == RESET)
/**
  * @}
  */


/** @defgroup RCC_APB2_Clock_Enable_Disable RCC APB2 Clock Enable Disable
  * @brief  Enable or disable the High Speed APB (APB2) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_SYSCFG_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_ADC1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SPI1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM16_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM17_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM17EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM17EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_DBGMCU_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_DBGMCUEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_DBGMCUEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_SYSCFG_CLK_DISABLE() (RCC->APB2ENR &= ~(RCC_APB2ENR_SYSCFGEN))
#define __HAL_RCC_ADC1_CLK_DISABLE()   (RCC->APB2ENR &= ~(RCC_APB2ENR_ADC1EN))
#define __HAL_RCC_TIM1_CLK_DISABLE()   (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM1EN))
#define __HAL_RCC_SPI1_CLK_DISABLE()   (RCC->APB2ENR &= ~(RCC_APB2ENR_SPI1EN))
#define __HAL_RCC_TIM16_CLK_DISABLE()  (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM16EN))
#define __HAL_RCC_TIM17_CLK_DISABLE()  (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM17EN))
#define __HAL_RCC_USART1_CLK_DISABLE() (RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN))
#define __HAL_RCC_DBGMCU_CLK_DISABLE() (RCC->APB2ENR &= ~(RCC_APB2ENR_DBGMCUEN))
/**
  * @}
  */

/** @defgroup RCC_APB2_Peripheral_Clock_Enable_Disable_Status APB2 Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_SYSCFG_IS_CLK_ENABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_SYSCFGEN)) != RESET)
#define __HAL_RCC_ADC1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN))   != RESET)
#define __HAL_RCC_TIM1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_TIM1EN))   != RESET)
#define __HAL_RCC_SPI1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN))   != RESET)
#define __HAL_RCC_TIM16_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM16EN))  != RESET)
#define __HAL_RCC_TIM17_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM17EN))  != RESET)
#define __HAL_RCC_USART1_IS_CLK_ENABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) != RESET)
#define __HAL_RCC_DBGMCU_IS_CLK_ENABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_DBGMCUEN)) != RESET)
#define __HAL_RCC_SYSCFG_IS_CLK_DISABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_SYSCFGEN)) == RESET)
#define __HAL_RCC_ADC1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN))   == RESET)
#define __HAL_RCC_TIM1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM1EN))   == RESET)
#define __HAL_RCC_SPI1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN))   == RESET)
#define __HAL_RCC_TIM16_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_TIM16EN))  == RESET)
#define __HAL_RCC_TIM17_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_TIM17EN))  == RESET)
#define __HAL_RCC_USART1_IS_CLK_DISABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) == RESET)
#define __HAL_RCC_DBGMCU_IS_CLK_DISABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_DBGMCUEN)) == RESET)
/**
  * @}
  */

/** @defgroup RCC_AHB_Force_Release_Reset RCC AHB Force Release Reset
  * @brief  Force or release AHB peripheral reset.
  * @{
  */
#define __HAL_RCC_AHB_FORCE_RESET()     (RCC->AHBRSTR = 0xFFFFFFFFU)
#define __HAL_RCC_GPIOA_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOARST))
#define __HAL_RCC_GPIOB_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOBRST))
#define __HAL_RCC_GPIOC_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOCRST))
#define __HAL_RCC_GPIOF_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOFRST))

#define __HAL_RCC_AHB_RELEASE_RESET()   (RCC->AHBRSTR = 0x00000000U)
#define __HAL_RCC_GPIOA_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOARST))
#define __HAL_RCC_GPIOB_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOBRST))
#define __HAL_RCC_GPIOC_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOCRST))
#define __HAL_RCC_GPIOF_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOFRST))
/**
  * @}
  */

/** @defgroup RCC_APB1_Force_Release_Reset RCC APB1 Force Release Reset
  * @brief  Force or release APB1 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB1_FORCE_RESET()     (RCC->APB1RSTR = 0xFFFFFFFFU)
#define __HAL_RCC_TIM3_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM14_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM14RST))
#define __HAL_RCC_WWDG_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_I2C1_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_PWR_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_PWRRST))

#define __HAL_RCC_APB1_RELEASE_RESET()   (RCC->APB1RSTR = 0x00000000U)
#define __HAL_RCC_TIM3_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM14_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM14RST))
#define __HAL_RCC_WWDG_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_I2C1_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_PWR_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_PWRRST))
/**
  * @}
  */

/** @defgroup RCC_APB2_Force_Release_Reset RCC APB2 Force Release Reset
  * @brief  Force or release APB2 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB2_FORCE_RESET()     (RCC->APB2RSTR = 0xFFFFFFFFU)
#define __HAL_RCC_SYSCFG_FORCE_RESET()   (RCC->APB2RSTR |= (RCC_APB2RSTR_SYSCFGRST))
#define __HAL_RCC_ADC1_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_TIM1_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM1RST))
#define __HAL_RCC_SPI1_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_FORCE_RESET()   (RCC->APB2RSTR |= (RCC_APB2RSTR_USART1RST))
#define __HAL_RCC_TIM16_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM16RST))
#define __HAL_RCC_TIM17_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM17RST))
#define __HAL_RCC_DBGMCU_FORCE_RESET()   (RCC->APB2RSTR |= (RCC_APB2RSTR_DBGMCURST))

#define __HAL_RCC_APB2_RELEASE_RESET()   (RCC->APB2RSTR = 0x00000000U)
#define __HAL_RCC_SYSCFG_RELEASE_RESET() (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SYSCFGRST))
#define __HAL_RCC_ADC1_RELEASE_RESET()   (RCC->APB2RSTR &= ~(RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_TIM1_RELEASE_RESET()   (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM1RST))
#define __HAL_RCC_SPI1_RELEASE_RESET()   (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_RELEASE_RESET() (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART1RST))
#define __HAL_RCC_TIM16_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM16RST))
#define __HAL_RCC_TIM17_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM17RST))
#define __HAL_RCC_DBGMCU_RELEASE_RESET() (RCC->APB2RSTR &= ~(RCC_APB2RSTR_DBGMCURST))
/**
  * @}
  */
/** @defgroup RCC_HSI_Configuration HSI Configuration
  * @{
  */

/** @brief  Macros to enable or disable the Internal High Speed oscillator (HSI).
  * @note   The HSI is stopped by hardware when entering STOP and STANDBY modes.
  * @note   HSI can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI.
  * @note   After enabling the HSI, the application software should wait on HSIRDY
  *         flag to be set indicating that HSI clock is stable and can be used as
  *         system clock source.
  * @note   When the HSI is stopped, HSIRDY flag goes low after 6 HSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_HSI_ENABLE()  SET_BIT(RCC->CR, RCC_CR_HSION)
#define __HAL_RCC_HSI_DISABLE() CLEAR_BIT(RCC->CR, RCC_CR_HSION)

/** @brief  Macro to adjust the Internal High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI RC.
  * @param  _HSICALIBRATIONVALUE_ specifies the calibration trimming value.
  *         (default is RCC_HSICALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 0x1F.
  */
#define __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(_HSICALIBRATIONVALUE_) \
                  MODIFY_REG(RCC->CR, RCC_CR_HSITRIM, (uint32_t)(_HSICALIBRATIONVALUE_) << RCC_CR_HSITRIM_BitNumber)

/**
  * @}
  */

/** @defgroup RCC_LSI_Configuration  LSI Configuration
  * @{
  */

/** @brief Macro to enable the Internal Low Speed oscillator (LSI).
  * @note   After enabling the LSI, the application software should wait on
  *         LSIRDY flag to be set indicating that LSI clock is stable and can
  *         be used to clock the IWDG and/or the RTC.
  */
#define __HAL_RCC_LSI_ENABLE() SET_BIT(RCC->CSR, RCC_CSR_LSION)

/** @brief Macro to disable the Internal Low Speed oscillator (LSI).
  * @note   LSI can not be disabled if the IWDG is running.
  * @note   When the LSI is stopped, LSIRDY flag goes low after 6 LSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_LSI_DISABLE() CLEAR_BIT(RCC->CSR, RCC_CSR_LSION)

/**
  * @}
  */

/** @defgroup RCC_HSE_Configuration HSE Configuration
  * @{
  */

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
  * @param  __STATE__ specifies the new state of the HSE.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_HSE_OFF turn OFF the HSE oscillator, HSERDY flag goes low after
  *                              6 HSE oscillator clock cycles.
  *            @arg @ref RCC_HSE_ON turn ON the HSE oscillator
  *            @arg @ref RCC_HSE_BYPASS HSE oscillator bypassed with external clock
  */
#define __HAL_RCC_HSE_CONFIG(__STATE__)                                     \
                    do{                                                     \
                      if ((__STATE__) == RCC_HSE_ON)                        \
                      {                                                     \
                        SET_BIT(RCC->CR, RCC_CR_HSEON);                     \
                      }                                                     \
                      else if ((__STATE__) == RCC_HSE_OFF)                  \
                      {                                                     \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEON);                   \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);                  \
                      }                                                     \
                      else if ((__STATE__) == RCC_HSE_BYPASS)               \
                      {                                                     \
                        SET_BIT(RCC->CR, RCC_CR_HSEBYP);                    \
                        SET_BIT(RCC->CR, RCC_CR_HSEON);                     \
                      }                                                     \
                      else                                                  \
                      {                                                     \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEON);                   \
                        CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);                  \
                      }                                                     \
                    }while(0U)

/**
  * @brief  Macro to configure the External High Speed oscillator (HSE) Predivision factor for PLL.
  * @note   Predivision factor can not be changed if PLL is used as system clock
  *         In this case, you have to select another source of the system clock, disable the PLL and
  *         then change the HSE predivision factor.
  * @param  __HSE_PREDIV_VALUE__ specifies the division value applied to HSE.
  *         This parameter must be a number between RCC_HSE_PREDIV_DIV1 and RCC_HSE_PREDIV_DIV16.
  */
#define __HAL_RCC_HSE_PREDIV_CONFIG(__HSE_PREDIV_VALUE__) \
                  MODIFY_REG(RCC->CFGR2, RCC_CFGR2_PREDIV, (uint32_t)(__HSE_PREDIV_VALUE__))

/**
  * @}
  */

/** @defgroup RCC_LSE_Configuration LSE Configuration
  * @{
  */

/**
  * @brief  Macro to configure the External Low Speed oscillator (LSE).
  * @note Transitions LSE Bypass to LSE On and LSE On to LSE Bypass are not supported by this macro.
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         @ref HAL_PWR_EnableBkUpAccess() function before to configure the LSE
  *         (to be done once after reset).
  * @note   After enabling the LSE (RCC_LSE_ON or RCC_LSE_BYPASS), the application
  *         software should wait on LSERDY flag to be set indicating that LSE clock
  *         is stable and can be used to clock the RTC.
  * @param  __STATE__ specifies the new state of the LSE.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_LSE_OFF turn OFF the LSE oscillator, LSERDY flag goes low after
  *                              6 LSE oscillator clock cycles.
  *            @arg @ref RCC_LSE_ON turn ON the LSE oscillator.
  *            @arg @ref RCC_LSE_BYPASS LSE oscillator bypassed with external clock.
  */
#define __HAL_RCC_LSE_CONFIG(__STATE__)                                     \
                    do{                                                     \
                      if ((__STATE__) == RCC_LSE_ON)                        \
                      {                                                     \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);                   \
                      }                                                     \
                      else if ((__STATE__) == RCC_LSE_OFF)                  \
                      {                                                     \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);                 \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);                \
                      }                                                     \
                      else if ((__STATE__) == RCC_LSE_BYPASS)               \
                      {                                                     \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);                  \
                        SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);                   \
                      }                                                     \
                      else                                                  \
                      {                                                     \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);                 \
                        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);                \
                      }                                                     \
                    }while(0U)

/**
  * @}
  */

/** @defgroup RCC_HSI14_Configuration RCC_HSI14_Configuration
  * @{
  */

/** @brief  Macro to enable the Internal 14Mhz High Speed oscillator (HSI14).
  * @note   After enabling the HSI14 with @ref __HAL_RCC_HSI14_ENABLE(), the application software
  *         should wait on HSI14RDY flag to be set indicating that HSI clock is stable and can be
  *         used as system clock source. This is not necessary if @ref HAL_RCC_OscConfig() is used.
  *         clock cycles.
  */
#define __HAL_RCC_HSI14_ENABLE()  SET_BIT(RCC->CR2, RCC_CR2_HSI14ON)

/** @brief  Macro to disable the Internal 14Mhz High Speed oscillator (HSI14).
  * @note   The HSI14 is stopped by hardware when entering STOP and STANDBY modes.
  * @note   HSI14 can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI14.
  * @note   When the HSI14 is stopped, HSI14RDY flag goes low after 6 HSI14 oscillator
  *         clock cycles.
  */
#define __HAL_RCC_HSI14_DISABLE() CLEAR_BIT(RCC->CR2, RCC_CR2_HSI14ON)

/** @brief Macro to enable the Internal 14Mhz High Speed oscillator (HSI14) used by ADC.
  */
#define __HAL_RCC_HSI14ADC_ENABLE()  CLEAR_BIT(RCC->CR2, RCC_CR2_HSI14DIS)

/** @brief Macro to disable the Internal 14Mhz High Speed oscillator (HSI14) used by ADC.
  */
#define __HAL_RCC_HSI14ADC_DISABLE() SET_BIT(RCC->CR2, RCC_CR2_HSI14DIS)

/** @brief  Macro to adjust the Internal 14Mhz High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI14 RC.
  * @param  __HSI14CALIBRATIONVALUE__ specifies the calibration trimming value
  *         (default is RCC_HSI14CALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 0x1F.
  */
#define __HAL_RCC_HSI14_CALIBRATIONVALUE_ADJUST(__HSI14CALIBRATIONVALUE__) \
                  MODIFY_REG(RCC->CR2, RCC_CR2_HSI14TRIM, (uint32_t)(__HSI14CALIBRATIONVALUE__) << RCC_HSI14TRIM_BIT_NUMBER)
/**
  * @}
  */

/** @defgroup RCC_USARTx_Clock_Config RCC USARTx Clock Config
  * @{
  */

/** @brief  Macro to configure the USART1 clock (USART1CLK).
  * @param  __USART1CLKSOURCE__ specifies the USART1 clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK1 PCLK1 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE LSE selected as USART1 clock
  */
#define __HAL_RCC_USART1_CONFIG(__USART1CLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USART1SW, (uint32_t)(__USART1CLKSOURCE__))

/** @brief  Macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK1 PCLK1 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE LSE selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_USART1SW)))

/**
  * @}
  */

/** @defgroup RCC_I2Cx_Clock_Config RCC I2Cx Clock Config
  * @{
  */

/** @brief  Macro to configure the I2C1 clock (I2C1CLK).
  * @param  __I2C1CLKSOURCE__ specifies the I2C1 clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK System Clock selected as I2C1 clock
  */
#define __HAL_RCC_I2C1_CONFIG(__I2C1CLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR3, RCC_CFGR3_I2C1SW, (uint32_t)(__I2C1CLKSOURCE__))

/** @brief  Macro to get the I2C1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK System Clock selected as I2C1 clock
  */
#define __HAL_RCC_GET_I2C1_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR3, RCC_CFGR3_I2C1SW)))
/**
  * @}
  */

/** @defgroup RCC_PLL_Configuration PLL Configuration
  * @{
  */

/** @brief Macro to enable the main PLL.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The main PLL is disabled by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_PLL_ENABLE() SET_BIT(RCC->CR, RCC_CR_PLLON)

/** @brief Macro to disable the main PLL.
  * @note   The main PLL can not be disabled if it is used as system clock source
  */
#define __HAL_RCC_PLL_DISABLE() CLEAR_BIT(RCC->CR, RCC_CR_PLLON)

/** @brief  Macro to configure the PLL clock source, multiplication and division factors.
  * @note   This function must be used only when the main PLL is disabled.
  *
  * @param  __RCC_PLLSOURCE__ specifies the PLL entry clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_PLLSOURCE_HSI HSI oscillator clock selected as PLL clock entry
  *            @arg @ref RCC_PLLSOURCE_HSE HSE oscillator clock selected as PLL clock entry
  * @param  __PLLMUL__ specifies the multiplication factor for PLL VCO output clock
  *          This parameter can be one of the following values:
  *         This parameter must be a number between RCC_PLL_MUL2 and RCC_PLL_MUL16.
  * @param  __PREDIV__ specifies the predivider factor for PLL VCO input clock
  *         This parameter must be a number between RCC_PREDIV_DIV1 and RCC_PREDIV_DIV16.
  *
  */
#define __HAL_RCC_PLL_CONFIG(__RCC_PLLSOURCE__ , __PREDIV__, __PLLMUL__) \
                  do { \
                    MODIFY_REG(RCC->CFGR2, RCC_CFGR2_PREDIV, (__PREDIV__)); \
                    MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLMUL | RCC_CFGR_PLLSRC, (uint32_t)((__PLLMUL__)|(__RCC_PLLSOURCE__))); \
                  } while(0U)


/** @brief  Get oscillator clock selected as PLL input clock
  * @retval The clock source used for PLL entry. The returned value can be one
  *         of the following:
  *             @arg @ref RCC_PLLSOURCE_HSE HSE oscillator clock selected as PLL input clock
  */
#define __HAL_RCC_GET_PLL_OSCSOURCE() ((uint32_t)(READ_BIT(RCC->CFGR, RCC_CFGR_PLLSRC)))

/**
  * @}
  */

/** @defgroup RCC_Get_Clock_source Get Clock source
  * @{
  */

/**
  * @brief  Macro to configure the system clock source.
  * @param  __SYSCLKSOURCE__ specifies the system clock source.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_SYSCLKSOURCE_HSI HSI oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_HSE HSE oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_PLLCLK PLL output is used as system clock source.
  */
#define __HAL_RCC_SYSCLK_CONFIG(__SYSCLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, (__SYSCLKSOURCE__))

/** @brief  Macro to get the clock source used as system clock.
  * @retval The clock source used as system clock. The returned value can be one
  *         of the following:
  *             @arg @ref RCC_SYSCLKSOURCE_STATUS_HSI HSI used as system clock
  *             @arg @ref RCC_SYSCLKSOURCE_STATUS_HSE HSE used as system clock
  *             @arg @ref RCC_SYSCLKSOURCE_STATUS_PLLCLK PLL used as system clock
  */
#define __HAL_RCC_GET_SYSCLK_SOURCE() ((uint32_t)(READ_BIT(RCC->CFGR,RCC_CFGR_SWS)))

/**
  * @}
  */

/** @defgroup RCCEx_MCOx_Clock_Config RCC Extended MCOx Clock Config
  * @{
  */

#if defined(RCC_CFGR_MCOPRE)
/** @brief  Macro to configure the MCO clock.
  * @param  __MCOCLKSOURCE__ specifies the MCO clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK      No clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK       System Clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI          HSI oscillator clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSE          HSE selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSI          LSI selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSE          LSE selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI14        HSI14 selected as MCO clock
  @if STM32F042x6
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F048xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F071xB
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F072xB
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F078xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F091xC
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F098xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F030x6
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F030xC
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F031x6
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F038xx
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F070x6
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @elseif STM32F070xB
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLLCLK selected as MCO clock
  @endif
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK_DIV2  PLLCLK Divided by 2 selected as MCO clock
  * @param  __MCODIV__ specifies the MCO clock prescaler.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCODIV_1   MCO clock source is divided by 1
  *            @arg @ref RCC_MCODIV_2   MCO clock source is divided by 2
  *            @arg @ref RCC_MCODIV_4   MCO clock source is divided by 4
  *            @arg @ref RCC_MCODIV_8   MCO clock source is divided by 8
  *            @arg @ref RCC_MCODIV_16  MCO clock source is divided by 16
  *            @arg @ref RCC_MCODIV_32  MCO clock source is divided by 32
  *            @arg @ref RCC_MCODIV_64  MCO clock source is divided by 64
  *            @arg @ref RCC_MCODIV_128 MCO clock source is divided by 128
  */
#else
/** @brief  Macro to configure the MCO clock.
  * @param  __MCOCLKSOURCE__ specifies the MCO clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK     No clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK      System Clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI         HSI selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSE         HSE selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSI         LSI selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSE         LSE selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI14       HSI14 selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK_DIV2 PLLCLK Divided by 2 selected as MCO clock
  * @param  __MCODIV__ specifies the MCO clock prescaler.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_MCODIV_1 No division applied on MCO clock source
  */
#endif
#if   defined(RCC_CFGR_MCOPRE)
#define __HAL_RCC_MCO1_CONFIG(__MCOCLKSOURCE__, __MCODIV__) \
                 MODIFY_REG(RCC->CFGR, (RCC_CFGR_MCO | RCC_CFGR_MCOPRE), ((__MCOCLKSOURCE__) | (__MCODIV__)))
#else

#define __HAL_RCC_MCO1_CONFIG(__MCOCLKSOURCE__, __MCODIV__) \
                 MODIFY_REG(RCC->CFGR, RCC_CFGR_MCO, (__MCOCLKSOURCE__))

#endif

/**
  * @}
  */

  /** @defgroup RCC_RTC_Clock_Configuration RCC RTC Clock Configuration
  * @{
  */

/** @brief Macro to configure the RTC clock (RTCCLK).
  * @note   As the RTC clock configuration bits are in the Backup domain and write
  *         access is denied to this domain after reset, you have to enable write
  *         access using the Power Backup Access macro before to configure
  *         the RTC clock source (to be done once after reset).
  * @note   Once the RTC clock is configured it cannot be changed unless the
  *         Backup domain is reset using @ref __HAL_RCC_BACKUPRESET_FORCE() macro, or by
  *         a Power On Reset (POR).
  *
  * @param  __RTC_CLKSOURCE__ specifies the RTC clock source.
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_RTCCLKSOURCE_NO_CLK No clock selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_LSE LSE selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_LSI LSI selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV32 HSE clock divided by 32
  * @note   If the LSE or LSI is used as RTC clock source, the RTC continues to
  *         work in STOP and STANDBY modes, and can be used as wakeup source.
  *         However, when the LSI clock and HSE clock divided by 32 is used as RTC clock source,
  *         the RTC cannot be used in STOP and STANDBY modes.
  * @note   The system must always be configured so as to get a PCLK frequency greater than or
  *             equal to the RTCCLK frequency for a proper operation of the RTC.
  */
#define __HAL_RCC_RTC_CONFIG(__RTC_CLKSOURCE__) MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, (__RTC_CLKSOURCE__))

/** @brief Macro to get the RTC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_RTCCLKSOURCE_NO_CLK No clock selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_LSE LSE selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_LSI LSI selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_HSE_DIV32 HSE clock divided by 32
  */
#define __HAL_RCC_GET_RTC_SOURCE() (READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL))

/** @brief Macro to enable the the RTC clock.
  * @note   These macros must be used only after the RTC clock source was selected.
  */
#define __HAL_RCC_RTC_ENABLE() SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN)

/** @brief Macro to disable the the RTC clock.
  * @note  These macros must be used only after the RTC clock source was selected.
  */
#define __HAL_RCC_RTC_DISABLE() CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN)

/** @brief  Macro to force the Backup domain reset.
  * @note   This function resets the RTC peripheral (including the backup registers)
  *         and the RTC clock source selection in RCC_BDCR register.
  */
#define __HAL_RCC_BACKUPRESET_FORCE()  SET_BIT(RCC->BDCR, RCC_BDCR_BDRST)

/** @brief  Macros to release the Backup domain reset.
  */
#define __HAL_RCC_BACKUPRESET_RELEASE() CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST)

/**
  * @}
  */

/** @defgroup RCC_Flags_Interrupts_Management Flags Interrupts Management
  * @brief macros to manage the specified RCC Flags and interrupts.
  * @{
  */

/** @brief Enable RCC interrupt.
  * @param  __INTERRUPT__ specifies the RCC interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY main PLL ready interrupt
  *            @arg @ref RCC_IT_HSI14RDY HSI14 ready interrupt
  @if STM32F042x6
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F048xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F071xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F072xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F078xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F091xC
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F098xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @endif
  */
#define __HAL_RCC_ENABLE_IT(__INTERRUPT__) (*(__IO uint8_t *) RCC_CIR_BYTE1_ADDRESS |= (__INTERRUPT__))

/** @brief Disable RCC interrupt.
  * @param  __INTERRUPT__ specifies the RCC interrupt sources to be disabled.
  *          This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt
  *            @arg @ref RCC_IT_PLLRDY main PLL ready interrupt
  *            @arg @ref RCC_IT_HSI14RDY HSI14 ready interrupt
  @if STM32F042x6
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F048xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F071xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F072xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F078xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F091xC
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F098xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @endif
  */
#define __HAL_RCC_DISABLE_IT(__INTERRUPT__) (*(__IO uint8_t *) RCC_CIR_BYTE1_ADDRESS &= (uint8_t)(~(__INTERRUPT__)))

/** @brief Clear the RCC's interrupt pending bits.
  * @param  __INTERRUPT__ specifies the interrupt pending bit to clear.
  *          This parameter can be any combination of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt.
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt.
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt.
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt.
  *            @arg @ref RCC_IT_PLLRDY Main PLL ready interrupt.
  *            @arg @ref RCC_IT_CSS Clock Security System interrupt
  *            @arg @ref RCC_IT_HSI14RDY HSI14 ready interrupt
  @if STM32F042x6
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F048xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F071xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F072xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F078xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F091xC
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F098xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @endif
  */
#define __HAL_RCC_CLEAR_IT(__INTERRUPT__) (*(__IO uint8_t *) RCC_CIR_BYTE2_ADDRESS = (__INTERRUPT__))

/** @brief Check the RCC's interrupt has occurred or not.
  * @param  __INTERRUPT__ specifies the RCC interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_IT_LSIRDY LSI ready interrupt.
  *            @arg @ref RCC_IT_LSERDY LSE ready interrupt.
  *            @arg @ref RCC_IT_HSIRDY HSI ready interrupt.
  *            @arg @ref RCC_IT_HSERDY HSE ready interrupt.
  *            @arg @ref RCC_IT_PLLRDY Main PLL ready interrupt.
  *            @arg @ref RCC_IT_CSS Clock Security System interrupt
  *            @arg @ref RCC_IT_HSI14RDY HSI14 ready interrupt enable
  @if STM32F042x6
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F048xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F071xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F072xB
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F078xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F091xC
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @elseif STM32F098xx
  *            @arg @ref RCC_IT_HSI48RDY HSI48 ready interrupt
  @endif
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_IT(__INTERRUPT__) ((RCC->CIR & (__INTERRUPT__)) == (__INTERRUPT__))

/** @brief Set RMVF bit to clear the reset flags.
  *         The reset flags are RCC_FLAG_PINRST, RCC_FLAG_PORRST, RCC_FLAG_SFTRST,
  *         RCC_FLAG_OBLRST, RCC_FLAG_IWDGRST, RCC_FLAG_WWDGRST, RCC_FLAG_LPWRRST
  */
#define __HAL_RCC_CLEAR_RESET_FLAGS() (RCC->CSR |= RCC_CSR_RMVF)

/** @brief  Check RCC flag is set or not.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_FLAG_HSIRDY HSI oscillator clock ready.
  *            @arg @ref RCC_FLAG_HSERDY HSE oscillator clock ready.
  *            @arg @ref RCC_FLAG_PLLRDY Main PLL clock ready.
  *            @arg @ref RCC_FLAG_HSI14RDY HSI14 oscillator clock ready
  @if STM32F038xx
  *            @arg @ref RCC_FLAG_V18PWRRST Reset flag of the 1.8 V domain
  @elseif STM32F042x6
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  @elseif STM32F048xx
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  *            @arg @ref RCC_FLAG_V18PWRRST Reset flag of the 1.8 V domain
  @elseif STM32F058xx
  *            @arg @ref RCC_FLAG_V18PWRRST Reset flag of the 1.8 V domain
  @elseif STM32F071xB
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  @elseif STM32F072xB
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  @elseif STM32F078xx
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  *            @arg @ref RCC_FLAG_V18PWRRST Reset flag of the 1.8 V domain
  @elseif STM32F091xC
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  @elseif STM32F098xx
  *            @arg @ref RCC_FLAG_HSI48RDY HSI48 oscillator clock ready
  *            @arg @ref RCC_FLAG_V18PWRRST Reset flag of the 1.8 V domain
  @endif
  *            @arg @ref RCC_FLAG_LSERDY LSE oscillator clock ready.
  *            @arg @ref RCC_FLAG_LSIRDY LSI oscillator clock ready.
  *            @arg @ref RCC_FLAG_OBLRST Option Byte Load reset
  *            @arg @ref RCC_FLAG_PINRST  Pin reset.
  *            @arg @ref RCC_FLAG_PORRST  POR/PDR reset.
  *            @arg @ref RCC_FLAG_SFTRST  Software reset.
  *            @arg @ref RCC_FLAG_IWDGRST Independent Watchdog reset.
  *            @arg @ref RCC_FLAG_WWDGRST Window Watchdog reset.
  *            @arg @ref RCC_FLAG_LPWRRST Low Power reset.
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_FLAG(__FLAG__) (((((__FLAG__) >> 5U) == CR_REG_INDEX)? RCC->CR :      \
                                       (((__FLAG__) >> 5U) == CR2_REG_INDEX)? RCC->CR2 :    \
                                       (((__FLAG__) >> 5U) == BDCR_REG_INDEX) ? RCC->BDCR : \
                                       RCC->CSR) & (1U << ((__FLAG__) & RCC_FLAG_MASK)))

/**
  * @}
  */

/**
  * @}
  */

/* Include RCC HAL Extension module */
#include "stm32f0xx_hal_rcc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCC_Exported_Functions
  * @{
  */

/** @addtogroup RCC_Exported_Functions_Group1
  * @{
  */

/* Initialization and de-initialization functions  ******************************/
void              HAL_RCC_DeInit(void);
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t FLatency);

/**
  * @}
  */

/** @addtogroup RCC_Exported_Functions_Group2
  * @{
  */

/* Peripheral Control functions  ************************************************/
void              HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv);
void              HAL_RCC_EnableCSS(void);
/* CSS NMI IRQ handler */
void              HAL_RCC_NMI_IRQHandler(void);
/* User Callbacks in non blocking mode (IT mode) */
void              HAL_RCC_CSSCallback(void);
void              HAL_RCC_DisableCSS(void);
uint32_t          HAL_RCC_GetSysClockFreq(void);
uint32_t          HAL_RCC_GetHCLKFreq(void);
uint32_t          HAL_RCC_GetPCLK1Freq(void);
void              HAL_RCC_GetOscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct);
void              HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t *pFLatency);

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

#endif /* __STM32F0xx_HAL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

