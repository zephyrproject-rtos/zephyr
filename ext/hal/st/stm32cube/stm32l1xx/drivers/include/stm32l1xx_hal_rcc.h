/**
  ******************************************************************************
  * @file    stm32l1xx_hal_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL module.
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
#ifndef __STM32L1xx_HAL_RCC_H
#define __STM32L1xx_HAL_RCC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal_def.h"

/** @addtogroup STM32L1xx_HAL_Driver
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
#define MSI_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define HSI_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define LSI_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
#define PLL_TIMEOUT_VALUE          (2U)      /* 2 ms (minimum Tick + 1U) */
/**
  * @}
  */
  
/** @defgroup RCC_Register_Offset Register offsets
  * @{
  */
#define RCC_OFFSET                (RCC_BASE - PERIPH_BASE)
#define RCC_CR_OFFSET             0x00
#define RCC_CFGR_OFFSET           0x08
#define RCC_CIR_OFFSET            0x0C
#define RCC_CSR_OFFSET            0x34
/**
  * @}
  */

/** @defgroup RCC_BitAddress_AliasRegion BitAddress AliasRegion
  * @brief RCC registers bit address in the alias region
  * @{
  */
#define RCC_CR_OFFSET_BB          (RCC_OFFSET + RCC_CR_OFFSET)
#define RCC_CFGR_OFFSET_BB        (RCC_OFFSET + RCC_CFGR_OFFSET)
#define RCC_CIR_OFFSET_BB         (RCC_OFFSET + RCC_CIR_OFFSET)
#define RCC_CSR_OFFSET_BB         (RCC_OFFSET + RCC_CSR_OFFSET)

/* --- CR Register ---*/
/* Alias word address of HSION bit */
#define RCC_HSION_BIT_NUMBER      POSITION_VAL(RCC_CR_HSION)
#define RCC_CR_HSION_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_HSION_BIT_NUMBER * 4U)))
/* Alias word address of MSION bit */
#define RCC_MSION_BIT_NUMBER      POSITION_VAL(RCC_CR_MSION)
#define RCC_CR_MSION_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_MSION_BIT_NUMBER * 4U)))
/* Alias word address of HSEON bit */
#define RCC_HSEON_BIT_NUMBER      POSITION_VAL(RCC_CR_HSEON)
#define RCC_CR_HSEON_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_HSEON_BIT_NUMBER * 4U)))
/* Alias word address of CSSON bit */
#define RCC_CSSON_BIT_NUMBER      POSITION_VAL(RCC_CR_CSSON)
#define RCC_CR_CSSON_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_CSSON_BIT_NUMBER * 4U)))
/* Alias word address of PLLON bit */
#define RCC_PLLON_BIT_NUMBER      POSITION_VAL(RCC_CR_PLLON)
#define RCC_CR_PLLON_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CR_OFFSET_BB * 32U) + (RCC_PLLON_BIT_NUMBER * 4U)))

/* --- CSR Register ---*/
/* Alias word address of LSION bit */
#define RCC_LSION_BIT_NUMBER      POSITION_VAL(RCC_CSR_LSION)
#define RCC_CSR_LSION_BB          ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_LSION_BIT_NUMBER * 4U)))

/* Alias word address of RMVF bit */
#define RCC_RMVF_BIT_NUMBER       POSITION_VAL(RCC_CSR_RMVF)
#define RCC_CSR_RMVF_BB           ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_RMVF_BIT_NUMBER * 4U)))

/* Alias word address of LSEON bit */
#define RCC_LSEON_BIT_NUMBER      POSITION_VAL(RCC_CSR_LSEON)
#define RCC_CSR_LSEON_BB          ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_LSEON_BIT_NUMBER * 4U)))

/* Alias word address of LSEON bit */
#define RCC_LSEBYP_BIT_NUMBER     POSITION_VAL(RCC_CSR_LSEBYP)
#define RCC_CSR_LSEBYP_BB         ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_LSEBYP_BIT_NUMBER * 4U)))

/* Alias word address of RTCEN bit */
#define RCC_RTCEN_BIT_NUMBER      POSITION_VAL(RCC_CSR_RTCEN)
#define RCC_CSR_RTCEN_BB          ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_RTCEN_BIT_NUMBER * 4U)))

/* Alias word address of RTCRST bit */
#define RCC_RTCRST_BIT_NUMBER     POSITION_VAL(RCC_CSR_RTCRST)
#define RCC_CSR_RTCRST_BB         ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (RCC_RTCRST_BIT_NUMBER * 4U)))

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
#define CSR_REG_INDEX                    ((uint8_t)2U)

#define RCC_FLAG_MASK                    ((uint8_t)0x1FU)

/**
  * @}
  */

/** @addtogroup RCC_Private_Macros
  * @{
  */
#define IS_RCC_PLLSOURCE(__SOURCE__) (((__SOURCE__) == RCC_PLLSOURCE_HSI) || \
                                      ((__SOURCE__) == RCC_PLLSOURCE_HSE))
#define IS_RCC_OSCILLATORTYPE(__OSCILLATOR__) (((__OSCILLATOR__) == RCC_OSCILLATORTYPE_NONE)                           || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE) || \
                                               (((__OSCILLATOR__) & RCC_OSCILLATORTYPE_MSI) == RCC_OSCILLATORTYPE_MSI))
#define IS_RCC_HSE(__HSE__) (((__HSE__) == RCC_HSE_OFF) || ((__HSE__) == RCC_HSE_ON) || \
                             ((__HSE__) == RCC_HSE_BYPASS))
#define IS_RCC_LSE(__LSE__) (((__LSE__) == RCC_LSE_OFF) || ((__LSE__) == RCC_LSE_ON) || \
                             ((__LSE__) == RCC_LSE_BYPASS))
#define IS_RCC_HSI(__HSI__) (((__HSI__) == RCC_HSI_OFF) || ((__HSI__) == RCC_HSI_ON))
#define IS_RCC_CALIBRATION_VALUE(__VALUE__) ((__VALUE__) <= 0x1FU)
#define IS_RCC_MSICALIBRATION_VALUE(__VALUE__) ((__VALUE__) <= 0xFFU)
#define IS_RCC_MSI_CLOCK_RANGE(__RANGE__)  (((__RANGE__) == RCC_MSIRANGE_0) || \
                                            ((__RANGE__) == RCC_MSIRANGE_1) || \
                                            ((__RANGE__) == RCC_MSIRANGE_2) || \
                                            ((__RANGE__) == RCC_MSIRANGE_3) || \
                                            ((__RANGE__) == RCC_MSIRANGE_4) || \
                                            ((__RANGE__) == RCC_MSIRANGE_5) || \
                                            ((__RANGE__) == RCC_MSIRANGE_6))
#define IS_RCC_LSI(__LSI__) (((__LSI__) == RCC_LSI_OFF) || ((__LSI__) == RCC_LSI_ON))
#define IS_RCC_MSI(__MSI__) (((__MSI__) == RCC_MSI_OFF) || ((__MSI__) == RCC_MSI_ON))

#define IS_RCC_PLL(__PLL__) (((__PLL__) == RCC_PLL_NONE) || ((__PLL__) == RCC_PLL_OFF) || \
                             ((__PLL__) == RCC_PLL_ON))
#define IS_RCC_PLL_DIV(__DIV__) (((__DIV__) == RCC_PLL_DIV2) || \
                                 ((__DIV__) == RCC_PLL_DIV3) || ((__DIV__) == RCC_PLL_DIV4))

#define IS_RCC_PLL_MUL(__MUL__) (((__MUL__) == RCC_PLL_MUL3)  || ((__MUL__) == RCC_PLL_MUL4)  || \
                                 ((__MUL__) == RCC_PLL_MUL6)  || ((__MUL__) == RCC_PLL_MUL8)  || \
                                 ((__MUL__) == RCC_PLL_MUL12) || ((__MUL__) == RCC_PLL_MUL16) || \
                                 ((__MUL__) == RCC_PLL_MUL24) || ((__MUL__) == RCC_PLL_MUL32) || \
                                 ((__MUL__) == RCC_PLL_MUL48))
#define IS_RCC_CLOCKTYPE(CLK) ((((CLK) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK) || \
                               (((CLK) & RCC_CLOCKTYPE_HCLK)   == RCC_CLOCKTYPE_HCLK)   || \
                               (((CLK) & RCC_CLOCKTYPE_PCLK1)  == RCC_CLOCKTYPE_PCLK1)  || \
                               (((CLK) & RCC_CLOCKTYPE_PCLK2)  == RCC_CLOCKTYPE_PCLK2))
