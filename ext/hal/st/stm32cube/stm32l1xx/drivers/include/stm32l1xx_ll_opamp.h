/**
  ******************************************************************************
  * @file    stm32l1xx_ll_opamp.h
  * @author  MCD Application Team
  * @brief   Header file of OPAMP LL module.
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
#ifndef __STM32L1xx_LL_OPAMP_H
#define __STM32L1xx_LL_OPAMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/** @addtogroup STM32L1xx_LL_Driver
  * @{
  */

#if defined (OPAMP1) || defined (OPAMP2) || defined (OPAMP3) 

/** @defgroup OPAMP_LL OPAMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup OPAMP_LL_Private_Constants OPAMP Private Constants
  * @{
  */

/* Internal mask for OPAMP power mode:                                        */
/* To select into literal LL_OPAMP_POWERMODE_x the relevant bits for:         */
/* - OPAMP power mode into control register                                   */
/* - OPAMP trimming register offset                                           */

/* Internal register offset for OPAMP trimming configuration */
#define OPAMP_POWERMODE_OTR_REGOFFSET       (0x00000000U)
#define OPAMP_POWERMODE_LPOTR_REGOFFSET     (0x00000001U)
#define OPAMP_POWERMODE_OTR_REGOFFSET_MASK  (OPAMP_POWERMODE_OTR_REGOFFSET | OPAMP_POWERMODE_LPOTR_REGOFFSET)

/* Mask for OPAMP power mode into control register */
#define OPAMP_POWERMODE_CSR_BIT_MASK        (OPAMP_CSR_OPA1LPM)

/* Internal mask for OPAMP trimming of transistors differential pair NMOS     */
/* or PMOS.                                                                   */
/* To select into literal LL_OPAMP_TRIMMING_x the relevant bits for:          */
/* - OPAMP trimming selection of transistors differential pair                */
/* - OPAMP trimming values of transistors differential pair                   */
#define OPAMP_TRIMMING_SELECT_SW_OFFSET     (16U)
#define OPAMP_TRIMMING_SELECT_MASK          ((OPAMP_CSR_OPA1CAL_H | OPAMP_CSR_OPA1CAL_L) << OPAMP_TRIMMING_SELECT_SW_OFFSET)
#define OPAMP_TRIMMING_VALUE_MASK           (OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH | OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW)

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
  * @brief  Driver macro reserved for internal use: from OPAMP instance
  *         selected, return the instance number in decimal format.
  * @param  __OPAMP_INSTANCE__ OPAMP instance
  * @retval Instance number in decimal format: value "0" for OPAMP1,
  *         value "1" for OPAMP2, value "2" for OPAMP3.
*/
#define __OPAMP_INSTANCE_DECIMAL(__OPAMP_INSTANCE__)                           \
  ((uint32_t)(__OPAMP_INSTANCE__) - OPAMP_BASE)

/**
  * @brief  Driver macro reserved for internal use: from OPAMP instance
  *         selected, set offset of bits into OPAMP register.
  * @note   Since all OPAMP instances are sharing the same register
  *         with 3 area of bits with an offset of 8 bits (except bits
  *         OPAxCALOUT, OPARANGE, S7SEL2), this function
  *         returns .
  * @param  __OPAMP_INSTANCE__ OPAMP instance
  * @retval Bits offset in register 32 bits: value "0" for OPAMP1,
  *         value "8" for OPAMP2, value "16" for OPAMP3
*/
#define __OPAMP_INSTANCE_BITOFFSET(__OPAMP_INSTANCE__)                         \
  (((uint32_t)(__OPAMP_INSTANCE__) - OPAMP_BASE) << 3U)

/**
  * @brief  Driver macro reserved for internal use: from OPAMP instance
  *         selected, return whether it corresponds to instance OPAMP2.
  * @param  __OPAMP_INSTANCE__ OPAMP instance
  * @retval Instance number in decimal format: value "0" for OPAMP1 or OPAMP3,
  *         value "1" for OPAMP2.
*/
#define __OPAMP_IS_INSTANCE_OPAMP2(__OPAMP_INSTANCE__)                         \
  (((uint32_t)(__OPAMP_INSTANCE__) - OPAMP_BASE) % 2)

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
  uint32_t PowerMode;                   /*!< Set OPAMP power mode.
                                             This parameter can be a value of @ref OPAMP_LL_EC_POWERMODE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetPowerMode(). */

  uint32_t FunctionalMode;              /*!< Set OPAMP functional mode by setting internal connections: OPAMP operation in standalone, follower, ...
                                             This parameter can be a value of @ref OPAMP_LL_EC_FUNCTIONAL_MODE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetFunctionalMode(). */

  uint32_t InputNonInverting;           /*!< Set OPAMP input non-inverting connection.
                                             This parameter can be a value of @ref OPAMP_LL_EC_INPUT_NONINVERTING
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_OPAMP_SetInputNonInverting(). */

  uint32_t InputInverting;              /*!< Set OPAMP inverting input connection.
                                             This parameter can be a value of @ref OPAMP_LL_EC_INPUT_INVERTING
                                             @note OPAMP inverting input is used with OPAMP in mode standalone. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin), this parameter is discarded.
                                             
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

/** @defgroup OPAMP_LL_EC_POWERSUPPLY_RANGE OPAMP power supply range
  * @{
  */
