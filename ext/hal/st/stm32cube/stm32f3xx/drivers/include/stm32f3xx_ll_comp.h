/**
  ******************************************************************************
  * @file    stm32f3xx_ll_comp.h
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
#ifndef __STM32F3xx_LL_COMP_H
#define __STM32F3xx_LL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

/* Note: Devices of STM32F3 serie embed 1 out of 2 different comparator IP.   */
/*       - STM32F30x, STM32F31x, STM32F32x, STM32F33x, STM32F35x, STM32F39x:  */
/*         COMP IP from 3 to 7 instances and other specific features          */
/*         (comparator output blanking, ...) (refer to reference manual).     */
/*       - STM32F37x:                                                         */
/*         COMP IP with 2 instances                                           */
/*       This file contains the drivers of these COMP IP, located in 2 area    */
/*       delimited by compilation switches.                                   */

#if defined(COMP_V1_3_0_0)

#if defined (COMP1) || defined (COMP2) || defined (COMP3) || defined (COMP4) || defined (COMP5) || defined (COMP6) || defined (COMP7)

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
#define LL_COMP_OUTPUT_LEVEL_BITOFFSET_POS ((uint32_t)30U) /* Value equivalent to POSITION_VAL(COMPxOUT) */

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

  uint32_t InputHysteresis;             /*!< Set comparator hysteresis mode of the input minus.
                                             This parameter can be a value of @ref COMP_LL_EC_INPUT_HYSTERESIS

                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetInputHysteresis(). */

  uint32_t OutputSelection;             /*!< Set comparator output selection.
                                             This parameter can be a value of @ref COMP_LL_EC_OUTPUT_SELECTION

                                             This feature can be modified afterwards using unitary function @ref LL_COMP_SetOutputSelection(). */

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
#define LL_COMP_WINDOWMODE_DISABLE                 ((uint32_t)0x00000000U) /*!< Window mode disable: Comparators 1 and 2 are independent */
#if defined(COMP2_CSR_COMP2WNDWEN)
#define LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (COMP2_CSR_COMP2WNDWEN) /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP1 input plus (COMP2 input plus is no more accessible). */
#endif
#if defined(COMP4_CSR_COMP4WNDWEN)
#define LL_COMP_WINDOWMODE_COMP3_INPUT_PLUS_COMMON (COMP4_CSR_COMP4WNDWEN) /*!< Window mode enable: Comparators instances pair COMP3 and COMP4 have their input plus connected together. The common input is COMP3 input plus (COMP4 input plus is no more accessible). */
#endif
#if defined(COMP6_CSR_COMP6WNDWEN)
#define LL_COMP_WINDOWMODE_COMP5_INPUT_PLUS_COMMON (COMP6_CSR_COMP6WNDWEN) /*!< Window mode enable: Comparators instances pair COMP5 and COMP6 have their input plus connected together. The common input is COMP5 input plus (COMP6 input plus is no more accessible). */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_POWERMODE Comparator modes - Power mode
  * @{
  */
#define LL_COMP_POWERMODE_HIGHSPEED     ((uint32_t)0x00000000U)                       /*!< COMP power mode to high speed */
#if defined(COMP_CSR_COMPxMODE)
#define LL_COMP_POWERMODE_MEDIUMSPEED   (COMP_CSR_COMPxMODE_0)                        /*!< COMP power mode to medium speed */
#define LL_COMP_POWERMODE_LOWPOWER      (COMP_CSR_COMPxMODE_1)                        /*!< COMP power mode to low power */
#define LL_COMP_POWERMODE_ULTRALOWPOWER (COMP_CSR_COMPxMODE_1 | COMP_CSR_COMPxMODE_0) /*!< COMP power mode to ultra-low power */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_PLUS Comparator inputs - Input plus (input non-inverting) selection
  * @{
  */
#if !defined(COMP_CSR_COMPxNONINSEL)
#define LL_COMP_INPUT_PLUS_IO1          ((uint32_t)0x00000000U)  /*!< Comparator input plus connected to IO1 (pin PA1 for COMP1, PA3 for COMP2 (except STM32F334xx: PA7), PB14 for COMP3, PB0 for COMP4, PD12 for COMP5, PD11 for COMP6, PA0  for COMP7) (COMP instance availability depends on the selected device) */
#define LL_COMP_INPUT_PLUS_IO2          ((uint32_t)0x00000000U)  /*!< Comparator input plus connected to IO2: Same as IO1 */
#else
#define LL_COMP_INPUT_PLUS_IO1          ((uint32_t)0x00000000U)  /*!< Comparator input plus connected to IO1 (pin PA7 for COMP2, PB14 for COMP3, PB0 for COMP4, PD12 for COMP5, PD11 for COMP6, PA0  for COMP7) (COMP instance availability depends on the selected device) */
#define LL_COMP_INPUT_PLUS_IO2          (COMP_CSR_COMPxNONINSEL) /*!< Comparator input plus connected to IO2 (pin PA3 for COMP2, PD14 for COMP3, PE7 for COMP4, PB13 for COMP5, PB11 for COMP6, PC1 for COMP7) (COMP instance availability depends on the selected device) */
#endif
#if defined(STM32F302xC) || defined(STM32F302xE) || defined(STM32F303xC) || defined(STM32F303xE) || defined(STM32F358xx) || defined(STM32F398xx)
#define LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 (COMP_CSR_COMPxSW1)    /*!< Comparator input plus connected to DAC1 channel 1 (DAC_OUT1), through dedicated switch (Note: this switch is solely intended to redirect signals onto high impedance input, such as COMP1 input plus (highly resistive switch)) (specific to COMP instance: COMP1) */

/* Note: Comparator input plus specific to COMP instances, defined with       */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_INPUT_PLUS_DAC1_CH1     LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1      /*!< Comparator input plus connected to DAC1 channel 1 (DAC_OUT1), through dedicated switch. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F301x8) || defined(STM32F318xx) || defined(STM32F302x8)
#define LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2 (COMP_CSR_COMPxSW1)    /*!< Comparator input plus connected to DAC1 channel 1 (DAC_OUT1), through dedicated switch (Note: this switch is solely intended to redirect signals onto high impedance input, such as COMP2 input plus (highly resistive switch)) (specific to COMP instance: COMP2) */

/* Note: Comparator input plus specific to COMP instances, defined with       */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_INPUT_PLUS_DAC1_CH1     LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2      /*!< Comparator input plus connected to DAC1 channel 1 (DAC_OUT1), through dedicated switch. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_MINUS Comparator inputs - Input minus (input inverting) selection
  * @{
  */
#define LL_COMP_INPUT_MINUS_1_4VREFINT  ((uint32_t)0x00000000U)                                                /*!< Comparator input minus connected to 1/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_1_2VREFINT  (                                               COMP_CSR_COMPxINSEL_0) /*!< Comparator input minus connected to 1/2 VrefInt  */
#define LL_COMP_INPUT_MINUS_3_4VREFINT  (                       COMP_CSR_COMPxINSEL_1                        ) /*!< Comparator input minus connected to 3/4 VrefInt  */
#define LL_COMP_INPUT_MINUS_VREFINT     (                       COMP_CSR_COMPxINSEL_1 | COMP_CSR_COMPxINSEL_0) /*!< Comparator input minus connected to VrefInt */
#define LL_COMP_INPUT_MINUS_DAC1_CH1    (COMP_CSR_COMPxINSEL_2                                               ) /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1)  */
#if defined(STM32F301x8) || defined(STM32F318xx) || defined(STM32F302x8) || defined(STM32F302xC) || defined(STM32F302xE)
/* This device has no comparator input minus DAC1_CH2 */
#else
#define LL_COMP_INPUT_MINUS_DAC1_CH2    (COMP_CSR_COMPxINSEL_2                        | COMP_CSR_COMPxINSEL_0) /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2)  */
#endif
#if defined(STM32F301x8) || defined(STM32F318xx) || defined(STM32F334x8)
#define LL_COMP_INPUT_MINUS_IO1         (COMP_CSR_COMPxINSEL_2 | COMP_CSR_COMPxINSEL_1                        ) /*!< Comparator input minus connected to IO1 (pin PA2 for COMP2) */
#else
#define LL_COMP_INPUT_MINUS_IO1         (COMP_CSR_COMPxINSEL_2 | COMP_CSR_COMPxINSEL_1                        ) /*!< Comparator input minus connected to IO1 (pin PA0 for COMP1, pin PA2 for COMP2, PD15 for COMP3, PE8 for COMP4, PD13 for COMP5, PD10 for COMP6, PC0 for COMP7 (COMP instance availability depends on the selected device)) */
#endif
#define LL_COMP_INPUT_MINUS_IO2         (COMP_CSR_COMPxINSEL_2 | COMP_CSR_COMPxINSEL_1 | COMP_CSR_COMPxINSEL_0) /*!< Comparator input minus connected to IO2 (PB12 for COMP3, PB2 for COMP4, PB10 for COMP5, PB15 for COMP6 (COMP instance availability depends on the selected device)) */
#if defined(STM32F301x8) || defined(STM32F318xx) || defined(STM32F334x8) || defined(STM32F302x8) || defined(STM32F303x8) || defined(STM32F328xx)
/* This device has no comparator input minus IO3 */
#else
#define LL_COMP_INPUT_MINUS_IO3         (COMP_CSR_COMPxINSEL_2                         | COMP_CSR_COMPxINSEL_0) /*!< Comparator input minus connected to IO3 (pin PA5 for COMP1/2/3/4/5/6/7 (COMP instance availability depends on the selected device)) */
#endif
#define LL_COMP_INPUT_MINUS_IO4         (COMP_CSR_COMPxINSEL_2                                                ) /*!< Comparator input minus connected to IO4 (pin PA4 for COMP1/2/3/4/5/6/7 (COMP instance availability depends on the selected device)) */
#if defined(STM32F303x8) || defined(STM32F328xx) || defined(STM32F334x8)
#define LL_COMP_INPUT_MINUS_DAC2_CH1    (COMP_CSR_COMPxINSEL_3                                                ) /*!< Comparator input minus connected to DAC2 channel 1 (DAC2_OUT1)  */
#else
/* This device has no comparator input minus DAC2_CH1 */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_INPUT_HYSTERESIS Comparator input - Hysteresis
  * @{
  */
#define LL_COMP_HYSTERESIS_NONE         ((uint32_t)0x00000000U)                       /*!< No hysteresis */
#if defined(COMP_CSR_COMPxHYST)
#define LL_COMP_HYSTERESIS_LOW          (                       COMP_CSR_COMPxHYST_0) /*!< Hysteresis level low (available only on devices: STM32F303xB/C, STM32F358xC) */
#define LL_COMP_HYSTERESIS_MEDIUM       (COMP_CSR_COMPxHYST_1                       ) /*!< Hysteresis level medium (available only on devices: STM32F303xB/C, STM32F358xC) */
#define LL_COMP_HYSTERESIS_HIGH         (COMP_CSR_COMPxHYST_1 | COMP_CSR_COMPxHYST_0) /*!< Hysteresis level high (available only on devices: STM32F303xB/C, STM32F358xC) */
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_SELECTION Comparator output - Output selection
  * @{
  */
#define LL_COMP_OUTPUT_NONE             ((uint32_t)0x00000000)                                                      /*!< COMP output is not connected to other peripherals (except GPIO and EXTI that are always connected to COMP output) (specific to COMP instance: COMP2) */
#if defined(COMP_CSR_COMPxOUT)
/* Note: Output redirection common to all COMP instances, all STM32F3 serie   */
/*       devices.                                                             */
#define LL_COMP_OUTPUT_TIM1_BKIN        (COMP_CSR_COMPxOUTSEL_0)                                                    /*!< COMP output connected to TIM1 break input (BKIN) */
#define LL_COMP_OUTPUT_TIM1_BKIN2       (COMP_CSR_COMPxOUTSEL_1)                                                    /*!< COMP output connected to TIM1 break input 2 (BKIN2) */

#if defined(STM32F301x8) || defined(STM32F318xx)
/* Note: Output redirection specific to COMP instance: COMP2 */
#define LL_COMP_OUTPUT_TIM1_IC1_COMP2    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM1 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_IC4_COMP2    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM1 input capture 4 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM1_OCCLR_COMP2  (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM1 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP2  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP2) */
/* Note: Output redirection specific to COMP instance: COMP4 */
#define LL_COMP_OUTPUT_TIM15_IC2_COMP4   (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM15 input capture 1 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_OCCLR_COMP4 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM15 OCREF clear (specific to COMP instance: COMP4) */
/* Note: Output redirection specific to COMP instance: COMP6 */
#define LL_COMP_OUTPUT_TIM2_IC2_COMP6    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM2 input capture 2 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP6  (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_IC1_COMP6   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM16 input capture 1 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_OCCLR_COMP6 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM16 OCREF clear (specific to COMP instance: COMP6) */

/* Note: Output redirection specific to COMP instances, defined with          */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
/* Note: Some output redirections cannot have a generic naming,               */
/*       due to literal value different depending on COMP instance.           */
/*       (For exemple: LL_COMP_OUTPUT_TIM2_OCCLR_COMP2 and                    */
/*       LL_COMP_OUTPUT_TIM2_OCCLR_COMP6).                                    */
#define LL_COMP_OUTPUT_TIM1_IC1          LL_COMP_OUTPUT_TIM1_IC1_COMP2         /*!< COMP output connected to TIM1 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM1_OCCLR        LL_COMP_OUTPUT_TIM1_OCCLR_COMP2       /*!< COMP output connected to TIM1 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC2          LL_COMP_OUTPUT_TIM2_IC2_COMP6         /*!< COMP output connected to TIM2 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC4          LL_COMP_OUTPUT_TIM2_IC4_COMP2         /*!< COMP output connected to TIM2 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_IC2         LL_COMP_OUTPUT_TIM15_IC2_COMP4        /*!< COMP output connected to TIM15 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_OCCLR       LL_COMP_OUTPUT_TIM15_OCCLR_COMP4      /*!< COMP output connected to TIM15 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_IC1         LL_COMP_OUTPUT_TIM16_IC1_COMP6        /*!< COMP output connected to TIM16 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_OCCLR       LL_COMP_OUTPUT_TIM16_OCCLR_COMP6      /*!< COMP output connected to TIM16 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
/* Note: Output redirection specific to COMP instances, defined with          */
/*       partially generic naming grouping COMP instance constraints.         */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3  LL_COMP_OUTPUT_TIM2_OCCLR_COMP2   /*!< COMP output connected to TIM2 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F303x8) || defined(STM32F328xx) || defined(STM32F334x8)|| defined(STM32F302x8)
/* Note: Output redirection specific to COMP instance: COMP2, COMP4 */
#define LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear (specific to COMP instance: COMP2, COMP4) */
/* Note: Output redirection specific to COMP instance: COMP2 */
#define LL_COMP_OUTPUT_TIM1_IC1_COMP2    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM1 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_IC4_COMP2    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM1 input capture 4 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM1_OCCLR_COMP2  (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM1 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP2  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_IC1_COMP2    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 input capture 1 (specific to COMP instance: COMP2) */
/* Note: Output redirection specific to COMP instance: COMP4 */
#define LL_COMP_OUTPUT_TIM3_IC3_COMP4    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM3 input capture 3 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_IC2_COMP4   (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM15 input capture 1 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_OCCLR_COMP4 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM15 OCREF clear (specific to COMP instance: COMP4) */
/* Note: Output redirection specific to COMP instance: COMP6 */
#define LL_COMP_OUTPUT_TIM2_IC2_COMP6    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM2 input capture 2 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP6  (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_IC1_COMP6   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM16 input capture 1 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_OCCLR_COMP6 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM16 OCREF clear (specific to COMP instance: COMP6) */

/* Note: Output redirection specific to COMP instances, defined with          */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
/* Note: Some output redirections cannot have a generic naming,               */
/*       due to literal value different depending on COMP instance.           */
/*       (For exemple: LL_COMP_OUTPUT_TIM2_OCCLR_COMP2 and                    */
/*       LL_COMP_OUTPUT_TIM2_OCCLR_COMP6).                                    */
#define LL_COMP_OUTPUT_TIM1_IC1          LL_COMP_OUTPUT_TIM1_IC1_COMP2         /*!< COMP output connected to TIM1 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM1_OCCLR        LL_COMP_OUTPUT_TIM1_OCCLR_COMP2       /*!< COMP output connected to TIM1 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC2          LL_COMP_OUTPUT_TIM2_IC2_COMP6         /*!< COMP output connected to TIM2 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC4          LL_COMP_OUTPUT_TIM2_IC4_COMP2         /*!< COMP output connected to TIM2 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC1          LL_COMP_OUTPUT_TIM3_IC1_COMP2         /*!< COMP output connected to TIM3 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC3          LL_COMP_OUTPUT_TIM3_IC3_COMP4         /*!< COMP output connected to TIM3 input capture 3.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_OCCLR        LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4     /*!< COMP output connected to TIM3 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_IC2         LL_COMP_OUTPUT_TIM15_IC2_COMP4        /*!< COMP output connected to TIM15 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_OCCLR       LL_COMP_OUTPUT_TIM15_OCCLR_COMP4      /*!< COMP output connected to TIM15 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_IC1         LL_COMP_OUTPUT_TIM16_IC1_COMP6        /*!< COMP output connected to TIM16 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_OCCLR       LL_COMP_OUTPUT_TIM16_OCCLR_COMP6      /*!< COMP output connected to TIM16 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
/* Note: Output redirection specific to COMP instances, defined with          */
/*       partially generic naming grouping COMP instance constraints.         */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3  LL_COMP_OUTPUT_TIM2_OCCLR_COMP2   /*!< COMP output connected to TIM2 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F302xC) || defined(STM32F302xE)
/* Note: Output redirection specific to COMP instance: COMP1, COMP2, COMP4 */
#define LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear (specific to COMP instance: COMP2, COMP4) */
/* Note: Output redirection specific to COMP instance: COMP1, COMP2 */
#define LL_COMP_OUTPUT_TIM1_IC1_COMP1_2    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM1 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_IC4_COMP1_2    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 input capture 4 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2  (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM1 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_IC1_COMP1_2    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 input capture 1 (specific to COMP instance: COMP2) */
/* Note: Output redirection specific to COMP instance: COMP4 */
#define LL_COMP_OUTPUT_TIM3_IC3_COMP4      (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM3 input capture 3 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM4_IC2_COMP4      (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM4 input capture 2 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_IC2_COMP4     (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM15 input capture 1 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_OCCLR_COMP4   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM15 OCREF clear (specific to COMP instance: COMP4) */
/* Note: Output redirection specific to COMP instance: COMP6 */
#define LL_COMP_OUTPUT_TIM2_IC2_COMP6      (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM2 input capture 2 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP6    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM4_IC4_COMP6      (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM4 input capture 4 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_IC1_COMP6     (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM16 input capture 1 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_OCCLR_COMP6   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM16 OCREF clear (specific to COMP instance: COMP6) */

/* Note: Output redirection specific to COMP instances, defined with          */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
/* Note: Some output redirections cannot have a generic naming,               */
/*       due to literal value different depending on COMP instance.           */
/*       (For exemple: LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2 and                  */
/*       LL_COMP_OUTPUT_TIM2_OCCLR_COMP6).                                    */
#define LL_COMP_OUTPUT_TIM1_IC1          LL_COMP_OUTPUT_TIM1_IC1_COMP1_2       /*!< COMP output connected to TIM1 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM1_OCCLR        LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2     /*!< COMP output connected to TIM1 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC2          LL_COMP_OUTPUT_TIM2_IC2_COMP6         /*!< COMP output connected to TIM2 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC4          LL_COMP_OUTPUT_TIM2_IC4_COMP1_2       /*!< COMP output connected to TIM2 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC1          LL_COMP_OUTPUT_TIM3_IC1_COMP1_2       /*!< COMP output connected to TIM3 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC3          LL_COMP_OUTPUT_TIM3_IC3_COMP4         /*!< COMP output connected to TIM3 input capture 3.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_OCCLR        LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4   /*!< COMP output connected to TIM3 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC2          LL_COMP_OUTPUT_TIM4_IC2_COMP4         /*!< COMP output connected to TIM4 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC4          LL_COMP_OUTPUT_TIM4_IC4_COMP6         /*!< COMP output connected to TIM4 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_IC2         LL_COMP_OUTPUT_TIM15_IC2_COMP4        /*!< COMP output connected to TIM15 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_OCCLR       LL_COMP_OUTPUT_TIM15_OCCLR_COMP4      /*!< COMP output connected to TIM15 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_IC1         LL_COMP_OUTPUT_TIM16_IC1_COMP6        /*!< COMP output connected to TIM16 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_OCCLR       LL_COMP_OUTPUT_TIM16_OCCLR_COMP6      /*!< COMP output connected to TIM16 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
/* Note: Output redirection specific to COMP instances, defined with          */
/*       partially generic naming grouping COMP instance constraints.         */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3  LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2   /*!< COMP output connected to TIM2 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F303xC) || defined(STM32F358xx) || defined(STM32F303xE) || defined(STM32F398xx)
/* Note: Output redirection common to all COMP instances */
#define LL_COMP_OUTPUT_TIM8_BKIN         (COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM8 break input (BKIN) */
#define LL_COMP_OUTPUT_TIM8_BKIN2        (COMP_CSR_COMPxOUTSEL_2)                                                   /*!< COMP output connected to TIM8 break input 2 (BKIN2) */
#define LL_COMP_OUTPUT_TIM1_TIM8_BKIN2   (COMP_CSR_COMPxOUTSEL_2| COMP_CSR_COMPxOUTSEL_0)                           /*!< COMP output connected to TIM1 break input 2 and TIM8 break input 2 (BKIN2) */
#if defined(STM32F303xE) || defined(STM32F398xx)
#define LL_COMP_OUTPUT_TIM20_BKIN        (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_2)                          /*!< COMP output connected to TIM8 break input (BKIN) */
#define LL_COMP_OUTPUT_TIM20_BKIN2       (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM8 break input 2 (BKIN2) */
#define LL_COMP_OUTPUT_TIM1_TIM8_TIM20_BKIN2 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_2| COMP_CSR_COMPxOUTSEL_1) /*!< COMP output connected to TIM1 break input 2, TIM8 break input 2 and TIM20 break input 2 (BKIN2) */
#endif
/* Note: Output redirection specific to COMP instance: COMP1, COMP2, COMP3, COMP7 */
#define LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7 (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM1 OCREF clear (specific to COMP instance: COMP1, COMP2, COMP3, COMP7) */
/* Note: Output redirection specific to COMP instance: COMP1, COMP2, COMP3 */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP1, COMP2, COMP3) */
/* Note: Output redirection specific to COMP instance: COMP1, COMP2, COMP4, COMP5 */
#define LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear (specific to COMP instance: COMP1, COMP2, COMP4, COMP5) */
/* Note: Output redirection specific to COMP instance: COMP4, COMP5, COMP6, COMP7 */
#define LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7 (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM8 OCREF clear (specific to COMP instance: COMP4, COMP5, COMP6, COMP7) */
/* Note: Output redirection specific to COMP instance: COMP1, COMP2 */
#define LL_COMP_OUTPUT_TIM1_IC1_COMP1_2  (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM1 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM2_IC4_COMP1_2  (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 input capture 4 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_IC1_COMP1_2  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM3 input capture 1 (specific to COMP instance: COMP2) */
#if defined(STM32F303xE) || defined(STM32F398xx)
#define LL_COMP_OUTPUT_TIM20_OCCLR_COMP2 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM20 OCREF clear (specific to COMP instance: COMP2) */
#endif
/* Note: Output redirection specific to COMP instance: COMP3 */
#define LL_COMP_OUTPUT_TIM3_IC2_COMP3    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM3 input capture 2 (specific to COMP instance: COMP3) */
#define LL_COMP_OUTPUT_TIM4_IC1_COMP3    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM4 input capture 1 (specific to COMP instance: COMP3) */
#define LL_COMP_OUTPUT_TIM15_IC1_COMP3   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM15 input capture 1 (specific to COMP instance: COMP3) */
#define LL_COMP_OUTPUT_TIM15_BKIN_COMP3  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM15 break input (BKIN) */
/* Note: Output redirection specific to COMP instance: COMP4 */
#define LL_COMP_OUTPUT_TIM3_IC3_COMP4    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM3 input capture 3 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM4_IC2_COMP4    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM4 input capture 2 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_IC2_COMP4   (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM15 input capture 1 (specific to COMP instance: COMP4) */
#define LL_COMP_OUTPUT_TIM15_OCCLR_COMP4 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM15 OCREF clear (specific to COMP instance: COMP4) */
/* Note: Output redirection specific to COMP instance: COMP5 */
#define LL_COMP_OUTPUT_TIM2_IC1_COMP5    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM2 input capture 1 (specific to COMP instance: COMP5) */
#define LL_COMP_OUTPUT_TIM4_IC3_COMP5    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM4 input capture 3 (specific to COMP instance: COMP5) */
#define LL_COMP_OUTPUT_TIM17_IC1_COMP5   (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM17 input capture 1 (specific to COMP instance: COMP5) */
#define LL_COMP_OUTPUT_TIM16_BKIN_COMP5  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM16 break input (BKIN) */
/* Note: Output redirection specific to COMP instance: COMP6 */
#define LL_COMP_OUTPUT_TIM2_IC2_COMP6    (COMP_CSR_COMPxOUTSEL_2 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM2 input capture 2 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM2_OCCLR_COMP6  (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM2 OCREF clear (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM4_IC4_COMP6    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM4 input capture 4 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_IC1_COMP6   (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM16 input capture 1 (specific to COMP instance: COMP6) */
#define LL_COMP_OUTPUT_TIM16_OCCLR_COMP6 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM16 OCREF clear (specific to COMP instance: COMP6) */
/* Note: Output redirection specific to COMP instance: COMP7 */
#define LL_COMP_OUTPUT_TIM1_IC2_COMP7    (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_0)                          /*!< COMP output connected to TIM2 input capture 1 (specific to COMP instance: COMP7) */
#define LL_COMP_OUTPUT_TIM2_IC3_COMP7    (COMP_CSR_COMPxOUTSEL_3)                                                   /*!< COMP output connected to TIM4 input capture 3 (specific to COMP instance: COMP7) */
#define LL_COMP_OUTPUT_TIM17_OCCLR_COMP7 (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1)                          /*!< COMP output connected to TIM17 OCREF clear (specific to COMP instance: COMP7) */
#define LL_COMP_OUTPUT_TIM17_BKIN_COMP7  (COMP_CSR_COMPxOUTSEL_3 | COMP_CSR_COMPxOUTSEL_1 | COMP_CSR_COMPxOUTSEL_0) /*!< COMP output connected to TIM17 break input (BKIN) */

/* Note: Output redirection specific to COMP instances, defined with          */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
/* Note: Some output redirections cannot have a generic naming,               */
/*       due to literal value different depending on COMP instance.           */
/*       (For exemple: LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3 and                */
/*       LL_COMP_OUTPUT_TIM2_OCCLR_COMP6).                                    */
#define LL_COMP_OUTPUT_TIM1_IC1          LL_COMP_OUTPUT_TIM1_IC1_COMP1_2       /*!< COMP output connected to TIM1 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM1_IC2          LL_COMP_OUTPUT_TIM1_IC2_COMP7         /*!< COMP output connected to TIM2 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM1_OCCLR        LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7 /*!< COMP output connected to TIM1 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC1          LL_COMP_OUTPUT_TIM2_IC1_COMP5         /*!< COMP output connected to TIM2 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC2          LL_COMP_OUTPUT_TIM2_IC2_COMP6         /*!< COMP output connected to TIM2 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC3          LL_COMP_OUTPUT_TIM2_IC3_COMP7         /*!< COMP output connected to TIM4 input capture 3.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM2_IC4          LL_COMP_OUTPUT_TIM2_IC4_COMP1_2       /*!< COMP output connected to TIM2 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC1          LL_COMP_OUTPUT_TIM3_IC1_COMP1_2       /*!< COMP output connected to TIM3 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC2          LL_COMP_OUTPUT_TIM3_IC2_COMP3         /*!< COMP output connected to TIM3 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_IC3          LL_COMP_OUTPUT_TIM3_IC3_COMP4         /*!< COMP output connected to TIM3 input capture 3.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM3_OCCLR        LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5 /*!< COMP output connected to TIM3 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC1          LL_COMP_OUTPUT_TIM4_IC1_COMP3         /*!< COMP output connected to TIM4 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC2          LL_COMP_OUTPUT_TIM4_IC2_COMP4         /*!< COMP output connected to TIM4 input capture 2.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC3          LL_COMP_OUTPUT_TIM4_IC3_COMP5         /*!< COMP output connected to TIM4 input capture 3.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC4          LL_COMP_OUTPUT_TIM4_IC4_COMP6         /*!< COMP output connected to TIM4 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM8_OCCLR        LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7 /*!< COMP output connected to TIM8 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_IC1         LL_COMP_OUTPUT_TIM15_IC1_COMP3        /*!< COMP output connected to TIM15 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_IC2         LL_COMP_OUTPUT_TIM15_IC2_COMP4        /*!< COMP output connected to TIM15 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_BKIN        LL_COMP_OUTPUT_TIM15_BKIN_COMP3       /*!< COMP output connected to TIM15 break input (BKIN). Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM15_OCCLR       LL_COMP_OUTPUT_TIM15_OCCLR_COMP4      /*!< COMP output connected to TIM15 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_IC1         LL_COMP_OUTPUT_TIM16_IC1_COMP6        /*!< COMP output connected to TIM16 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_BKIN        LL_COMP_OUTPUT_TIM16_BKIN_COMP5       /*!< COMP output connected to TIM16 break input (BKIN). Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_OCCLR       LL_COMP_OUTPUT_TIM16_OCCLR_COMP6      /*!< COMP output connected to TIM16 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM17_IC1         LL_COMP_OUTPUT_TIM17_IC1_COMP5        /*!< COMP output connected to TIM17 input capture 1.    Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM17_BKIN        LL_COMP_OUTPUT_TIM17_BKIN_COMP7       /*!< COMP output connected to TIM17 break input (BKIN). Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM17_OCCLR       LL_COMP_OUTPUT_TIM17_OCCLR_COMP7      /*!< COMP output connected to TIM17 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#if defined(STM32F303xE) || defined(STM32F398xx)
#define LL_COMP_OUTPUT_TIM20_OCCLR       LL_COMP_OUTPUT_TIM20_OCCLR_COMP2      /*!< COMP output connected to TIM20 OCREF clear.        Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#endif

#endif
#endif
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_POLARITY Comparator output - Output polarity
  * @{
  */
#define LL_COMP_OUTPUTPOL_NONINVERTED   ((uint32_t)0x00000000U) /*!< COMP output polarity is not inverted: comparator output is high when the plus (non-inverting) input is at a higher voltage than the minus (inverting) input */
#define LL_COMP_OUTPUTPOL_INVERTED      (COMP_CSR_COMPxPOL)     /*!< COMP output polarity is inverted: comparator output is low when the plus (non-inverting) input is at a lower voltage than the minus (inverting) input */
/**
  * @}
  */

/** @defgroup COMP_LL_EC_OUTPUT_BLANKING_SOURCE Comparator output - Blanking source
  * @{
  */
#define LL_COMP_BLANKINGSRC_NONE        ((uint32_t)0x00000000U)                                   /*!<Comparator output without blanking */
#if defined(COMP_CSR_COMPxBLANKING)
#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/* Note: Output blanking source specific to COMP instance: COMP2 */
#define LL_COMP_BLANKINGSRC_TIM1_OC5_COMP2  (COMP_CSR_COMPxBLANKING_0)                            /*!< Comparator output blanking source TIM1 OC5 (specific to COMP instance: COMP2) */
#define LL_COMP_BLANKINGSRC_TIM2_OC3_COMP2  (COMP_CSR_COMPxBLANKING_1)                            /*!< Comparator output blanking source TIM2 OC3 (specific to COMP instance: COMP2) */
#define LL_COMP_BLANKINGSRC_TIM3_OC3_COMP2  (COMP_CSR_COMPxBLANKING_1 | COMP_CSR_COMPxBLANKING_0) /*!< Comparator output blanking source TIM3 OC3 (specific to COMP instance: COMP2) */
/* Note: Output blanking source specific to COMP instance: COMP4 */
#define LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4  (COMP_CSR_COMPxBLANKING_0)                            /*!< Comparator output blanking source TIM3 OC4 (specific to COMP instance: COMP4) */
#define LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4 (COMP_CSR_COMPxBLANKING_0 | COMP_CSR_COMPxBLANKING_1) /*!< Comparator output blanking source TIM15 OC1 (specific to COMP instance: COMP4) */
/* Note: Output blanking source specific to COMP instance: COMP6 */
#define LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6  (COMP_CSR_COMPxBLANKING_0 | COMP_CSR_COMPxBLANKING_1) /*!< Comparator output blanking source TIM2 OC4 (specific to COMP instance: COMP6) */
#define LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6 (COMP_CSR_COMPxBLANKING_0)                            /*!< Comparator output blanking source TIM15 OC2 (specific to COMP instance: COMP6) */

/* Note: Output blanking source specific to COMP instances, defined with      */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_BLANKINGSRC_TIM1_OC5     LL_COMP_BLANKINGSRC_TIM1_OC5_COMP2                        /*!< Comparator output blanking source TIM1 OC5.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC3     LL_COMP_BLANKINGSRC_TIM2_OC3_COMP2                        /*!< Comparator output blanking source TIM2 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC4     LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6                        /*!< Comparator output blanking source TIM2 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC3     LL_COMP_BLANKINGSRC_TIM3_OC3_COMP2                        /*!< Comparator output blanking source TIM3 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC4     LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4                        /*!< Comparator output blanking source TIM3 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC1    LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4                       /*!< Comparator output blanking source TIM15 OC1. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC2    LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6                       /*!< Comparator output blanking source TIM15 OC2. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F302xE) || defined(STM32F302xC)
/* Note: Output blanking source specific to COMP instance: COMP1, COMP2 */
#define LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2 (COMP_CSR_COMPxBLANKING_0)                            /*!< Comparator output blanking source TIM1 OC5 (specific to COMP instance: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2 (COMP_CSR_COMPxBLANKING_1)                            /*!< Comparator output blanking source TIM2 OC3 (specific to COMP instance: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2 (COMP_CSR_COMPxBLANKING_1 | COMP_CSR_COMPxBLANKING_0) /*!< Comparator output blanking source TIM3 OC3 (specific to COMP instance: COMP1, COMP2) */
/* Note: Output blanking source specific to COMP instance: COMP4 */
#define LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4  (COMP_CSR_COMPxBLANKING_0)                             /*!< Comparator output blanking source TIM3 OC4 (specific to COMP instance: COMP4) */
#define LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4 (COMP_CSR_COMPxBLANKING_0 | COMP_CSR_COMPxBLANKING_1)  /*!< Comparator output blanking source TIM15 OC1 (specific to COMP instance: COMP4) */
/* Note: Output blanking source specific to COMP instance: COMP6 */
#define LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6  (COMP_CSR_COMPxBLANKING_0 | COMP_CSR_COMPxBLANKING_1)  /*!< Comparator output blanking source TIM2 OC4 (specific to COMP instance: COMP6) */
#define LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6 (COMP_CSR_COMPxBLANKING_0)                             /*!< Comparator output blanking source TIM15 OC2 (specific to COMP instance: COMP6) */

/* Note: Output blanking source specific to COMP instances, defined with      */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_BLANKINGSRC_TIM1_OC5     LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2                      /*!< Comparator output blanking source TIM1 OC5.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC3     LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2                      /*!< Comparator output blanking source TIM2 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC4     LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6                        /*!< Comparator output blanking source TIM2 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC3     LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2                      /*!< Comparator output blanking source TIM3 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC4     LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4                        /*!< Comparator output blanking source TIM3 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC1    LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4                       /*!< Comparator output blanking source TIM15 OC1. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC2    LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6                       /*!< Comparator output blanking source TIM15 OC2. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#elif defined(STM32F303xE) || defined(STM32F398xx) || defined(STM32F303xC) || defined(STM32F358xx)
/* Note: Output blanking source specific to COMP instance: COMP1, COMP2, COMP7 */
#define LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7 (COMP_CSR_COMPxBLANKING_0)                          /*!< Comparator output blanking source TIM1 OC5 (specific to COMP instance: COMP1, COMP2, COMP7) */
/* Note: Output blanking source specific to COMP instance: COMP1, COMP2 */
#define LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2 (COMP_CSR_COMPxBLANKING_1)                            /*!< Comparator output blanking source TIM2 OC3 (specific to COMP instance: COMP1, COMP2) */
#define LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2 (COMP_CSR_COMPxBLANKING_1 | COMP_CSR_COMPxBLANKING_0) /*!< Comparator output blanking source TIM3 OC3 (specific to COMP instance: COMP1, COMP2) */
/* Note: Output blanking source specific to COMP instance: COMP3, COMP6 */
#define LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6 (COMP_CSR_COMPxBLANKING_1 | COMP_CSR_COMPxBLANKING_0) /*!< Comparator output blanking source TIM2 OC4 (specific to COMP instance: COMP3, COMP6) */
/* Note: Output blanking source specific to COMP instance: COMP4, COMP5, COMP6, COMP7 */
#define LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7 (COMP_CSR_COMPxBLANKING_1)                        /*!< Comparator output blanking source TIM8 OC5 (specific to COMP instance: COMP4, COMP5, COMP6, COMP7) */
/* Note: Output blanling source specific to COMP instance: COMP6, COMP7 */
#define LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7 (COMP_CSR_COMPxBLANKING_2)                           /*!< Comparator output blanking source TIM15 OC2 (specific to COMP instance: COMP6, COMP7) */
/* Note: Output blanking source specific to COMP instance: COMP4 */
#define LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4   (COMP_CSR_COMPxBLANKING_0)                            /*!< Comparator output blanking source TIM3 OC4 (specific to COMP instance: COMP4) */
#define LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4  (COMP_CSR_COMPxBLANKING_1 | COMP_CSR_COMPxBLANKING_0) /*!< Comparator output blanking source TIM15 OC1 (specific to COMP instance: COMP4) */

/* Note: Output blanking source specific to COMP instances, defined with      */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
#define LL_COMP_BLANKINGSRC_TIM1_OC5     LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7                    /*!< Comparator output blanking source TIM1 OC5.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC3     LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2                      /*!< Comparator output blanking source TIM2 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM2_OC4     LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6                      /*!< Comparator output blanking source TIM2 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC3     LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2                      /*!< Comparator output blanking source TIM3 OC3.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM3_OC4     LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4                        /*!< Comparator output blanking source TIM3 OC4.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM8_OC5     LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7                  /*!< Comparator output blanking source TIM8 OC5.  Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC1    LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4                       /*!< Comparator output blanking source TIM15 OC1. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_BLANKINGSRC_TIM15_OC2    LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7                     /*!< Comparator output blanking source TIM15 OC2. Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */

#endif
#endif
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
#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)
#define LL_COMP_DELAY_STARTUP_US          ((uint32_t) 60U)  /*!< Delay for COMP startup time */
#else
#define LL_COMP_DELAY_STARTUP_US          ((uint32_t) 10U)  /*!< Delay for COMP startup time */
#endif

/* Delay for comparator voltage scaler stabilization time.                    */
/* Note: Voltage scaler is used when selecting comparator input               */
/*       based on VrefInt: VrefInt or subdivision of VrefInt.                 */
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
#if defined(COMP1) && defined(COMP2) && defined(COMP3) && defined(COMP4) && defined(COMP5) && defined(COMP6) && defined(COMP7)
/* Note: On STM32F3 serie devices with 7 comparator instances,                */
/*       COMP instance COMP7 has no other comparator instance to work         */
/*       in pair with: window mode is not available for COMP7.                */
#define __LL_COMP_COMMON_INSTANCE(__COMPx__)                                   \
  ((((__COMPx__) == COMP1) || ((__COMPx__) == COMP2))                          \
    ? (                                                                        \
       (COMP12_COMMON)                                                         \
      )                                                                        \
      :                                                                        \
      ((((__COMPx__) == COMP3) || ((__COMPx__) == COMP4))                      \
        ? (                                                                    \
           (COMP34_COMMON)                                                     \
          )                                                                    \
          :                                                                    \
          ((((__COMPx__) == COMP5) || ((__COMPx__) == COMP6))                  \
            ? (                                                                \
               (COMP56_COMMON)                                                 \
              )                                                                \
              :                                                                \
              (                                                                \
               ((uint32_t)0U)                                                  \
              )                                                                \
          )                                                                    \
      )                                                                        \
  )
#elif defined(COMP1) && defined(COMP2) && defined(COMP4) && defined(COMP6)
#define __LL_COMP_COMMON_INSTANCE(__COMPx__)                                   \
  ((((__COMPx__) == COMP1) || ((__COMPx__) == COMP2))                          \
    ? (                                                                        \
       (COMP12_COMMON)                                                         \
      )                                                                        \
      :                                                                        \
      (                                                                        \
       ((uint32_t)0U)                                                          \
      )                                                                        \
  )
#elif defined(COMP2) && defined(COMP4) && defined(COMP6)
/* Note: On STM32F3 serie devices with 3 comparator instances (COMP2, 4, 6)   */
/*       COMP instances have no other comparator instance to work             */
/*       in pair with: window mode is not available for all COMP instances.   */
#define __LL_COMP_COMMON_INSTANCE(__COMPx__)                                   \
  ((uint32_t)0U)
#endif

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
  * @rmtoll CSR      COMPxWNDWEN    LL_COMP_SetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @param  WindowMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (1)
  *         @arg @ref LL_COMP_WINDOWMODE_COMP3_INPUT_PLUS_COMMON (2)
  *         @arg @ref LL_COMP_WINDOWMODE_COMP5_INPUT_PLUS_COMMON (2)
  *
  *         (1) Parameter available on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  *         (2) Parameter available on devices: STM32F303xB/C, STM32F358xC
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON, uint32_t WindowMode)
{
#if defined(COMP_CSR_COMPxWNDWEN)
  MODIFY_REG(COMPxy_COMMON->CSR, COMP_CSR_COMPxWNDWEN, WindowMode);
#else
  /* Device without pair of comparator working in window mode */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_WINDOWMODE_DISABLE").                                           */
#endif
}

/**
  * @brief  Get window mode of a pair of comparators instances
  *         (2 consecutive COMP instances odd and even COMP<x> and COMP<x+1>).
  * @rmtoll CSR      COMPxWNDWEN    LL_COMP_GetCommonWindowMode
  * @param  COMPxy_COMMON Comparator common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_COMP_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_WINDOWMODE_DISABLE
  *         @arg @ref LL_COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (1)
  *         @arg @ref LL_COMP_WINDOWMODE_COMP3_INPUT_PLUS_COMMON (2)
  *         @arg @ref LL_COMP_WINDOWMODE_COMP5_INPUT_PLUS_COMMON (2)
  *
  *         (1) Parameter available on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  *         (2) Parameter available on devices: STM32F303xB/C, STM32F358xC
  */
__STATIC_INLINE uint32_t LL_COMP_GetCommonWindowMode(COMP_Common_TypeDef *COMPxy_COMMON)
{
#if defined(COMP_CSR_COMPxWNDWEN)
  return (uint32_t)(READ_BIT(COMPxy_COMMON->CSR, COMP_CSR_COMPxWNDWEN));
#else
  /* Device without pair of comparator working in window mode */
  return (LL_COMP_WINDOWMODE_DISABLE);
#endif
}

/**
  * @}
  */

/** @defgroup COMP_LL_EF_Configuration_comparator_modes Configuration of comparator modes
  * @{
  */

/**
  * @brief  Set comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      COMPxMODE      LL_COMP_SetPowerMode
  * @param  COMPx Comparator instance
  * @param  PowerMode This parameter can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED   (1)
  *         @arg @ref LL_COMP_POWERMODE_LOWPOWER      (1)
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER (1)
  *
  *         (1) Parameter available only on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetPowerMode(COMP_TypeDef *COMPx, uint32_t PowerMode)
{
#if defined(COMP_CSR_COMPxMODE)
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxMODE, PowerMode);
#else
  /* Device without comparator power mode configurable */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_POWERMODE_HIGHSPEED").                                          */
#endif
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR      COMPxMODE      LL_COMP_GetPowerMode
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_POWERMODE_HIGHSPEED
  *         @arg @ref LL_COMP_POWERMODE_MEDIUMSPEED   (1)
  *         @arg @ref LL_COMP_POWERMODE_LOWPOWER      (1)
  *         @arg @ref LL_COMP_POWERMODE_ULTRALOWPOWER (1)
  *
  *         (1) Parameter available only on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  */
__STATIC_INLINE uint32_t LL_COMP_GetPowerMode(COMP_TypeDef *COMPx)
{
#if defined(COMP_CSR_COMPxMODE)
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxMODE));
#else
  /* Device without comparator power mode configurable */
  return (LL_COMP_POWERMODE_HIGHSPEED);
#endif
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
  *         Refer to device datasheet, parameter "tS_SC".
  * @rmtoll CSR      INMSEL         LL_COMP_ConfigInputs\n
  *         CSR      NONINSEL       LL_COMP_ConfigInputs
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2   (3)
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1   (2)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3        (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         (1) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302x6/8, STM32F303x6/8, STM32F328xx, STM32F334xx.\n
  *         (2) Parameter available only on devices STM32F303x6/8, STM32F328x8, STM32F334xx.\n
  *         (3) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302xx.\n
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2            (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2 (3)
  *
  *         (1) Parameter available only on devices STM32F302xB/C, STM32F303xB/C, STM32F358xC.\n
  *         (2) Parameter available on devices: STM32F302xB/C, STM32F302xD/E, STM32F303xB/C/D/E, STM32F358xC, STM32F398xE.\n
  *         (3) Parameter available on devices: STM32F301x6/8, STM32F318xx, STM32F302x6/8.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_ConfigInputs(COMP_TypeDef *COMPx, uint32_t InputMinus, uint32_t InputPlus)
{
#if defined(COMP_CSR_COMPxNONINSEL) && defined(COMP_CSR_COMPxSW1)
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_COMPxINSEL | COMP_CSR_COMPxNONINSEL | COMP_CSR_COMPxSW1,
             InputMinus | InputPlus);
#elif defined(COMP_CSR_COMPxNONINSEL)
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_COMPxINSEL | COMP_CSR_COMPxNONINSEL,
             InputMinus | InputPlus);
#elif defined(COMP_CSR_COMPxSW1)
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_COMPxINSEL | COMP_CSR_COMPxSW1,
             InputMinus | InputPlus);
#else
  /* Device without comparator input plus configurable */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_INPUT_PLUS_IO1" or "LL_COMP_INPUT_PLUS_IO2" compared to         */
  /* other STM32F3 devices, depending on comparator instance                  */
  /* (refer to reference manual)).                                            */
  MODIFY_REG(COMPx->CSR,
             COMP_CSR_COMPxINSEL,
             InputMinus);
#endif
}

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      NONINSEL       LL_COMP_SetInputPlus
  * @param  COMPx Comparator instance
  * @param  InputPlus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2            (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2 (3)
  *
  *         (1) Parameter available only on devices STM32F302xB/C, STM32F303xB/C, STM32F358xC.\n
  *         (2) Parameter available on devices: STM32F302xB/C, STM32F302xD/E, STM32F303xB/C/D/E, STM32F358xC, STM32F398xE.\n
  *         (3) Parameter available on devices: STM32F301x6/8, STM32F318xx, STM32F302x6/8.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputPlus(COMP_TypeDef *COMPx, uint32_t InputPlus)
{
#if defined(COMP_CSR_COMPxNONINSEL) && defined(COMP_CSR_COMPxSW1)
  MODIFY_REG(COMPx->CSR, (COMP_CSR_COMPxNONINSEL | COMP_CSR_COMPxSW1), InputPlus);
#elif defined(COMP_CSR_COMPxNONINSEL)
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxNONINSEL, InputPlus);
#elif defined(COMP_CSR_COMPxSW1)
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxSW1, InputPlus);
#else
  /* Device without comparator input plus configurable */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_INPUT_PLUS_IO1" or "LL_COMP_INPUT_PLUS_IO2" compared to         */
  /* other STM32F3 devices, depending on comparator instance                  */
  /* (refer to reference manual)).                                            */
#endif
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      NONINSEL       LL_COMP_GetInputPlus
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_PLUS_IO1
  *         @arg @ref LL_COMP_INPUT_PLUS_IO2            (1)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 (2)
  *         @arg @ref LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2 (3)
  *
  *         (1) Parameter available only on devices STM32F302xB/C, STM32F303xB/C, STM32F358xC.\n
  *         (2) Parameter available on devices: STM32F302xB/C, STM32F302xD/E, STM32F303xB/C/D/E, STM32F358xC, STM32F398xE.\n
  *         (3) Parameter available on devices: STM32F301x6/8, STM32F318xx, STM32F302x6/8.
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputPlus(COMP_TypeDef *COMPx)
{
#if defined(COMP_CSR_COMPxNONINSEL) && defined(COMP_CSR_COMPxSW1)
  return (uint32_t)(READ_BIT(COMPx->CSR, (COMP_CSR_COMPxNONINSEL | COMP_CSR_COMPxSW1)));
#elif defined(COMP_CSR_COMPxNONINSEL)
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxNONINSEL));
#elif defined(COMP_CSR_COMPxSW1)
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxSW1));
#else
  /* Device without comparator input plus configurable */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_INPUT_PLUS_IO1" or "LL_COMP_INPUT_PLUS_IO2" compared to         */
  /* other STM32F3 devices, depending on comparator instance                  */
  /* (refer to reference manual)).                                            */
  return (LL_COMP_INPUT_PLUS_IO1);