#define IS_RCC_SYSCLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_SYSCLKSOURCE_MSI) || \
                                         ((__SOURCE__) == RCC_SYSCLKSOURCE_HSI) || \
                                         ((__SOURCE__) == RCC_SYSCLKSOURCE_HSE) || \
                                         ((__SOURCE__) == RCC_SYSCLKSOURCE_PLLCLK))
#define IS_RCC_SYSCLKSOURCE_STATUS(__SOURCE__) (((__SOURCE__) == RCC_SYSCLKSOURCE_STATUS_MSI) || \
                                                ((__SOURCE__) == RCC_SYSCLKSOURCE_STATUS_HSI) || \
                                                ((__SOURCE__) == RCC_SYSCLKSOURCE_STATUS_HSE) || \
                                                ((__SOURCE__) == RCC_SYSCLKSOURCE_STATUS_PLLCLK))
#define IS_RCC_HCLK(__HCLK__) (((__HCLK__) == RCC_SYSCLK_DIV1) || ((__HCLK__) == RCC_SYSCLK_DIV2) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV4) || ((__HCLK__) == RCC_SYSCLK_DIV8) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV16) || ((__HCLK__) == RCC_SYSCLK_DIV64) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV128) || ((__HCLK__) == RCC_SYSCLK_DIV256) || \
                               ((__HCLK__) == RCC_SYSCLK_DIV512))
#define IS_RCC_PCLK(__PCLK__) (((__PCLK__) == RCC_HCLK_DIV1) || ((__PCLK__) == RCC_HCLK_DIV2) || \
                               ((__PCLK__) == RCC_HCLK_DIV4) || ((__PCLK__) == RCC_HCLK_DIV8) || \
                               ((__PCLK__) == RCC_HCLK_DIV16))
#define IS_RCC_MCO(__MCO__)  ((__MCO__) == RCC_MCO)
#define IS_RCC_MCODIV(__DIV__) (((__DIV__) == RCC_MCODIV_1) || ((__DIV__) == RCC_MCODIV_2) || \
                                ((__DIV__) == RCC_MCODIV_4) || ((__DIV__) == RCC_MCODIV_8) || \
                                ((__DIV__) == RCC_MCODIV_16)) 
#define IS_RCC_MCO1SOURCE(__SOURCE__) (((__SOURCE__) == RCC_MCO1SOURCE_SYSCLK) || ((__SOURCE__) == RCC_MCO1SOURCE_MSI) \
                                    || ((__SOURCE__) == RCC_MCO1SOURCE_HSI)    || ((__SOURCE__) == RCC_MCO1SOURCE_LSE) \
                                    || ((__SOURCE__) == RCC_MCO1SOURCE_LSI)    || ((__SOURCE__) == RCC_MCO1SOURCE_HSE) \
                                    || ((__SOURCE__) == RCC_MCO1SOURCE_PLLCLK) || ((__SOURCE__) == RCC_MCO1SOURCE_NOCLOCK))
#define IS_RCC_RTCCLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_RTCCLKSOURCE_NO_CLK)   || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_LSE)      || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_LSI)      || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV2) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV4) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV8) || \
                                         ((__SOURCE__) == RCC_RTCCLKSOURCE_HSE_DIV16))

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

  uint32_t PLLDIV;        /*!< PLLDIV: Division factor for PLL VCO input clock
                              This parameter must be a value of @ref RCC_PLL_Division_Factor*/       
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

  uint32_t LSIState;              /*!< The new state of the LSI.
                                       This parameter can be a value of @ref RCC_LSI_Config */

  uint32_t MSIState;              /*!< The new state of the MSI.
                                       This parameter can be a value of @ref RCC_MSI_Config */

  uint32_t MSICalibrationValue;   /*!< The MSI calibration trimming value. (default is RCC_MSICALIBRATION_DEFAULT).
                                       This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFU */

  uint32_t MSIClockRange;         /*!< The MSI  frequency  range.
                                        This parameter can be a value of @ref RCC_MSI_Clock_Range */

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
                                       This parameter can be a value of @ref RCC_APB1_APB2_Clock_Source */

  uint32_t APB2CLKDivider;        /*!< The APB2 clock (PCLK2) divider. This clock is derived from the AHB clock (HCLK).
                                       This parameter can be a value of @ref RCC_APB1_APB2_Clock_Source */
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

#define RCC_PLLSOURCE_HSI           RCC_CFGR_PLLSRC_HSI        /*!< HSI clock selected as PLL entry clock source */
#define RCC_PLLSOURCE_HSE           RCC_CFGR_PLLSRC_HSE        /*!< HSE clock selected as PLL entry clock source */

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
#define RCC_OSCILLATORTYPE_MSI             (0x00000010U)
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

/** @defgroup RCC_MSI_Clock_Range MSI Clock Range
  * @{
  */

#define RCC_MSIRANGE_0                   RCC_ICSCR_MSIRANGE_0 /*!< MSI = 65.536 KHz  */
#define RCC_MSIRANGE_1                   RCC_ICSCR_MSIRANGE_1 /*!< MSI = 131.072 KHz */
#define RCC_MSIRANGE_2                   RCC_ICSCR_MSIRANGE_2 /*!< MSI = 262.144 KHz */
#define RCC_MSIRANGE_3                   RCC_ICSCR_MSIRANGE_3 /*!< MSI = 524.288 KHz */
#define RCC_MSIRANGE_4                   RCC_ICSCR_MSIRANGE_4 /*!< MSI = 1.048 MHz   */
#define RCC_MSIRANGE_5                   RCC_ICSCR_MSIRANGE_5 /*!< MSI = 2.097 MHz   */
#define RCC_MSIRANGE_6                   RCC_ICSCR_MSIRANGE_6 /*!< MSI = 4.194 MHz   */

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

/** @defgroup RCC_MSI_Config MSI Config
  * @{
  */
#define RCC_MSI_OFF                      (0x00000000U)
#define RCC_MSI_ON                       (0x00000001U)

#define RCC_MSICALIBRATION_DEFAULT       (0x00000000U)   /* Default MSI calibration trimming value */

