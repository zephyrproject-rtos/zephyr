/**
  ******************************************************************************
  * @file    stm32f0xx_ll_comp.h
  * @author  MCD Application Team
  * @brief   Header file of COMP LL module.
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
#ifndef __STM32F0xx_LL_COMP_H
#define __STM32F0xx_LL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/** @addtogroup STM32F0xx_LL_Driver
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

/* Differentiation between COMP instances */
/* Note: Value not corresponding to a register offset since both              */
/*       COMP instances are sharing the same register) .                      */
#define COMPX_BASE  COMP_BASE
#define COMPX       (COMP1 - COMP2)

/* COMP registers bits positions */
#define LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS ((uint32_t)14U) /* Value equivalent to POSITION_VAL(COMP_CSR_COMP1OUT) */

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
  ((~(((uint32_t)(__COMP_INSTANCE__) - COMP_BASE) >> 1U)) & 0x00000001)

/**
  * @brief  Driver macro reserved for internal use: if COMP instance selected
  *         is even (COMP2, COMP4, ...), return value '1', else return '0'.
  * @param  __COMP_INSTANCE__ COMP instance
  * @retval If COMP instance is even, value '1'. Else, value '0'.
*/
#define __COMP_IS_INSTANCE_EVEN(__COMP_INSTANCE__)                             \
  (((uint32_t)(__COMP_INSTANCE__) - COMP_BASE) >> 1U)