#define LL_OPAMP_POWERSUPPLY_RANGE_LOW  (0x00000000U)           /*!< Power supply range low. On STM32L1 serie: Vdda lower than 2.4V. */
#define LL_OPAMP_POWERSUPPLY_RANGE_HIGH (OPAMP_CSR_AOP_RANGE)   /*!< Power supply range high. On STM32L1 serie: Vdda higher than 2.4V. */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_POWERMODE OPAMP power mode
  * @{
  */
#define LL_OPAMP_POWERMODE_NORMAL       (OPAMP_POWERMODE_OTR_REGOFFSET)                       /*!< OPAMP power mode normal */
#define LL_OPAMP_POWERMODE_LOWPOWER     (OPAMP_POWERMODE_LPOTR_REGOFFSET | OPAMP_CSR_OPA1LPM) /*!< OPAMP power mode low-power */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_MODE OPAMP mode calibration or functional.
  * @{
  */
#define LL_OPAMP_MODE_FUNCTIONAL        (0x00000000U)                                                                                  /*!< OPAMP functional mode */
#define LL_OPAMP_MODE_CALIBRATION       (OPAMP_CSR_S3SEL1 | OPAMP_CSR_S4SEL1 | OPAMP_CSR_S5SEL1 | OPAMP_CSR_S6SEL1 | OPAMP_CSR_S7SEL2) /*!< OPAMP calibration mode (on STM32L1 serie, it corresponds to all OPAMP input internal switches opened) */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_FUNCTIONAL_MODE OPAMP functional mode
  * @{
  */
#define LL_OPAMP_MODE_STANDALONE        (0x00000000U)           /*!< OPAMP functional mode, OPAMP operation in standalone (on STM32L1 serie, it corresponds to OPAMP internal switches S3 opened (switch SanB state depends on switch S4 state)) */
#define LL_OPAMP_MODE_FOLLOWER          (OPAMP_CSR_S3SEL1)      /*!< OPAMP functional mode, OPAMP operation in follower (on STM32L1 serie, it corresponds to OPAMP internal switches S3 and SanB closed) */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_NONINVERTING OPAMP input non-inverting
  * @{
  */
#define LL_OPAMP_INPUT_NONINVERT_IO0      (OPAMP_CSR_S5SEL1)      /*!< OPAMP non inverting input connected to GPIO pin (low leakage input) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH1    (OPAMP_CSR_S6SEL1)      /*!< OPAMP non inverting input connected to DAC1 channel1 output (specific to OPAMP instances: OPAMP1, OPAMP2) */
#define LL_OPAMP_INPUT_NONINV_DAC1_CH2    (OPAMP_CSR_S7SEL2)      /*!< OPAMP non inverting input connected to DAC1 channel2 output (specific to OPAMP instances: OPAMP2, OPAMP3) */
#if defined(OPAMP3)
#define LL_OPAMP_INPUT_NONINV_DAC1_CH2_OPAMP3 (OPAMP_CSR_S6SEL1)  /*!< OPAMP non inverting input connected to DAC1 channel2 output (specific to OPAMP instances: OPAMP3) */
#endif
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_INPUT_INVERTING OPAMP input inverting
  * @{
  */
#define LL_OPAMP_INPUT_INVERT_IO0        (OPAMP_CSR_S4SEL1)      /*!< OPAMP inverting input connected to GPIO pin (low leakage input). Note: OPAMP inverting input is used with OPAMP in mode standalone. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
#define LL_OPAMP_INPUT_INVERT_IO1        (OPAMP_CSR_ANAWSEL1)    /*!< OPAMP inverting input connected to GPIO pin (alternative IO pin, not low leakage, availability depends on STM32L1 serie devices packages). Note: OPAMP inverting input is used with OPAMP in mode standalone. Otherwise (OPAMP in mode follower), OPAMP inverting input is not used (not connected to GPIO pin). */
#define LL_OPAMP_INPUT_INVERT_CONNECT_NO (0x00000000U)           /*!< OPAMP inverting input not externally connected (intended for OPAMP in mode follower) */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_TRIMMING_MODE OPAMP trimming mode
  * @{
  */
#define LL_OPAMP_TRIMMING_FACTORY       (0x00000000U)           /*!< OPAMP trimming factors set to factory values */
#define LL_OPAMP_TRIMMING_USER          (OPAMP_OTR_OT_USER)     /*!< OPAMP trimming factors set to user values */
/**
  * @}
  */

/** @defgroup OPAMP_LL_EC_TRIMMING_TRANSISTORS_DIFF_PAIR OPAMP trimming of transistors differential pair NMOS or PMOS
  * @{
  */
#define LL_OPAMP_TRIMMING_NMOS          (OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH | (OPAMP_CSR_OPA1CAL_H << OPAMP_TRIMMING_SELECT_SW_OFFSET)) /*!< OPAMP trimming of transistors differential pair NMOS */
#define LL_OPAMP_TRIMMING_PMOS          (OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW  | (OPAMP_CSR_OPA1CAL_L << OPAMP_TRIMMING_SELECT_SW_OFFSET)) /*!< OPAMP trimming of transistors differential pair PMOS */
#define LL_OPAMP_TRIMMING_NONE          (0x00000000U)                                                                                   /*!< OPAMP trimming unselect transistors differential pair NMOS and PMOs */
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
/*        - OPAMP in mode low power                                           */
/*        - load impedance of 4kOhm (min), 50pF (max)                         */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tWAKEUP").                                                      */
/* Unit: us                                                                   */
#define LL_OPAMP_DELAY_STARTUP_US         (30U)  /*!< Delay for OPAMP startup time */

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

/** @defgroup OPAMP_LL_EM_HELPER_MACRO OPAMP helper macro
  * @{
  */

/**
  * @brief  Helper macro to select the OPAMP common instance
  *         to which is belonging the selected OPAMP instance.
  * @note   OPAMP common register instance can be used to
  *         set parameters common to several OPAMP instances.
  *         Refer to functions having argument "OPAMPxy_COMMON" as parameter.
  * @param  __OPAMPx__ OPAMP instance
  * @retval OPAMP common instance
  */
#if defined(OPAMP1) && defined(OPAMP2) && defined(OPAMP3)
#define __LL_OPAMP_COMMON_INSTANCE(__OPAMPx__)                                 \
  (OPAMP123_COMMON)
#else
#define __LL_OPAMP_COMMON_INSTANCE(__OPAMPx__)                                 \
  (OPAMP12_COMMON)
#endif

/**
  * @brief  Helper macro to check if all OPAMP instances sharing the same
  *         OPAMP common instance are disabled.
  * @note   This check is required by functions with setting conditioned to
  *         OPAMP state:
  *         All OPAMP instances of the OPAMP common group must be disabled.
  *         Refer to functions having argument "OPAMPxy_COMMON" as parameter.
  * @retval 0: All OPAMP instances sharing the same OPAMP common instance
  *            are disabled.
  *         1: At least one OPAMP instance sharing the same OPAMP common instance
  *            is enabled
  */
#if defined(OPAMP1) && defined(OPAMP2) && defined(OPAMP3)
#define __LL_OPAMP_IS_ENABLED_ALL_COMMON_INSTANCE()                            \
  (LL_OPAMP_IsEnabled(OPAMP1) |                                                \
   LL_OPAMP_IsEnabled(OPAMP2) |                                                \
   LL_OPAMP_IsEnabled(OPAMP3)  )
#else
#define __LL_OPAMP_IS_ENABLED_ALL_COMMON_INSTANCE()                            \
  (LL_OPAMP_IsEnabled(OPAMP1) |                                                \
   LL_OPAMP_IsEnabled(OPAMP2)  )
#endif

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

/** @defgroup OPAMP_LL_EF_Configuration_opamp_common Configuration of OPAMP hierarchical scope: common to several OPAMP instances
  * @{
  */

/**
  * @brief  Set OPAMP power range.
  * @note   The OPAMP power range applies to several OPAMP instances
  *         (if several OPAMP instances available on the selected device).
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         OPAMP state:
  *         All OPAMP instances of the OPAMP common group must be disabled.
  *         This check can be done with function @ref LL_OPAMP_IsEnabled() for each
  *         OPAMP instance or by using helper macro
  *         @ref __LL_OPAMP_IS_ENABLED_ALL_COMMON_INSTANCE().
  * @rmtoll CSR      AOP_RANGE      LL_OPAMP_SetCommonPowerRange
  * @param  OPAMPxy_COMMON OPAMP common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_OPAMP_COMMON_INSTANCE() )
  * @param  PowerRange This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERSUPPLY_RANGE_LOW
  *         @arg @ref LL_OPAMP_POWERSUPPLY_RANGE_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetCommonPowerRange(OPAMP_Common_TypeDef *OPAMPxy_COMMON, uint32_t PowerRange)
{
  MODIFY_REG(OPAMP->CSR, OPAMP_CSR_AOP_RANGE, PowerRange);
}

/**
  * @brief  Get OPAMP power range.
  * @note   The OPAMP power range applies to several OPAMP instances
  *         (if several OPAMP instances available on the selected device).
  * @rmtoll CSR      AOP_RANGE      LL_OPAMP_GetCommonPowerRange
  * @param  OPAMPxy_COMMON OPAMP common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_OPAMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERSUPPLY_RANGE_LOW
  *         @arg @ref LL_OPAMP_POWERSUPPLY_RANGE_HIGH
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetCommonPowerRange(OPAMP_Common_TypeDef *OPAMPxy_COMMON)
{
  return (uint32_t)(READ_BIT(OPAMP->CSR, OPAMP_CSR_AOP_RANGE));
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_CONFIGURATION_OPAMP_INSTANCE Configuration of OPAMP hierarchical scope: OPAMP instance
  * @{
  */

/**
  * @brief  Set OPAMP power mode.
  * @note   The OPAMP must be disabled to change this configuration.
  * @rmtoll CSR      OPA1LPM        LL_OPAMP_SetPowerMode\n
  *         CSR      OPA2LPM        LL_OPAMP_SetPowerMode\n
  *         CSR      OPA3LPM        LL_OPAMP_SetPowerMode
  * @param  OPAMPx OPAMP instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERMODE_NORMAL
  *         @arg @ref LL_OPAMP_POWERMODE_LOWPOWER
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetPowerMode(OPAMP_TypeDef *OPAMPx, uint32_t PowerMode)
{
  MODIFY_REG(OPAMP->CSR,
             OPAMP_CSR_OPA1LPM << __OPAMP_INSTANCE_BITOFFSET(OPAMPx),
             (PowerMode & OPAMP_POWERMODE_CSR_BIT_MASK) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx));
}

/**
  * @brief  Get OPAMP power mode.
  * @rmtoll CSR      OPA1LPM        LL_OPAMP_GetPowerMode\n
  *         CSR      OPA2LPM        LL_OPAMP_GetPowerMode\n
  *         CSR      OPA3LPM        LL_OPAMP_GetPowerMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERMODE_NORMAL
  *         @arg @ref LL_OPAMP_POWERMODE_LOWPOWER
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetPowerMode(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t power_mode = (READ_BIT(OPAMP->CSR, OPAMP_CSR_OPA1LPM << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)));
  
  /* Shift variable to position corresponding to bitfield of OPAMP1 */
  power_mode >>= __OPAMP_INSTANCE_BITOFFSET(OPAMPx);
  
  /* Construct data corresponding to literal LL_OPAMP_POWERMODE_x */
  return (uint32_t)(power_mode | (power_mode >> (POSITION_VAL(OPAMP_CSR_OPA1LPM))));
}

/**
  * @brief  Set OPAMP mode calibration or functional.
  * @note   OPAMP mode corresponds to functional or calibration mode:
  *          - functional mode: OPAMP operation in standalone, follower, ...
  *            Set functional mode using function
  *            @ref LL_OPAMP_SetFunctionalMode().
  *          - calibration mode: offset calibration of the selected
  *            transistors differential pair NMOS or PMOS.
  * @note   On this STM32 serie, entering in calibration mode makes
  *         loosing OPAMP internal switches configuration.
  *         Therefore, when going back to functional mode,
  *         functional mode must be set again using
  *         @ref LL_OPAMP_SetFunctionalMode().
  * @rmtoll CSR      S3SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S4SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S5SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S6SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S7SEL2         LL_OPAMP_SetMode
  * @param  OPAMPx OPAMP instance
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_FUNCTIONAL
  *         @arg @ref LL_OPAMP_MODE_CALIBRATION
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetMode(OPAMP_TypeDef *OPAMPx, uint32_t Mode)
{
  CLEAR_BIT(OPAMP->CSR,
            ((Mode & ~OPAMP_CSR_S7SEL2) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | ((Mode & OPAMP_CSR_S7SEL2) * __OPAMP_IS_INSTANCE_OPAMP2(OPAMPx)));
}

/**
  * @brief  Get OPAMP mode calibration or functional.
  * @note   OPAMP mode corresponds to functional or calibration mode:
  *          - functional mode: OPAMP operation in standalone, follower, ...
  *            Set functional mode using function
  *            @ref LL_OPAMP_SetFunctionalMode().
  *          - calibration mode: offset calibration of the selected
  *            transistors differential pair NMOS or PMOS.
  * @rmtoll CSR      S3SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S4SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S5SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S6SELx         LL_OPAMP_SetMode\n
  * @rmtoll CSR      S7SEL2         LL_OPAMP_SetMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_FUNCTIONAL
  *         @arg @ref LL_OPAMP_MODE_CALIBRATION
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(((READ_BIT(OPAMP->CSR,
                               ((LL_OPAMP_MODE_CALIBRATION & ~OPAMP_CSR_S7SEL2) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | (OPAMP_CSR_S7SEL2 * __OPAMP_IS_INSTANCE_OPAMP2(OPAMPx)))
                     ) == 0U) * LL_OPAMP_MODE_CALIBRATION);
}

/**
  * @brief  Set OPAMP functional mode by setting internal connections.
  *         OPAMP operation in standalone, follower, ...
  * @note   This function reset bit of calibration mode to ensure
  *         to be in functional mode, in order to have OPAMP parameters
  *         (inputs selection, ...) set with the corresponding OPAMP mode
  *         to be effective.
  * @rmtoll CSR      S3SELx        LL_OPAMP_SetFunctionalMode
  * @param  OPAMPx OPAMP instance
  * @param  FunctionalMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_STANDALONE
  *         @arg @ref LL_OPAMP_MODE_FOLLOWER
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetFunctionalMode(OPAMP_TypeDef *OPAMPx, uint32_t FunctionalMode)
{
  /* Note: Bits OPAMP_CSR_OPAxCAL_y reset to ensure to be in functional mode */
  MODIFY_REG(OPAMP->CSR,
             (OPAMP_CSR_S3SEL1 | OPAMP_CSR_OPA1CAL_H | OPAMP_CSR_OPA1CAL_L) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx),
             FunctionalMode << __OPAMP_INSTANCE_BITOFFSET(OPAMPx));
}

/**
  * @brief  Get OPAMP functional mode from setting of internal connections.
  *         OPAMP operation in standalone, follower, ...
  * @rmtoll CSR      S3SELx        LL_OPAMP_GetFunctionalMode
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_MODE_STANDALONE
  *         @arg @ref LL_OPAMP_MODE_FOLLOWER
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetFunctionalMode(OPAMP_TypeDef *OPAMPx)
{
  return (uint32_t)(READ_BIT(OPAMP->CSR, OPAMP_CSR_S3SEL1 << __OPAMP_INSTANCE_BITOFFSET(OPAMPx))
                    >> __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
                   );
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_CONFIGURATION_INPUTS Configuration of OPAMP inputs
  * @{
  */

/**
  * @brief  Set OPAMP non-inverting input connection.
  * @rmtoll CSR      S5SELx         LL_OPAMP_SetInputNonInverting\n
  * @rmtoll CSR      S6SELx         LL_OPAMP_SetInputNonInverting\n
  * @rmtoll CSR      S7SEL2         LL_OPAMP_SetInputNonInverting
  * @param  OPAMPx OPAMP instance
  * @param  InputNonInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1 (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2 (2)
  *         
  *         (1) Parameter specific to OPAMP instances: OPAMP1, OPAMP2.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP2, OPAMP3.
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputNonInverting(OPAMP_TypeDef *OPAMPx, uint32_t InputNonInverting)
{
  MODIFY_REG(OPAMP->CSR,
             ((OPAMP_CSR_S5SEL1 | OPAMP_CSR_S6SEL1) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | (OPAMP_CSR_S7SEL2 * __OPAMP_IS_INSTANCE_OPAMP2(OPAMPx)),
             (InputNonInverting << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | ((InputNonInverting & OPAMP_CSR_S7SEL2) * __OPAMP_IS_INSTANCE_OPAMP2(OPAMPx))
            );
}

/**
  * @brief  Get OPAMP non-inverting input connection.
  * @rmtoll CSR      S5SELx         LL_OPAMP_GetInputNonInverting\n
  * @rmtoll CSR      S6SELx         LL_OPAMP_GetInputNonInverting\n
  * @rmtoll CSR      S7SEL2         LL_OPAMP_GetInputNonInverting
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_NONINVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH1 (1)
  *         @arg @ref LL_OPAMP_INPUT_NONINV_DAC1_CH2 (2)
  *         
  *         (1) Parameter specific to OPAMP instances: OPAMP1, OPAMP2.\n
  *         (2) Parameter specific to OPAMP instances: OPAMP2, OPAMP3.
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputNonInverting(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t input_non_inverting_opamp_x = READ_BIT(OPAMP->CSR,
                                                           (OPAMP_CSR_S5SEL1 | OPAMP_CSR_S6SEL1) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
                                                           | (OPAMP_CSR_S7SEL2 * __OPAMP_IS_INSTANCE_OPAMP2(OPAMPx))
                                                          );
  
  return (((input_non_inverting_opamp_x & ~OPAMP_CSR_S7SEL2) >> __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | (input_non_inverting_opamp_x & OPAMP_CSR_S7SEL2));
}

/**
  * @brief  Set OPAMP inverting input connection.
  * @note   OPAMP inverting input is used with OPAMP in mode standalone.
  *         Otherwise (OPAMP in mode follower), OPAMP inverting input
  *         is not used (not connected to GPIO pin).
  * @rmtoll CSR      S4SELx         LL_OPAMP_SetInputInverting\n
  * @rmtoll CSR      ANAWSELx        LL_OPAMP_SetInputInverting
  * @param  OPAMPx OPAMP instance
  * @param  InputInverting This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1        (1)
  *         @arg @ref LL_OPAMP_INPUT_INVERT_CONNECT_NO
  *         
  *         (1) Alternative IO pin, not low leakage, availability depends on STM32L1 serie devices packages.
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetInputInverting(OPAMP_TypeDef *OPAMPx, uint32_t InputInverting)
{
  MODIFY_REG(OPAMP->CSR,
             ((OPAMP_CSR_S4SEL1) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | ((OPAMP_CSR_ANAWSEL1) << __OPAMP_INSTANCE_DECIMAL(OPAMPx)),
             ((InputInverting & OPAMP_CSR_S4SEL1) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)) | ((InputInverting & OPAMP_CSR_ANAWSEL1) << __OPAMP_INSTANCE_DECIMAL(OPAMPx))
            );
}

/**
  * @brief  Get OPAMP inverting input connection.
  * @rmtoll CSR      S4SELx         LL_OPAMP_SetInputInverting\n
  * @rmtoll CSR      ANAWSELx        LL_OPAMP_SetInputInverting
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO0
  *         @arg @ref LL_OPAMP_INPUT_INVERT_IO1        (1)
  *         @arg @ref LL_OPAMP_INPUT_INVERT_CONNECT_NO
  *         
  *         (1) Alternative IO pin, not low leakage, availability depends on STM32L1 serie devices packages.
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetInputInverting(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t input_inverting_opamp_x = READ_BIT(OPAMP->CSR,
                                                         (OPAMP_CSR_S4SEL1) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
                                                       | (OPAMP_CSR_ANAWSEL1) << __OPAMP_INSTANCE_DECIMAL(OPAMPx)
                                                      );
  
#if defined(OPAMP3)
  return (  ((input_inverting_opamp_x & (OPAMP_CSR_S4SEL1 | OPAMP_CSR_S4SEL2 | OPAMP_CSR_S4SEL3)) >> __OPAMP_INSTANCE_BITOFFSET(OPAMPx))
          | ((input_inverting_opamp_x & (OPAMP_CSR_ANAWSEL1 | OPAMP_CSR_ANAWSEL2 | OPAMP_CSR_ANAWSEL3)) >> __OPAMP_INSTANCE_DECIMAL(OPAMPx)));
#else
  return (  ((input_inverting_opamp_x & (OPAMP_CSR_S4SEL1 | OPAMP_CSR_S4SEL2)) >> __OPAMP_INSTANCE_BITOFFSET(OPAMPx))
          | ((input_inverting_opamp_x & (OPAMP_CSR_ANAWSEL1 | OPAMP_CSR_ANAWSEL2)) >> __OPAMP_INSTANCE_DECIMAL(OPAMPx)));
#endif
}

/**
  * @}
  */

/** @defgroup OPAMP_LL_EF_OPAMP_TRIMMING Configuration and operation of OPAMP trimming
  * @{
  */

/**
  * @brief  Set OPAMP trimming mode.
  * @note   The OPAMP trimming mode applies to several OPAMP instances
  *         (if several OPAMP instances available on the selected device).
  * @rmtoll OTR      OT_USER        LL_OPAMP_SetCommonTrimmingMode
  * @param  OPAMPxy_COMMON OPAMP common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_OPAMP_COMMON_INSTANCE() )
  * @param  TrimmingMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_FACTORY
  *         @arg @ref LL_OPAMP_TRIMMING_USER
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetCommonTrimmingMode(OPAMP_Common_TypeDef *OPAMPxy_COMMON, uint32_t TrimmingMode)
{
  /* Note: On STM32L1 serie, OPAMP trimming mode bit "OPAMP_OTR_OT_USER" is   */
  /*       write only, cannot be read.                                        */
  MODIFY_REG(OPAMPxy_COMMON->OTR,
             OPAMP_OTR_OT_USER,
             TrimmingMode);
}

/**
  * @brief  Get OPAMP trimming mode.
  * @note   The OPAMP trimming mode applies to several OPAMP instances
  *         (if several OPAMP instances available on the selected device).
  * @rmtoll OTR      OT_USER        LL_OPAMP_GetCommonTrimmingMode
  * @param  OPAMPxy_COMMON OPAMP common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_OPAMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_FACTORY
  *         @arg @ref LL_OPAMP_TRIMMING_USER
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetCommonTrimmingMode(OPAMP_Common_TypeDef *OPAMPxy_COMMON)
{
  return (uint32_t)(READ_BIT(OPAMPxy_COMMON->OTR, OPAMP_OTR_OT_USER));
}

/**
  * @brief  Set OPAMP offset to calibrate the selected transistors
  *         differential pair NMOS or PMOS.
  * @note   Preliminarily, OPAMP must be set in mode calibration
  *         using function @ref LL_OPAMP_SetMode().
  * @rmtoll CSR      OPA1CAL_H      LL_OPAMP_SetCalibrationSelection\n
  *         CSR      OPA1CAL_L      LL_OPAMP_SetCalibrationSelection
  * @param  OPAMPx OPAMP instance
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  *         @arg @ref LL_OPAMP_TRIMMING_NONE
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetCalibrationSelection(OPAMP_TypeDef *OPAMPx, uint32_t TransistorsDiffPair)
{
  /* Parameter used with mask "OPAMP_TRIMMING_SELECT_MASK" because            */
  /* containing other bits reserved for other purpose.                        */
  MODIFY_REG(OPAMP->CSR,
             (OPAMP_CSR_OPA1CAL_H | OPAMP_CSR_OPA1CAL_L) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx),
             ((TransistorsDiffPair & OPAMP_TRIMMING_SELECT_MASK) >> OPAMP_TRIMMING_SELECT_SW_OFFSET) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
            );
}