/**
  * @}
  */

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
#define RCC_CLOCKTYPE_PCLK2              (0x00000008U) /*!< PCLK2 to configure */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source System Clock Source
  * @{
  */
#define RCC_SYSCLKSOURCE_MSI             RCC_CFGR_SW_MSI /*!< MSI selected as system clock */
#define RCC_SYSCLKSOURCE_HSI             RCC_CFGR_SW_HSI /*!< HSI selected as system clock */
#define RCC_SYSCLKSOURCE_HSE             RCC_CFGR_SW_HSE /*!< HSE selected as system clock */
#define RCC_SYSCLKSOURCE_PLLCLK          RCC_CFGR_SW_PLL /*!< PLL selected as system clock */

/**
  * @}
  */

/** @defgroup RCC_System_Clock_Source_Status System Clock Source Status
  * @{
  */
#define RCC_SYSCLKSOURCE_STATUS_MSI      RCC_CFGR_SWS_MSI            /*!< MSI used as system clock */
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
  
/** @defgroup RCC_APB1_APB2_Clock_Source APB1 APB2 Clock Source
  * @{
  */
#define RCC_HCLK_DIV1                    RCC_CFGR_PPRE1_DIV1  /*!< HCLK not divided */
#define RCC_HCLK_DIV2                    RCC_CFGR_PPRE1_DIV2  /*!< HCLK divided by 2 */
#define RCC_HCLK_DIV4                    RCC_CFGR_PPRE1_DIV4  /*!< HCLK divided by 4 */
#define RCC_HCLK_DIV8                    RCC_CFGR_PPRE1_DIV8  /*!< HCLK divided by 8 */
#define RCC_HCLK_DIV16                   RCC_CFGR_PPRE1_DIV16 /*!< HCLK divided by 16 */

/**
  * @}
  */

/** @defgroup RCC_HAL_EC_RTC_HSE_DIV RTC HSE Prescaler
  * @{
  */
#define RCC_RTC_HSE_DIV_2               0x00000000U /*!< HSE is divided by 2 for RTC clock  */
#define RCC_RTC_HSE_DIV_4               RCC_CR_RTCPRE_0       /*!< HSE is divided by 4 for RTC clock  */
#define RCC_RTC_HSE_DIV_8               RCC_CR_RTCPRE_1       /*!< HSE is divided by 8 for RTC clock  */
#define RCC_RTC_HSE_DIV_16              RCC_CR_RTCPRE         /*!< HSE is divided by 16 for RTC clock */
/**
  * @}
  */

/** @defgroup RCC_RTC_LCD_Clock_Source RTC LCD Clock Source
  * @{
  */
#define RCC_RTCCLKSOURCE_NO_CLK          (0x00000000U)                 /*!< No clock */
#define RCC_RTCCLKSOURCE_LSE             RCC_CSR_RTCSEL_LSE                  /*!< LSE oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_LSI             RCC_CSR_RTCSEL_LSI                  /*!< LSI oscillator clock used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIVX        RCC_CSR_RTCSEL_HSE                         /*!< HSE oscillator clock divided by X used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV2        (RCC_RTC_HSE_DIV_2  | RCC_CSR_RTCSEL_HSE)  /*!< HSE oscillator clock divided by 2 used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV4        (RCC_RTC_HSE_DIV_4  | RCC_CSR_RTCSEL_HSE)  /*!< HSE oscillator clock divided by 4 used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV8        (RCC_RTC_HSE_DIV_8  | RCC_CSR_RTCSEL_HSE)  /*!< HSE oscillator clock divided by 8 used as RTC clock */
#define RCC_RTCCLKSOURCE_HSE_DIV16       (RCC_RTC_HSE_DIV_16 | RCC_CSR_RTCSEL_HSE)  /*!< HSE oscillator clock divided by 16 used as RTC clock */
/**
  * @}
  */

/** @defgroup RCC_PLL_Division_Factor PLL Division Factor
  * @{
  */

#define RCC_PLL_DIV2                    RCC_CFGR_PLLDIV2
#define RCC_PLL_DIV3                    RCC_CFGR_PLLDIV3
#define RCC_PLL_DIV4                    RCC_CFGR_PLLDIV4

/**
  * @}
  */

/** @defgroup RCC_PLL_Multiplication_Factor PLL Multiplication Factor
  * @{
  */

#define RCC_PLL_MUL3                    RCC_CFGR_PLLMUL3
#define RCC_PLL_MUL4                    RCC_CFGR_PLLMUL4
#define RCC_PLL_MUL6                    RCC_CFGR_PLLMUL6
#define RCC_PLL_MUL8                    RCC_CFGR_PLLMUL8
#define RCC_PLL_MUL12                   RCC_CFGR_PLLMUL12
#define RCC_PLL_MUL16                   RCC_CFGR_PLLMUL16
#define RCC_PLL_MUL24                   RCC_CFGR_PLLMUL24
#define RCC_PLL_MUL32                   RCC_CFGR_PLLMUL32 
#define RCC_PLL_MUL48                   RCC_CFGR_PLLMUL48

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

/** @defgroup RCC_MCOx_Clock_Prescaler MCO Clock Prescaler
  * @{
  */
#define RCC_MCODIV_1                    ((uint32_t)RCC_CFGR_MCO_DIV1)
#define RCC_MCODIV_2                    ((uint32_t)RCC_CFGR_MCO_DIV2)
#define RCC_MCODIV_4                    ((uint32_t)RCC_CFGR_MCO_DIV4)
#define RCC_MCODIV_8                    ((uint32_t)RCC_CFGR_MCO_DIV8)
#define RCC_MCODIV_16                   ((uint32_t)RCC_CFGR_MCO_DIV16)

/**
  * @}
  */

/** @defgroup RCC_MCO1_Clock_Source MCO1 Clock Source
  * @{
  */
#define RCC_MCO1SOURCE_NOCLOCK           RCC_CFGR_MCO_NOCLOCK
#define RCC_MCO1SOURCE_SYSCLK            RCC_CFGR_MCO_SYSCLK
#define RCC_MCO1SOURCE_MSI               RCC_CFGR_MCO_MSI
#define RCC_MCO1SOURCE_HSI               RCC_CFGR_MCO_HSI
#define RCC_MCO1SOURCE_LSE               RCC_CFGR_MCO_LSE
#define RCC_MCO1SOURCE_LSI               RCC_CFGR_MCO_LSI
#define RCC_MCO1SOURCE_HSE               RCC_CFGR_MCO_HSE
#define RCC_MCO1SOURCE_PLLCLK            RCC_CFGR_MCO_PLL

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
#define RCC_IT_MSIRDY                    ((uint8_t)RCC_CIR_MSIRDYF)   /*!< MSI Ready Interrupt flag */
#define RCC_IT_LSECSS                    ((uint8_t)RCC_CIR_LSECSSF)   /*!< LSE Clock Security System Interrupt flag */
#define RCC_IT_CSS                       ((uint8_t)RCC_CIR_CSSF)      /*!< Clock Security System Interrupt flag */
/**
  * @}
  */ 
  
/** @defgroup RCC_Flag Flags
  *        Elements values convention: XXXYYYYYb
  *           - YYYYY  : Flag position in the register
  *           - XXX  : Register index
  *                 - 001: CR register
  *                 - 010: CSR register
  * @{
  */
/* Flags in the CR register */
#define RCC_FLAG_HSIRDY                  ((uint8_t)((CR_REG_INDEX << 5U) | POSITION_VAL(RCC_CR_HSIRDY))) /*!< Internal High Speed clock ready flag */
#define RCC_FLAG_MSIRDY                  ((uint8_t)((CR_REG_INDEX << 5U) | POSITION_VAL(RCC_CR_MSIRDY))) /*!< MSI clock ready flag */
#define RCC_FLAG_HSERDY                  ((uint8_t)((CR_REG_INDEX << 5U) | POSITION_VAL(RCC_CR_HSERDY))) /*!< External High Speed clock ready flag */
#define RCC_FLAG_PLLRDY                  ((uint8_t)((CR_REG_INDEX << 5U) | POSITION_VAL(RCC_CR_PLLRDY))) /*!< PLL clock ready flag */

/* Flags in the CSR register */
#define RCC_FLAG_LSIRDY                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_LSIRDY)))   /*!< Internal Low Speed oscillator Ready */
#define RCC_FLAG_LSECSS                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_LSECSSD)))  /*!< CSS on LSE failure Detection */
#define RCC_FLAG_OBLRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_OBLRSTF)))  /*!< Options bytes loading reset flag */
#define RCC_FLAG_PINRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_PINRSTF)))  /*!< PIN reset flag */
#define RCC_FLAG_PORRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_PORRSTF)))  /*!< POR/PDR reset flag */
#define RCC_FLAG_SFTRST                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_SFTRSTF)))  /*!< Software Reset flag */
#define RCC_FLAG_IWDGRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_IWDGRSTF))) /*!< Independent Watchdog reset flag */
#define RCC_FLAG_WWDGRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_WWDGRSTF))) /*!< Window watchdog reset flag */
#define RCC_FLAG_LPWRRST                 ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_LPWRRSTF))) /*!< Low-Power reset flag */
#define RCC_FLAG_LSERDY                  ((uint8_t)((CSR_REG_INDEX << 5U) | POSITION_VAL(RCC_CSR_LSERDY))) /*!< External Low Speed oscillator Ready */

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

/** @defgroup RCC_Peripheral_Clock_Enable_Disable Peripheral Clock Enable Disable
  * @brief  Enable or disable the AHB1 peripheral clock.
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
#define __HAL_RCC_GPIOD_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIODEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIODEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOH_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOHEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOHEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_CRC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_FLITF_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_FLITFEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FLITFEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_DMA1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_GPIOA_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_GPIOAEN))
#define __HAL_RCC_GPIOB_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_GPIOBEN))
#define __HAL_RCC_GPIOC_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_GPIOCEN))
#define __HAL_RCC_GPIOD_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_GPIODEN))
#define __HAL_RCC_GPIOH_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_GPIOHEN))

#define __HAL_RCC_CRC_CLK_DISABLE()       (RCC->AHBENR &= ~(RCC_AHBENR_CRCEN))
#define __HAL_RCC_FLITF_CLK_DISABLE()     (RCC->AHBENR &= ~(RCC_AHBENR_FLITFEN))
#define __HAL_RCC_DMA1_CLK_DISABLE()      (RCC->AHBENR &= ~(RCC_AHBENR_DMA1EN))

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Enable_Disable APB1 Clock Enable Disable
  * @brief  Enable or disable the Low Speed APB (APB1) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it. 
  * @{   
  */
#define __HAL_RCC_TIM2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM3_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM4_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM4EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM4EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM6_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM7_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_WWDG_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_WWDGEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_WWDGEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SPI2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART3_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART3EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART3EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_I2C1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_I2C2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USB_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_PWR_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_DAC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_COMP_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_COMPEN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_COMPEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)


#define __HAL_RCC_TIM2_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_TIM3_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM3EN))
#define __HAL_RCC_TIM4_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM4EN))
#define __HAL_RCC_TIM6_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_TIM7_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM7EN))
#define __HAL_RCC_WWDG_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_WWDGEN))
#define __HAL_RCC_SPI2_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_SPI2EN))
#define __HAL_RCC_USART2_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_USART2EN))
#define __HAL_RCC_USART3_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_USART3EN))
#define __HAL_RCC_I2C1_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_I2C2_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_I2C2EN))
#define __HAL_RCC_USB_CLK_DISABLE()       (RCC->APB1ENR &= ~(RCC_APB1ENR_USBEN))
#define __HAL_RCC_PWR_CLK_DISABLE()       (RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN))
#define __HAL_RCC_DAC_CLK_DISABLE()       (RCC->APB1ENR &= ~(RCC_APB1ENR_DACEN))
#define __HAL_RCC_COMP_CLK_DISABLE()      (RCC->APB1ENR &= ~(RCC_APB1ENR_COMPEN))

/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Enable_Disable APB2 Clock Enable Disable
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
#define __HAL_RCC_TIM9_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM9EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM9EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM10_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM10EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM10EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM11_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM11EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM11EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_ADC1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SPI1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_USART1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_SYSCFG_CLK_DISABLE()    (RCC->APB2ENR &= ~(RCC_APB2ENR_SYSCFGEN))
#define __HAL_RCC_TIM9_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM9EN))
#define __HAL_RCC_TIM10_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM10EN))
#define __HAL_RCC_TIM11_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM11EN))
#define __HAL_RCC_ADC1_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_ADC1EN))
#define __HAL_RCC_SPI1_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_SPI1EN))
#define __HAL_RCC_USART1_CLK_DISABLE()    (RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN))

/**
  * @}
  */

/** @defgroup RCC_Peripheral_Clock_Force_Release RCC Peripheral Clock Force Release
  * @brief  Force or release AHB peripheral reset.
  * @{
  */ 
#define __HAL_RCC_AHB_FORCE_RESET()       (RCC->AHBRSTR = 0xFFFFFFFFU)
#define __HAL_RCC_GPIOA_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOARST))
#define __HAL_RCC_GPIOB_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOBRST))
#define __HAL_RCC_GPIOC_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOCRST))
#define __HAL_RCC_GPIOD_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIODRST))
#define __HAL_RCC_GPIOH_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOHRST))

#define __HAL_RCC_CRC_FORCE_RESET()       (RCC->AHBRSTR |= (RCC_AHBRSTR_CRCRST))
#define __HAL_RCC_FLITF_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_FLITFRST))
#define __HAL_RCC_DMA1_FORCE_RESET()      (RCC->AHBRSTR |= (RCC_AHBRSTR_DMA1RST))

#define __HAL_RCC_AHB_RELEASE_RESET()     (RCC->AHBRSTR = 0x00000000U)
#define __HAL_RCC_GPIOA_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOARST))
#define __HAL_RCC_GPIOB_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOBRST))
#define __HAL_RCC_GPIOC_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOCRST))
#define __HAL_RCC_GPIOD_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIODRST))
#define __HAL_RCC_GPIOH_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOHRST))

#define __HAL_RCC_CRC_RELEASE_RESET()     (RCC->AHBRSTR &= ~(RCC_AHBRSTR_CRCRST))
#define __HAL_RCC_FLITF_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_FLITFRST))
#define __HAL_RCC_DMA1_RELEASE_RESET()    (RCC->AHBRSTR &= ~(RCC_AHBRSTR_DMA1RST))

/**
  * @}
  */

/** @defgroup RCC_APB1_Force_Release_Reset APB1 Force Release Reset
  * @brief  Force or release APB1 peripheral reset.
  * @{   
  */
#define __HAL_RCC_APB1_FORCE_RESET()      (RCC->APB1RSTR = 0xFFFFFFFFU)  
#define __HAL_RCC_TIM2_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM4_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM4RST))
#define __HAL_RCC_TIM6_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_TIM7_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_WWDG_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_SPI2_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_USART2_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_USART3_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_USART3RST))
#define __HAL_RCC_I2C1_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_USB_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_USBRST))
#define __HAL_RCC_PWR_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_PWRRST))
#define __HAL_RCC_DAC_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_DACRST))
#define __HAL_RCC_COMP_FORCE_RESET()      (RCC->APB1RSTR |= (RCC_APB1RSTR_COMPRST))

#define __HAL_RCC_APB1_RELEASE_RESET()    (RCC->APB1RSTR = 0x00000000U) 
#define __HAL_RCC_TIM2_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM4_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM4RST))
#define __HAL_RCC_TIM6_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_TIM7_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_WWDG_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_SPI2_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_USART2_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_USART3_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART3RST))
#define __HAL_RCC_I2C1_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_USB_RELEASE_RESET()     (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USBRST))
#define __HAL_RCC_PWR_RELEASE_RESET()     (RCC->APB1RSTR &= ~(RCC_APB1RSTR_PWRRST))
#define __HAL_RCC_DAC_RELEASE_RESET()     (RCC->APB1RSTR &= ~(RCC_APB1RSTR_DACRST))
#define __HAL_RCC_COMP_RELEASE_RESET()    (RCC->APB1RSTR &= ~(RCC_APB1RSTR_COMPRST))

/**
  * @}
  */

/** @defgroup RCC_APB2_Force_Release_Reset APB2 Force Release Reset
  * @brief  Force or release APB1 peripheral reset.
  * @{   
  */
#define __HAL_RCC_APB2_FORCE_RESET()      (RCC->APB2RSTR = 0xFFFFFFFFU)  
#define __HAL_RCC_SYSCFG_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_SYSCFGRST))
#define __HAL_RCC_TIM9_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM9RST))
#define __HAL_RCC_TIM10_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM10RST))
#define __HAL_RCC_TIM11_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM11RST))
#define __HAL_RCC_ADC1_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_USART1RST))

#define __HAL_RCC_APB2_RELEASE_RESET()    (RCC->APB2RSTR = 0x00000000U)
#define __HAL_RCC_SYSCFG_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SYSCFGRST))
#define __HAL_RCC_TIM9_RELEASE_RESET()    (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM9RST))
#define __HAL_RCC_TIM10_RELEASE_RESET()   (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM10RST))
#define __HAL_RCC_TIM11_RELEASE_RESET()   (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM11RST))
#define __HAL_RCC_ADC1_RELEASE_RESET()    (RCC->APB2RSTR &= ~(RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_RELEASE_RESET()    (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART1RST))

/**
  * @}
  */

/** @defgroup RCC_Peripheral_Clock_Sleep_Enable_Disable RCC Peripheral Clock Sleep Enable Disable
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#define __HAL_RCC_GPIOA_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOALPEN))
#define __HAL_RCC_GPIOB_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOBLPEN))
#define __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOCLPEN))
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIODLPEN))
#define __HAL_RCC_GPIOH_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOHLPEN))

#define __HAL_RCC_CRC_CLK_SLEEP_ENABLE()      (RCC->AHBLPENR |= (RCC_AHBLPENR_CRCLPEN))
#define __HAL_RCC_FLITF_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_FLITFLPEN))
#define __HAL_RCC_DMA1_CLK_SLEEP_ENABLE()     (RCC->AHBLPENR |= (RCC_AHBLPENR_DMA1LPEN))

#define __HAL_RCC_GPIOA_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOALPEN))
#define __HAL_RCC_GPIOB_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOBLPEN))
#define __HAL_RCC_GPIOC_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOCLPEN))
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIODLPEN))
#define __HAL_RCC_GPIOH_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOHLPEN))

#define __HAL_RCC_CRC_CLK_SLEEP_DISABLE()     (RCC->AHBLPENR &= ~(RCC_AHBLPENR_CRCLPEN))
#define __HAL_RCC_FLITF_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_FLITFLPEN))
#define __HAL_RCC_DMA1_CLK_SLEEP_DISABLE()    (RCC->AHBLPENR &= ~(RCC_AHBLPENR_DMA1LPEN))

/** @brief  Enable or disable the APB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *           power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  */
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM2LPEN))
#define __HAL_RCC_TIM3_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM3LPEN))
#define __HAL_RCC_TIM4_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM4LPEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM6LPEN))
#define __HAL_RCC_TIM7_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM7LPEN))
#define __HAL_RCC_WWDG_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_WWDGLPEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_SPI2LPEN))
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()   (RCC->APB1LPENR |= (RCC_APB1LPENR_USART2LPEN))
#define __HAL_RCC_USART3_CLK_SLEEP_ENABLE()   (RCC->APB1LPENR |= (RCC_APB1LPENR_USART3LPEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_I2C1LPEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_I2C2LPEN))
#define __HAL_RCC_USB_CLK_SLEEP_ENABLE()      (RCC->APB1LPENR |= (RCC_APB1LPENR_USBLPEN))
#define __HAL_RCC_PWR_CLK_SLEEP_ENABLE()      (RCC->APB1LPENR |= (RCC_APB1LPENR_PWRLPEN))
#define __HAL_RCC_DAC_CLK_SLEEP_ENABLE()      (RCC->APB1LPENR |= (RCC_APB1LPENR_DACLPEN))
#define __HAL_RCC_COMP_CLK_SLEEP_ENABLE()     (RCC->APB1LPENR |= (RCC_APB1LPENR_COMPLPEN))

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM2LPEN))
#define __HAL_RCC_TIM3_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM3LPEN))
#define __HAL_RCC_TIM4_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM4LPEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM6LPEN))
#define __HAL_RCC_TIM7_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM7LPEN))
#define __HAL_RCC_WWDG_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_WWDGLPEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_SPI2LPEN))
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()  (RCC->APB1LPENR &= ~(RCC_APB1LPENR_USART2LPEN))
#define __HAL_RCC_USART3_CLK_SLEEP_DISABLE()  (RCC->APB1LPENR &= ~(RCC_APB1LPENR_USART3LPEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_I2C1LPEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_I2C2LPEN))
#define __HAL_RCC_USB_CLK_SLEEP_DISABLE()     (RCC->APB1LPENR &= ~(RCC_APB1LPENR_USBLPEN))
#define __HAL_RCC_PWR_CLK_SLEEP_DISABLE()     (RCC->APB1LPENR &= ~(RCC_APB1LPENR_PWRLPEN))
#define __HAL_RCC_DAC_CLK_SLEEP_DISABLE()     (RCC->APB1LPENR &= ~(RCC_APB1LPENR_DACLPEN))
#define __HAL_RCC_COMP_CLK_SLEEP_DISABLE()    (RCC->APB1LPENR &= ~(RCC_APB1LPENR_COMPLPEN))