/**
  * @brief  Driver macro reserved for internal use: from COMP instance
  *         selected, set offset of bits into COMP register.
  * @note   Since both COMP instances are sharing the same register
  *         with 2 area of bits with an offset of 16 bits, this function
  *         returns value "0" if COMP1 is selected and "16" if COMP2 is
  *         selected.
  * @param  __COMP_INSTANCE__ COMP instance
  * @retval Bits offset in register 32 bits
*/
#define __COMP_BITOFFSET_INSTANCE(__COMP_INSTANCE__)                           \
  (((uint32_t)(__COMP_INSTANCE__) - COMP_BASE) << 3U)

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

  uint32_t InputHysteresis;             /*!< Set comparator hysteresis mode of the input minus.
                                             This parameter can be a value of @ref COMP_LL_EC_INPUT_HYSTERESIS
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetInputHysteresis(). */

  uint32_t OutputSelection;             /*!< Set comparator output selection.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_SELECTION
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputSelection(). */

  uint32_t OutputPolarity;              /*!< Set comparator output polarity.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_POLARITY
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputPolarity(). */

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
#define LL_COMP_WINDOWMODE_DISABLE                 ((uint32_t)0x00000000U) /*!< Window mode disable: Comparators 1 and 2 are independent */
#define LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (COMP_CSR_WNDWEN)       /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP1 input plus (COMP2 input plus is no more accessible). */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_POWERMODE Comparator modes - Power mode
  * @{
  */
#define LL_COMP_POWERMODE_HIGHSPEED     ((uint32_t)0x00000000U)                       /*!< COMP power mode to high speed */
#define LL_COMP_POWERMODE_MEDIUMSPEED   (COMP_CSR_COMP1MODE_0)                        /*!< COMP power mode to medium speed */
#define LL_COMP_POWERMODE_LOWPOWER      (COMP_CSR_COMP1MODE_1)                        /*!< COMP power mode to low power */
#define LL_COMP_POWERMODE_ULTRALOWPOWER (COMP_CSR_COMP1MODE_1 | COMP_CSR_COMP1MODE_0) /*!< COMP power mode to ultra-low power */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PLUS Comparator inputs - Input plus (input non-inverting) selection
  * @{
  */
#define LL_COMP_INPUT_PLUS_IO1          ((uint32_t)0x00000000U) /*!< Comparator input plus connected to IO1 (pin PA1 for COMP1, pin PA3 for COMP2) */
#define LL_COMP_INPUT_PLUS_DAC1_CH1     (COMP_CSR_COMP1SW1)     /*!< Comparator input plus connected to DAC1 channel 1 (DAC_OUT1), through dedicated switch (Note: this switch is solely intended to redirect signals onto high impedance input, such as COMP1 input plus (highly resistive switch)) (specific to COMP instance: COMP1) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_MINUS Comparator inputs - Input minus (input inverting) selection
  * @{
  */
#define LL_COMP_INPUT_MINUS_1_4VREFINT  ((uint32_t)0x00000000U)                                                 /*!< Comparator input minus connected to 1/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_1_2VREFINT  (                                                COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to 1/2 VrefInt  */
#define LL_COMP_INPUT_MINUS_3_4VREFINT  (                        COMP_CSR_COMP1INSEL_1                        ) /*!< Comparator input minus connected to 3/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_VREFINT     (                        COMP_CSR_COMP1INSEL_1 | COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to VrefInt */
#define LL_COMP_INPUT_MINUS_DAC1_CH1    (COMP_CSR_COMP1INSEL_2                                                ) /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1)  */
#define LL_COMP_INPUT_MINUS_DAC1_CH2    (COMP_CSR_COMP1INSEL_2                         | COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2)  */
#define LL_COMP_INPUT_MINUS_IO1         (COMP_CSR_COMP1INSEL_2 | COMP_CSR_COMP1INSEL_1                        ) /*!< Comparator input minus connected to IO1 (pin PA0 for COMP1, pin PA2 for COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_HYSTERESIS Comparator input - Hysteresis
  * @{
  */
#define LL_COMP_HYSTERESIS_NONE         ((uint32_t)0x00000000U)                       /*!< No hysteresis */
#define LL_COMP_HYSTERESIS_LOW          (                       COMP_CSR_COMP1HYST_0) /*!< Hysteresis level low */
#define LL_COMP_HYSTERESIS_MEDIUM       (COMP_CSR_COMP1HYST_1                       ) /*!< Hysteresis level medium */
#define LL_COMP_HYSTERESIS_HIGH         (COMP_CSR_COMP1HYST_1 | COMP_CSR_COMP1HYST_0) /*!< Hysteresis level high */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_SELECTION Comparator output - Output selection
  * @{
  */
/* Note: Output redirection is common for COMP1 and COMP2 */
#define LL_COMP_OUTPUT_NONE             ((uint32_t)0x00000000U)                                                    /*!< COMP output is not connected to other peripherals (except GPIO and EXTI that are always connected to COMP output) */
#define LL_COMP_OUTPUT_TIM1_BKIN        (COMP_CSR_COMP1OUTSEL_0)                                                   /*!< COMP output connected to TIM1 break input (BKIN) */
#define LL_COMP_OUTPUT_TIM1_IC1         (COMP_CSR_COMP1OUTSEL_1)                                                   /*!< COMP output connected to TIM1 input capture 1 */
#define LL_COMP_OUTPUT_TIM1_OCCLR       (COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0)                          /*!< COMP output connected to TIM1 OCREF clear */
#define LL_COMP_OUTPUT_TIM2_IC4         (COMP_CSR_COMP1OUTSEL_2)                                                   /*!< COMP output connected to TIM2 input capture 4 */
#define LL_COMP_OUTPUT_TIM2_OCCLR       (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear */
#define LL_COMP_OUTPUT_TIM3_IC1         (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1)                          /*!< COMP output connected to TIM3 input capture 1 */
#define LL_COMP_OUTPUT_TIM3_OCCLR       (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_POLARITY Comparator output - Output polarity
  * @{
  */
#define LL_COMP_OUTPUTPOL_NONINVERTED   ((uint32_t)0x00000000U)  /*!< COMP output polarity is not inverted: comparator output is high when the plus (non-inverting) input is at a higher voltage than the minus (inverting) input */
#define LL_COMP_OUTPUTPOL_INVERTED      (COMP_CSR_COMP1POL)      /*!< COMP output polarity is inverted: comparator output is low when the plus (non-inverting) input is at a lower voltage than the minus (inverting) input */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_LEVEL Comparator output - Output level
  * @{
  */
#define LL_COMP_OUTPUT_LEVEL_LOW        ((uint32_t)0x00000000U) /*!< Comparator output level low (if the polarity is not inverted, otherwise to be complemented) */
#define LL_COMP_OUTPUT_LEVEL_HIGH       ((uint32_t)0x00000001U) /*!< Comparator output level high (if the polarity is not inverted, otherwise to be complemented) */
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
#define LL_COMP_DELAY_STARTUP_US          ((uint32_t) 60U)  /*!< Delay for COMP startup time */

/* Delay for comparator voltage scaler stabilization time                     */
/* (voltage from VrefInt, delay based on VrefInt startup time).               */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tS_SC").                                                        */
/* Unit: us                                                                   */
#define LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US ((uint32_t) 200U)  /*!< Delay for COMP voltage scaler stabilization time */


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
  * @rmtoll CSR      WNDWEN         LL_COMP_SetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowMode)
{
  MODIFY_REG(COMPxy_COMMON->CSR, COMP_CSR_WNDWEN, WindowMode);
}

/**
  * @brief  Get window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WNDWEN         LL_COMP_GetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON)
{
  return (uint32_t)(READ_BIT(COMPxy_COMMON->CSR, COMP_CSR_WNDWEN));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_modes Configuration of comparator modes
  * @{
  */

/**
  * @brief  Set comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      COMP1MODE      LL_COMP_SetPowerMode\n
  *                  COMP2MODE      LL_COMP_SetPowerMode
  * @param  COMPx Comparator instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED
  *         @arg @ref LL_COMP_POWERMODE_LOWPOWER
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetPowerMode(COMP_TypeDef *COMPx, uint32_t PowerMode)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1MODE << __COMP_BITOFFSET_INSTANCE(COMPx),
             PowerMode          << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      COMP1MODE      LL_COMP_GetPowerMode\n
  *                     COMP2MODE       LL_COMP_GetPowerMode
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED
  *         @arg @ref LL_COMP_POWERMODE_LOWPOWER
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER
  */
__STATIC_INLINE uint32_t LL_COMP_GetPowerMode(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1MODE << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_inputs Configuration of comparator inputs
  * @{
  */

/**
  * @brief  Set comparator inputs minus (inverting) and plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP1INSEL     LL_COMP_ConfigInputs\n
  *         CSR      COMP2INSEL     LL_COMP_ConfigInputs\n
  *         CSR      COMP1SW1       LL_COMP_ConfigInputs
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1 (1)
  *         
  *         (1) Parameter available only on COMP instance: COMP1.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_ConfigInputs(COMP_TypeDef *COMPx, uint32_t InputMinus, uint32_t InputPlus)
{
  /* Note: Connection switch is applicable only to COMP instance COMP1,       */
  /*       therefore if COMP2 is selected the equivalent bit is               */
  /*       kept unmodified.                                                   */
  MODIFY_REG(COMP->CSR,
             (COMP_CSR_COMP1INSEL | (COMP_CSR_COMP1SW1 * __COMP_IS_INSTANCE_ODD(COMPx))) << __COMP_BITOFFSET_INSTANCE(COMPx),
             (InputMinus | InputPlus)                                        << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP1INSEL     LL_COMP_SetInputPlus\n
  *         CSR      COMP2INSEL     LL_COMP_SetInputPlus
  * @param  COMPx Comparator instance
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1 (1)
  *         
  *         (1) Parameter available only on COMP instance: COMP1.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPlus(COMP_TypeDef *COMPx, uint32_t InputPlus)
{
  /* Note: Connection switch is applicable only to COMP instance COMP1,       */
  /*       therefore if COMP2 is selected the equivalent bit is               */
  /*       kept unmodified.                                                   */
  MODIFY_REG(COMP->CSR,
             (COMP_CSR_COMP1SW1 * __COMP_IS_INSTANCE_ODD(COMPx)) << __COMP_BITOFFSET_INSTANCE(COMPx),
             InputPlus                                           << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP1INSEL     LL_COMP_GetInputPlus\n
  *         CSR      COMP2INSEL     LL_COMP_GetInputPlus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1 (1)
  *         
  *         (1) Parameter available only on COMP instance: COMP1.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPlus(COMP_TypeDef *COMPx)
{
  /* Note: Connection switch is applicable only to COMP instance COMP1,       */
  /*       therefore is COMP2 is selected the returned value will be null.    */
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1SW1 << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );
}

/**
  * @brief  Set comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP1SW1       LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1INSEL << __COMP_BITOFFSET_INSTANCE(COMPx),
             InputMinus          << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMP1SW1       LL_COMP_GetInputMinus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputMinus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1INSEL << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );
}

/**
  * @brief  Set comparator instance hysteresis mode of the input minus (inverting input).
  * @rmtoll CSR      COMP1HYST      LL_COMP_SetInputHysteresis\n
  *                  COMP2HYST      LL_COMP_SetInputHysteresis
  * @param  COMPx Comparator instance
  * @param  InputHysteresis This parameter can be one of the following values:
  *         @arg @ref LL_COMP_HYSTERESIS_NONE
  *         @arg @ref LL_COMP_HYSTERESIS_LOW
  *         @arg @ref LL_COMP_HYSTERESIS_MEDIUM
  *         @arg @ref LL_COMP_HYSTERESIS_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputHysteresis(COMP_TypeDef *COMPx, uint32_t InputHysteresis)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1HYST << __COMP_BITOFFSET_INSTANCE(COMPx),
             InputHysteresis    << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator instance hysteresis mode of the minus (inverting) input.
  * @rmtoll CSR      COMP1HYST      LL_COMP_GetInputHysteresis\n
  *                  COMP2HYST      LL_COMP_GetInputHysteresis
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_HYSTERESIS_NONE
  *         @arg @ref LL_COMP_HYSTERESIS_LOW
  *         @arg @ref LL_COMP_HYSTERESIS_MEDIUM
  *         @arg @ref LL_COMP_HYSTERESIS_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputHysteresis(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1HYST << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );

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
  * @rmtoll CSR      COMP1OUTSEL    LL_COMP_SetOutputSelection\n
  *                  COMP2OUTSEL    LL_COMP_SetOutputSelection
  * @param  COMPx Comparator instance
  * @param  OutputSelection This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN      (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_OCCLR     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR     (1)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputSelection(COMP_TypeDef *COMPx, uint32_t OutputSelection)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1OUTSEL << __COMP_BITOFFSET_INSTANCE(COMPx),
             OutputSelection      << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator output selection.
  * @note   Availability of parameters of output selection to timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      COMP1OUTSEL    LL_COMP_GetOutputSelection\n
  *                  COMP2OUTSEL    LL_COMP_GetOutputSelection
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN      (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_OCCLR     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR     (1)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputSelection(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1OUTSEL << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );
}

/**
  * @brief  Set comparator instance output polarity.
  * @rmtoll CSR      COMP1POL       LL_COMP_SetOutputPolarity\n
  *                  COMP2POL       LL_COMP_SetOutputPolarity
  * @param  COMPx Comparator instance
  * @param  OutputPolarity This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputPolarity(COMP_TypeDef *COMPx, uint32_t OutputPolarity)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1POL << __COMP_BITOFFSET_INSTANCE(COMPx),
             OutputPolarity    << __COMP_BITOFFSET_INSTANCE(COMPx) );
}

/**
  * @brief  Get comparator instance output polarity.
  * @rmtoll CSR      COMP1POL       LL_COMP_GetOutputPolarity\n
  *                  COMP2POL       LL_COMP_GetOutputPolarity
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputPolarity(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1POL << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> __COMP_BITOFFSET_INSTANCE(COMPx)
                   );
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
  *         CSR      COMP2EN        LL_COMP_Enable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  SET_BIT(COMP->CSR, COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Disable comparator instance.
  * @rmtoll CSR      COMP1EN        LL_COMP_Disable\n
  *         CSR      COMP2EN        LL_COMP_Disable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Disable(COMP_TypeDef *COMPx)
{
  CLEAR_BIT(COMP->CSR, COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator enable state
  *         (0: COMP is disabled, 1: COMP is enabled)
  * @rmtoll CSR      COMP1EN        LL_COMP_IsEnabled\n
  *         CSR      COMP2EN        LL_COMP_IsEnabled
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsEnabled(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMP->CSR, COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx)) == COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Lock comparator instance.
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll CSR      COMP1LOCK      LL_COMP_Lock\n
  *         CSR      COMP2LOCK      LL_COMP_Lock
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Lock(COMP_TypeDef *COMPx)
{
  SET_BIT(COMP->CSR, COMP_CSR_COMP1LOCK << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator lock state
  *         (0: COMP is unlocked, 1: COMP is locked).
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll CSR      COMP1LOCK      LL_COMP_IsLocked\n
  *         CSR      COMP2LOCK      LL_COMP_IsLocked
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsLocked(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMP->CSR, COMP_CSR_COMP1LOCK << __COMP_BITOFFSET_INSTANCE(COMPx)) == COMP_CSR_COMP1LOCK << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Read comparator instance output level.
  * @note   The comparator output level depends on the selected polarity
  *         (Refer to function @ref LL_COMP_SetOutputPolarity()).
  *         If the comparator polarity is not inverted:
  *          - Comparator output is low when the input plus
  *            is at a lower voltage than the input minus
  *          - Comparator output is high when the input plus
  *            is at a higher voltage than the input minus
  *         If the comparator polarity is inverted:
  *          - Comparator output is high when the input plus
  *            is at a lower voltage than the input minus
  *          - Comparator output is low when the input plus
  *            is at a higher voltage than the input minus
  * @rmtoll CSR      COMP1OUT       LL_COMP_ReadOutputLevel\n
  *         CSR      COMP2OUT       LL_COMP_ReadOutputLevel
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_ReadOutputLevel(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMP->CSR,
                             COMP_CSR_COMP1OUT << __COMP_BITOFFSET_INSTANCE(COMPx))
                    >> (__COMP_BITOFFSET_INSTANCE(COMPx) + LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS)
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

#endif /* __STM32F0xx_LL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
