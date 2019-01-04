/**
  ******************************************************************************
  * @file    stm32l1xx_ll_comp.h
  * @author  MCD Application Team
  * @brief   Header file of COMP LL module.
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
#ifndef __STM32L1xx_LL_COMP_H
#define __STM32L1xx_LL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/** @addtogroup STM32L1xx_LL_Driver
  * @{
  */

#if defined (COMP1) || defined (COMP2)

/** @defgroup COMP_LL COMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup COMP_LL_Private_Constants COMP Private Constants
  * @{
  */

/* COMP registers bits positions */
#define LL_COMP_OUTPUT_LEVEL_COMP1_BITOFFSET_POS ( 7U) /* Value equivalent to POSITION_VAL(COMP_CSR_CMP1OUT) */
#define LL_COMP_OUTPUT_LEVEL_COMP2_BITOFFSET_POS (13U) /* Value equivalent to POSITION_VAL(COMP_CSR_CMP2OUT) */
#define LL_COMP_ENABLE_COMP1_BITOFFSET_POS       ( 4U) /* Value equivalent to POSITION_VAL(COMP_CSR_CMP1EN) */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup COMP_LL_Private_Macros COMP Private Macros
  * @{
  */

/**
  * @brief  Driver macro reserved for internal use: if COMP instance selected
  *         is odd (COMP1, COMP3, ...), return value '1', else return '0'.
  * @param  __COMP_INSTANCE__ COMP instance
  * @retval If COMP instance is odd, value '1'. Else, value '0'.
*/
#define __COMP_IS_INSTANCE_ODD(__COMP_INSTANCE__)                              \
  ((~((uint32_t)(__COMP_INSTANCE__) - COMP_BASE)) & 0x00000001)

/**
  * @brief  Driver macro reserved for internal use: if COMP instance selected
  *         is even (COMP2, COMP4, ...), return value '1', else return '0'.
  * @param  __COMP_INSTANCE__ COMP instance
  * @retval If COMP instance is even, value '1'. Else, value '0'.
*/
#define __COMP_IS_INSTANCE_EVEN(__COMP_INSTANCE__)                             \
  ((uint32_t)(__COMP_INSTANCE__) - COMP_BASE)

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup COMP_LL_ES_INIT COMP Exported Init structure
  * @{
  */

/**
  * @brief  Structure definition of some features of COMP instance.
  */
typedef struct
{
  uint32_t PowerMode;                   /*!< Set comparator operating mode to adjust power and speed.
                                             This parameter can be a value of @ref COMP_LL_EC_POWERMODE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetPowerMode(). */

  uint32_t InputPlus;                   /*!< Set comparator input plus (non-inverting input).
                                             This parameter can be a value of @ref COMP_LL_EC_INPUT_PLUS
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetInputPlus(). */

  uint32_t InputMinus;                  /*!< Set comparator input minus (inverting input).
                                             This parameter can be a value of @ref COMP_LL_EC_INPUT_MINUS
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetInputMinus(). */

  uint32_t OutputSelection;             /*!< Set comparator output selection.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_SELECTION
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputSelection(). */

} LL_COMP_InitTypeDef;

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup COMP_LL_Exported_Constants COMP Exported Constants
  * @{
  */

/** @defgroup COMP_LL_EC_COMMON_WINDOWMODE Comparator common modes - Window mode
  * @{
  */
#define LL_COMP_WINDOWMODE_DISABLE                 (0x00000000U)           /*!< Window mode disable: Comparators 1 and 2 are independent */
#define LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON (COMP_CSR_WNDWE)        /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP2 input plus (COMP1 input plus is no more accessible, either from GPIO and from ADC channel VCOMP). */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_POWERMODE Comparator modes - Power mode
  * @{
  */
#define LL_COMP_POWERMODE_ULTRALOWPOWER   (0x00000000U)               /*!< COMP power mode to low speed (specific to COMP instance: COMP2) */
#define LL_COMP_POWERMODE_MEDIUMSPEED     (COMP_CSR_SPEED)            /*!< COMP power mode to fast speed (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PLUS Comparator inputs - Input plus (input non-inverting) selection
  * @{
  */
#define LL_COMP_INPUT_PLUS_NONE         (0x00000000U)           /*!< Comparator input plus connected not connected */
#define LL_COMP_INPUT_PLUS_IO1          (RI_ASCR2_GR6_1)        /*!< Comparator input plus connected to IO1 (pin PB4 for COMP2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_PLUS_IO2          (RI_ASCR2_GR6_2)        /*!< Comparator input plus connected to IO1 (pin PB5 for COMP2) (specific to COMP instance: COMP2) */
#if defined(RI_ASCR1_CH_31)
#define LL_COMP_INPUT_PLUS_IO3          (RI_ASCR2_GR6_3)        /*!< Comparator input plus connected to IO1 (pin PB6 for COMP2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_PLUS_IO4          (RI_ASCR2_GR6_4)        /*!< Comparator input plus connected to IO1 (pin PB7 for COMP2) (specific to COMP instance: COMP2) */
#endif
#define LL_COMP_INPUT_PLUS_IO5          (RI_ASCR1_CH_0)         /*!< Comparator input plus connected to IO5 (pin PA0 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO6          (RI_ASCR1_CH_1)         /*!< Comparator input plus connected to IO6 (pin PA1 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO7          (RI_ASCR1_CH_2)         /*!< Comparator input plus connected to IO7 (pin PA2 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO8          (RI_ASCR1_CH_3)         /*!< Comparator input plus connected to IO8 (pin PA3 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO9          (RI_ASCR1_CH_4)         /*!< Comparator input plus connected to IO9 (pin PA4 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO10         (RI_ASCR1_CH_5)         /*!< Comparator input plus connected to IO10 (pin PA5 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO11         (RI_ASCR1_CH_5)         /*!< Comparator input plus connected to IO11 (pin PA5 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO12         (RI_ASCR1_CH_7)         /*!< Comparator input plus connected to IO12 (pin PA7 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO13         (RI_ASCR1_CH_8)         /*!< Comparator input plus connected to IO13 (pin PB0 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO14         (RI_ASCR1_CH_9)         /*!< Comparator input plus connected to IO14 (pin PB1 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO15         (RI_ASCR1_CH_10)        /*!< Comparator input plus connected to IO15 (pin PC0 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO16         (RI_ASCR1_CH_11)        /*!< Comparator input plus connected to IO16 (pin PC1 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO17         (RI_ASCR1_CH_12)        /*!< Comparator input plus connected to IO17 (pin PC2 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO18         (RI_ASCR1_CH_13)        /*!< Comparator input plus connected to IO18 (pin PC3 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO19         (RI_ASCR1_CH_14)        /*!< Comparator input plus connected to IO19 (pin PC4 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO20         (RI_ASCR1_CH_15)        /*!< Comparator input plus connected to IO20 (pin PC5 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO21         (RI_ASCR1_CH_18)        /*!< Comparator input plus connected to IO21 (pin PB12 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO22         (RI_ASCR1_CH_19)        /*!< Comparator input plus connected to IO22 (pin PB13 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO23         (RI_ASCR1_CH_20)        /*!< Comparator input plus connected to IO23 (pin PB14 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO24         (RI_ASCR1_CH_21)        /*!< Comparator input plus connected to IO24 (pin PB15 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO25         (RI_ASCR1_CH_22)        /*!< Comparator input plus connected to IO25 (pin PE7 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO26         (RI_ASCR1_CH_23)        /*!< Comparator input plus connected to IO26 (pin PE8 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO27         (RI_ASCR1_CH_24)        /*!< Comparator input plus connected to IO27 (pin PE9 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO28         (RI_ASCR1_CH_25)        /*!< Comparator input plus connected to IO28 (pin PE10 for COMP1) (specific to COMP instance: COMP1) */
#if defined(RI_ASCR1_CH_31)
#define LL_COMP_INPUT_PLUS_IO29         (RI_ASCR1_CH_27)        /*!< Comparator input plus connected to IO29 (pin PF6 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO30         (RI_ASCR1_CH_28)        /*!< Comparator input plus connected to IO30 (pin PF7 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO31         (RI_ASCR1_CH_29)        /*!< Comparator input plus connected to IO31 (pin PF8 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO32         (RI_ASCR1_CH_30)        /*!< Comparator input plus connected to IO32 (pin PF9 for COMP1) (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_IO33         (RI_ASCR1_CH_31)        /*!< Comparator input plus connected to IO33 (pin PF10 for COMP1) (specific to COMP instance: COMP1) */
#endif
#if defined(OPAMP1)
#define LL_COMP_INPUT_PLUS_OPAMP1       (RI_ASCR1_CH_3)         /*!< Comparator input plus connected to OPAMP1 output (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_PLUS_OPAMP2       (RI_ASCR1_CH_8)         /*!< Comparator input plus connected to OPAMP2 output (specific to COMP instance: COMP1) */
#endif
#if defined(OPAMP3)
#define LL_COMP_INPUT_PLUS_OPAMP3       (RI_ASCR1_CH_13)        /*!< Comparator input plus connected to OPAMP3 output (specific to COMP instance: COMP1) */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_MINUS Comparator inputs - Input minus (input inverting) selection
  * @{
  */
#define LL_COMP_INPUT_MINUS_1_4VREFINT  (COMP_CSR_INSEL_2                    | COMP_CSR_INSEL_0) /*!< Comparator input minus connected to 1/4 VrefInt (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_1_2VREFINT  (COMP_CSR_INSEL_2                                      ) /*!< Comparator input minus connected to 1/2 VrefInt (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_3_4VREFINT  (                   COMP_CSR_INSEL_1 | COMP_CSR_INSEL_0) /*!< Comparator input minus connected to 3/4 VrefInt (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_VREFINT     (                   COMP_CSR_INSEL_1                   ) /*!< Comparator input minus connected to VrefInt */
#define LL_COMP_INPUT_MINUS_DAC1_CH1    (COMP_CSR_INSEL_2 | COMP_CSR_INSEL_1                   ) /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_DAC1_CH2    (COMP_CSR_INSEL_2 | COMP_CSR_INSEL_1 | COMP_CSR_INSEL_0) /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_IO1         (                                      COMP_CSR_INSEL_0) /*!< Comparator input minus connected to IO1 (pin PB3 for COMP2) (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PULLING_RESISTOR Comparator input - Pulling resistor
  * @{
  */
#define LL_COMP_INPUT_MINUS_PULL_NO        (0x00000000U)           /*!< Comparator input minus not connected to any pulling resistor */
#define LL_COMP_INPUT_MINUS_PULL_UP_10K    (COMP_CSR_10KPU)        /*!< Comparator input minus connected to pull-up resistor of 10kOhm (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_MINUS_PULL_UP_400K   (COMP_CSR_400KPU)       /*!< Comparator input minus connected to pull-up resistor of 400kOhm (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_MINUS_PULL_DOWN_10K  (COMP_CSR_10KPD)        /*!< Comparator input minus connected to pull-down resistor of 10kOhm (specific to COMP instance: COMP1) */
#define LL_COMP_INPUT_MINUS_PULL_DOWN_400K (COMP_CSR_400KPD)       /*!< Comparator input minus connected to pull-down resistor of 400kOhm (specific to COMP instance: COMP1) */

/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_SELECTION Comparator output - Output selection
  * @{
  */
#define LL_COMP_OUTPUT_NONE             (COMP_CSR_OUTSEL_2 | COMP_CSR_OUTSEL_1 | COMP_CSR_OUTSEL_0) /*!< COMP output is not connected to other peripherals (except GPIO and EXTI that are always connected to COMP output) (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_IC4         (0x00000000)                                                /*!< COMP output connected to TIM2 input capture 4  (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_OCREFCLR    (                                        COMP_CSR_OUTSEL_0) /*!< COMP output connected to TIM2 OCREF clear      (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_IC4         (                    COMP_CSR_OUTSEL_1                    ) /*!< COMP output connected to TIM3 input capture 4  (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_OCREFCLR    (                    COMP_CSR_OUTSEL_1 | COMP_CSR_OUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear      (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM4_IC4         (COMP_CSR_OUTSEL_2                                        ) /*!< COMP output connected to TIM4 input capture 4  (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM4_OCREFCLR    (COMP_CSR_OUTSEL_2                     | COMP_CSR_OUTSEL_0) /*!< COMP output connected to TIM4 OCREF clear      (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM10_IC1        (COMP_CSR_OUTSEL_2 | COMP_CSR_OUTSEL_1                    ) /*!< COMP output connected to TIM10 input capture 1 (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_LEVEL Comparator output - Output level
  * @{
  */
#define LL_COMP_OUTPUT_LEVEL_LOW        (0x00000000U) /*!< Comparator output level low (if the polarity is not inverted, otherwise to be complemented) */
#define LL_COMP_OUTPUT_LEVEL_HIGH       (0x00000001U) /*!< Comparator output level high (if the polarity is not inverted, otherwise to be complemented) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_HW_DELAYS  Definitions of COMP hardware constraints delays
  * @note   Only COMP IP HW delays are defined in COMP LL driver driver,
  *         not timeout values.
  *         For details on delays values, refer to descriptions in source code
  *         above each literal definition.
  * @{
  */

/* Delay for comparator startup time.                                         */
/* Note: Delay required to reach propagation delay specification.             */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tSTART").                                                       */
/* Unit: us                                                                   */
#define LL_COMP_DELAY_STARTUP_US          (25U)  /*!< Delay for COMP startup time */


/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup COMP_LL_Exported_Macros COMP Exported Macros
  * @{
  */
/** @defgroup COMP_LL_EM_WRITE_READ Common write and read registers macro
  * @{
  */

/**
  * @brief  Write a value in COMP register
  * @param  __INSTANCE__ comparator instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_COMP_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in COMP register
  * @param  __INSTANCE__ comparator instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_COMP_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/** @defgroup COMP_LL_EM_HELPER_MACRO COMP helper macro
  * @{
  */

/**
  * @brief  Helper macro to select the COMP common instance
  *         to which is belonging the selected COMP instance.
  * @note   COMP common register instance can be used to
  *         set parameters common to several COMP instances.
  *         Refer to functions having argument "COMPxy_COMMON" as parameter.
  * @param  __COMPx__ COMP instance
  * @retval COMP common instance or value "0" if there is no COMP common instance.
  */
#define __LL_COMP_COMMON_INSTANCE(__COMPx__)                                   \
  (COMP12_COMMON)

/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup COMP_LL_Exported_Functions COMP Exported Functions
  * @{
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_common Configuration of COMP hierarchical scope: common to several COMP instances
  * @{
  */

/**
  * @brief  Set window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WNDWE          LL_COMP_SetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowMode)
{
  MODIFY_REG(COMPxy_COMMON->CSR, COMP_CSR_WNDWE, WindowMode);
}

/**
  * @brief  Get window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WNDWE          LL_COMP_GetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON)
{
  return (uint32_t)(READ_BIT(COMPxy_COMMON->CSR, COMP_CSR_WNDWE));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_modes Configuration of comparator modes
  * @{
  */

/**
  * @brief  Set comparator instance operating mode to adjust power and speed.
  * @rmtoll COMP2_CSR   SPEED           LL_COMP_SetPowerMode
  * @param  COMPx Comparator instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED   (1)
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER (1)
  *         
  *         (1) Available only on COMP instance: COMP2.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetPowerMode(COMP_TypeDef *COMPx, uint32_t PowerMode)
{
  MODIFY_REG(COMP->CSR, COMP_CSR_SPEED, PowerMode);
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @rmtoll COMP2_CSR   SPEED           LL_COMP_GetPowerMode
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED   (1)
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER (1)
  *         
  *         (1) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetPowerMode(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR, COMP_CSR_SPEED));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_inputs Configuration of comparator inputs
  * @{
  */

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll RI       RI_ASCR1_CH    LL_COMP_SetInputPlus\n
  *         RI       RI_ASCR2_GR6   LL_COMP_SetInputPlus
  * @param  COMPx Comparator instance
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_NONE
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3 (2)(5)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO4 (2)(5)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO5 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO6 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO7 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO8 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO9 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO10 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO11 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO12 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO13 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO14 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO15 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO16 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO17 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO18 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO19 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO20 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO21 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO22 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO23 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO24 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO25 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO26 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO27 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO28 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO29 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO30 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO31 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO32 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO33 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP1 (1)(3)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP2 (1)(3)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP3 (1)(4)
  *         
  *         (1) Available only on COMP instance: COMP1. \n
  *         (2) Available only on COMP instance: COMP2. \n
  *         (3) Available on devices: STM32L100xB, STM32L151xB, STM32L152xB, STM32L100xBA, STM32L151xBA, STM32L152xBA, STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD \n
  *         (4) Available on devices: STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD \n
  *         (5) Available on devices: STM32L100xC, STM32L151xC, STM32L152xC, STM32L162xC, STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX, STM32L152xE, STM32L152xDX, STM32L162xE, STM32L162xDX
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPlus(COMP_TypeDef *COMPx, uint32_t InputPlus)
{
  /* Set switch in routing interface (RI) register ASCR1 or ASCR2 */
  /* Note: If COMP instance COMP1 is selected, this function performs         */
  /*       necessary actions on routing interface:                            */
  /*        - close switch netween comparator 1 and switch matrix             */
  /*          (RI_ASCR1_VCOMP)                                                */
  /*        - enable IO switch control mode (RI_ASCR1_SCM)                    */
  /*          If ADC needs to be used afterwards, disable IO switch control   */
  /*          mode using function @ref LL_RI_DisableSwitchControlMode().      */
  register uint32_t *preg = ((uint32_t *)((uint32_t) ((uint32_t)(&(RI->ASCR1)) + ((__COMP_IS_INSTANCE_EVEN(COMPx)) << 2U))));
  
  MODIFY_REG(*preg,
             (RI_ASCR1_CH * __COMP_IS_INSTANCE_ODD(COMPx)) | (RI_ASCR2_GR6 * __COMP_IS_INSTANCE_EVEN(COMPx)),
             InputPlus | ((RI_ASCR1_VCOMP | RI_ASCR1_SCM) * __COMP_IS_INSTANCE_ODD(COMPx)));
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll RI       RI_ASCR1_CH    LL_COMP_GetInputPlus\n
  *         RI       RI_ASCR2_GR6   LL_COMP_GetInputPlus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_NONE
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3 (2)(5)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO4 (2)(5)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO5 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO6 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO7 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO8 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO9 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO10 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO11 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO12 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO13 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO14 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO15 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO16 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO17 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO18 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO19 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO20 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO21 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO22 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO23 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO24 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO25 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO26 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO27 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO28 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO29 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO30 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO31 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO32 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO33 (1)(4)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP1 (1)(3)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP2 (1)(3)
  *         @arg @ref LL_COMP_INPUT_PLUS_OPAMP3 (1)(4)
  *         
  *         (1) Available only on COMP instance: COMP1. \n
  *         (2) Available only on COMP instance: COMP2. \n
  *         (3) Available on devices: STM32L100xB, STM32L151xB, STM32L152xB, STM32L100xBA, STM32L151xBA, STM32L152xBA, STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD \n
  *         (4) Available on devices: STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD \n
  *         (5) Available on devices: STM32L100xC, STM32L151xC, STM32L152xC, STM32L162xC, STM32L151xCA, STM32L151xD, STM32L152xCA, STM32L152xD, STM32L162xCA, STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX, STM32L152xE, STM32L152xDX, STM32L162xE, STM32L162xDX
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPlus(COMP_TypeDef *COMPx)
{
  /* Get switch state in routing interface (RI) register ASCR1 or ASCR2 */
  register uint32_t *preg = ((uint32_t *)((uint32_t) ((uint32_t)(&(RI->ASCR1)) + ((__COMP_IS_INSTANCE_EVEN(COMPx)) << 2U))));
  
  return (uint32_t)(READ_BIT(*preg,
                             (RI_ASCR1_CH * __COMP_IS_INSTANCE_ODD(COMPx)) | (RI_ASCR2_GR6 * __COMP_IS_INSTANCE_EVEN(COMPx))));
}

/**
  * @brief  Set comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP_CSR_INSEL LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1        (1)
  *         
  *         (1) Available only on COMP instance: COMP2.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  /* On this STM32 serie, only COMP instance COMP1 input minus is fixed to   */
  /* VrefInt. Check of comparator instance is implemented to modify register  */
  /* only if COMP2 is selected.                                               */
  MODIFY_REG(COMP->CSR,
             COMP_CSR_INSEL * __COMP_IS_INSTANCE_EVEN(COMPx),
             InputMinus     * __COMP_IS_INSTANCE_EVEN(COMPx));
}

/**
  * @brief  Get comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP_CSR_INSEL LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1        (1)
  *         
  *         (1) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputMinus(COMP_TypeDef *COMPx)
{
  /* On this STM32 serie, only COMP instance COMP1 input minus is fixed to   */
  /* VrefInt. Check of comparator instance is implemented to return           */
  /* the comparator input plus depending on COMP instance selected.           */
  return (uint32_t)((READ_BIT(COMP->CSR, COMP_CSR_INSEL) * __COMP_IS_INSTANCE_EVEN(COMPx))
                     | (LL_COMP_INPUT_MINUS_VREFINT * __COMP_IS_INSTANCE_ODD(COMPx)));
}

/**
  * @brief  Set comparator input pulling resistor.
  * @rmtoll CSR      10KPU          LL_COMP_SetInputPullingResistor\n
  *         CSR      400KPU         LL_COMP_SetInputPullingResistor\n
  *         CSR      10KPD          LL_COMP_SetInputPullingResistor\n
  *         CSR      400KPD         LL_COMP_SetInputPullingResistor
  * @param  COMPx Comparator instance
  * @param  InputPullingResistor This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_NO
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_UP_10K    (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_UP_400K   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_DOWN_10K  (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_DOWN_400K (1)
  *         
  *         (1) Available only on COMP instance: COMP1.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPullingResistor(COMP_TypeDef *COMPx, uint32_t InputPullingResistor)
{
  /* On this STM32 serie, only COMP instance COMP1 has input pulling         */
  /* resistor. Check of comparator instance is implemented to modify register */
  /* only if COMP1 is selected.                                               */
  MODIFY_REG(COMP->CSR,
             (COMP_CSR_10KPU | COMP_CSR_400KPU | COMP_CSR_10KPD | COMP_CSR_400KPD) * __COMP_IS_INSTANCE_ODD(COMPx),
             InputPullingResistor * __COMP_IS_INSTANCE_ODD(COMPx));
}

/**
  * @brief  Get comparator input pulling resistor.
  * @rmtoll CSR      10KPU          LL_COMP_SetInputPullingResistor\n
  *         CSR      400KPU         LL_COMP_SetInputPullingResistor\n
  *         CSR      10KPD          LL_COMP_SetInputPullingResistor\n
  *         CSR      400KPD         LL_COMP_SetInputPullingResistor
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_NO
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_UP_10K    (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_UP_400K   (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_DOWN_10K  (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_PULL_DOWN_400K (1)
  *         
  *         (1) Available only on COMP instance: COMP1.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPullingResistor(COMP_TypeDef *COMPx)
{
  /* On this STM32 serie, only COMP instance COMP1 has input pulling         */
  /* resistor. Check of comparator instance is implemented to return          */
  /* the comparator input pulling resistor depending on COMP instance         */
  /* selected.                                                                */
  /* On this STM32 serie, only COMP instance COMP1 input minus is fixed to   */
  /* VrefInt. Check of comparator instance is implemented to return           */
  /* the comparator input plus depending on COMP instance selected.           */
  return (uint32_t)((READ_BIT(COMP->CSR, (COMP_CSR_10KPU | COMP_CSR_400KPU | COMP_CSR_10KPD | COMP_CSR_400KPD)) * __COMP_IS_INSTANCE_ODD(COMPx))
                     | (LL_COMP_INPUT_MINUS_PULL_NO * __COMP_IS_INSTANCE_EVEN(COMPx)));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_output Configuration of comparator output
  * @{
  */

/**
  * @brief  Set comparator output selection.
  * @note   Availability of parameters of output selection to timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      OUTSEL         LL_COMP_SetOutputSelection
  * @param  COMPx Comparator instance
  * @param  OutputSelection This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM10_IC1     (1)(2)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  *         (2) Available only on COMP instance: COMP2.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputSelection(COMP_TypeDef *COMPx, uint32_t OutputSelection)
{
  /* On this STM32 serie, only COMP instance COMP2 has feature output        */
  /* selection. Check of comparator instance is implemented to modify register*/
  /* only if COMP2 is selected.                                               */
  MODIFY_REG(COMP->CSR,
             COMP_CSR_OUTSEL * __COMP_IS_INSTANCE_EVEN(COMPx),
             OutputSelection * __COMP_IS_INSTANCE_EVEN(COMPx));
}

/**
  * @brief  Get comparator output selection.
  * @note   Availability of parameters of output selection to timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      OUTSEL         LL_COMP_GetOutputSelection
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC4      (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_OCREFCLR (1)(2)
  *         @arg @ref LL_COMP_OUTPUT_TIM10_IC1     (1)(2)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  *         (2) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputSelection(COMP_TypeDef *COMPx)
{
  /* On this STM32 serie, only COMP instance COMP2 has feature output        */
  /* selection. Check of comparator instance is implemented to return         */
  /* the comparator output depending on COMP instance selected.               */
  return (uint32_t)((READ_BIT(COMP->CSR, COMP_CSR_OUTSEL) * __COMP_IS_INSTANCE_EVEN(COMPx))
                     | (LL_COMP_OUTPUT_NONE * __COMP_IS_INSTANCE_ODD(COMPx)));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Operation Operation on comparator instance
  * @{
  */

/**
  * @brief  Enable comparator instance.
  * @note   After enable from off state, comparator requires a delay
  *         to reach reach propagation delay specification.
  *         Refer to device datasheet, parameter "tSTART".
  * @rmtoll CSR      COMP1EN        LL_COMP_Enable\n
  *         CSR      COMP_CSR_INSEL LL_COMP_Enable
  * @param  COMPx Comparator instance (1)
  *         
  *         (1) On this STM32 serie, the only COMP instance that can be enabled
  *             using this function is COMP1.
  *             COMP2 is enabled by setting input minus.
  *             Refer to function @ref LL_COMP_SetInputMinus().
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  /* On this STM32 serie, only COMP instance COMP1 has a dedicated bit       */
  /* for comparator enable. Check of comparator instance is implemented       */
  /* to modify register only if COMP1 is selected.                            */
  SET_BIT(COMP->CSR, __COMP_IS_INSTANCE_ODD(COMPx) << LL_COMP_ENABLE_COMP1_BITOFFSET_POS);
}

/**
  * @brief  Disable comparator instance.
  * @note   On this STM32 serie, COMP2 is disabled by clearing input minus
  *         selection. If COMP2 must be enabled afterwards, input minus must
  *         be set. Refer to function @ref LL_COMP_SetInputMinus().
  * @rmtoll CSR      COMP1EN        LL_COMP_Disable\n
  *         CSR      COMP_CSR_INSEL LL_COMP_Disable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Disable(COMP_TypeDef *COMPx)
{
  /* Note: On this STM32 serie, COMP2 is enabled by setting input minus.     */
  /*       Refer to function @ref LL_COMP_SetInputMinus().                    */
  /*       To disable COMP2, bitfield of input minus selection is reset.      */
  CLEAR_BIT(COMP->CSR, (COMP_CSR_CMP1EN * __COMP_IS_INSTANCE_ODD(COMPx)) | (COMP_CSR_INSEL * __COMP_IS_INSTANCE_EVEN(COMPx)));
}

/**
  * @brief  Get comparator enable state
  *         (0: COMP is disabled, 1: COMP is enabled)
  * @rmtoll CSR      COMP1EN        LL_COMP_IsEnabled\n
  *         CSR      COMP_CSR_INSEL LL_COMP_IsEnabled
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsEnabled(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMP->CSR, (COMP_CSR_CMP1EN * __COMP_IS_INSTANCE_ODD(COMPx)) | (COMP_CSR_INSEL * __COMP_IS_INSTANCE_EVEN(COMPx))) != (0U));
}

/**
  * @brief  Read comparator instance output level.
  * @note   On this STM32 serie, comparator polarity is not settable
  *         and not inverted:
  *          - Comparator output is low when the input plus
  *            is at a lower voltage than the input minus
  *          - Comparator output is high when the input plus
  *            is at a higher voltage than the input minus
  * @rmtoll CSR      CMP1OUT        LL_COMP_ReadOutputLevel\n
  *         CSR      CMP2OUT        LL_COMP_ReadOutputLevel
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_ReadOutputLevel(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             ((__COMP_IS_INSTANCE_ODD(COMPx) << LL_COMP_OUTPUT_LEVEL_COMP1_BITOFFSET_POS) | (__COMP_IS_INSTANCE_EVEN(COMPx) << LL_COMP_OUTPUT_LEVEL_COMP2_BITOFFSET_POS)))
                    >> (LL_COMP_OUTPUT_LEVEL_COMP1_BITOFFSET_POS + ((LL_COMP_OUTPUT_LEVEL_COMP2_BITOFFSET_POS - LL_COMP_OUTPUT_LEVEL_COMP1_BITOFFSET_POS) * __COMP_IS_INSTANCE_EVEN(COMPx)))
                   );
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup COMP_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

ErrorStatus LL_COMP_DeInit(COMP_TypeDef *COMPx);
ErrorStatus LL_COMP_Init(COMP_TypeDef *COMPx, LL_COMP_InitTypeDef *COMP_InitStruct);
void        LL_COMP_StructInit(LL_COMP_InitTypeDef *COMP_InitStruct);

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

#endif /* COMP1 || COMP2 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32L1xx_LL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