/** @brief  Enable or disable the APB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *           power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  */
#define __HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE()   (RCC->APB2LPENR |= (RCC_APB2LPENR_SYSCFGLPEN))
#define __HAL_RCC_TIM9_CLK_SLEEP_ENABLE()     (RCC->APB2LPENR |= (RCC_APB2LPENR_TIM9LPEN))
#define __HAL_RCC_TIM10_CLK_SLEEP_ENABLE()    (RCC->APB2LPENR |= (RCC_APB2LPENR_TIM10LPEN))
#define __HAL_RCC_TIM11_CLK_SLEEP_ENABLE()    (RCC->APB2LPENR |= (RCC_APB2LPENR_TIM11LPEN))
#define __HAL_RCC_ADC1_CLK_SLEEP_ENABLE()     (RCC->APB2LPENR |= (RCC_APB2LPENR_ADC1LPEN))
#define __HAL_RCC_SPI1_CLK_SLEEP_ENABLE()     (RCC->APB2LPENR |= (RCC_APB2LPENR_SPI1LPEN))
#define __HAL_RCC_USART1_CLK_SLEEP_ENABLE()   (RCC->APB2LPENR |= (RCC_APB2LPENR_USART1LPEN))

#define __HAL_RCC_SYSCFG_CLK_SLEEP_DISABLE()  (RCC->APB2LPENR &= ~(RCC_APB2LPENR_SYSCFGLPEN))
#define __HAL_RCC_TIM9_CLK_SLEEP_DISABLE()    (RCC->APB2LPENR &= ~(RCC_APB2LPENR_TIM9LPEN))
#define __HAL_RCC_TIM10_CLK_SLEEP_DISABLE()   (RCC->APB2LPENR &= ~(RCC_APB2LPENR_TIM10LPEN))
#define __HAL_RCC_TIM11_CLK_SLEEP_DISABLE()   (RCC->APB2LPENR &= ~(RCC_APB2LPENR_TIM11LPEN))
#define __HAL_RCC_ADC1_CLK_SLEEP_DISABLE()    (RCC->APB2LPENR &= ~(RCC_APB2LPENR_ADC1LPEN))
#define __HAL_RCC_SPI1_CLK_SLEEP_DISABLE()    (RCC->APB2LPENR &= ~(RCC_APB2LPENR_SPI1LPEN))
#define __HAL_RCC_USART1_CLK_SLEEP_DISABLE()  (RCC->APB2LPENR &= ~(RCC_APB2LPENR_USART1LPEN))

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

