/**
  ******************************************************************************
  * @file    stm32f3xx_ll_opamp.h
  * @author  MCD Application Team
  * @brief   Header file of OPAMP LL module.
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
#ifndef __STM32F3xx_LL_OPAMP_H
#define __STM32F3xx_LL_OPAMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

#if defined (OPAMP1) || defined (OPAMP2) || defined (OPAMP3) || defined (OPAMP4)

/** @defgroup OPAMP_LL OPAMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup OPAMP_LL_Private_Constants OPAMP Private Constants
  * @{
  */

/* Internal mask for OPAMP trimming of transistors differential pair NMOS     */
/* or PMOS.                                                                   */
/* To select into literal LL_OPAMP_TRIMMING_x the relevant bits for:          */
/* - OPAMP trimming selection of transistors differential pair                */
/* - OPAMP trimming values of transistors differential pair                   */
#define OPAMP_TRIMMING_SELECT_MASK          (OPAMP_CSR_CALSEL)
#define OPAMP_TRIMMING_VALUE_MASK           (OPAMP_CSR_TRIMOFFSETN | OPAMP_CSR_TRIMOFFSETP)

/**
  * @}
  */


/* Private macros ------------------------------------------------------------*/
/** @defgroup OPAMP_LL_Private_Macros OPAMP Private Macros
  * @{
  */

/**
  * @brief  Driver macro reserved for internal use: set a pointer to
  *         a register from a register basis from which an offset
  *         is applied.
  * @param  __REG__ Register basis from which the offset is applied.
  * @param  __REG_OFFSET__ Offset to be applied (unit: number of registers).
  * @retval Register address
*/
#define __OPAMP_PTR_REG_OFFSET(__REG__, __REG_OFFSET__)                        \
 ((uint32_t *)((uint32_t) ((uint32_t)(&(__REG__)) + ((__REG_OFFSET__) << 2U))))




/**
  * @}
  */