/**
  * @brief  Get OPAMP offset to calibrate the selected transistors
  *         differential pair NMOS or PMOS.
  * @note   Preliminarily, OPAMP must be set in mode calibration
  *         using function @ref LL_OPAMP_SetMode().
  * @rmtoll CSR      OPA1CAL_H      LL_OPAMP_SetCalibrationSelection\n
  *         CSR      OPA1CAL_L      LL_OPAMP_SetCalibrationSelection
  * @param  OPAMPx OPAMP instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  *         @arg @ref LL_OPAMP_TRIMMING_NONE
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetCalibrationSelection(OPAMP_TypeDef *OPAMPx)
{
  register uint32_t CalibrationSelection = (uint32_t)(READ_BIT(OPAMP->CSR,
                                                               (OPAMP_CSR_OPA1CAL_H | OPAMP_CSR_OPA1CAL_L) << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
                                                              )
                                                      >> __OPAMP_INSTANCE_BITOFFSET(OPAMPx)
                                                     );
  
  return ((CalibrationSelection << OPAMP_TRIMMING_SELECT_SW_OFFSET) |
          ((OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW) << (OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH_Pos * ((CalibrationSelection & OPAMP_CSR_OPA1CAL_H) != 0U))));
}

/**
  * @brief  Get OPAMP calibration result of toggling output.
  * @note   This functions returns:
  *         0 if OPAMP calibration output is reset
  *         1 if OPAMP calibration output is set
  * @rmtoll CSR      OPAxCALOUT     LL_OPAMP_IsCalibrationOutputSet
  * @param  OPAMPx OPAMP instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_OPAMP_IsCalibrationOutputSet(OPAMP_TypeDef *OPAMPx)
{
  return (READ_BIT(OPAMP->CSR, (OPAMP_CSR_OPA1CALOUT << __OPAMP_INSTANCE_DECIMAL(OPAMPx)))
          == (OPAMP_CSR_OPA1CALOUT << __OPAMP_INSTANCE_DECIMAL(OPAMPx)));
}

/**
  * @brief  Set OPAMP trimming factor for the selected transistors
  *         differential pair NMOS or PMOS, corresponding to the selected
  *         power mode.
  * @note   On STM32L1 serie, OPAMP trimming mode must be re-configured 
  *         at each update of trimming values in power mode normal.
  *         Refer to function @ref LL_OPAMP_SetCommonTrimmingMode().
  * @rmtoll OTR      AOx_OPT_OFFSET_TRIM_HIGH    LL_OPAMP_SetTrimmingValue\n
  *         OTR      AOx_OPT_OFFSET_TRIM_LOW     LL_OPAMP_SetTrimmingValue\n
  *         LPOTR    AOx_OPT_OFFSET_TRIM_LP_HIGH LL_OPAMP_SetTrimmingValue\n
  *         LPOTR    AOx_OPT_OFFSET_TRIM_LP_LOW  LL_OPAMP_SetTrimmingValue
  * @param  OPAMPx OPAMP instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERMODE_NORMAL
  *         @arg @ref LL_OPAMP_POWERMODE_LOWPOWER
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  * @param  TrimmingValue 0x00...0x1F
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_SetTrimmingValue(OPAMP_TypeDef* OPAMPx, uint32_t PowerMode, uint32_t TransistorsDiffPair, uint32_t TrimmingValue)
{
  register uint32_t *preg = __OPAMP_PTR_REG_OFFSET(OPAMP->OTR, (PowerMode & OPAMP_POWERMODE_OTR_REGOFFSET_MASK));
  
  /* Set bits with position in register depending on parameter                */
  /* "TransistorsDiffPair".                                                   */
  /* Parameter used with mask "OPAMP_TRIMMING_VALUE_MASK" because             */
  /* containing other bits reserved for other purpose.                        */
  MODIFY_REG(*preg,
             (TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK) << (OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos * __OPAMP_INSTANCE_DECIMAL(OPAMPx)),
             TrimmingValue << (POSITION_VAL(TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK) + (OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos * __OPAMP_INSTANCE_DECIMAL(OPAMPx))));
}