#define __HAL_RCC_GPIOA_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOAEN)) != RESET)
#define __HAL_RCC_GPIOB_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOBEN)) != RESET)
#define __HAL_RCC_GPIOC_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOCEN)) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIODEN)) != RESET)
#define __HAL_RCC_GPIOH_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOHEN)) != RESET)
#define __HAL_RCC_CRC_IS_CLK_ENABLED()         ((RCC->AHBENR & (RCC_AHBENR_CRCEN)) != RESET)
#define __HAL_RCC_FLITF_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_FLITFEN)) != RESET)
#define __HAL_RCC_DMA1_IS_CLK_ENABLED()        ((RCC->AHBENR & (RCC_AHBENR_DMA1EN)) != RESET)
#define __HAL_RCC_GPIOA_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOAEN)) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOBEN)) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOCEN)) == RESET)
#define __HAL_RCC_GPIOD_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIODEN)) == RESET)
#define __HAL_RCC_GPIOH_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOHEN)) == RESET)
#define __HAL_RCC_CRC_IS_CLK_DISABLED()        ((RCC->AHBENR & (RCC_AHBENR_CRCEN)) == RESET)
#define __HAL_RCC_FLITF_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_FLITFEN)) == RESET)
#define __HAL_RCC_DMA1_IS_CLK_DISABLED()       ((RCC->AHBENR & (RCC_AHBENR_DMA1EN)) == RESET)

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

#define __HAL_RCC_TIM2_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM2EN)) != RESET)
#define __HAL_RCC_TIM3_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM3EN)) != RESET)
#define __HAL_RCC_TIM4_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM4EN)) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM6EN)) != RESET)
#define __HAL_RCC_TIM7_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM7EN)) != RESET)
#define __HAL_RCC_WWDG_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_WWDGEN)) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_SPI2EN)) != RESET)
#define __HAL_RCC_USART2_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_USART2EN)) != RESET)
#define __HAL_RCC_USART3_IS_CLK_ENABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_USART3EN)) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN)) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_I2C2EN)) != RESET)
#define __HAL_RCC_USB_IS_CLK_ENABLED()         ((RCC->APB1ENR & (RCC_APB1ENR_USBEN)) != RESET)
#define __HAL_RCC_PWR_IS_CLK_ENABLED()         ((RCC->APB1ENR & (RCC_APB1ENR_PWREN)) != RESET)
#define __HAL_RCC_DAC_IS_CLK_ENABLED()         ((RCC->APB1ENR & (RCC_APB1ENR_DACEN)) != RESET)
#define __HAL_RCC_COMP_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_COMPEN)) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM2EN)) == RESET)
#define __HAL_RCC_TIM3_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM3EN)) == RESET)
#define __HAL_RCC_TIM4_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM4EN)) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM6EN)) == RESET)
#define __HAL_RCC_TIM7_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM7EN)) == RESET)
#define __HAL_RCC_WWDG_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_WWDGEN)) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_SPI2EN)) == RESET)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_USART2EN)) == RESET)
#define __HAL_RCC_USART3_IS_CLK_DISABLED()     ((RCC->APB1ENR & (RCC_APB1ENR_USART3EN)) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN)) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_I2C2EN)) == RESET)
#define __HAL_RCC_USB_IS_CLK_DISABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_USBEN)) == RESET)
#define __HAL_RCC_PWR_IS_CLK_DISABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_PWREN)) == RESET)
#define __HAL_RCC_DAC_IS_CLK_DISABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_DACEN)) == RESET)
#define __HAL_RCC_COMP_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_COMPEN)) == RESET)

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