#endif
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
  *         Refer to device datasheet, parameter "tS_SC".
  * @rmtoll CSR      INMSEL         LL_COMP_SetInputMinus
  * @param  COMPx Comparator instance
  * @param  InputMinus This parameter can be one of the following values:
  *         @arg @ref LL_COMP_INPUT_MINUS_1_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_1_2VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_3_4VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_VREFINT
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH1
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2   (3)
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1   (2)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3        (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         (1) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302x6/8, STM32F303x6/8, STM32F328xx, STM32F334xx.\n
  *         (2) Parameter available only on devices STM32F303x6/8, STM32F328x8, STM32F334xx.\n
  *         (3) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302xx.\n
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxINSEL, InputMinus);
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
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC1_CH2   (3)
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1   (2)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO1
  *         @arg @ref LL_COMP_INPUT_MINUS_IO2
  *         @arg @ref LL_COMP_INPUT_MINUS_IO3        (1)
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         (1) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302x6/8, STM32F303x6/8, STM32F328xx, STM32F334xx.\n
  *         (2) Parameter available only on devices STM32F303x6/8, STM32F328x8, STM32F334xx.\n
  *         (3) Parameter available on all devices except STM32F301x6/8, STM32F318x8, STM32F302xx.\n
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputMinus(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxINSEL));
}