/**
  * @brief  Get OPAMP trimming factor for the selected transistors
  *         differential pair NMOS or PMOS, corresponding to the selected
  *         power mode.
  * @rmtoll OTR      AOx_OPT_OFFSET_TRIM_HIGH    LL_OPAMP_GetTrimmingValue\n
  *         OTR      AOx_OPT_OFFSET_TRIM_LOW     LL_OPAMP_GetTrimmingValue\n
  *         LPOTR    AOx_OPT_OFFSET_TRIM_LP_HIGH LL_OPAMP_GetTrimmingValue\n
  *         LPOTR    AOx_OPT_OFFSET_TRIM_LP_LOW  LL_OPAMP_GetTrimmingValue
  * @param  OPAMPx OPAMP instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_POWERMODE_NORMAL
  *         @arg @ref LL_OPAMP_POWERMODE_LOWPOWER
  * @param  TransistorsDiffPair This parameter can be one of the following values:
  *         @arg @ref LL_OPAMP_TRIMMING_NMOS
  *         @arg @ref LL_OPAMP_TRIMMING_PMOS
  * @retval 0x0...0x1F
  */
__STATIC_INLINE uint32_t LL_OPAMP_GetTrimmingValue(OPAMP_TypeDef* OPAMPx, uint32_t PowerMode, uint32_t TransistorsDiffPair)
{
  register uint32_t *preg = __OPAMP_PTR_REG_OFFSET(OPAMP->OTR, (PowerMode & OPAMP_POWERMODE_OTR_REGOFFSET_MASK));
  
  /* Retrieve bits with position in register depending on parameter           */
  /* "TransistorsDiffPair".                                                   */
  /* Parameter used with mask "OPAMP_TRIMMING_VALUE_MASK" because             */
  /* containing other bits reserved for other purpose.                        */
  return (uint32_t)(READ_BIT(*preg, (TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK) << (OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos * __OPAMP_INSTANCE_DECIMAL(OPAMPx)))
                    >> (POSITION_VAL(TransistorsDiffPair & OPAMP_TRIMMING_VALUE_MASK) + (OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos * __OPAMP_INSTANCE_DECIMAL(OPAMPx)))
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
  * @rmtoll CSR      OPAxPD         LL_OPAMP_Enable
  * @param  OPAMPx OPAMP instance
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_Enable(OPAMP_TypeDef *OPAMPx)
{
  CLEAR_BIT(OPAMP->CSR, OPAMP_CSR_OPA1PD << __OPAMP_INSTANCE_BITOFFSET(OPAMPx));
}

/**
  * @brief  Disable OPAMP instance.
  * @rmtoll CSR      OPAxPD         LL_OPAMP_Disable
  * @param  OPAMPx OPAMP instance
  * @retval None
  */
__STATIC_INLINE void LL_OPAMP_Disable(OPAMP_TypeDef *OPAMPx)
{
  SET_BIT(OPAMP->CSR, OPAMP_CSR_OPA1PD << __OPAMP_INSTANCE_BITOFFSET(OPAMPx));
}

/**
  * @brief  Get OPAMP instance enable state
  *         (0: OPAMP is disabled, 1: OPAMP is enabled)
  * @rmtoll CSR      OPAxPD         LL_OPAMP_IsEnabled
  * @param  OPAMPx OPAMP instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_OPAMP_IsEnabled(OPAMP_TypeDef *OPAMPx)
{
  return (READ_BIT(OPAMP->CSR, OPAMP_CSR_OPA1PD << __OPAMP_INSTANCE_BITOFFSET(OPAMPx))
          != (OPAMP_CSR_OPA1PD << __OPAMP_INSTANCE_BITOFFSET(OPAMPx)));
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

#endif /* OPAMP1 || OPAMP2 || OPAMP3 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32L1xx_LL_OPAMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