#define __HAL_RCC_SYSCFG_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_SYSCFGEN)) != RESET)
#define __HAL_RCC_TIM9_IS_CLK_ENABLED()        ((RCC->APB2ENR & (RCC_APB2ENR_TIM9EN)) != RESET)
#define __HAL_RCC_TIM10_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_TIM10EN)) != RESET)
#define __HAL_RCC_TIM11_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_TIM11EN)) != RESET)
#define __HAL_RCC_ADC1_IS_CLK_ENABLED()        ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN)) != RESET)
#define __HAL_RCC_SPI1_IS_CLK_ENABLED()        ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN)) != RESET)
#define __HAL_RCC_USART1_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) != RESET)
#define __HAL_RCC_SYSCFG_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_SYSCFGEN)) == RESET)
#define __HAL_RCC_TIM9_IS_CLK_DISABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_TIM9EN)) == RESET)
#define __HAL_RCC_TIM10_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM10EN)) == RESET)
#define __HAL_RCC_TIM11_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM11EN)) == RESET)
#define __HAL_RCC_ADC1_IS_CLK_DISABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN)) == RESET)
#define __HAL_RCC_SPI1_IS_CLK_DISABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN)) == RESET)
#define __HAL_RCC_USART1_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) == RESET)
  
/**
  * @}
  */

/** @defgroup RCC_AHB_Clock_Sleep_Enable_Disable_Status AHB Peripheral Clock Sleep Enable Disable Status
  * @brief  Get the enable or disable status of the AHB peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_GPIOA_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOALPEN)) != RESET)
#define __HAL_RCC_GPIOB_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOBLPEN)) != RESET)
#define __HAL_RCC_GPIOC_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOCLPEN)) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIODLPEN)) != RESET)
#define __HAL_RCC_GPIOH_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOHLPEN)) != RESET)
#define __HAL_RCC_CRC_IS_CLK_SLEEP_ENABLED()         ((RCC->AHBLPENR & (RCC_AHBLPENR_CRCLPEN)) != RESET)
#define __HAL_RCC_FLITF_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_FLITFLPEN)) != RESET)
#define __HAL_RCC_DMA1_IS_CLK_SLEEP_ENABLED()        ((RCC->AHBLPENR & (RCC_AHBLPENR_DMA1LPEN)) != RESET)
#define __HAL_RCC_GPIOA_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOALPEN)) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOBLPEN)) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOCLPEN)) == RESET)
#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIODLPEN)) == RESET)
#define __HAL_RCC_GPIOH_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOHLPEN)) == RESET)
#define __HAL_RCC_CRC_IS_CLK_SLEEP_DISABLED()        ((RCC->AHBLPENR & (RCC_AHBLPENR_CRCLPEN)) == RESET)
#define __HAL_RCC_FLITF_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_FLITFLPEN)) == RESET)
#define __HAL_RCC_DMA1_IS_CLK_SLEEP_DISABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_DMA1LPEN)) == RESET)

/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Sleep_Enable_Disable_Status APB1 Peripheral Clock Sleep Enable Disable Status
  * @brief  Get the enable or disable status of the APB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_TIM2_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM2LPEN)) != RESET)
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM3LPEN)) != RESET)
#define __HAL_RCC_TIM4_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM4LPEN)) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM6LPEN)) != RESET)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM7LPEN)) != RESET)
#define __HAL_RCC_WWDG_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_WWDGLPEN)) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_SPI2LPEN)) != RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_ENABLED()      ((RCC->APB1LPENR & (RCC_APB1LPENR_USART2LPEN)) != RESET)
#define __HAL_RCC_USART3_IS_CLK_SLEEP_ENABLED()      ((RCC->APB1LPENR & (RCC_APB1LPENR_USART3LPEN)) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_I2C1LPEN)) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_I2C2LPEN)) != RESET)
#define __HAL_RCC_USB_IS_CLK_SLEEP_ENABLED()         ((RCC->APB1LPENR & (RCC_APB1LPENR_USBLPEN)) != RESET)
#define __HAL_RCC_PWR_IS_CLK_SLEEP_ENABLED()         ((RCC->APB1LPENR & (RCC_APB1LPENR_PWRLPEN)) != RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_ENABLED()         ((RCC->APB1LPENR & (RCC_APB1LPENR_DACLPEN)) != RESET)
#define __HAL_RCC_COMP_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_COMPLPEN)) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM2LPEN)) == RESET)
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM3LPEN)) == RESET)
#define __HAL_RCC_TIM4_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM4LPEN)) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM6LPEN)) == RESET)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM7LPEN)) == RESET)
#define __HAL_RCC_WWDG_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_WWDGLPEN)) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_SPI2LPEN)) == RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_DISABLED()     ((RCC->APB1LPENR & (RCC_APB1LPENR_USART2LPEN)) == RESET)
#define __HAL_RCC_USART3_IS_CLK_SLEEP_DISABLED()     ((RCC->APB1LPENR & (RCC_APB1LPENR_USART3LPEN)) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_I2C1LPEN)) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_I2C2LPEN)) == RESET)
#define __HAL_RCC_USB_IS_CLK_SLEEP_DISABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_USBLPEN)) == RESET)
#define __HAL_RCC_PWR_IS_CLK_SLEEP_DISABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_PWRLPEN)) == RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_DISABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_DACLPEN)) == RESET)
#define __HAL_RCC_COMP_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_COMPLPEN)) == RESET)

/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Sleep_Enable_Disable_Status APB2 Peripheral Clock Sleep Enable Disable Status
  * @brief  Get the enable or disable status of the APB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#define __HAL_RCC_SYSCFG_IS_CLK_SLEEP_ENABLED()      ((RCC->APB2LPENR & (RCC_APB2LPENR_SYSCFGLPEN)) != RESET)
#define __HAL_RCC_TIM9_IS_CLK_SLEEP_ENABLED()        ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM9LPEN)) != RESET)
#define __HAL_RCC_TIM10_IS_CLK_SLEEP_ENABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM10LPEN)) != RESET)
#define __HAL_RCC_TIM11_IS_CLK_SLEEP_ENABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM11LPEN)) != RESET)
#define __HAL_RCC_ADC1_IS_CLK_SLEEP_ENABLED()        ((RCC->APB2LPENR & (RCC_APB2LPENR_ADC1LPEN)) != RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_ENABLED()        ((RCC->APB2LPENR & (RCC_APB2LPENR_SPI1LPEN)) != RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_ENABLED()      ((RCC->APB2LPENR & (RCC_APB2LPENR_USART1LPEN)) != RESET)
#define __HAL_RCC_SYSCFG_IS_CLK_SLEEP_DISABLED()     ((RCC->APB2LPENR & (RCC_APB2LPENR_SYSCFGLPEN)) == RESET)
#define __HAL_RCC_TIM9_IS_CLK_SLEEP_DISABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM9LPEN)) == RESET)
#define __HAL_RCC_TIM10_IS_CLK_SLEEP_DISABLED()      ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM10LPEN)) == RESET)
#define __HAL_RCC_TIM11_IS_CLK_SLEEP_DISABLED()      ((RCC->APB2LPENR & (RCC_APB2LPENR_TIM11LPEN)) == RESET)
#define __HAL_RCC_ADC1_IS_CLK_SLEEP_DISABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_ADC1LPEN)) == RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_DISABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_SPI1LPEN)) == RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_DISABLED()     ((RCC->APB2LPENR & (RCC_APB2LPENR_USART1LPEN)) == RESET)

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
#define __HAL_RCC_HSI_ENABLE()  (*(__IO uint32_t *) RCC_CR_HSION_BB = ENABLE)
#define __HAL_RCC_HSI_DISABLE() (*(__IO uint32_t *) RCC_CR_HSION_BB = DISABLE)

/** @brief  Macro to adjust the Internal High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI RC.
  * @param  _HSICALIBRATIONVALUE_ specifies the calibration trimming value.
  *         (default is RCC_HSICALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 0x1F.
  */  
#define __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(_HSICALIBRATIONVALUE_) \
          (MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, (uint32_t)(_HSICALIBRATIONVALUE_) << POSITION_VAL(RCC_ICSCR_HSITRIM)))

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
#define __HAL_RCC_LSI_ENABLE()  (*(__IO uint32_t *) RCC_CSR_LSION_BB = ENABLE)

/** @brief Macro to disable the Internal Low Speed oscillator (LSI).
  * @note   LSI can not be disabled if the IWDG is running.  
  * @note   When the LSI is stopped, LSIRDY flag goes low after 6 LSI oscillator
  *         clock cycles. 
  */
