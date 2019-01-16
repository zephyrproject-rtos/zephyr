/**
  ******************************************************************************
  * @file    stm32g0xx_ll_comp.h
  * @author  MCD Application Team
  * @brief   Header file of COMP LL module.
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
#ifndef STM32G0xx_LL_COMP_H
#define STM32G0xx_LL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
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

/* Internal mask for pair of comparators instances window mode:               */
/* To select into literals LL_COMP_WINDOWMODE_COMPx_INPUT_PLUS_COMMON         */
/* the relevant bits for:                                                     */
/* (concatenation of multiple bits used in different registers)               */
/* - Comparator instance selected as master for window mode : register offset */
/* - Window mode enable or disable: bit value */
#define LL_COMP_WINDOWMODE_COMP_ODD_REGOFFSET_MASK  (0x00000000UL) /* Register of COMP instance odd (COMP1_CSR, ...) defined as reference register */
#define LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK (0x00000001UL) /* Register of COMP instance even (COMP2_CSR, ...) offset vs register of COMP instance odd */
#define LL_COMP_WINDOWMODE_COMPX_REGOFFSET_MASK     (LL_COMP_WINDOWMODE_COMP_ODD_REGOFFSET_MASK | LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK)
#define LL_COMP_WINDOWMODE_COMPX_SETTING_MASK       (COMP_CSR_WINMODE)
#define LL_COMP_WINDOWOUTPUT_COMPX_SETTING_MASK     (COMP_CSR_WINOUT)
#define LL_COMP_WINDOWOUTPUT_BOTH_SETTING_MASK      (COMP_CSR_WINOUT << 1UL)
#define LL_COMP_WINDOWOUTPUT_BOTH_POS_VS_WINDOW     (1UL)