/**
  * @brief  Set comparator instance hysteresis mode of the input minus (inverting input).
  * @rmtoll CSR      COMPxHYST      LL_COMP_SetInputHysteresis
  * @param  COMPx Comparator instance
  * @param  InputHysteresis This parameter can be one of the following values:
  *         @arg @ref LL_COMP_HYSTERESIS_NONE
  *         @arg @ref LL_COMP_HYSTERESIS_LOW         (1)
  *         @arg @ref LL_COMP_HYSTERESIS_MEDIUM      (1)
  *         @arg @ref LL_COMP_HYSTERESIS_HIGH        (1)
  *
  *         (1) Parameter available only on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputHysteresis(COMP_TypeDef *COMPx, uint32_t InputHysteresis)
{
#if defined(COMP_CSR_COMPxHYST)
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxHYST, InputHysteresis);
#else
  /* Device without comparator input hysteresis */
  /* No update of comparator register (corresponds to setting                 */
  /* "LL_COMP_HYSTERESIS_NONE").                                              */
#endif
}

/**
  * @brief  Get comparator instance hysteresis mode of the minus (inverting) input.
  * @rmtoll CSR      COMPxHYST      LL_COMP_GetInputHysteresis
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_HYSTERESIS_NONE
  *         @arg @ref LL_COMP_HYSTERESIS_LOW         (1)
  *         @arg @ref LL_COMP_HYSTERESIS_MEDIUM      (1)
  *         @arg @ref LL_COMP_HYSTERESIS_HIGH        (1)
  *
  *         (1) Parameter available only on devices: STM32F302xB/C, STM32F303xB/C, STM32F358xC
  */