#define __HAL_RCC_LSI_DISABLE() (*(__IO uint32_t *) RCC_CSR_LSION_BB = DISABLE)

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
                        SET_BIT(RCC->CSR, RCC_CSR_LSEON);                   \
                      }                                                     \
                      else if ((__STATE__) == RCC_LSE_OFF)                  \
                      {                                                     \
                        CLEAR_BIT(RCC->CSR, RCC_CSR_LSEON);                 \
                        CLEAR_BIT(RCC->CSR, RCC_CSR_LSEBYP);                \
                      }                                                     \
                      else if ((__STATE__) == RCC_LSE_BYPASS)               \
                      {                                                     \
                        SET_BIT(RCC->CSR, RCC_CSR_LSEBYP);                  \
                        SET_BIT(RCC->CSR, RCC_CSR_LSEON);                   \
                      }                                                     \
                      else                                                  \
                      {                                                     \
                        CLEAR_BIT(RCC->CSR, RCC_CSR_LSEON);                 \
                        CLEAR_BIT(RCC->CSR, RCC_CSR_LSEBYP);                \
                      }                                                     \
                    }while(0U)

/**
  * @}
  */

/** @defgroup RCC_MSI_Configuration  MSI Configuration
  * @{   
  */

/** @brief  Macro to enable Internal Multi Speed oscillator (MSI).
  * @note   After enabling the MSI, the application software should wait on MSIRDY
  *         flag to be set indicating that MSI clock is stable and can be used as
  *         system clock source.  
  */
#define __HAL_RCC_MSI_ENABLE()  (*(__IO uint32_t *) RCC_CR_MSION_BB = ENABLE)
                                       
/** @brief  Macro to disable the Internal Multi Speed oscillator (MSI).
  * @note   The MSI is stopped by hardware when entering STOP and STANDBY modes.
  *         It is used (enabled by hardware) as system clock source after startup
  *         from Reset, wakeup from STOP and STANDBY mode, or in case of failure
  *         of the HSE used directly or indirectly as system clock (if the Clock
  *         Security System CSS is enabled).             
  * @note   MSI can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the MSI.  
  * @note   When the MSI is stopped, MSIRDY flag goes low after 6 MSI oscillator
  *         clock cycles.  
  */
#define __HAL_RCC_MSI_DISABLE() (*(__IO uint32_t *) RCC_CR_MSION_BB = DISABLE)

/** @brief  Macro adjusts Internal Multi Speed oscillator (MSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal MSI RC.
  * @param  _MSICALIBRATIONVALUE_ specifies the calibration trimming value.
  *         (default is RCC_MSICALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 0xFF.
  */  
#define __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(_MSICALIBRATIONVALUE_) \
          (MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSITRIM, (uint32_t)(_MSICALIBRATIONVALUE_) << POSITION_VAL(RCC_ICSCR_MSITRIM)))
          
/* @brief  Macro to configures the Internal Multi Speed oscillator (MSI) clock range.
  * @note     After restart from Reset or wakeup from STANDBY, the MSI clock is 
  *           around 2.097 MHz. The MSI clock does not change after wake-up from
  *           STOP mode.
  * @note    The MSI clock range can be modified on the fly.     
  * @param  _MSIRANGEVALUE_ specifies the MSI Clock range.
  *   This parameter must be one of the following values:
  *     @arg @ref RCC_MSIRANGE_0 MSI clock is around 65.536 KHz
  *     @arg @ref RCC_MSIRANGE_1 MSI clock is around 131.072 KHz
  *     @arg @ref RCC_MSIRANGE_2 MSI clock is around 262.144 KHz
  *     @arg @ref RCC_MSIRANGE_3 MSI clock is around 524.288 KHz
  *     @arg @ref RCC_MSIRANGE_4 MSI clock is around 1.048 MHz
  *     @arg @ref RCC_MSIRANGE_5 MSI clock is around 2.097 MHz (default after Reset or wake-up from STANDBY)
  *     @arg @ref RCC_MSIRANGE_6 MSI clock is around 4.194 MHz
  */  
#define __HAL_RCC_MSI_RANGE_CONFIG(_MSIRANGEVALUE_) (MODIFY_REG(RCC->ICSCR, \
          RCC_ICSCR_MSIRANGE, (uint32_t)(_MSIRANGEVALUE_)))

/** @brief  Macro to get the Internal Multi Speed oscillator (MSI) clock range in run mode
  * @retval MSI clock range.
  *         This parameter must be one of the following values:
  *     @arg @ref RCC_MSIRANGE_0 MSI clock is around 65.536 KHz
  *     @arg @ref RCC_MSIRANGE_1 MSI clock is around 131.072 KHz
  *     @arg @ref RCC_MSIRANGE_2 MSI clock is around 262.144 KHz
  *     @arg @ref RCC_MSIRANGE_3 MSI clock is around 524.288 KHz
  *     @arg @ref RCC_MSIRANGE_4 MSI clock is around 1.048 MHz
  *     @arg @ref RCC_MSIRANGE_5 MSI clock is around 2.097 MHz (default after Reset or wake-up from STANDBY)
  *     @arg @ref RCC_MSIRANGE_6 MSI clock is around 4.194 MHz
  */
#define __HAL_RCC_GET_MSI_RANGE() (uint32_t)(READ_BIT(RCC->ICSCR, RCC_ICSCR_MSIRANGE))

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
#define __HAL_RCC_PLL_ENABLE()          (*(__IO uint32_t *) RCC_CR_PLLON_BB = ENABLE)

/** @brief Macro to disable the main PLL.
  * @note   The main PLL can not be disabled if it is used as system clock source
  */
#define __HAL_RCC_PLL_DISABLE()         (*(__IO uint32_t *) RCC_CR_PLLON_BB = DISABLE)

/** @brief Macro to configure the main PLL clock source, multiplication and division factors.
  * @note   This function must be used only when the main PLL is disabled.
  *  
  * @param  __RCC_PLLSOURCE__ specifies the PLL entry clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_PLLSOURCE_HSI HSI oscillator clock selected as PLL clock entry
  *            @arg @ref RCC_PLLSOURCE_HSE HSE oscillator clock selected as PLL clock entry
  * @param  __PLLMUL__ specifies the multiplication factor for PLL VCO output clock
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_PLL_MUL3   PLLVCO = PLL clock entry x 3
  *             @arg @ref RCC_PLL_MUL4   PLLVCO = PLL clock entry x 4
  *             @arg @ref RCC_PLL_MUL6   PLLVCO = PLL clock entry x 6
  *             @arg @ref RCC_PLL_MUL8   PLLVCO = PLL clock entry x 8
  *             @arg @ref RCC_PLL_MUL12  PLLVCO = PLL clock entry x 12
  *             @arg @ref RCC_PLL_MUL16  PLLVCO = PLL clock entry x 16
  *             @arg @ref RCC_PLL_MUL24  PLLVCO = PLL clock entry x 24
  *             @arg @ref RCC_PLL_MUL32  PLLVCO = PLL clock entry x 32
  *             @arg @ref RCC_PLL_MUL48  PLLVCO = PLL clock entry x 48
  * @note The PLL VCO clock frequency must not exceed 96 MHz when the product is in
  *          Range 1, 48 MHz when the product is in Range 2 and 24 MHz when the product is
  *          in Range 3.
  *
  * @param  __PLLDIV__ specifies the division factor for PLL VCO input clock
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_PLL_DIV2 PLL clock output = PLLVCO / 2
  *             @arg @ref RCC_PLL_DIV3 PLL clock output = PLLVCO / 3
  *             @arg @ref RCC_PLL_DIV4 PLL clock output = PLLVCO / 4
  *   
  */
#define __HAL_RCC_PLL_CONFIG(__RCC_PLLSOURCE__, __PLLMUL__, __PLLDIV__)\
          MODIFY_REG(RCC->CFGR, (RCC_CFGR_PLLSRC|RCC_CFGR_PLLMUL|RCC_CFGR_PLLDIV),((__RCC_PLLSOURCE__) | (__PLLMUL__) | (__PLLDIV__)))

/** @brief  Get oscillator clock selected as PLL input clock
  * @retval The clock source used for PLL entry. The returned value can be one
  *         of the following:
  *             @arg @ref RCC_PLLSOURCE_HSI HSI oscillator clock selected as PLL input clock
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
  *              @arg @ref RCC_SYSCLKSOURCE_MSI MSI oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_HSI HSI oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_HSE HSE oscillator is used as system clock source.
  *              @arg @ref RCC_SYSCLKSOURCE_PLLCLK PLL output is used as system clock source.
  */