/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup OPAMP_LL_ES_INIT OPAMP Exported Init structure
  * @{
  */

/**
  * @brief  Structure definition of some features of OPAMP instance.
  */
typedef struct
{
  uint32_t FunctionalMode;              /*!< Set OPAMP functional mode by setting internal connections: OPAMP operation in standalone, follower, ...
                                             This parameter can be a value of @ref OPAMP_LL_EC_FUNCTIONAL_MODE
                                             @note If OPAMP is configured in mode PGA, the gain can be configured using function @ref LL_OPAMP_SetPGAGain().

                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetFunctionalMode(). */

  uint32_t InputNonInverting;           /*!< Set OPAMP input non-inverting connection.
                                             This parameter can be a value of @ref OPAMP_LL_EC_INPUT_NONINVERTING

                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetInputNonInverting(). */

  uint32_t InputInverting;              /*!< Set OPAMP inverting input connection.
                                             This parameter can be a value of @ref OPAMP_LL_EC_INPUT_INVERTING
                                             @note OPAMP inverting input is used with OPAMP in mode standalone or PGA with external capacitors for filtering circuit. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin), this parameter is discarded.

                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetInputInverting(). */

} LL_OPAMP_InitTypeDef;

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup OPAMP_LL_Exported_Constants OPAMP Exported Constants
  * @{
  */

/** @defgroup OPAMP_LL_EC_MODE OPAMP mode calibration or functional.
  * @{
  */
#define LL_OPAMP_MODE_FUNCTIONAL        ((uint32_t)0x00000000U)                     /*!< OPAMP functional mode */
#define LL_OPAMP_MODE_CALIBRATION       (OPAMP_CSR_CALON)                           /*!< OPAMP calibration mode */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_FUNCTIONAL_MODE OPAMP functional mode
  * @{
  */
#define LL_OPAMP_MODE_STANDALONE        ((uint32_t)0x00000000U)                     /*!< OPAMP functional mode, OPAMP operation in standalone */
#define LL_OPAMP_MODE_FOLLOWER          (OPAMP_CSR_VMSEL_1 | OPAMP_CSR_VMSEL_0)     /*!< OPAMP functional mode, OPAMP operation in follower */
#define LL_OPAMP_MODE_PGA               (OPAMP_CSR_VMSEL_1)                         /*!< OPAMP functional mode, OPAMP operation in PGA */
#define LL_OPAMP_MODE_PGA_EXT_FILT_IO0  (OPAMP_CSR_PGGAIN_3                      | OPAMP_CSR_VMSEL_1) /*!< OPAMP functional mode, OPAMP operation in PGA with external filtering on OPAMP input IO0. */
#define LL_OPAMP_MODE_PGA_EXT_FILT_IO1  (OPAMP_CSR_PGGAIN_3 | OPAMP_CSR_PGGAIN_2 | OPAMP_CSR_VMSEL_1) /*!< OPAMP functional mode, OPAMP operation in PGA with external filtering on OPAMP input IO1. */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_MODE_PGA_GAIN OPAMP PGA gain (relevant when OPAMP is in functional mode PGA)
  * @{
  */
#define LL_OPAMP_PGA_GAIN_2             ((uint32_t)0x00000000U)                    /*!< OPAMP PGA gain 2 */
#define LL_OPAMP_PGA_GAIN_4             (OPAMP_CSR_PGGAIN_0)                       /*!< OPAMP PGA gain 4 */
#define LL_OPAMP_PGA_GAIN_8             (OPAMP_CSR_PGGAIN_1)                       /*!< OPAMP PGA gain 8 */
#define LL_OPAMP_PGA_GAIN_16            (OPAMP_CSR_PGGAIN_1 | OPAMP_CSR_PGGAIN_0 ) /*!< OPAMP PGA gain 16 */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_NONINVERTING OPAMP input non-inverting
  * @{
  */
#define LL_OPAMP_INPUT_NONINVERT_IO0      (OPAMP_CSR_VPSEL)       /*!< OPAMP non inverting input connected to GPIO pin (pin PA1 for OPAMP1, pin PA7  for OPAMP2, pin PB0  for OPAMP3, pin PB13 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO1      ((uint32_t)0x00000000)  /*!< OPAMP non inverting input connected to GPIO pin (pin PA7 for OPAMP1, pin PD14 for OPAMP2, pin PB13 for OPAMP3, pin PD11 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO2      (OPAMP_CSR_VPSEL_1)     /*!< OPAMP non inverting input connected to GPIO pin (pin PA3 for OPAMP1, pin PB0  for OPAMP2, pin PA1  for OPAMP3, pin PB11 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO3      (OPAMP_CSR_VPSEL_0)     /*!< OPAMP non inverting input connected to GPIO pin (pin PA5 for OPAMP1, pin PB14 for OPAMP2, pin PA5  for OPAMP3, pin PA4  for OPAMP4) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH1    (LL_OPAMP_INPUT_NONINVERT_IO3) /*!< OPAMP non inverting input connected to DAC1 channel1 output (specific to OPAMP instances: OPAMP4) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH2    (LL_OPAMP_INPUT_NONINVERT_IO3) /*!< OPAMP non inverting input connected to DAC1 channel2 output (specific to OPAMP instances: OPAMP1, OPAMP3) */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_INVERTING OPAMP input inverting
  * @{
  */
#define LL_OPAMP_INPUT_INVERT_IO0        ((uint32_t)0x00000000U) /*!< OPAMP inverting input connected to GPIO pin (pin PC5 for OPAMP1, pin PC5 for OPAMP2, pin PB10  for OPAMP3, pin PB10 for OPAMP4). Note: OPAMP inverting input is used with OPAMP in mode standalone or PGA with external capacitors for filtering circuit. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
#define LL_OPAMP_INPUT_INVERT_IO1        (OPAMP_CSR_VMSEL_0)     /*!< OPAMP inverting input connected to GPIO pin (pin PA3 for OPAMP1, pin PA5 for OPAMP2, pin PB2   for OPAMP3, pin PD8  for OPAMP4). Note: OPAMP inverting input is used with OPAMP in mode standalone or PGA with external capacitors for filtering circuit. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
#define LL_OPAMP_INPUT_INVERT_CONNECT_NO (OPAMP_CSR_VMSEL_1)     /*!< OPAMP inverting input not externally connected (intended for OPAMP in mode follower or PGA without external capacitors for filtering). Note: On this STM32 serie, this literal include cases of value 0x11 for mode follower and value 0x10 for mode PGA. */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_NONINVERTING_SECONDARY OPAMP input non-inverting secondary
  * @{
  */
#define LL_OPAMP_INPUT_NONINVERT_IO0_SEC   (LL_OPAMP_INPUT_NONINVERT_IO0   << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to GPIO pin (pin PA1 for OPAMP1, pin PA7  for OPAMP2, pin PB0  for OPAMP3, pin PB13 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO1_SEC   (LL_OPAMP_INPUT_NONINVERT_IO1   << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to GPIO pin (pin PA7 for OPAMP1, pin PD14 for OPAMP2, pin PB13 for OPAMP3, pin PD11 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO2_SEC   (LL_OPAMP_INPUT_NONINVERT_IO2   << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to GPIO pin (pin PA3 for OPAMP1, pin PB0  for OPAMP2, pin PA1  for OPAMP3, pin PB11 for OPAMP4) */
#define LL_OPAMP_INPUT_NONINVERT_IO3_SEC   (LL_OPAMP_INPUT_NONINVERT_IO3   << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to GPIO pin (pin PA5 for OPAMP1, pin PD14 for OPAMP2, pin PA5  for OPAMP3, pin PA4  for OPAMP4) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH1_SEC (LL_OPAMP_INPUT_NONINV_DAC1_CH1 << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to DAC1 channel1 output (specific to OPAMP instances: OPAMP4) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH2_SEC (LL_OPAMP_INPUT_NONINV_DAC1_CH2 << (OPAMP_CSR_VPSSEL_Pos - OPAMP_CSR_VPSEL_Pos)) /*!< OPAMP non inverting input secondary connected to DAC1 channel2 output (specific to OPAMP instances: OPAMP1, OPAMP3) */

/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_INVERTING_SECONDARY OPAMP input inverting secondary
  * @{
  */
#define LL_OPAMP_INPUT_INVERT_IO0_SEC    (LL_OPAMP_INPUT_INVERT_IO0 << (OPAMP_CSR_VMSSEL_Pos - OPAMP_CSR_VMSEL_Pos)) /*!< OPAMP inverting input secondary connected to GPIO pin (pin PC5 for OPAMP1, pin PC5 for OPAMP2, pin PB10  for OPAMP3, pin PB10 for OPAMP4). Note: OPAMP inverting input is used with OPAMP in mode standalone or PGA with external capacitors for filtering circuit. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
#define LL_OPAMP_INPUT_INVERT_IO1_SEC    (LL_OPAMP_INPUT_INVERT_IO1 << (OPAMP_CSR_VMSSEL_Pos - OPAMP_CSR_VMSEL_Pos)) /*!< OPAMP inverting input secondary connected to GPIO pin (pin PA3 for OPAMP1, pin PA5 for OPAMP2, pin PB2   for OPAMP3, pin PD8  for OPAMP4). Note: OPAMP inverting input is used with OPAMP in mode standalone or PGA with external capacitors for filtering circuit. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_MUX_MODE OPAMP inputs multiplexer mode
  * @{
  */
#define LL_OPAMP_INPUT_MUX_DISABLE       ((uint32_t)0x00000000U)  /*!< OPAMP inputs multiplexer mode dosabled. */
#define LL_OPAMP_INPUT_MUX_TIM1_CH6    (OPAMP_CSR_TCMEN)        /*!< OPAMP inputs multiplexer mode enabled, controlled by TIM1 CC6. */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_VREF_OUTPUT OPAMP internal reference voltage path state to output
  * @{
  */
#define LL_OPAMP_VREF_OUTPUT_DISABLE    ((uint32_t)0x00000000U)  /*!< OPAMP internal reference voltage path to output is disabled. */
#define LL_OPAMP_VREF_OUTPUT_ENABLE     (OPAMP_CSR_TSTREF)       /*!< OPAMP internal reference voltage path to output is enabled. */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_TRIMMING_MODE OPAMP trimming mode
  * @{
  */
#define LL_OPAMP_TRIMMING_FACTORY       ((uint32_t)0x00000000U) /*!< OPAMP trimming factors set to factory values */
#define LL_OPAMP_TRIMMING_USER          (OPAMP_CSR_USERTRIM)    /*!< OPAMP trimming factors set to user values */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_TRIMMING_TRANSISTORS_DIFF_PAIR OPAMP trimming of transistors differential pair NMOS or PMOS
  * @{
  */
#define LL_OPAMP_TRIMMING_NMOS_VREF_90PC_VDDA  (OPAMP_CSR_TRIMOFFSETN | OPAMP_CSR_CALSEL_1 | OPAMP_CSR_CALSEL_0) /*!< OPAMP trimming of transistors differential pair NMOS (internal reference voltage set to 0.9*Vdda). Default parameters to be used for calibration using two trimming steps (one with each transistors differential pair NMOS and PMOS). */
#define LL_OPAMP_TRIMMING_NMOS_VREF_50PC_VDDA  (OPAMP_CSR_TRIMOFFSETN | OPAMP_CSR_CALSEL_1                     ) /*!< OPAMP trimming of transistors differential pair NMOS (internal reference voltage set to 0.5*Vdda). */
#define LL_OPAMP_TRIMMING_PMOS_VREF_10PC_VDDA  (OPAMP_CSR_TRIMOFFSETP                      | OPAMP_CSR_CALSEL_0) /*!< OPAMP trimming of transistors differential pair PMOS (internal reference voltage set to 0.1*Vdda). Default parameters to be used for calibration using two trimming steps (one with each transistors differential pair NMOS and PMOS). */
#define LL_OPAMP_TRIMMING_PMOS_VREF_3_3PC_VDDA (OPAMP_CSR_TRIMOFFSETP                                          ) /*!< OPAMP trimming of transistors differential pair PMOS (internal reference voltage set to 0.33*Vdda). */
#define LL_OPAMP_TRIMMING_NMOS          (LL_OPAMP_TRIMMING_NMOS_VREF_90PC_VDDA) /*!< OPAMP trimming of transistors differential pair NMOS (internal reference voltage set to 0.9*Vdda). Default parameters to be used for calibration using two trimming steps (one with each transistors differential pair NMOS and PMOS). */
#define LL_OPAMP_TRIMMING_PMOS          (LL_OPAMP_TRIMMING_PMOS_VREF_10PC_VDDA) /*!< OPAMP trimming of transistors differential pair PMOS (internal reference voltage set to 0.1*Vdda). Default parameters to be used for calibration using two trimming steps (one with each transistors differential pair NMOS and PMOS). */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_HW_DELAYS  Definitions of OPAMP hardware constraints delays
  * @note   Only OPAMP IP HW delays are defined in OPAMP LL driver driver,
  *         not timeout values.
  *         For details on delays values, refer to descriptions in source code
  *         above each literal definition.
  * @{
  */

/* Delay for OPAMP startup time (transition from state disable to enable).    */
/* Note: OPAMP startup time depends on board application environment:         */
/*       impedance connected to OPAMP output.                                 */
/*       The delay below is specified under conditions:                       */
/*        - OPAMP in functional mode follower                                 */
/*        - load impedance of 4kOhm (min), 50pF (max)                         */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tWAKEUP").                                                      */
/* Unit: us                                                                   */
#define LL_OPAMP_DELAY_STARTUP_US         ((uint32_t)  5U)  /*!< Delay for OPAMP startup time */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup OPAMP_LL_Exported_Macros OPAMP Exported Macros
  * @{
  */
/** @defgroup OPAMP_LL_EM_WRITE_READ Common write and read registers macro
  * @{
  */
/**
  * @brief  Write a value in OPAMP register
  * @param  __INSTANCE__ OPAMP Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_OPAMP_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in OPAMP register
  * @param  __INSTANCE__ OPAMP Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_OPAMP_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup OPAMP_LL_Exported_Functions OPAMP Exported Functions
  * @{
  */

/** @defgroup OPAMP_LL_EF_CONFIGURATION_OPAMP_INSTANCE Configuration of OPAMP hierarchical scope: OPAMP instance
  * @{
  */

/**
  * @brief  Set OPAMP mode calibration or functional.
  * @note   OPAMP mode corresponds to functional or calibration mode:
  *          - functional mode: OPAMP operation in standalone, follower, ...
  *            Set functional mode using function
  *            @ref LL_OPAMP_SetFunctionalMode().
  *          - calibration mode: offset calibration of the selected
  *            transistors differential pair NMOS or PMOS.
  * @rmtoll CSR      CALON          LL_OPAMP_SetMode
  * @param  OPAMPx OPAMP instance
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_FUNCTIONAL
  *         @arg @ref LL_OPAMP_MODE_CALIBRATION
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetMode(OPAMP_TypeDef *OPAMPx, uint32_t Mode)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_CALON, Mode);
}

/**
  * @brief  Get OPAMP mode calibration or functional.
  * @note   OPAMP mode corresponds to functional or calibration mode:
  *          - functional mode: OPAMP operation in standalone, follower, ...
  *            Set functional mode using function
  *            @ref LL_OPAMP_SetFunctionalMode().
  *          - calibration mode: offset calibration of the selected
  *            transistors differential pair NMOS or PMOS.
  * @rmtoll CSR      CALON          LL_OPAMP_GetMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_FUNCTIONAL
  *         @arg @ref LL_OPAMP_MODE_CALIBRATION
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_CALON));
}

/**
  * @brief  Set OPAMP functional mode by setting internal connections.
  *         OPAMP operation in standalone, follower, ...
  * @note   This function reset bit of calibration mode to ensure
  *         to be in functional mode, in order to have OPAMP parameters
  *         (inputs selection, ...) set with the corresponding OPAMP mode
  *         to be effective.
  * @rmtoll CSR      VMSEL          LL_OPAMP_SetFunctionalMode
  * @param  OPAMPx OPAMP instance
  * @param  FunctionalMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_STANDALONE
  *         @arg @ref LL_OPAMP_MODE_FOLLOWER
  *         @arg @ref LL_OPAMP_MODE_PGA
  *         @arg @ref LL_OPAMP_MODE_PGA_EXT_FILT_IO0
  *         @arg @ref LL_OPAMP_MODE_PGA_EXT_FILT_IO1
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetFunctionalMode(OPAMP_TypeDef *OPAMPx, uint32_t FunctionalMode)
{
  /* Note: Bit OPAMP_CSR_CALON reset to ensure to be in functional mode */
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_PGGAIN_3 | OPAMP_CSR_PGGAIN_2 | OPAMP_CSR_VMSEL | OPAMP_CSR_CALON, FunctionalMode);
}

/**
  * @brief  Get OPAMP functional mode from setting of internal connections.
  *         OPAMP operation in standalone, follower, ...
  * @rmtoll CSR      VMSEL          LL_OPAMP_GetFunctionalMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_STANDALONE
  *         @arg @ref LL_OPAMP_MODE_FOLLOWER
  *         @arg @ref LL_OPAMP_MODE_PGA
  *         @arg @ref LL_OPAMP_MODE_PGA_EXT_FILT_IO0
  *         @arg @ref LL_OPAMP_MODE_PGA_EXT_FILT_IO1
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetFunctionalMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_PGGAIN_3 | OPAMP_CSR_PGGAIN_2 | OPAMP_CSR_VMSEL));
}

/**
  * @brief  Set OPAMP PGA gain.
  * @note   Preliminarily, OPAMP must be set in mode PGA
  *         using function @ref LL_OPAMP_SetFunctionalMode().
  * @rmtoll CSR      PGGAIN         LL_OPAMP_SetPGAGain
  * @param  OPAMPx OPAMP instance
  * @param  PGAGain This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_PGA_GAIN_2
  *         @arg @ref LL_OPAMP_PGA_GAIN_4
  *         @arg @ref LL_OPAMP_PGA_GAIN_8
  *         @arg @ref LL_OPAMP_PGA_GAIN_16
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetPGAGain(OPAMP_TypeDef *OPAMPx, uint32_t PGAGain)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_PGGAIN_1 | OPAMP_CSR_PGGAIN_0, PGAGain);
}

/**
  * @brief  Get OPAMP PGA gain.
  * @note   Preliminarily, OPAMP must be set in mode PGA
  *         using function @ref LL_OPAMP_SetFunctionalMode().
  * @rmtoll CSR      PGGAIN         LL_OPAMP_GetPGAGain
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_PGA_GAIN_2
  *         @arg @ref LL_OPAMP_PGA_GAIN_4
  *         @arg @ref LL_OPAMP_PGA_GAIN_8
  *         @arg @ref LL_OPAMP_PGA_GAIN_16
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetPGAGain(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_PGGAIN_1 | OPAMP_CSR_PGGAIN_0));
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_CONFIGURATION_INPUTS Configuration of OPAMP inputs
  * @{
  */

/**
  * @brief  Set OPAMP non-inverting input connection.
  * @rmtoll CSR      VPSEL          LL_OPAMP_SetInputNonInverting
  * @param  OPAMPx OPAMP instance
  * @param  InputNonInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO1
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO2
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO3
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1 (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2 (2)
  *
  *         (1) Parameter specific to OPAMP instances: OPAMP4.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP1, OPAMP3.
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputNonInverting(OPAMP_TypeDef *OPAMPx, uint32_t InputNonInverting)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_VPSEL, InputNonInverting);
}

/**
  * @brief  Get OPAMP non-inverting input connection.
  * @rmtoll CSR      VPSEL          LL_OPAMP_GetInputNonInverting
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO1
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO2
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO3
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1 (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2 (2)
  *
  *         (1) Parameter specific to OPAMP instances: OPAMP4.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP1, OPAMP3.
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputNonInverting(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_VPSEL));
}

/**
  * @brief  Set OPAMP inverting input connection.
  * @note   OPAMP inverting input is used with OPAMP in mode standalone
  *         or PGA with external capacitors for filtering circuit.
  *         Otherwise (OPAMP in mode follower), OPAMP inverting input
  *         is not used (not connected to GPIO pin).
  * @rmtoll CSR      VMSEL          LL_OPAMP_SetInputInverting
  * @param  OPAMPx OPAMP instance
  * @param  InputInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1
  *         @arg @ref LL_OPAMP_INPUT_INVERT_CONNECT_NO
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputInverting(OPAMP_TypeDef *OPAMPx, uint32_t InputInverting)
{
  /* Manage cases of OPAMP inverting input not connected (0x10 and 0x11)      */
  /* to not modify OPAMP mode follower or PGA.                                */
  /* Bit OPAMP_CSR_VMSEL_1 is set by OPAMP mode (follower, PGA). */
  MODIFY_REG(OPAMPx->CSR, (~(InputInverting >> 1)) & OPAMP_CSR_VMSEL_0, InputInverting);
}

/**
  * @brief  Get OPAMP inverting input connection.
  * @rmtoll CSR      VMSEL          LL_OPAMP_GetInputInverting
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1
  *         @arg @ref LL_OPAMP_INPUT_INVERT_CONNECT_NO
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputInverting(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t input_inverting = READ_BIT(OPAMPx->CSR, OPAMP_CSR_VMSEL);

  /* Manage cases 0x10 and 0x11 to return the same value: OPAMP inverting     */
  /* input not connected.                                                     */
  return (input_inverting & ~((input_inverting >> 1) & OPAMP_CSR_VMSEL_0));
}

/**
  * @brief  Set OPAMP non-inverting input secondary connection.
  * @rmtoll CSR      VPSSEL         LL_OPAMP_SetInputNonInvertingSecondary
  * @param  OPAMPx OPAMP instance
  * @param  InputNonInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO1_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO2_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO3_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1_SEC (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2_SEC (2)
  *
  *         (1) Parameter specific to OPAMP instances: OPAMP4.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP1, OPAMP3.
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputNonInvertingSecondary(OPAMP_TypeDef *OPAMPx, uint32_t InputNonInverting)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_VPSSEL, InputNonInverting);
}

/**
  * @brief  Get OPAMP non-inverting input secondary connection.
  * @rmtoll CSR      VPSSEL         LL_OPAMP_GetInputNonInvertingSecondary
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO1_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO2_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO3_SEC
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1_SEC (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2_SEC (2)
  *
  *         (1) Parameter specific to OPAMP instances: OPAMP4.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP1, OPAMP3.
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputNonInvertingSecondary(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_VPSSEL));
}

/**
  * @brief  Set OPAMP inverting input secondary connection.
  * @note   OPAMP inverting input is used with OPAMP in mode standalone
  *         or PGA with external capacitors for filtering circuit.
  *         Otherwise (OPAMP in mode follower), OPAMP inverting input
  *         is not used (not connected to GPIO pin).
  * @rmtoll CSR      VMSSEL         LL_OPAMP_SetInputInvertingSecondary
  * @param  OPAMPx OPAMP instance
  * @param  InputInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0_SEC
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1_SEC
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputInvertingSecondary (OPAMP_TypeDef *OPAMPx, uint32_t InputInverting)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_VMSSEL, InputInverting);
}

/**
  * @brief  Get OPAMP inverting input secondary connection.
  * @rmtoll CSR      VMSSEL         LL_OPAMP_GetInputInvertingSecondary
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0_SEC
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1_SEC
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputInvertingSecondary(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_VMSSEL));
}

/**
  * @brief  Set OPAMP inputs multiplexer mode.
  * @rmtoll CSR      TCMEN          LL_OPAMP_SetInputsMuxMode
  * @param  OPAMPx OPAMP instance
  * @param  InputsMuxMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_MUX_DISABLE
  *         @arg @ref LL_OPAMP_INPUT_MUX_TIM1_CH6
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputsMuxMode(OPAMP_TypeDef *OPAMPx, uint32_t InputsMuxMode)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_TCMEN, InputsMuxMode);
}

/**
  * @brief  Get OPAMP inputs multiplexer mode.
  * @rmtoll CSR      TCMEN          LL_OPAMP_GetInputsMuxMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_MUX_DISABLE
  *         @arg @ref LL_OPAMP_INPUT_MUX_TIM1_CH6
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputsMuxMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_TCMEN));
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_OPAMP_TRIMMING Configuration and operation of OPAMP trimming
  * @{
  */

/**
  * @brief  Set OPAMP trimming mode.
  * @rmtoll CSR      USERTRIM       LL_OPAMP_SetTrimmingMode
  * @param  OPAMPx OPAMP instance
  * @param  TrimmingMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_FACTORY
  *         @arg @ref LL_OPAMP_TRIMMING_USER
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetTrimmingMode(OPAMP_TypeDef *OPAMPx, uint32_t TrimmingMode)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_USERTRIM, TrimmingMode);
}

/**
  * @brief  Get OPAMP trimming mode.
  * @rmtoll CSR      USERTRIM       LL_OPAMP_GetTrimmingMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_FACTORY
  *         @arg @ref LL_OPAMP_TRIMMING_USER
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetTrimmingMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_USERTRIM));
}

/**
  * @brief  Set OPAMP offset to calibrate the selected transistors
  *         differential pair NMOS or PMOS.
  * @note   Preliminarily, OPAMP must be set in mode calibration
  *         using function @ref LL_OPAMP_SetMode().
  * @rmtoll CSR      CALSEL         LL_OPAMP_SetCalibrationSelection
  * @param  OPAMPx OPAMP instance
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS            (1)
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS            (1)
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS_VREF_50PC_VDDA
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS_VREF_3_3PC_VDDA
  *
  *         (1) Default parameters to be used for calibration
  *             using two trimming steps (one with each transistors differential
  *             pair NMOS and PMOS)
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetCalibrationSelection(OPAMP_TypeDef *OPAMPx, uint32_t TransistorsDiffPair)
{
  /* Parameter used with mask "OPAMP_TRIMMING_SELECT_MASK" because            */
  /* containing other bits reserved for other purpose.                        */
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_CALSEL, (TransistorsDiffPair & OPAMP_TRIMMING_SELECT_MASK));
}

/**
  * @brief  Get OPAMP offset to calibrate the selected transistors
  *         differential pair NMOS or PMOS.
  * @note   Preliminarily, OPAMP must be set in mode calibration
  *         using function @ref LL_OPAMP_SetMode().
  * @rmtoll CSR      CALSEL         LL_OPAMP_GetCalibrationSelection
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS            (1)
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS            (1)
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS_VREF_50PC_VDDA
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS_VREF_3_3PC_VDDA
  *
  *         (1) Default parameters to be used for calibration
  *             using two trimming steps (one with each transistors differential
  *             pair NMOS and PMOS)
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetCalibrationSelection(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t CalibrationSelection = (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_CALSEL));

  return (CalibrationSelection |
          ((OPAMP_CSR_TRIMOFFSETN) << (POSITION_VAL(OPAMP_CSR_TRIMOFFSETP) * (CalibrationSelection && OPAMP_CSR_CALSEL))));
}

/**
  * @brief  Set OPAMP calibration internal reference voltage to output.
  * @rmtoll CSR      TSTREF         LL_OPAMP_SetCalibrationVrefOutput
  * @param  OPAMPx OPAMP instance
  * @param  CalibrationVrefOutput This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_VREF_OUTPUT_DISABLE
  *         @arg @ref LL_OPAMP_VREF_OUTPUT_ENABLE
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetCalibrationVrefOutput(OPAMP_TypeDef *OPAMPx, uint32_t CalibrationVrefOutput)
{
  MODIFY_REG(OPAMPx->CSR, OPAMP_CSR_TSTREF, CalibrationVrefOutput);
}

/**
  * @brief  Get OPAMP calibration internal reference voltage to output.
  * @rmtoll CSR      TSTREF         LL_OPAMP_GetCalibrationVrefOutput
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_VREF_OUTPUT_DISABLE
  *         @arg @ref LL_OPAMP_VREF_OUTPUT_ENABLE
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetCalibrationVrefOutput(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, OPAMP_CSR_TSTREF));
}

/**
  * @brief  Get OPAMP calibration result of toggling output.
  * @note   This functions returns:
  *         0 if OPAMP calibration output is reset
  *         1 if OPAMP calibration output is set
  * @rmtoll CSR      OUTCAL         LL_OPAMP_IsCalibrationOutputSet
  * @param  OPAMPx OPAMP instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_OPAMP_IsCalibrationOutputSet(OPAMP_TypeDef *OPAMPx)
{
  return (READ_BIT(OPAMPx->CSR, OPAMP_CSR_OUTCAL) == OPAMP_CSR_OUTCAL);
}

/**
  * @brief  Set OPAMP trimming factor for the selected transistors
  *         differential pair NMOS or PMOS, corresponding to the selected
  *         power mode.
  * @rmtoll CSR      TRIMOFFSETN    LL_OPAMP_SetTrimmingValue\n
  *         CSR      TRIMOFFSETP    LL_OPAMP_SetTrimmingValue
  * @param  OPAMPx OPAMP instance
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  * @param  TrimmingValue 0x00...0x1F
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetTrimmingValue(OPAMP_TypeDef* OPAMPx, uint32_t TransistorsDiffPair, uint32_t TrimmingValue)
{
  MODIFY_REG(OPAMPx->CSR,
             (TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK),
             TrimmingValue << (POSITION_VAL(TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK)));
}

/**
  * @brief  Get OPAMP trimming factor for the selected transistors
  *         differential pair NMOS or PMOS, corresponding to the selected
  *         power mode.
  * @rmtoll CSR      TRIMOFFSETN    LL_OPAMP_GetTrimmingValue\n
  *         CSR      TRIMOFFSETP    LL_OPAMP_GetTrimmingValue
  * @param  OPAMPx OPAMP instance
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  * @retval 0x0...0x1F
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetTrimmingValue(OPAMP_TypeDef* OPAMPx, uint32_t TransistorsDiffPair)
{
  return (uint32_t)(READ_BIT(OPAMPx->CSR, (TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK))
                    >> (POSITION_VAL(TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK))
                   );
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_OPERATION Operation on OPAMP instance
  * @{
  */
/**
  * @brief  Enable OPAMP instance.
  * @note   After enable from off state, OPAMP requires a delay
  *         to fullfill wake up time specification.
  *         Refer to device datasheet, parameter "tWAKEUP".
  * @rmtoll CSR      OPAMPXEN       LL_OPAMP_Enable
  * @param  OPAMPx OPAMP instance
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_Enable(OPAMP_TypeDef *OPAMPx)
{
  SET_BIT(OPAMPx->CSR, OPAMP_CSR_OPAMPxEN);
}

/**
  * @brief  Disable OPAMP instance.
  * @rmtoll CSR      OPAMPXEN       LL_OPAMP_Disable
  * @param  OPAMPx OPAMP instance
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_Disable(OPAMP_TypeDef *OPAMPx)
{
  CLEAR_BIT(OPAMPx->CSR, OPAMP_CSR_OPAMPxEN);
}

/**
  * @brief  Get OPAMP instance enable state
  *         (0: OPAMP is disabled, 1: OPAMP is enabled)
  * @rmtoll CSR      OPAMPXEN       LL_OPAMP_IsEnabled
  * @param  OPAMPx OPAMP instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_OPAMP_IsEnabled(OPAMP_TypeDef *OPAMPx)
{
  return (READ_BIT(OPAMPx->CSR, OPAMP_CSR_OPAMPxEN) == (OPAMP_CSR_OPAMPxEN));
}

/**
  * @brief  Lock OPAMP instance.
  * @note   Once locked, OPAMP configuration can be accessed in read-only.
  * @note   The only way to unlock the OPAMP is a device hardware reset.
  * @rmtoll CSR      LOCK           LL_OPAMP_Lock
  * @param  OPAMPx OPAMP instance
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_Lock(OPAMP_TypeDef *OPAMPx)
{
  SET_BIT(OPAMPx->CSR, OPAMP_CSR_LOCK);
}

/**
  * @brief  Get OPAMP lock state
  *         (0: OPAMP is unlocked, 1: OPAMP is locked).
  * @note   Once locked, OPAMP configuration can be accessed in read-only.
  * @note   The only way to unlock the OPAMP is a device hardware reset.
  * @rmtoll CSR      LOCK           LL_OPAMP_IsLocked
  * @param  OPAMPx OPAMP instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_OPAMP_IsLocked(OPAMP_TypeDef *OPAMPx)
{
  return (READ_BIT(OPAMPx->CSR, OPAMP_CSR_LOCK) == (OPAMP_CSR_LOCK));
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup OPAMP_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

ErrorStatus LL_OPAMP_DeInit(OPAMP_TypeDef *OPAMPx);
ErrorStatus LL_OPAMP_Init(OPAMP_TypeDef *OPAMPx, LL_OPAMP_InitTypeDef *OPAMP_InitStruct);
void        LL_OPAMP_StructInit(LL_OPAMP_InitTypeDef *OPAMP_InitStruct);

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

#endif /* OPAMP1 || OPAMP2 || OPAMP3 || OPAMP4 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F3xx_LL_OPAMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