__STATIC_INLINE uint32_t LL_COMP_GetInputHysteresis(COMP_TypeDef *COMPx)
{
#if defined(COMP_CSR_COMPxHYST)
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxHYST));
#else
  /* Device without comparator input hysteresis */
  return (LL_COMP_HYSTERESIS_NONE);
#endif
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
  * @rmtoll CSR      COMPxOUTSEL    LL_COMP_SetOutputSelection
  * @param  COMPx Comparator instance
  * @param  OutputSelection This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN2
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_BKIN2
  *         @arg @ref LL_COMP_OUTPUT_TIM8_BKIN              (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM8_BKIN2             (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_BKIN2        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_BKIN             (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_BKIN2            (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_TIM20_BKIN2  (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3   (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4     (6)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1_COMP2         (2)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4_COMP2         (2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1_COMP2         (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_OCCLR_COMP2      (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC2_COMP3         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC1_COMP3         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM15_IC1_COMP3        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM15_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC3_COMP4         (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC2_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM15_IC2_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM15_OCCLR_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC1_COMP5         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC3_COMP5         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_IC1_COMP5        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM16_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC2_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC4_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM16_IC1_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM16_OCCLR_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC2_COMP7         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC3_COMP7         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_OCCLR_COMP7      (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_BKIN             (4)
  *
  *         (1) Parameter available on devices: STM32F302x8, STM32F318xx, STM32F303x8, STM32F328xx, STM32F334x8, STM32F302xC, STM32F302xE, STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (2) Parameter available on devices: STM32F301x8, STM32F302x8, STM32F318xx, STM32F303x8, STM32F328xx, STM32F334x8.\n
  *         (3) Parameter available on devices: STM32F302xC, STM32F302xE, STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (4) Parameter available on devices: STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (5) Parameter available on devices: STM32F303xE, STM32F398xx.\n
  *         (6) Parameter available on devices: STM32F303x8, STM32F328xx, STM32F334x8.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputSelection(COMP_TypeDef *COMPx, uint32_t OutputSelection)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxOUTSEL, OutputSelection);
}

/**
  * @brief  Get comparator output selection.
  * @note   Availability of parameters of output selection to timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      COMPxOUTSEL    LL_COMP_GetOutputSelection
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM1_BKIN2
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_BKIN2
  *         @arg @ref LL_COMP_OUTPUT_TIM8_BKIN              (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM8_BKIN2             (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_BKIN2        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_BKIN             (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_BKIN2            (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_TIM8_TIM20_BKIN2  (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3   (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7 (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4     (6)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1_COMP2         (2)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4_COMP2         (2)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1_COMP2         (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC1_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC4_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC1_COMP1_2       (3)
  *         @arg @ref LL_COMP_OUTPUT_TIM20_OCCLR_COMP2      (5)
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC2_COMP3         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC1_COMP3         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM15_IC1_COMP3        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM15_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM3_IC3_COMP4         (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC2_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM15_IC2_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM15_OCCLR_COMP4
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC1_COMP5         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC3_COMP5         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_IC1_COMP5        (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM16_BKIN
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC2_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM2_OCCLR_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC4_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM16_IC1_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM16_OCCLR_COMP6
  *         @arg @ref LL_COMP_OUTPUT_TIM1_IC2_COMP7         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM2_IC3_COMP7         (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_OCCLR_COMP7      (4)
  *         @arg @ref LL_COMP_OUTPUT_TIM17_BKIN             (4)
  *
  *         (1) Parameter available on devices: STM32F302x8, STM32F318xx, STM32F303x8, STM32F328xx, STM32F334x8, STM32F302xC, STM32F302xE, STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (2) Parameter available on devices: STM32F301x8, STM32F302x8, STM32F318xx, STM32F303x8, STM32F328xx, STM32F334x8.\n
  *         (3) Parameter available on devices: STM32F302xC, STM32F302xE, STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (4) Parameter available on devices: STM32F303xC, STM32F358xx, STM32F303xE, STM32F398xx.\n
  *         (5) Parameter available on devices: STM32F303xE, STM32F398xx.\n
  *         (6) Parameter available on devices: STM32F303x8, STM32F328xx, STM32F334x8.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputSelection(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxOUTSEL));
}

/**
  * @brief  Set comparator instance output polarity.
  * @rmtoll CSR      COMPxPOL       LL_COMP_SetOutputPolarity
  * @param  COMPx Comparator instance
  * @param  OutputPolarity This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputPolarity(COMP_TypeDef *COMPx, uint32_t OutputPolarity)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxPOL, OutputPolarity);
}

/**
  * @brief  Get comparator instance output polarity.
  * @rmtoll CSR      COMPxPOL       LL_COMP_GetOutputPolarity
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUTPOL_NONINVERTED
  *         @arg @ref LL_COMP_OUTPUTPOL_INVERTED
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputPolarity(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxPOL));
}

/**
  * @brief  Set comparator instance blanking source.
  * @note   Blanking source may be specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @note   Availability of parameters of blanking source from timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR      COMPxBLANKING  LL_COMP_SetOutputBlankingSource
  * @param  COMPx Comparator instance
  * @param  BlankingSource This parameter can be one of the following values:
  *         @arg @ref LL_COMP_BLANKINGSRC_NONE
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6       (2)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6      (1)(2)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7   (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6     (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7 (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7    (3)
  *
  *         (1) Parameter available on devices: STM32F301x8, STM32F302x8, STM32F318xx, STM32F303x8, STM32F334x8, STM32F328xx.\n
  *         (2) Parameter available on devices: STM32F302xE, STM32F302xC.\n
  *         (3) Parameter available on devices: STM32F303xE, STM32F398xx, STM32F303xC, STM32F358xx.
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetOutputBlankingSource(COMP_TypeDef *COMPx, uint32_t BlankingSource)
{
  MODIFY_REG(COMPx->CSR, COMP_CSR_COMPxBLANKING, BlankingSource);
}

/**
  * @brief  Get comparator instance blanking source.
  * @note   Availability of parameters of blanking source from timer
  *         depends on timers availability on the selected device.
  * @note   Blanking source may be specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR      COMPxBLANKING  LL_COMP_GetOutputBlankingSource
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_BLANKINGSRC_NONE
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3_COMP2       (1)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2     (2)(3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6       (2)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6      (1)(2)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7   (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6     (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7 (3)
  *         @arg @ref LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7    (3)
  *
  *         (1) Parameter available on devices: STM32F301x8, STM32F302x8, STM32F318xx, STM32F303x8, STM32F334x8, STM32F328xx.\n
  *         (2) Parameter available on devices: STM32F302xE, STM32F302xC.\n
  *         (3) Parameter available on devices: STM32F303xE, STM32F398xx, STM32F303xC, STM32F358xx.
  */
__STATIC_INLINE uint32_t LL_COMP_GetOutputBlankingSource(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxBLANKING));
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
  * @rmtoll CSR      COMPxEN        LL_COMP_Enable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  SET_BIT(COMPx->CSR, COMP_CSR_COMPxEN);
}

/**
  * @brief  Disable comparator instance.
  * @rmtoll CSR      COMPxEN        LL_COMP_Disable
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
  * @rmtoll CSR      COMPxEN        LL_COMP_IsEnabled
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
  * @rmtoll CSR      COMPxLOCK      LL_COMP_Lock
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
  * @rmtoll CSR      COMPxLOCK      LL_COMP_IsLocked
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
  * @rmtoll CSR      COMPxOUT       LL_COMP_ReadOutputLevel
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref LL_COMP_OUTPUT_LEVEL_HIGH
  */
__STATIC_INLINE uint32_t LL_COMP_ReadOutputLevel(COMP_TypeDef *COMPx)
{
  return (uint32_t)(READ_BIT(COMPx->CSR, COMP_CSR_COMPxOUT)
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

#endif /* COMP1 || COMP2 || COMP3 || COMP4 || COMP5 || COMP6 || COMP7 */


#endif /* STM32F301x8 || STM32F302x8 || STM32F302xC || STM32F302xE || STM32F303x8 || STM32F303xC || STM32F303xE || STM32F318xx || STM32F328xx || STM32F334x8 || STM32F358xx || STM32F398xx */

#if defined (COMP_V1_1_0_0)

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
#define LL_COMP_INPUT_MINUS_IO2         (COMP_CSR_COMP1INSEL_2 | COMP_CSR_COMP1INSEL_1 | COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to IO2 (pin PA6 for COMP1 & COMP2) */
#define LL_COMP_INPUT_MINUS_IO3         (COMP_CSR_COMP1INSEL_2 |                         COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to IO3 (pin PA5 for COMP1 & COMP2) */
#define LL_COMP_INPUT_MINUS_IO4         (COMP_CSR_COMP1INSEL_2                                                ) /*!< Comparator input minus connected to IO4 (pin PA4 for COMP1 & COMP2) */
#define LL_COMP_INPUT_MINUS_DAC2_CH1    (COMP_CSR_COMP1INSEL_2 | COMP_CSR_COMP1INSEL_1 | COMP_CSR_COMP1INSEL_0) /*!< Comparator input minus connected to DAC2 channel 1 (DAC2_OUT1)  */
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
#define LL_COMP_OUTPUT_TIM2_IC4         (COMP_CSR_COMP1OUTSEL_2)                                                   /*!< COMP output connected to TIM2 input capture 4 */
#define LL_COMP_OUTPUT_TIM2_OCCLR       (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_0)                          /*!< COMP output connected to TIM2 OCREF clear */

/* Note: Output redirection specific to COMP instance: COMP1 */
#define LL_COMP_OUTPUT_TIM15_BKIN_COMP1 (COMP_CSR_COMP1OUTSEL_0)                                                   /*!< COMP output connected to TIM15 break input (BKIN) (specific to COMP instance: COMP1) */
#define LL_COMP_OUTPUT_TIM3_IC1_COMP1   (COMP_CSR_COMP1OUTSEL_1)                                                   /*!< COMP output connected to TIM3 input capture 1 (specific to COMP instance: COMP1) */
#define LL_COMP_OUTPUT_TIM3_OCCLR_COMP1 (COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0)                          /*!< COMP output connected to TIM3 OCREF clear (specific to COMP instance: COMP1) */
#define LL_COMP_OUTPUT_TIM5_IC4_COMP1   (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1)                          /*!< COMP output connected to TIM5 input capture 4 (specific to COMP instance: COMP1) */
#define LL_COMP_OUTPUT_TIM5_OCCLR_COMP1 (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0) /*!< COMP output connected to TIM5 OCREF clear (specific to COMP instance: COMP1) */

/* Note: Output redirection specific to COMP instance: COMP2 */
#define LL_COMP_OUTPUT_TIM16_BKIN_COMP2 (COMP_CSR_COMP1OUTSEL_0)                                                   /*!< COMP output connected to TIM16 break input (BKIN) (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM4_IC1_COMP2   (COMP_CSR_COMP1OUTSEL_1)                                                   /*!< COMP output connected to TIM4 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM4_OCCLR_COMP2 (COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0)                          /*!< COMP output connected to TIM4 OCREF clear (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_IC1_COMP2   (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1)                          /*!< COMP output connected to TIM3 input capture 1 (specific to COMP instance: COMP2) */
#define LL_COMP_OUTPUT_TIM3_OCCLR_COMP2 (COMP_CSR_COMP1OUTSEL_2 | COMP_CSR_COMP1OUTSEL_1 | COMP_CSR_COMP1OUTSEL_0) /*!< COMP output connected to TIM3 OCREF clear (specific to COMP instance: COMP2) */

/* Note: Output redirection specific to COMP instances, defined with          */
/*       generic naming not taking into account COMP instance constraints.    */
/*       Refer to literal definitions above for COMP instance constraints.    */
/* Note: Some output redirections cannot have a generic naming,               */
/*       due to literal value different depending on COMP instance.           */
/*       (For exemple: LL_COMP_OUTPUT_TIM3_IC1_COMP1 and                      */
/*       LL_COMP_OUTPUT_TIM3_IC1_COMP2).                                      */
#define LL_COMP_OUTPUT_TIM15_BKIN       LL_COMP_OUTPUT_TIM15_BKIN_COMP1       /*!< COMP output connected to TIM15 break input (BKIN). Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM16_BKIN       LL_COMP_OUTPUT_TIM16_BKIN_COMP2       /*!< COMP output connected to TIM16 break input (BKIN). Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_IC1         LL_COMP_OUTPUT_TIM4_IC1_COMP2         /*!< COMP output connected to TIM4 input capture 1.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM4_OCCLR       LL_COMP_OUTPUT_TIM4_OCCLR_COMP2       /*!< COMP output connected to TIM4 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM5_IC4         LL_COMP_OUTPUT_TIM5_IC1_COMP1         /*!< COMP output connected to TIM5 input capture 4.     Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
#define LL_COMP_OUTPUT_TIM5_OCCLR       LL_COMP_OUTPUT_TIM5_OCCLR_COMP1       /*!< COMP output connected to TIM5 OCREF clear.         Caution: Parameter specific to COMP instances, defined with generic naming, not taking into account COMP instance constraints. Refer to literal definitions above for COMP instance constraints. */
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
  * @rmtoll CSR         WNDWEN          LL_COMP_SetCommonWindowMode
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
  * @rmtoll CSR         WNDWEN          LL_COMP_GetCommonWindowMode
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
  * @rmtoll CSR         COMP1MODE       LL_COMP_SetPowerMode\n
  *                     COMP2MODE       LL_COMP_SetPowerMode
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
             PowerMode          << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator instance operating mode to adjust power and speed.
  * @rmtoll CSR         COMP1MODE       LL_COMP_GetPowerMode\n
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
  * @rmtoll CSR         COMP1INSEL      LL_COMP_ConfigInputs\n
  *         CSR         COMP2INSEL      LL_COMP_ConfigInputs\n
  *         CSR         COMP1SW1        LL_COMP_ConfigInputs
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
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1
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
             (InputMinus | InputPlus)                                        << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Set comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR         COMP1INSEL      LL_COMP_SetInputPlus\n
  *         CSR         COMP2INSEL      LL_COMP_SetInputPlus
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
             InputPlus                                           << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator input plus (non-inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR         COMP1INSEL      LL_COMP_GetInputPlus\n
  *         CSR         COMP2INSEL      LL_COMP_GetInputPlus
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
  * @rmtoll CSR         COMP1SW1        LL_COMP_SetInputMinus
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
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1
  * @retval None
  */
__STATIC_INLINE void LL_COMP_SetInputMinus(COMP_TypeDef *COMPx, uint32_t InputMinus)
{
  MODIFY_REG(COMP->CSR,
             COMP_CSR_COMP1INSEL << __COMP_BITOFFSET_INSTANCE(COMPx),
             InputMinus          << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator input minus (inverting).
  * @note   In case of comparator input selected to be connected to IO:
  *         GPIO pins are specific to each comparator instance.
  *         Refer to description of parameters or to reference manual.
  * @rmtoll CSR         COMP1SW1        LL_COMP_GetInputMinus
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
  *         @arg @ref LL_COMP_INPUT_MINUS_IO4
  *         @arg @ref LL_COMP_INPUT_MINUS_DAC2_CH1
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
  * @rmtoll CSR     COMP1HYST       LL_COMP_SetInputHysteresis\n
  *                 COMP2HYST       LL_COMP_SetInputHysteresis
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
             InputHysteresis    << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator instance hysteresis mode of the minus (inverting) input.
  * @rmtoll CSR     COMP1HYST       LL_COMP_GetInputHysteresis\n
  *                 COMP2HYST       LL_COMP_GetInputHysteresis
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
  * @rmtoll CSR     COMP1OUTSEL     LL_COMP_SetOutputSelection\n
  *                 COMP2OUTSEL     LL_COMP_SetOutputSelection
  * @param  COMPx Comparator instance
  * @param  OutputSelection This parameter can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM16_BKIN     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_OCCLR     (1)
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
             OutputSelection      << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator output selection.
  * @note   Availability of parameters of output selection to timer
  *         depends on timers availability on the selected device.
  * @rmtoll CSR     COMP1OUTSEL     LL_COMP_GetOutputSelection\n
  *                 COMP2OUTSEL     LL_COMP_GetOutputSelection
  * @param  COMPx Comparator instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_COMP_OUTPUT_NONE
  *         @arg @ref LL_COMP_OUTPUT_TIM16_BKIN     (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_IC1       (1)
  *         @arg @ref LL_COMP_OUTPUT_TIM4_OCCLR     (1)
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
  * @rmtoll CSR         COMP1POL        LL_COMP_SetOutputPolarity\n
  *                     COMP2POL        LL_COMP_SetOutputPolarity
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
             OutputPolarity    << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Get comparator instance output polarity.
  * @rmtoll CSR         COMP1POL        LL_COMP_GetOutputPolarity\n
  *                     COMP2POL        LL_COMP_GetOutputPolarity
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
  * @rmtoll CSR         COMP1EN         LL_COMP_Enable\n
  *                     COMP2EN         LL_COMP_Enable
  * @param  COMPx Comparator instance
  * @retval None
  */
__STATIC_INLINE void LL_COMP_Enable(COMP_TypeDef *COMPx)
{
  SET_BIT(COMP->CSR, COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Disable comparator instance.
  * @rmtoll CSR         COMP1EN         LL_COMP_Disable\n
  *                     COMP2EN         LL_COMP_Disable
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
  * @rmtoll CSR         COMP1EN         LL_COMP_IsEnabled\n
  *                     COMP2EN         LL_COMP_IsEnabled
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsEnabled(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMP->CSR, COMP_CSR_COMP1EN << __COMP_BITOFFSET_INSTANCE(COMPx)) == COMP_CSR_COMP1EN <<
          __COMP_BITOFFSET_INSTANCE(COMPx));
}

/**
  * @brief  Lock comparator instance.
  * @note   Once locked, comparator configuration can be accessed in read-only.
  * @note   The only way to unlock the comparator is a device hardware reset.
  * @rmtoll CSR         COMP1LOCK       LL_COMP_Lock\n
  *                     COMP2LOCK       LL_COMP_Lock
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
  * @rmtoll CSR         COMP1LOCK       LL_COMP_IsLocked\n
  *                     COMP2LOCK       LL_COMP_IsLocked
  * @param  COMPx Comparator instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_COMP_IsLocked(COMP_TypeDef *COMPx)
{
  return (READ_BIT(COMP->CSR, COMP_CSR_COMP1LOCK << __COMP_BITOFFSET_INSTANCE(COMPx)) == COMP_CSR_COMP1LOCK <<
          __COMP_BITOFFSET_INSTANCE(COMPx));
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
  * @rmtoll CSR         COMP1OUT        LL_COMP_ReadOutputLevel\n
  *                     COMP2OUT        LL_COMP_ReadOutputLevel
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


#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F3xx_LL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