/* COMP registers bits positions */
#define LL_COMP_WINDOWMODE_BITOFFSET_POS   (11UL) /* Value equivalent to POSITION_VAL(COMP_CSR_WINMODE) */
#define LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS (30UL) /* Value equivalent to POSITION_VAL(COMP_CSR_VALUE) */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup COMP_LL_Private_Macros COMP Private Macros
  * @{
  */

/**
  * @brief  Driver macro reserved for internal use: set a pointer to
  *         a register from a register basis from which an offset
  *         is applied.
  * @param  __REG__ Register basis from which the offset is applied.
  * @param  __REG_OFFFSET__ Offset to be applied (unit: number of registers).
  * @retval Pointer to register address
  */
#define __COMP_PTR_REG_OFFSET(__REG__, __REG_OFFFSET__)                        \
 ((uint32_t *)((uint32_t) ((uint32_t)(&(__REG__)) + ((__REG_OFFFSET__) << 2UL))))

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

  uint32_t OutputPolarity;              /*!< Set comparator output polarity.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_POLARITY
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputPolarity(). */

  uint32_t OutputBlankingSource;        /*!< Set comparator blanking source.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_BLANKING_SOURCE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputBlankingSource(). */

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
#define LL_COMP_WINDOWMODE_DISABLE                 (0x00000000UL)                                                   /*!< Window mode disable: Comparators 1 and 2 are independent */
#define LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (COMP_CSR_WINMODE | LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK) /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP1 input plus (COMP2 input plus is no more accessible). */
#define LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON (COMP_CSR_WINMODE | LL_COMP_WINDOWMODE_COMP_ODD_REGOFFSET_MASK)  /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP2 input plus (COMP1 input plus is no more accessible). */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_COMMON_WINDOWOUTPUT Comparator common modes - Window output
  * @{
  */
#define LL_COMP_WINDOWOUTPUT_EACH_COMP               (0x00000000UL)                                                  /*!< Window output default mode: Comparators output are indicating each their own state. To know window mode state: each comparator output must be read, if "((COMPx exclusive or COMPy) == 1)" then monitored signal is within comparators window. The same way, if both comparators output are high, then monitored signal is below window. */
#define LL_COMP_WINDOWOUTPUT_COMP1                   (COMP_CSR_WINOUT | LL_COMP_WINDOWMODE_COMP_ODD_REGOFFSET_MASK)  /*!< Window output synthetized on COMP1 output: COMP1 output is no more indicating its own state, but global window mode state (logical high means monitored signal is within comparators window). */
#define LL_COMP_WINDOWOUTPUT_COMP2                   (COMP_CSR_WINOUT | LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK) /*!< Window output synthetized on COMP2 output: COMP2 output is no more indicating its own state, but global window mode state (logical high means monitored signal is within comparators window). */
#define LL_COMP_WINDOWOUTPUT_BOTH                    (COMP_CSR_WINOUT | LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK | LL_COMP_WINDOWOUTPUT_BOTH_SETTING_MASK)      /*!< Window output synthetized on both COMP1 and COMP2 output: COMP1 and COMP2 outputs are no more indicating their own state, but global window mode state (logical high means monitored signal is within comparators window). This is a specific configuraton (technically possible but not relevant from application point of view: 2 comparators output used for the same signal level), standard configuraton for window mode is one of the 3 settings above. */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_POWERMODE Comparator modes - Power mode
  * @{
  */
#define LL_COMP_POWERMODE_HIGHSPEED     (0x00000000UL)                            /*!< COMP power mode to high speed */
#define LL_COMP_POWERMODE_MEDIUMSPEED   (COMP_CSR_PWRMODE_0)                      /*!< COMP power mode to medium speed */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PLUS Comparator inputs - Input plus (input non-inverting) selection
  * @{
  */
#define LL_COMP_INPUT_PLUS_IO1          (0x00000000UL)                          /*!< Comparator input plus connected to IO1 (pin PC5 for COMP1, pin PB4 for COMP2) */
#define LL_COMP_INPUT_PLUS_IO2          (COMP_CSR_INPSEL_0)                     /*!< Comparator input plus connected to IO2 (pin PB2 for COMP1, pin PB6 for COMP2) */
#define LL_COMP_INPUT_PLUS_IO3          (COMP_CSR_INPSEL_1)                     /*!< Comparator input plus connected to IO3 (pin PA1 for COMP1, pin PA3 for COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_MINUS Comparator inputs - Input minus (input inverting) selection
  * @{
  */
#define LL_COMP_INPUT_MINUS_1_4VREFINT  (0x00000000UL)                                                                  /*!< Comparator input minus connected to 1/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_1_2VREFINT  (                                                            COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to 1/2 VrefInt  */
#define LL_COMP_INPUT_MINUS_3_4VREFINT  (                                        COMP_CSR_INMSEL_1                    ) /*!< Comparator input minus connected to 3/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_VREFINT     (                                        COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to VrefInt */
#define LL_COMP_INPUT_MINUS_DAC1_CH1    (                    COMP_CSR_INMSEL_2                                        ) /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1)  */
#define LL_COMP_INPUT_MINUS_DAC1_CH2    (                    COMP_CSR_INMSEL_2                     | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2)  */
#define LL_COMP_INPUT_MINUS_IO1         (                    COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1                    ) /*!< Comparator input minus connected to IO1 (pin PA9 for COMP1, pin PB3 for COMP2) */
#define LL_COMP_INPUT_MINUS_IO2         (                    COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to IO2 (pin PC4 for COMP1, pin PB7 for COMP2) */
#define LL_COMP_INPUT_MINUS_IO3         (COMP_CSR_INMSEL_3                                                            ) /*!< Comparator input minus connected to IO3 (pin PA0 for COMP1, pin PA2 for COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_HYSTERESIS Comparator input - Hysteresis
  * @{
  */
#define LL_COMP_HYSTERESIS_NONE         (0x00000000UL)                      /*!< No hysteresis */
#define LL_COMP_HYSTERESIS_LOW          (                  COMP_CSR_HYST_0) /*!< Hysteresis level low */
#define LL_COMP_HYSTERESIS_MEDIUM       (COMP_CSR_HYST_1                  ) /*!< Hysteresis level medium */
#define LL_COMP_HYSTERESIS_HIGH         (COMP_CSR_HYST_1 | COMP_CSR_HYST_0) /*!< Hysteresis level high */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_POLARITY Comparator output - Output polarity
  * @{
  */
#define LL_COMP_OUTPUTPOL_NONINVERTED   (0x00000000UL)          /*!< COMP output polarity is not inverted: comparator output is high when the plus (non-inverting) input is at a higher voltage than the minus (inverting) input */
#define LL_COMP_OUTPUTPOL_INVERTED      (COMP_CSR_POLARITY)     /*!< COMP output polarity is inverted: comparator output is low when the plus (non-inverting) input is at a lower voltage than the minus (inverting) input */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_BLANKING_SOURCE Comparator output - Blanking source
  * @{
  */
#define LL_COMP_BLANKINGSRC_NONE            (0x00000000UL)          /*!<Comparator output without blanking */
/* Note: Output blanking source common to all COMP instances */
#define LL_COMP_BLANKINGSRC_TIM1_OC4        (COMP_CSR_BLANKING_0)   /*!< Comparator output blanking source TIM1 OC4 (common to all COMP instances: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM1_OC5        (COMP_CSR_BLANKING_1)   /*!< Comparator output blanking source TIM1 OC5 (common to all COMP instances: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM2_OC3        (COMP_CSR_BLANKING_2)   /*!< Comparator output blanking source TIM2 OC3 (common to all COMP instances: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM3_OC3        (COMP_CSR_BLANKING_3)   /*!< Comparator output blanking source TIM3 OC3 (common to all COMP instances: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM15_OC2       (COMP_CSR_BLANKING_4)   /*!< Comparator output blanking source TIM15 OC2 (common to all COMP instances: COMP1, COMP2) */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_LEVEL Comparator output - Output level
  * @{
  */
#define LL_COMP_OUTPUT_LEVEL_LOW        (0x00000000UL)          /*!< Comparator output level low (if the polarity is not inverted, otherwise to be complemented) */
#define LL_COMP_OUTPUT_LEVEL_HIGH       (0x00000001UL)          /*!< Comparator output level high (if the polarity is not inverted, otherwise to be complemented) */
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
#define LL_COMP_DELAY_STARTUP_US          ( 80UL) /*!< Delay for COMP startup time */

/* Delay for comparator voltage scaler stabilization time.                    */
/* Note: Voltage scaler is used when selecting comparator input               */
/*       based on VrefInt: VrefInt or subdivision of VrefInt.                 */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tSTART_SCALER").                                                */
/* Unit: us                                                                   */
#define LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US ( 200UL) /*!< Delay for COMP voltage scaler stabilization time */

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
#define LL_COMP_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG((__INSTANCE__)->__REG__, (__VALUE__))

/**
  * @brief  Read a value in COMP register
  * @param  __INSTANCE__ comparator instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_COMP_ReadReg(__INSTANCE__, __REG__) READ_REG((__INSTANCE__)->__REG__)
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
  * @rmtoll CSR      WINMODE        LL_COMP_SetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  *         @arg @ref LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowMode)
{
  /* Note: On this STM32 serie, window mode can be set from one of the pair   */
  /*       of COMP instances: COMP1 or COMP2.                                 */
  register uint32_t *preg = __COMP_PTR_REG_OFFSET(COMPxy_COMMON->CSR_ODD, (WindowMode & LL_COMP_WINDOWMODE_COMPX_REGOFFSET_MASK));
  
  /* Clear the potential previous setting of window mode */
  register uint32_t *preg_clear = __COMP_PTR_REG_OFFSET(COMPxy_COMMON->CSR_ODD, (~(WindowMode & LL_COMP_WINDOWMODE_COMPX_REGOFFSET_MASK) & 0x1UL));
  CLEAR_BIT(*preg_clear,
            COMP_CSR_WINMODE
           );
  
  /* Set window mode */
  MODIFY_REG(*preg,
             COMP_CSR_WINMODE,
             (WindowMode & LL_COMP_WINDOWMODE_COMPX_SETTING_MASK)
            );
}

/**
  * @brief  Get window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WINMODE        LL_COMP_GetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON
  *         @arg @ref LL_COMP_WINDOWMODE_COMP2_INPUT_PLUS_COMMON
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON)
{
  /* Note: On this STM32 serie, window mode can be set from one of the pair   */
  /*       of COMP instances: COMP1 or COMP2.                                 */
  register uint32_t window_mode_comp_odd = READ_BIT(COMPxy_COMMON->CSR_ODD, COMP_CSR_WINMODE);
  register uint32_t window_mode_comp_even = READ_BIT(COMPxy_COMMON->CSR_EVEN, COMP_CSR_WINMODE);
  
  return (uint32_t)(  window_mode_comp_odd
                    | window_mode_comp_even
                    | ((window_mode_comp_even >> LL_COMP_WINDOWMODE_BITOFFSET_POS) * LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK));
}

/**
  * @brief  Set window output of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WINOUT         LL_COMP_SetCommonWindowOutput
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowOutput This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWOUTPUT_EACH_COMP
  *         @arg @ref LL_COMP_WINDOWOUTPUT_COMP1
  *         @arg @ref LL_COMP_WINDOWOUTPUT_COMP2
  *         @arg @ref LL_COMP_WINDOWOUTPUT_BOTH
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowOutput(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowOutput)
{
  register uint32_t *preg = __COMP_PTR_REG_OFFSET(COMPxy_COMMON->CSR_ODD, (WindowOutput & LL_COMP_WINDOWMODE_COMPX_REGOFFSET_MASK));
  
  /* Clear the potential previous setting of window output on the relevant comparator instance */
  /* (clear bit of window output unless specific case of setting of comparator both output selected) */
  register uint32_t *preg_clear = __COMP_PTR_REG_OFFSET(COMPxy_COMMON->CSR_ODD, (~(WindowOutput & LL_COMP_WINDOWMODE_COMPX_REGOFFSET_MASK) & 0x1UL));
  MODIFY_REG(*preg_clear,
             COMP_CSR_WINOUT,
             ((WindowOutput & LL_COMP_WINDOWOUTPUT_BOTH_SETTING_MASK) >> LL_COMP_WINDOWOUTPUT_BOTH_POS_VS_WINDOW)
            );
            
  /* Set window output */
  MODIFY_REG(*preg,
             COMP_CSR_WINOUT,
             (WindowOutput & LL_COMP_WINDOWOUTPUT_COMPX_SETTING_MASK)
            );
}

/**
  * @brief  Get window output of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      WINMODE        LL_COMP_GetCommonWindowOutput
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWOUTPUT_EACH_COMP
  *         @arg @ref LL_COMP_WINDOWOUTPUT_COMP1
  *         @arg @ref LL_COMP_WINDOWOUTPUT_COMP2
  *         @arg @ref LL_COMP_WINDOWOUTPUT_BOTH
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowOutput(COMP_Common_TypeDef *COMPxy_COMMON)
{
  register uint32_t window_output_comp_odd = READ_BIT(COMPxy_COMMON->CSR_ODD, COMP_CSR_WINOUT);
  register uint32_t window_output_comp_even = READ_BIT(COMPxy_COMMON->CSR_EVEN, COMP_CSR_WINOUT);
  
  /* Construct value corresponding to LL_COMP_WINDOWOUTPUT_xxx */
  return (uint32_t)(  window_output_comp_odd
                    | window_output_comp_even
                    | ((window_output_comp_even >> COMP_CSR_WINOUT_Pos) * LL_COMP_WINDOWMODE_COMP_EVEN_REGOFFSET_MASK)
                    | (window_output_comp_odd + window_output_comp_even));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_modes Configuration of comparator modes
  * @{
  */

/**
  * @brief  Set comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      PWRMODE        LL_COMP_SetPowerMode
  * @param  COMPx Comparator instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetPowerMode(COMP_TypeDef *COMPx, uint32_t PowerMode)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_PWRMODE, PowerMode);
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      PWRMODE        LL_COMP_GetPowerMode
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED
  */
__STATIC_INLINE uint32_t LL_COMP_GetPowerMode(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_PWRMODE));
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
  * @note   On this STM32 serie, a voltage scaler is used
  *         when COMP input is based on VrefInt (VrefInt or subdivision
  *         of VrefInt):
  *         Voltage scaler requires a delay for voltage stabilization.
  *         Refer to device datasheet, parameter "xxxx".
*/
//#warning "Specify parameter name in STM32G0 datasheet"
/*
  * @rmtoll CSR      INMSEL         LL_COMP_ConfigInputs\n
  *         CSR      INPSEL         LL_COMP_ConfigInputs\n
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3
  * @retval None
  */
__STATIC_INLINE void LL_COMP_ConfigInputs(COMP_TypeDef *COMPx, uint32_t InputMinus, uint32_t InputPlus)
{
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_INMSEL | COMP_CSR_INPSEL,
             InputMinus | InputPlus);
}

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      INPSEL         LL_COMP_SetInputPlus
  * @param  COMPx Comparator instance
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPlus(COMP_TypeDef *COMPx, uint32_t InputPlus)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_INPSEL, InputPlus);
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      INPSEL         LL_COMP_GetInputPlus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2
  *         @arg @ref LL_COMP_INPUT_PLUS_IO3
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPlus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_INPSEL));
}

/**
  * @brief  Set comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @note   On this STM32 serie, a voltage scaler is used
  *         when COMP input is based on VrefInt (VrefInt or subdivision
  *         of VrefInt):
  *         Voltage scaler requires a delay for voltage stabilization.
  *         Refer to device datasheet, parameter "xxxx".
*/
//#warning "Specify parameter name in STM32G0 datasheet"
/*
  * @rmtoll CSR      INMSEL         LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_INMSEL, InputMinus);
}

/**
  * @brief  Get comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      INMSEL         LL_COMP_GetInputMinus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputMinus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_INMSEL));
}

/**
  * @brief  Set comparator instance hysteresis mode of the input minus (inverting input).
  * @rmtoll CSR      HYST           LL_COMP_SetInputHysteresis
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
  MODIFY_REG(COMPx->CSR, COMP_CSR_HYST, InputHysteresis);
}

/**
  * @brief  Get comparator instance hysteresis mode of the minus (inverting) input.
  * @rmtoll CSR      HYST           LL_COMP_GetInputHysteresis
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_HYSTERESIS_NONE
  *         @arg @ref LL_COMP_HYSTERESIS_LOW
  *         @arg @ref LL_COMP_HYSTERESIS_MEDIUM
  *         @arg @ref LL_COMP_HYSTERESIS_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputHysteresis(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_HYST));
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_output Configuration of comparator output
  * @{
  */

/**
  * @brief  Set comparator instance output polarity.
  * @rmtoll CSR      POLARITY       LL_COMP_SetOutputPolarity
  * @param  COMPx Comparator instance
  * @param  OutputPolarity This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputPolarity(COMP_TypeDef *COMPx, uint32_t OutputPolarity)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_POLARITY, OutputPolarity);
}

/**
  * @brief  Get comparator instance output polarity.
  * @rmtoll CSR      POLARITY       LL_COMP_GetOutputPolarity
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputPolarity(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_POLARITY));
}

/**
  * @brief  Set comparator instance blanking source.
  * @note   Blanking source may be specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @note   Availability of parameters of blanking source from timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      BLANKING       LL_COMP_SetOutputBlankingSource
  * @param  COMPx Comparator instance
  * @param  BlankingSource This parameter can be one of the following values:
  *         @arg @ref LL_COMP_BLANKINGSRC_NONE
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC4  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2 (1)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputBlankingSource(COMP_TypeDef *COMPx, uint32_t BlankingSource)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_BLANKING, BlankingSource);
}

/**
  * @brief  Get comparator instance blanking source.
  * @note   Availability of parameters of blanking source from timer
  *         depends on timers availability on the selected device.
  * @note   Blanking source may be specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      BLANKING       LL_COMP_GetOutputBlankingSource
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_BLANKINGSRC_NONE
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC4  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3  (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2 (1)
  *         
  *         (1) Parameter availability depending on timer availability
  *             on the selected device.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputBlankingSource(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_BLANKING));
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
*/
//#warning "Specify startup time naming in datasheet for the selected IP"
/*
  * @rmtoll CSR      EN             LL_COMP_Enable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  SET_BIT(COMPx->CSR, COMP_CSR_EN);
}

/**
  * @brief  Disable comparator instance.
  * @rmtoll CSR      EN             LL_COMP_Disable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Disable(COMP_TypeDef *COMPx)
{
  CLEAR_BIT(COMPx->CSR, COMP_CSR_EN);
}

/**
  * @brief  Get comparator enable state
  *         (0: COMP is disabled, 1: COMP is enabled)
  * @rmtoll CSR      EN             LL_COMP_IsEnabled
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsEnabled(COMP_TypeDef *COMPx)
{
  return ((READ_BIT(COMPx->CSR, COMP_CSR_EN) == (COMP_CSR_EN)) ? 1UL : 0UL);
}

/**
  * @brief  Lock comparator instance.
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll CSR      LOCK           LL_COMP_Lock
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Lock(COMP_TypeDef *COMPx)
{
  SET_BIT(COMPx->CSR, COMP_CSR_LOCK);
}

/**
  * @brief  Get comparator lock state
  *         (0: COMP is unlocked, 1: COMP is locked).
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll CSR      LOCK           LL_COMP_IsLocked
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsLocked(COMP_TypeDef *COMPx)
{
  return ((READ_BIT(COMPx->CSR, COMP_CSR_LOCK) == (COMP_CSR_LOCK)) ? 1UL : 0UL);
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
  * @rmtoll CSR      VALUE          LL_COMP_ReadOutputLevel
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_ReadOutputLevel(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_VALUE)
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

#endif /* STM32G0xx_LL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
