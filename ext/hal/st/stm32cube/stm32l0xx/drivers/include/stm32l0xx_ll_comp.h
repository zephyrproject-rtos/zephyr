/**
  ******************************************************************************
  * @file    stm32l0xx_ll_comp.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
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
#ifndef __STM32L0xx_LL_COMP_H
#define __STM32L0xx_LL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"

/** @addtogroup STM32L0xx_LL_Driver
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
#define LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS ((uint32_t)30U) /* Value equivalent to POSITION_VAL(COMP_CSR_COMP1VALUE) */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
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
#define LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (COMP_CSR_COMP1WM)      /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP1 input plus (COMP2 input plus is no more accessible). */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_POWERMODE Comparator modes - Power mode
  * @{
  */
#define LL_COMP_POWERMODE_ULTRALOWPOWER   ((uint32_t)0x00000000U)     /*!< COMP power mode to low speed (specific to COMP instance: COMP2) */
#define LL_COMP_POWERMODE_MEDIUMSPEED     (COMP_CSR_COMP2SPEED)       /*!< COMP power mode to fast speed (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PLUS Comparator inputs - Input plus (input non-inverting) selection
  * @{
  */
#define LL_COMP_INPUT_PLUS_IO1          ((uint32_t)0x00000000U)                            /*!< Comparator input plus connected to IO1 (pin PA1 for COMP1, pin PA3 for COMP2) */
#define LL_COMP_INPUT_PLUS_IO2          (COMP_CSR_COMP2INPSEL_0)                           /*!< Comparator input plus connected to IO2 (pin PB4 for COMP2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_PLUS_IO3          (COMP_CSR_COMP2INPSEL_1)                           /*!< Comparator input plus connected to IO3 (pin PA5 for COMP2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_PLUS_IO4          (COMP_CSR_COMP2INPSEL_0 | COMP_CSR_COMP2INPSEL_1)  /*!< Comparator input plus connected to IO4 (pin PB6 for COMP2) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_PLUS_IO5          (COMP_CSR_COMP2INPSEL_2)                           /*!< Comparator input plus connected to IO5 (pin PB7 for COMP2) (specific to COMP instance: COMP2) */
#if defined (STM32L011xx) || defined (STM32L021xx)
#define LL_COMP_INPUT_PLUS_IO6          (COMP_CSR_COMP2INPSEL_2 | COMP_CSR_COMP2INPSEL_0)  /*!< Comparator input plus connected to IO6 (pin PA7 for COMP2) (specific to COMP instance: COMP2) (Available only on devices STM32L0 category 1) */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_MINUS Comparator inputs - Input minus (input inverting) selection
  * @{
  */
#define LL_COMP_INPUT_MINUS_1_4VREFINT  (COMP_CSR_COMP2INNSEL_2                                                  ) /*!< Comparator input minus connected to 1/4 VrefInt (specifity of COMP2 related to path to enable via SYSCFG: refer to comment in function @ref LL_COMP_SetInputMinus() ) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_1_2VREFINT  (COMP_CSR_COMP2INNSEL_2 |                          COMP_CSR_COMP2INNSEL_0) /*!< Comparator input minus connected to 1/2 VrefInt (specifity of COMP2 related to path to enable via SYSCFG: refer to comment in function @ref LL_COMP_SetInputMinus() ) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_3_4VREFINT  (COMP_CSR_COMP2INNSEL_2 | COMP_CSR_COMP2INNSEL_1                         ) /*!< Comparator input minus connected to 3/4 VrefInt (specifity of COMP2 related to path to enable via SYSCFG: refer to comment in function @ref LL_COMP_SetInputMinus() ) (specific to COMP instance: COMP2) */
#define LL_COMP_INPUT_MINUS_VREFINT     ((uint32_t)0x00000000U)                                                    /*!< Comparator input minus connected to VrefInt (specifity of COMP2 related to path to enable via SYSCFG: refer to comment in function @ref LL_COMP_SetInputMinus() ) */
#define LL_COMP_INPUT_MINUS_DAC1_CH1    (                         COMP_CSR_COMP2INNSEL_1                         ) /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1)  */
#define LL_COMP_INPUT_MINUS_DAC1_CH2    (                         COMP_CSR_COMP2INNSEL_1 | COMP_CSR_COMP2INNSEL_0) /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2)  */
#define LL_COMP_INPUT_MINUS_IO1         (                                                  COMP_CSR_COMP2INNSEL_0) /*!< Comparator input minus connected to IO1 (pin PA0 for COMP1, pin PA2 for COMP2) */
#define LL_COMP_INPUT_MINUS_IO2         (COMP_CSR_COMP2INNSEL_2 | COMP_CSR_COMP2INNSEL_1 | COMP_CSR_COMP2INNSEL_0) /*!< Comparator input minus connected to IO2 (pin PB3 for COMP2) (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_SELECTION_LPTIM Comparator output - Output selection specific to LPTIM peripheral
  * @{
  */
#define LL_COMP_OUTPUT_LPTIM1_IN1_COMP1         (COMP_CSR_COMP1LPTIM1IN1)                                                   /*!< COMP output connected to TIM2 input capture 4 */
#define LL_COMP_OUTPUT_LPTIM1_IN1_COMP2         (COMP_CSR_COMP2LPTIM1IN1)                                                   /*!< COMP output connected to TIM2 input capture 4 */
#define LL_COMP_OUTPUT_LPTIM1_IN2_COMP2         (COMP_CSR_COMP2LPTIM1IN2)                                                   /*!< COMP output connected to TIM2 input capture 4 */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_POLARITY Comparator output - Output polarity
  * @{
  */
#define LL_COMP_OUTPUTPOL_NONINVERTED   ((uint32_t)0x00000000U)  /*!< COMP output polarity is not inverted: comparator output is high when the plus (non-inverting) input is at a higher voltage than the minus (inverting) input */
#define LL_COMP_OUTPUTPOL_INVERTED      (COMP_CSR_COMP1POLARITY) /*!< COMP output polarity is inverted: comparator output is low when the plus (non-inverting) input is at a lower voltage than the minus (inverting) input */

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
#define LL_COMP_DELAY_STARTUP_US          ((uint32_t) 25U)  /*!< Delay for COMP startup time */

/* Delay for comparator voltage scaler stabilization time                     */
/* (voltage from VrefInt, delay based on VrefInt startup time).               */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "TVREFINT").                                                     */
/* Unit: us                                                                   */
#define LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US ((uint32_t)3000U)  /*!< Delay for COMP voltage scaler stabilization time */

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
  * @rmtoll COMP1_CSR   COMP1WM         LL_COMP_SetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowMode)
{
  MODIFY_REG(COMPxy_COMMON->CSR, COMP_CSR_COMP1WM, WindowMode);
}

/**
  * @brief  Get window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll COMP1_CSR   COMP1WM         LL_COMP_GetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON)
{
  return (uint32_t)(READ_BIT(COMPxy_COMMON->CSR, COMP_CSR_COMP1WM));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_modes Configuration of comparator modes
  * @{
  */

/**
  * @brief  Set comparator instance operating mode to adjust power and speed.
  * @rmtoll COMP2_CSR   COMP2SPEED      LL_COMP_SetPowerMode
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
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMP2SPEED, PowerMode);
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @note  Available only on COMP instance: COMP2.
  * @rmtoll COMP2_CSR   COMP2SPEED      LL_COMP_GetPowerMode\n
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED   (1)
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER (1)
  *         
  *         (1) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetPowerMode(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMP2SPEED));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_inputs Configuration of comparator inputs
  * @{
  */

/**
  * @brief  Set comparator inputs minus (inverting) and plus (non-inverting).
  * @note   This function shall only be used for COMP2.
  *         For setting COMP1 input it is recommended to use LL_COMP_SetInputMinus()
  *         Plus (non-inverting) input is not configurable on COMP1.
  *         Using this function for COMP1 will corrupt COMP1WM register
  * @note   On this STM32 serie, specificity if using COMP instance COMP2
  *         with COMP input based on VrefInt (VrefInt or subdivision
  *         of VrefInt): scaler bridge is based on VrefInt and requires
  *         to enable path from VrefInt (refer to literal
  *         SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP).
  * @rmtoll COMP2_CSR   COMP2INNSEL     LL_COMP_ConfigInputs\n
  *         COMP2_CSR   COMP2INPSEL     LL_COMP_ConfigInputs
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO4 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO5 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO6 (1)(2)
  *
  *         (1) Available only on COMP instance: COMP2.
  *         (2) Available only on devices STM32L0 category 1.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_ConfigInputs(COMP_TypeDef *COMPx, uint32_t InputMinus, uint32_t InputPlus)
{
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_COMP2INNSEL | COMP_CSR_COMP2INPSEL,
             InputMinus | InputPlus);
}

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   Only COMP2 allows to set the input plus (non-inverting).
  *         For COMP1 it is always PA1 IO, except when Windows Mode is selected.
  * @rmtoll COMP2_CSR   COMP2INPSEL     LL_COMP_SetInputPlus
  * @param  COMPx Comparator instance
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO4 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO5 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO6 (1)(2)
  *
  *         (1) Available only on COMP instance: COMP2.
  *         (2) Available only on devices STM32L0 category 1.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPlus(COMP_TypeDef *COMPx, uint32_t InputPlus)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMP2INPSEL, InputPlus);
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   Only COMP2 allows to set the input plus (non-inverting).
  *         For COMP1 it is always PA1 IO, except when Windows Mode is selected.
  * @rmtoll COMP2_CSR   COMP2INPSEL     LL_COMP_GetInputPlus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO4 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO5 (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_IO6 (1)(2)
  *
  *         (1) Available only on COMP instance: COMP2.
  *         (2) Available only on devices STM32L0 category 1.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPlus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMP2INPSEL));
}

/**
  * @brief  Set comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @note   On this STM32 serie, specificity if using COMP instance COMP2
  *         with COMP input based on VrefInt (VrefInt or subdivision
  *         of VrefInt): scaler bridge is based on VrefInt and requires
  *         to enable path from VrefInt (refer to literal
  *         SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP).
  * @rmtoll COMP1_CSR   COMP1INNSEL     LL_COMP_SetInputMinus\n
  *         COMP2_CSR   COMP2INNSEL     LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2        (*)
  *         
  *         (*) Available only on COMP instance: COMP2.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMP2INNSEL, InputMinus);
}

/**
  * @brief  Get comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll COMP1_CSR   COMP1INNSEL     LL_COMP_GetInputMinus\n
  *         COMP2_CSR   COMP2INNSEL     LL_COMP_GetInputMinus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT (*)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2        (*)
  *         
  *         (*) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputMinus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMP2INNSEL));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_output Configuration of comparator output
  * @{
  */

/**
  * @brief  Set comparator output LPTIM.
  * @rmtoll COMP1_CSR   COMP1LPTIMIN1   LL_COMP_SetOutputLPTIM\n
  *         COMP2_CSR   COMP2LPTIMIN1   LL_COMP_SetOutputLPTIM\n
  *         COMP2_CSR   COMP2LPTIMIN2   LL_COMP_SetOutputLPTIM
  * @param  COMPx Comparator instance
  * @param  OutputLptim This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN1_COMP1 (*)
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN1_COMP2 (**)
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN2_COMP2 (**)
  *         
  *         (*)  Available only on COMP instance: COMP1.\n
  *         (**) Available only on COMP instance: COMP2.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputLPTIM(COMP_TypeDef *COMPx, uint32_t OutputLptim)
{
  MODIFY_REG(COMPx->CSR, (COMP_CSR_COMP1LPTIM1IN1 | COMP_CSR_COMP2LPTIM1IN1 | COMP_CSR_COMP2LPTIM1IN2), OutputLptim);
}

/**
  * @brief  Get comparator output LPTIM.
  * @rmtoll COMP1_CSR   COMP1LPTIMIN1   LL_COMP_GetOutputLPTIM\n
  *         COMP2_CSR   COMP2LPTIMIN1   LL_COMP_GetOutputLPTIM\n
  *         COMP2_CSR   COMP2LPTIMIN2   LL_COMP_GetOutputLPTIM
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN1_COMP1 (*)
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN1_COMP2 (**)
  *         @arg @ref LL_COMP_OUTPUT_LPTIM1_IN2_COMP2 (**)
  *         
  *         (*)  Available only on COMP instance: COMP1.\n
  *         (**) Available only on COMP instance: COMP2.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputLPTIM(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, (COMP_CSR_COMP1LPTIM1IN1 | COMP_CSR_COMP2LPTIM1IN1 | COMP_CSR_COMP2LPTIM1IN2)));
}

/**
  * @brief  Set comparator instance output polarity.
  * @rmtoll COMP     COMP1POLARITY  LL_COMP_SetOutputPolarity
  * @param  COMPx Comparator instance
  * @param  OutputPolarity This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputPolarity(COMP_TypeDef *COMPx, uint32_t OutputPolarity)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxPOLARITY, OutputPolarity);
}

/**
  * @brief  Get comparator instance output polarity.
  * @rmtoll COMP     COMP1POLARITY  LL_COMP_GetOutputPolarity
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputPolarity(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxPOLARITY));
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
  * @rmtoll COMP1_CSR   COMP1EN         LL_COMP_Enable\n
  *         COMP2_CSR   COMP2EN         LL_COMP_Enable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  SET_BIT(COMPx->CSR, COMP_CSR_COMPxEN);
}

/**
  * @brief  Disable comparator instance.
  * @rmtoll COMP1_CSR   COMP1EN         LL_COMP_Disable\n
  *         COMP2_CSR   COMP2EN         LL_COMP_Disable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Disable(COMP_TypeDef *COMPx)
{
  CLEAR_BIT(COMPx->CSR, COMP_CSR_COMPxEN);
}

/**
  * @brief  Get comparator enable state
  *         (0: COMP is disabled, 1: COMP is enabled)
  * @rmtoll COMP1_CSR   COMP1EN         LL_COMP_IsEnabled\n
  *         COMP2_CSR   COMP2EN         LL_COMP_IsEnabled
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsEnabled(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMPx->CSR, COMP_CSR_COMPxEN) == (COMP_CSR_COMPxEN));
}

/**
  * @brief  Lock comparator instance.
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll COMP1_CSR   COMP1LOCK       LL_COMP_Lock\n
  *         COMP2_CSR   COMP2LOCK       LL_COMP_Lock
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Lock(COMP_TypeDef *COMPx)
{
  SET_BIT(COMPx->CSR, COMP_CSR_COMPxLOCK);
}

/**
  * @brief  Get comparator lock state
  *         (0: COMP is unlocked, 1: COMP is locked).
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll COMP1_CSR   COMP1LOCK       LL_COMP_IsLocked\n
  *         COMP2_CSR   COMP2LOCK       LL_COMP_IsLocked
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsLocked(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMPx->CSR, COMP_CSR_COMPxLOCK) == (COMP_CSR_COMPxLOCK));
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
  * @rmtoll COMP1_CSR   COMP1VALUE      LL_COMP_ReadOutputLevel\n
  *         COMP2_CSR   COMP2VALUE      LL_COMP_ReadOutputLevel
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_ReadOutputLevel(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxOUTVALUE)
                    >> LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS);
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

#endif /* __STM32L0xx_LL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