#define __HAL_RCC_SYSCLK_CONFIG(__SYSCLKSOURCE__) \
                  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, (__SYSCLKSOURCE__))

/** @brief  Macro to get the clock source used as system clock.
  * @retval The clock source used as system clock. The returned value can be one
  *         of the following:
  *             @arg @ref RCC_SYSCLKSOURCE_STATUS_MSI MSI used as system clock
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

/** @brief  Macro to configure the MCO clock.
  * @param  __MCOCLKSOURCE__ specifies the MCO clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK      No clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK       System Clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI          HSI oscillator clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_MSI          MSI oscillator clock selected as MCO clock 
  *            @arg @ref RCC_MCO1SOURCE_HSE HSE oscillator clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK       PLL clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSI          LSI clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSE          LSE clock selected as MCO clock
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
  * @note   RTC prescaler cannot be modified if HSE is enabled (HSEON = 1).
  *
  * @param  __RTC_CLKSOURCE__ specifies the RTC clock source.
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_RTCCLKSOURCE_NO_CLK No clock selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_LSE LSE selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_LSI LSI selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV2 HSE divided by 2 selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV4 HSE divided by 4 selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV8 HSE divided by 8 selected as RTC clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV16 HSE divided by 16 selected as RTC clock
  * @note   If the LSE or LSI is used as RTC clock source, the RTC continues to
  *         work in STOP and STANDBY modes, and can be used as wakeup source.
  *         However, when the HSE clock is used as RTC clock source, the RTC
  *         cannot be used in STOP and STANDBY modes.    
  * @note   The maximum input clock frequency for RTC is 1MHz (when using HSE as
  *         RTC clock source).
  */
#define __HAL_RCC_RTC_CLKPRESCALER(__RTC_CLKSOURCE__) do { \
            if(((__RTC_CLKSOURCE__) & RCC_CSR_RTCSEL_HSE) == RCC_CSR_RTCSEL_HSE)          \
            {                                                                             \
              MODIFY_REG(RCC->CR, RCC_CR_RTCPRE, ((__RTC_CLKSOURCE__) & RCC_CR_RTCPRE));  \
            }                                                                             \
          } while (0U)

#define __HAL_RCC_RTC_CONFIG(__RTC_CLKSOURCE__) do { \
                                      __HAL_RCC_RTC_CLKPRESCALER(__RTC_CLKSOURCE__);      \
                                      RCC->CSR |= ((__RTC_CLKSOURCE__) & RCC_CSR_RTCSEL); \
                                    } while (0U)
                                                   
/** @brief Macro to get the RTC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_RTCCLKSOURCE_NO_CLK No clock selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_LSE LSE selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_LSI LSI selected as RTC clock
  *            @arg @ref RCC_RTCCLKSOURCE_HSE_DIVX HSE divided by X selected as RTC clock (X can be retrieved thanks to @ref __HAL_RCC_GET_RTC_HSE_PRESCALER()
  */
#define __HAL_RCC_GET_RTC_SOURCE() (READ_BIT(RCC->CSR, RCC_CSR_RTCSEL))

/**
  * @brief   Get the RTC and LCD HSE clock divider (RTCCLK / LCDCLK).
  *
  * @retval Returned value can be one of the following values:
  *         @arg @ref RCC_RTC_HSE_DIV_2  HSE divided by 2 selected as RTC clock
  *         @arg @ref RCC_RTC_HSE_DIV_4  HSE divided by 4 selected as RTC clock
  *         @arg @ref RCC_RTC_HSE_DIV_8  HSE divided by 8 selected as RTC clock
  *         @arg @ref RCC_RTC_HSE_DIV_16 HSE divided by 16 selected as RTC clock
  *
  */
#define  __HAL_RCC_GET_RTC_HSE_PRESCALER() ((uint32_t)(READ_BIT(RCC->CR, RCC_CR_RTCPRE)))                                                   

/** @brief Macro to enable the the RTC clock.
  * @note   These macros must be used only after the RTC clock source was selected.
  */
#define __HAL_RCC_RTC_ENABLE()          (*(__IO uint32_t *) RCC_CSR_RTCEN_BB = ENABLE)

/** @brief Macro to disable the the RTC clock.
  * @note  These macros must be used only after the RTC clock source was selected.
  */
#define __HAL_RCC_RTC_DISABLE()         (*(__IO uint32_t *) RCC_CSR_RTCEN_BB = DISABLE)

/** @brief  Macro to force the Backup domain reset.
  * @note   This function resets the RTC peripheral (including the backup registers)
  *         and the RTC clock source selection in RCC_CSR register.
  * @note   The BKPSRAM is not affected by this reset.   
  */
#define __HAL_RCC_BACKUPRESET_FORCE()   (*(__IO uint32_t *) RCC_CSR_RTCRST_BB = ENABLE)

/** @brief  Macros to release the Backup domain reset.
  */
#define __HAL_RCC_BACKUPRESET_RELEASE() (*(__IO uint32_t *) RCC_CSR_RTCRST_BB = DISABLE)

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
  *            @arg @ref RCC_IT_MSIRDY MSI ready interrupt
  *            @arg @ref RCC_IT_LSECSS LSE CSS interrupt (not available for STM32L100xB || STM32L151xB || STM32L152xB devices)
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
  *            @arg @ref RCC_IT_MSIRDY MSI ready interrupt
  *            @arg @ref RCC_IT_LSECSS LSE CSS interrupt (not available for STM32L100xB || STM32L151xB || STM32L152xB devices)
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
  *            @arg @ref RCC_IT_MSIRDY MSI ready interrupt
  *            @arg @ref RCC_IT_LSECSS LSE CSS interrupt (not available for STM32L100xB || STM32L151xB || STM32L152xB devices)
  *            @arg @ref RCC_IT_CSS Clock Security System interrupt
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
  *            @arg @ref RCC_IT_MSIRDY MSI ready interrupt
  *            @arg @ref RCC_IT_LSECSS LSE CSS interrupt (not available for STM32L100xB || STM32L151xB || STM32L152xB devices)
  *            @arg @ref RCC_IT_CSS Clock Security System interrupt
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_IT(__INTERRUPT__) ((RCC->CIR & (__INTERRUPT__)) == (__INTERRUPT__))

/** @brief Set RMVF bit to clear the reset flags.
  *         The reset flags are RCC_FLAG_PINRST, RCC_FLAG_PORRST, RCC_FLAG_SFTRST,
  *         RCC_FLAG_IWDGRST, RCC_FLAG_WWDGRST, RCC_FLAG_LPWRRST
  */
#define __HAL_RCC_CLEAR_RESET_FLAGS() (RCC->CSR |= RCC_CSR_RMVF)

/** @brief  Check RCC flag is set or not.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_FLAG_HSIRDY HSI oscillator clock ready.
  *            @arg @ref RCC_FLAG_MSIRDY MSI oscillator clock ready.
  *            @arg @ref RCC_FLAG_HSERDY HSE oscillator clock ready.
  *            @arg @ref RCC_FLAG_PLLRDY Main PLL clock ready.
  *            @arg @ref RCC_FLAG_LSERDY LSE oscillator clock ready.
  *            @arg @ref RCC_FLAG_LSECSS CSS on LSE failure Detection (*)
  *            @arg @ref RCC_FLAG_LSIRDY LSI oscillator clock ready.
  *            @arg @ref RCC_FLAG_OBLRST Option Byte Load reset
  *            @arg @ref RCC_FLAG_PINRST  Pin reset.
  *            @arg @ref RCC_FLAG_PORRST  POR/PDR reset.
  *            @arg @ref RCC_FLAG_SFTRST  Software reset.
  *            @arg @ref RCC_FLAG_IWDGRST Independent Watchdog reset.
  *            @arg @ref RCC_FLAG_WWDGRST Window Watchdog reset.
  *            @arg @ref RCC_FLAG_LPWRRST Low Power reset.
  * @note (*) This bit is available in high and medium+ density devices only.
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_RCC_GET_FLAG(__FLAG__) (((((__FLAG__) >> 5U) == CR_REG_INDEX)? RCC->CR :RCC->CSR) & (1U << ((__FLAG__) & RCC_FLAG_MASK)))   

/**
  * @}
  */

/**
  * @}
  */

/* Include RCC HAL Extension module */
#include "stm32l1xx_hal_rcc_ex.h"

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
uint32_t          HAL_RCC_GetPCLK2Freq(void);
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

#endif /* __STM32L1xx_HAL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

