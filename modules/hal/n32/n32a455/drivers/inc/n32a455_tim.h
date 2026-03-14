/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_tim.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_TIM_H__
#define __N32A455_TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"
#include "stdbool.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup TIM
 * @{
 */

/** @addtogroup TIM_Exported_Types
 * @{
 */

/**
 * @brief  TIM Time Base Init structure definition
 * @note   This structure is used with all TIMx except for TIM6 and TIM7.
 */

typedef struct
{
    uint16_t Prescaler; /*!< Specifies the prescaler value used to divide the TIM clock.
                                 This parameter can be a number between 0x0000 and 0xFFFF */

    uint16_t CntMode; /*!< Specifies the counter mode.
                                   This parameter can be a value of @ref TIM_Counter_Mode */

    uint16_t Period; /*!< Specifies the period value to be loaded into the active
                              Auto-Reload Register at the next update event.
                              This parameter must be a number between 0x0000 and 0xFFFF.  */

    uint16_t ClkDiv; /*!< Specifies the clock division.
                                    This parameter can be a value of @ref TIM_Clock_Division_CKD */

    uint8_t RepetCnt; /*!< Specifies the repetition counter value. Each time the REPCNT downcounter
                                        reaches zero, an update event is generated and counting restarts
                                        from the REPCNT value (N).
                                        This means in PWM mode that (N+1) corresponds to:
                                           - the number of PWM periods in edge-aligned mode
                                           - the number of half PWM period in center-aligned mode
                                        This parameter must be a number between 0x00 and 0xFF.
                                        @note This parameter is valid only for TIM1 and TIM8. */

    bool CapCh1FromCompEn;    /*!< channel 1 select capture in from comp if 1, from IOM if 0
                                Tim1,Tim8,Tim2,Tim3,Tim4,Tim5 valid*/
    bool CapCh2FromCompEn;    /*!< channel 2 select capture in from comp if 1, from IOM if 0
                                Tim2,Tim3,Tim4,Tim5 valid*/
    bool CapCh3FromCompEn;    /*!< channel 3 select capture in from comp if 1, from IOM if 0
                                Tim2,Tim3,Tim4,Tim5 valid*/
    bool CapCh4FromCompEn;    /*!< channel 4 select capture in from comp if 1, from IOM if 0
                                Tim2,Tim3,Tim4 valid*/
    bool CapEtrClrFromCompEn; /*!< etr clearref select from comp if 1, from ETR IOM if 0
                                Tim2,Tim3,Tim4 valid*/
    bool CapEtrSelFromTscEn;  /*!< etr select from TSC if 1, from IOM if 0
                                 Tim2,Tim4 valid*/
} TIM_TimeBaseInitType;

/**
 * @brief  TIM Output Compare Init structure definition
 */

typedef struct
{
    uint16_t OcMode; /*!< Specifies the TIM mode.
                              This parameter can be a value of @ref TIM_Output_Compare_and_PWM_modes */

    uint16_t OutputState; /*!< Specifies the TIM Output Compare state.
                                   This parameter can be a value of @ref TIM_Output_Compare_state */

    uint16_t OutputNState; /*!< Specifies the TIM complementary Output Compare state.
                                    This parameter can be a value of @ref TIM_Output_Compare_N_state
                                    @note This parameter is valid only for TIM1 and TIM8. */

    uint16_t Pulse; /*!< Specifies the pulse value to be loaded into the Capture Compare Register.
                             This parameter can be a number between 0x0000 and 0xFFFF */

    uint16_t OcPolarity; /*!< Specifies the output polarity.
                                  This parameter can be a value of @ref TIM_Output_Compare_Polarity */

    uint16_t OcNPolarity; /*!< Specifies the complementary output polarity.
                                   This parameter can be a value of @ref TIM_Output_Compare_N_Polarity
                                   @note This parameter is valid only for TIM1 and TIM8. */

    uint16_t OcIdleState; /*!< Specifies the TIM Output Compare pin state during Idle state.
                                   This parameter can be a value of @ref TIM_Output_Compare_Idle_State
                                   @note This parameter is valid only for TIM1 and TIM8. */

    uint16_t OcNIdleState; /*!< Specifies the TIM Output Compare pin state during Idle state.
                                    This parameter can be a value of @ref TIM_Output_Compare_N_Idle_State
                                    @note This parameter is valid only for TIM1 and TIM8. */
} OCInitType;

/**
 * @brief  TIM Input Capture Init structure definition
 */

typedef struct
{
    uint16_t Channel; /*!< Specifies the TIM channel.
                               This parameter can be a value of @ref Channel */

    uint16_t IcPolarity; /*!< Specifies the active edge of the input signal.
                                  This parameter can be a value of @ref TIM_Input_Capture_Polarity */

    uint16_t IcSelection; /*!< Specifies the input.
                                   This parameter can be a value of @ref TIM_Input_Capture_Selection */

    uint16_t IcPrescaler; /*!< Specifies the Input Capture Prescaler.
                                   This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

    uint16_t IcFilter; /*!< Specifies the input capture filter.
                                This parameter can be a number between 0x0 and 0xF */
} TIM_ICInitType;

/**
 * @brief  BKDT structure definition
 * @note   This structure is used only with TIM1 and TIM8.
 */

typedef struct
{
    uint16_t OssrState; /*!< Specifies the Off-State selection used in Run mode.
                                 This parameter can be a value of @ref OSSR_Off_State_Selection_for_Run_mode_state */

    uint16_t OssiState; /*!< Specifies the Off-State used in Idle state.
                                 This parameter can be a value of @ref OSSI_Off_State_Selection_for_Idle_mode_state */

    uint16_t LockLevel; /*!< Specifies the LOCK level parameters.
                                 This parameter can be a value of @ref Lock_level */

    uint16_t DeadTime; /*!< Specifies the delay time between the switching-off and the
                                switching-on of the outputs.
                                This parameter can be a number between 0x00 and 0xFF  */

    uint16_t Break; /*!< Specifies whether the TIM Break input is enabled or not.
                             This parameter can be a value of @ref Break_Input_enable_disable */

    uint16_t BreakPolarity; /*!< Specifies the TIM Break Input pin polarity.
                                     This parameter can be a value of @ref Break_Polarity */

    uint16_t AutomaticOutput; /*!< Specifies whether the TIM Automatic Output feature is enabled or not.
                                       This parameter can be a value of @ref TIM_AOE_Bit_Set_Reset */
    bool IomBreakEn;          /*!< EXTENDMODE valid, open iom as break in*/
    bool LockUpBreakEn;       /*!< EXTENDMODE valid, open lockup(haldfault) as break in*/
    bool PvdBreakEn;          /*!< EXTENDMODE valid, open pvd(sys voltage too high or too low) as break in*/
} TIM_BDTRInitType;

/** @addtogroup TIM_Exported_constants
 * @{
 */

#define IsTimAllModule(PERIPH)                                                                                         \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM6) || ((PERIPH) == TIM7) || ((PERIPH) == TIM8))

/* LIST1: TIM 1 and 8 */
#define IsTimList1Module(PERIPH) (((PERIPH) == TIM1) || ((PERIPH) == TIM8))

/* LIST2: TIM 1, 8 */
#define IsTimList2Module(PERIPH) (((PERIPH) == TIM1) || ((PERIPH) == TIM8))

/* LIST3: TIM 1, 2, 3, 4, 5 and 8 */
#define IsTimList3Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM8))

/* LIST4: TIM 1, 2, 3, 4, 5, 8 */
#define IsTimList4Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM8))

/* LIST5: TIM 1, 2, 3, 4, 5, 8 */
#define IsTimList5Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM8))

/* LIST6: TIM 1, 2, 3, 4, 5, 8 */
#define IsTimList6Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM8))

/* LIST7: TIM 1, 2, 3, 4, 5, 6, 7, 8 */
#define IsTimList7Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM6) || ((PERIPH) == TIM7) || ((PERIPH) == TIM8))

/* LIST8: TIM 1, 2, 3, 4, 5, 8 */
#define IsTimList8Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM8))

/* LIST9: TIM 1, 2, 3, 4, 5, 6, 7, 8 */
#define IsTimList9Module(PERIPH)                                                                                       \
    (((PERIPH) == TIM1) || ((PERIPH) == TIM2) || ((PERIPH) == TIM3) || ((PERIPH) == TIM4) || ((PERIPH) == TIM5)        \
     || ((PERIPH) == TIM6) || ((PERIPH) == TIM7) || ((PERIPH) == TIM8))

/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_and_PWM_modes
 * @{
 */

#define TIM_OCMODE_TIMING   ((uint16_t)0x0000)
#define TIM_OCMODE_ACTIVE   ((uint16_t)0x0010)
#define TIM_OCMODE_INACTIVE ((uint16_t)0x0020)
#define TIM_OCMODE_TOGGLE   ((uint16_t)0x0030)
#define TIM_OCMODE_PWM1     ((uint16_t)0x0060)
#define TIM_OCMODE_PWM2     ((uint16_t)0x0070)
#define IsTimOcMode(MODE)                                                                                              \
    (((MODE) == TIM_OCMODE_TIMING) || ((MODE) == TIM_OCMODE_ACTIVE) || ((MODE) == TIM_OCMODE_INACTIVE)                 \
     || ((MODE) == TIM_OCMODE_TOGGLE) || ((MODE) == TIM_OCMODE_PWM1) || ((MODE) == TIM_OCMODE_PWM2))
#define IsTimOc(MODE)                                                                                                  \
    (((MODE) == TIM_OCMODE_TIMING) || ((MODE) == TIM_OCMODE_ACTIVE) || ((MODE) == TIM_OCMODE_INACTIVE)                 \
     || ((MODE) == TIM_OCMODE_TOGGLE) || ((MODE) == TIM_OCMODE_PWM1) || ((MODE) == TIM_OCMODE_PWM2)                    \
     || ((MODE) == TIM_FORCED_ACTION_ACTIVE) || ((MODE) == TIM_FORCED_ACTION_INACTIVE))
/**
 * @}
 */

/** @addtogroup TIM_One_Pulse_Mode
 * @{
 */

#define TIM_OPMODE_SINGLE ((uint16_t)0x0008)
#define TIM_OPMODE_REPET  ((uint16_t)0x0000)
#define IsTimOpMOde(MODE) (((MODE) == TIM_OPMODE_SINGLE) || ((MODE) == TIM_OPMODE_REPET))
/**
 * @}
 */

/** @addtogroup Channel
 * @{
 */

#define TIM_CH_1 ((uint16_t)0x0000)
#define TIM_CH_2 ((uint16_t)0x0004)
#define TIM_CH_3 ((uint16_t)0x0008)
#define TIM_CH_4 ((uint16_t)0x000C)
#define IsTimCh(CHANNEL)                                                                                               \
    (((CHANNEL) == TIM_CH_1) || ((CHANNEL) == TIM_CH_2) || ((CHANNEL) == TIM_CH_3) || ((CHANNEL) == TIM_CH_4))
#define IsTimPwmInCh(CHANNEL)         (((CHANNEL) == TIM_CH_1) || ((CHANNEL) == TIM_CH_2))
#define IsTimComplementaryCh(CHANNEL) (((CHANNEL) == TIM_CH_1) || ((CHANNEL) == TIM_CH_2) || ((CHANNEL) == TIM_CH_3))
/**
 * @}
 */

/** @addtogroup TIM_Clock_Division_CKD
 * @{
 */

#define TIM_CLK_DIV1     ((uint16_t)0x0000)
#define TIM_CLK_DIV2     ((uint16_t)0x0100)
#define TIM_CLK_DIV4     ((uint16_t)0x0200)
#define IsTimClkDiv(DIV) (((DIV) == TIM_CLK_DIV1) || ((DIV) == TIM_CLK_DIV2) || ((DIV) == TIM_CLK_DIV4))
/**
 * @}
 */

/** @addtogroup TIM_Counter_Mode
 * @{
 */

#define TIM_CNT_MODE_UP            ((uint16_t)0x0000)
#define TIM_CNT_MODE_DOWN          ((uint16_t)0x0010)
#define TIM_CNT_MODE_CENTER_ALIGN1 ((uint16_t)0x0020)
#define TIM_CNT_MODE_CENTER_ALIGN2 ((uint16_t)0x0040)
#define TIM_CNT_MODE_CENTER_ALIGN3 ((uint16_t)0x0060)
#define IsTimCntMode(MODE)                                                                                             \
    (((MODE) == TIM_CNT_MODE_UP) || ((MODE) == TIM_CNT_MODE_DOWN) || ((MODE) == TIM_CNT_MODE_CENTER_ALIGN1)            \
     || ((MODE) == TIM_CNT_MODE_CENTER_ALIGN2) || ((MODE) == TIM_CNT_MODE_CENTER_ALIGN3))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_Polarity
 * @{
 */

#define TIM_OC_POLARITY_HIGH      ((uint16_t)0x0000)
#define TIM_OC_POLARITY_LOW       ((uint16_t)0x0002)
#define IsTimOcPolarity(POLARITY) (((POLARITY) == TIM_OC_POLARITY_HIGH) || ((POLARITY) == TIM_OC_POLARITY_LOW))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_N_Polarity
 * @{
 */

#define TIM_OCN_POLARITY_HIGH      ((uint16_t)0x0000)
#define TIM_OCN_POLARITY_LOW       ((uint16_t)0x0008)
#define IsTimOcnPolarity(POLARITY) (((POLARITY) == TIM_OCN_POLARITY_HIGH) || ((POLARITY) == TIM_OCN_POLARITY_LOW))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_state
 * @{
 */

#define TIM_OUTPUT_STATE_DISABLE ((uint16_t)0x0000)
#define TIM_OUTPUT_STATE_ENABLE  ((uint16_t)0x0001)
#define IsTimOutputState(STATE)  (((STATE) == TIM_OUTPUT_STATE_DISABLE) || ((STATE) == TIM_OUTPUT_STATE_ENABLE))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_N_state
 * @{
 */

#define TIM_OUTPUT_NSTATE_DISABLE ((uint16_t)0x0000)
#define TIM_OUTPUT_NSTATE_ENABLE  ((uint16_t)0x0004)
#define IsTimOutputNState(STATE)  (((STATE) == TIM_OUTPUT_NSTATE_DISABLE) || ((STATE) == TIM_OUTPUT_NSTATE_ENABLE))
/**
 * @}
 */

/** @addtogroup TIM_Capture_Compare_state
 * @{
 */

#define TIM_CAP_CMP_ENABLE    ((uint16_t)0x0001)
#define TIM_CAP_CMP_DISABLE   ((uint16_t)0x0000)
#define IsTimCapCmpState(CCX) (((CCX) == TIM_CAP_CMP_ENABLE) || ((CCX) == TIM_CAP_CMP_DISABLE))
/**
 * @}
 */

/** @addtogroup TIM_Capture_Compare_N_state
 * @{
 */

#define TIM_CAP_CMP_N_ENABLE    ((uint16_t)0x0004)
#define TIM_CAP_CMP_N_DISABLE   ((uint16_t)0x0000)
#define IsTimCapCmpNState(CCXN) (((CCXN) == TIM_CAP_CMP_N_ENABLE) || ((CCXN) == TIM_CAP_CMP_N_DISABLE))
/**
 * @}
 */

/** @addtogroup Break_Input_enable_disable
 * @{
 */

#define TIM_BREAK_IN_ENABLE      ((uint16_t)0x1000)
#define TIM_BREAK_IN_DISABLE     ((uint16_t)0x0000)
#define IsTimBreakInState(STATE) (((STATE) == TIM_BREAK_IN_ENABLE) || ((STATE) == TIM_BREAK_IN_DISABLE))
/**
 * @}
 */

/** @addtogroup Break_Polarity
 * @{
 */

#define TIM_BREAK_POLARITY_LOW       ((uint16_t)0x0000)
#define TIM_BREAK_POLARITY_HIGH      ((uint16_t)0x2000)
#define IsTimBreakPalarity(POLARITY) (((POLARITY) == TIM_BREAK_POLARITY_LOW) || ((POLARITY) == TIM_BREAK_POLARITY_HIGH))
/**
 * @}
 */

/** @addtogroup TIM_AOE_Bit_Set_Reset
 * @{
 */

#define TIM_AUTO_OUTPUT_ENABLE      ((uint16_t)0x4000)
#define TIM_AUTO_OUTPUT_DISABLE     ((uint16_t)0x0000)
#define IsTimAutoOutputState(STATE) (((STATE) == TIM_AUTO_OUTPUT_ENABLE) || ((STATE) == TIM_AUTO_OUTPUT_DISABLE))
/**
 * @}
 */

/** @addtogroup Lock_level
 * @{
 */

#define TIM_LOCK_LEVEL_OFF ((uint16_t)0x0000)
#define TIM_LOCK_LEVEL_1   ((uint16_t)0x0100)
#define TIM_LOCK_LEVEL_2   ((uint16_t)0x0200)
#define TIM_LOCK_LEVEL_3   ((uint16_t)0x0300)
#define IsTimLockLevel(LEVEL)                                                                                          \
    (((LEVEL) == TIM_LOCK_LEVEL_OFF) || ((LEVEL) == TIM_LOCK_LEVEL_1) || ((LEVEL) == TIM_LOCK_LEVEL_2)                 \
     || ((LEVEL) == TIM_LOCK_LEVEL_3))
/**
 * @}
 */

/** @addtogroup OSSI_Off_State_Selection_for_Idle_mode_state
 * @{
 */

#define TIM_OSSI_STATE_ENABLE  ((uint16_t)0x0400)
#define TIM_OSSI_STATE_DISABLE ((uint16_t)0x0000)
#define IsTimOssiState(STATE)  (((STATE) == TIM_OSSI_STATE_ENABLE) || ((STATE) == TIM_OSSI_STATE_DISABLE))
/**
 * @}
 */

/** @addtogroup OSSR_Off_State_Selection_for_Run_mode_state
 * @{
 */

#define TIM_OSSR_STATE_ENABLE  ((uint16_t)0x0800)
#define TIM_OSSR_STATE_DISABLE ((uint16_t)0x0000)
#define IsTimOssrState(STATE)  (((STATE) == TIM_OSSR_STATE_ENABLE) || ((STATE) == TIM_OSSR_STATE_DISABLE))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_Idle_State
 * @{
 */

#define TIM_OC_IDLE_STATE_SET   ((uint16_t)0x0100)
#define TIM_OC_IDLE_STATE_RESET ((uint16_t)0x0000)
#define IsTimOcIdleState(STATE) (((STATE) == TIM_OC_IDLE_STATE_SET) || ((STATE) == TIM_OC_IDLE_STATE_RESET))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_N_Idle_State
 * @{
 */

#define TIM_OCN_IDLE_STATE_SET   ((uint16_t)0x0200)
#define TIM_OCN_IDLE_STATE_RESET ((uint16_t)0x0000)
#define IsTimOcnIdleState(STATE) (((STATE) == TIM_OCN_IDLE_STATE_SET) || ((STATE) == TIM_OCN_IDLE_STATE_RESET))
/**
 * @}
 */

/** @addtogroup TIM_Input_Capture_Polarity
 * @{
 */

#define TIM_IC_POLARITY_RISING   ((uint16_t)0x0000)
#define TIM_IC_POLARITY_FALLING  ((uint16_t)0x0002)
#define TIM_IC_POLARITY_BOTHEDGE ((uint16_t)0x000A)
#define IsTimIcPalaritySingleEdge(POLARITY)                                                                            \
    (((POLARITY) == TIM_IC_POLARITY_RISING) || ((POLARITY) == TIM_IC_POLARITY_FALLING))
#define IsTimIcPolarityAnyEdge(POLARITY)                                                                               \
    (((POLARITY) == TIM_IC_POLARITY_RISING) || ((POLARITY) == TIM_IC_POLARITY_FALLING)                                 \
     || ((POLARITY) == TIM_IC_POLARITY_BOTHEDGE))
/**
 * @}
 */

/** @addtogroup TIM_Input_Capture_Selection
 * @{
 */

#define TIM_IC_SELECTION_DIRECTTI                                                                                      \
    ((uint16_t)0x0001) /*!< TIM Input 1, 2, 3 or 4 is selected to be                                                   \
                           connected to IC1, IC2, IC3 or IC4, respectively */
#define TIM_IC_SELECTION_INDIRECTTI                                                                                    \
    ((uint16_t)0x0002)                          /*!< TIM Input 1, 2, 3 or 4 is selected to be                          \
                                                    connected to IC2, IC1, IC4 or IC3, respectively. */
#define TIM_IC_SELECTION_TRC ((uint16_t)0x0003) /*!< TIM Input 1, 2, 3 or 4 is selected to be connected to TRC. */
#define IsTimIcSelection(SELECTION)                                                                                    \
    (((SELECTION) == TIM_IC_SELECTION_DIRECTTI) || ((SELECTION) == TIM_IC_SELECTION_INDIRECTTI)                        \
     || ((SELECTION) == TIM_IC_SELECTION_TRC))
/**
 * @}
 */

/** @addtogroup TIM_Input_Capture_Prescaler
 * @{
 */

#define TIM_IC_PSC_DIV1 ((uint16_t)0x0000) /*!< Capture performed each time an edge is detected on the capture input.  \
                                            */
#define TIM_IC_PSC_DIV2 ((uint16_t)0x0004) /*!< Capture performed once every 2 events. */
#define TIM_IC_PSC_DIV4 ((uint16_t)0x0008) /*!< Capture performed once every 4 events. */
#define TIM_IC_PSC_DIV8 ((uint16_t)0x000C) /*!< Capture performed once every 8 events. */
#define IsTimIcPrescaler(PRESCALER)                                                                                    \
    (((PRESCALER) == TIM_IC_PSC_DIV1) || ((PRESCALER) == TIM_IC_PSC_DIV2) || ((PRESCALER) == TIM_IC_PSC_DIV4)          \
     || ((PRESCALER) == TIM_IC_PSC_DIV8))
/**
 * @}
 */

/** @addtogroup TIM_interrupt_sources
 * @{
 */

#define TIM_INT_UPDATE ((uint16_t)0x0001)
#define TIM_INT_CC1    ((uint16_t)0x0002)
#define TIM_INT_CC2    ((uint16_t)0x0004)
#define TIM_INT_CC3    ((uint16_t)0x0008)
#define TIM_INT_CC4    ((uint16_t)0x0010)
#define TIM_INT_COM    ((uint16_t)0x0020)
#define TIM_INT_TRIG   ((uint16_t)0x0040)
#define TIM_INT_BREAK  ((uint16_t)0x0080)
#define IsTimInt(IT)   ((((IT) & (uint16_t)0xFF00) == 0x0000) && ((IT) != 0x0000))

#define IsTimGetInt(IT)                                                                                                \
    (((IT) == TIM_INT_UPDATE) || ((IT) == TIM_INT_CC1) || ((IT) == TIM_INT_CC2) || ((IT) == TIM_INT_CC3)               \
     || ((IT) == TIM_INT_CC4) || ((IT) == TIM_INT_COM) || ((IT) == TIM_INT_TRIG) || ((IT) == TIM_INT_BREAK))
/**
 * @}
 */

/** @addtogroup TIM_DMA_Base_address
 * @{
 */

#define TIM_DMABASE_CTRL1      ((uint16_t)0x0000)
#define TIM_DMABASE_CTRL2      ((uint16_t)0x0001)
#define TIM_DMABASE_SMCTRL     ((uint16_t)0x0002)
#define TIM_DMABASE_DMAINTEN   ((uint16_t)0x0003)
#define TIM_DMABASE_STS        ((uint16_t)0x0004)
#define TIM_DMABASE_EVTGEN     ((uint16_t)0x0005)
#define TIM_DMABASE_CAPCMPMOD1 ((uint16_t)0x0006)
#define TIM_DMABASE_CAPCMPMOD2 ((uint16_t)0x0007)
#define TIM_DMABASE_CAPCMPEN   ((uint16_t)0x0008)
#define TIM_DMABASE_CNT        ((uint16_t)0x0009)
#define TIM_DMABASE_PSC        ((uint16_t)0x000A)
#define TIM_DMABASE_AR         ((uint16_t)0x000B)
#define TIM_DMABASE_REPCNT     ((uint16_t)0x000C)
#define TIM_DMABASE_CAPCMPDAT1 ((uint16_t)0x000D)
#define TIM_DMABASE_CAPCMPDAT2 ((uint16_t)0x000E)
#define TIM_DMABASE_CAPCMPDAT3 ((uint16_t)0x000F)
#define TIM_DMABASE_CAPCMPDAT4 ((uint16_t)0x0010)
#define TIM_DMABASE_BKDT       ((uint16_t)0x0011)
#define TIM_DMABASE_DMACTRL    ((uint16_t)0x0012)
#define TIM_DMABASE_CAPCMPMOD3 ((uint16_t)0x0013)
#define TIM_DMABASE_CAPCMPDAT5 ((uint16_t)0x0014)
#define TIM_DMABASE_CAPCMPDAT6 ((uint16_t)0x0015)

#define IsTimDmaBase(BASE)                                                                                             \
    (((BASE) == TIM_DMABASE_CTRL1) || ((BASE) == TIM_DMABASE_CTRL2) || ((BASE) == TIM_DMABASE_SMCTRL)                  \
     || ((BASE) == TIM_DMABASE_DMAINTEN) || ((BASE) == TIM_DMABASE_STS) || ((BASE) == TIM_DMABASE_EVTGEN)              \
     || ((BASE) == TIM_DMABASE_CAPCMPMOD1) || ((BASE) == TIM_DMABASE_CAPCMPMOD2) || ((BASE) == TIM_DMABASE_CAPCMPMOD3) \
     || ((BASE) == TIM_DMABASE_CAPCMPEN) || ((BASE) == TIM_DMABASE_CNT) || ((BASE) == TIM_DMABASE_PSC)                 \
     || ((BASE) == TIM_DMABASE_AR) || ((BASE) == TIM_DMABASE_REPCNT) || ((BASE) == TIM_DMABASE_CAPCMPDAT1)             \
     || ((BASE) == TIM_DMABASE_CAPCMPDAT2) || ((BASE) == TIM_DMABASE_CAPCMPDAT3) || ((BASE) == TIM_DMABASE_CAPCMPDAT4) \
     || ((BASE) == TIM_DMABASE_CAPCMPDAT5) || ((BASE) == TIM_DMABASE_CAPCMPDAT6) || ((BASE) == TIM_DMABASE_BKDT)       \
     || ((BASE) == TIM_DMABASE_DMACTRL))
/**
 * @}
 */

/** @addtogroup TIM_DMA_Burst_Length
 * @{
 */

#define TIM_DMABURST_LENGTH_1TRANSFER   ((uint16_t)0x0000)
#define TIM_DMABURST_LENGTH_2TRANSFERS  ((uint16_t)0x0100)
#define TIM_DMABURST_LENGTH_3TRANSFERS  ((uint16_t)0x0200)
#define TIM_DMABURST_LENGTH_4TRANSFERS  ((uint16_t)0x0300)
#define TIM_DMABURST_LENGTH_5TRANSFERS  ((uint16_t)0x0400)
#define TIM_DMABURST_LENGTH_6TRANSFERS  ((uint16_t)0x0500)
#define TIM_DMABURST_LENGTH_7TRANSFERS  ((uint16_t)0x0600)
#define TIM_DMABURST_LENGTH_8TRANSFERS  ((uint16_t)0x0700)
#define TIM_DMABURST_LENGTH_9TRANSFERS  ((uint16_t)0x0800)
#define TIM_DMABURST_LENGTH_10TRANSFERS ((uint16_t)0x0900)
#define TIM_DMABURST_LENGTH_11TRANSFERS ((uint16_t)0x0A00)
#define TIM_DMABURST_LENGTH_12TRANSFERS ((uint16_t)0x0B00)
#define TIM_DMABURST_LENGTH_13TRANSFERS ((uint16_t)0x0C00)
#define TIM_DMABURST_LENGTH_14TRANSFERS ((uint16_t)0x0D00)
#define TIM_DMABURST_LENGTH_15TRANSFERS ((uint16_t)0x0E00)
#define TIM_DMABURST_LENGTH_16TRANSFERS ((uint16_t)0x0F00)
#define TIM_DMABURST_LENGTH_17TRANSFERS ((uint16_t)0x1000)
#define TIM_DMABURST_LENGTH_18TRANSFERS ((uint16_t)0x1100)
#define TIM_DMABURST_LENGTH_19TRANSFERS ((uint16_t)0x1200)
#define TIM_DMABURST_LENGTH_20TRANSFERS ((uint16_t)0x1300)
#define TIM_DMABURST_LENGTH_21TRANSFERS ((uint16_t)0x1400)
#define IsTimDmaLength(LENGTH)                                                                                         \
    (((LENGTH) == TIM_DMABURST_LENGTH_1TRANSFER) || ((LENGTH) == TIM_DMABURST_LENGTH_2TRANSFERS)                       \
     || ((LENGTH) == TIM_DMABURST_LENGTH_3TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_4TRANSFERS)                   \
     || ((LENGTH) == TIM_DMABURST_LENGTH_5TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_6TRANSFERS)                   \
     || ((LENGTH) == TIM_DMABURST_LENGTH_7TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_8TRANSFERS)                   \
     || ((LENGTH) == TIM_DMABURST_LENGTH_9TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_10TRANSFERS)                  \
     || ((LENGTH) == TIM_DMABURST_LENGTH_11TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_12TRANSFERS)                 \
     || ((LENGTH) == TIM_DMABURST_LENGTH_13TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_14TRANSFERS)                 \
     || ((LENGTH) == TIM_DMABURST_LENGTH_15TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_16TRANSFERS)                 \
     || ((LENGTH) == TIM_DMABURST_LENGTH_17TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_18TRANSFERS)                 \
     || ((LENGTH) == TIM_DMABURST_LENGTH_19TRANSFERS) || ((LENGTH) == TIM_DMABURST_LENGTH_20TRANSFERS)                 \
     || ((LENGTH) == TIM_DMABURST_LENGTH_21TRANSFERS))
/**
 * @}
 */

/** @addtogroup TIM_DMA_sources
 * @{
 */

#define TIM_DMA_UPDATE      ((uint16_t)0x0100)
#define TIM_DMA_CC1         ((uint16_t)0x0200)
#define TIM_DMA_CC2         ((uint16_t)0x0400)
#define TIM_DMA_CC3         ((uint16_t)0x0800)
#define TIM_DMA_CC4         ((uint16_t)0x1000)
#define TIM_DMA_COM         ((uint16_t)0x2000)
#define TIM_DMA_TRIG        ((uint16_t)0x4000)
#define IsTimDmaSrc(SOURCE) ((((SOURCE) & (uint16_t)0x80FF) == 0x0000) && ((SOURCE) != 0x0000))

/**
 * @}
 */

/** @addtogroup TIM_External_Trigger_Prescaler
 * @{
 */

#define TIM_EXT_TRG_PSC_OFF  ((uint16_t)0x0000)
#define TIM_EXT_TRG_PSC_DIV2 ((uint16_t)0x1000)
#define TIM_EXT_TRG_PSC_DIV4 ((uint16_t)0x2000)
#define TIM_EXT_TRG_PSC_DIV8 ((uint16_t)0x3000)
#define IsTimExtPreDiv(PRESCALER)                                                                                      \
    (((PRESCALER) == TIM_EXT_TRG_PSC_OFF) || ((PRESCALER) == TIM_EXT_TRG_PSC_DIV2)                                     \
     || ((PRESCALER) == TIM_EXT_TRG_PSC_DIV4) || ((PRESCALER) == TIM_EXT_TRG_PSC_DIV8))
/**
 * @}
 */

/** @addtogroup TIM_Internal_Trigger_Selection
 * @{
 */

#define TIM_TRIG_SEL_IN_TR0  ((uint16_t)0x0000)
#define TIM_TRIG_SEL_IN_TR1  ((uint16_t)0x0010)
#define TIM_TRIG_SEL_IN_TR2  ((uint16_t)0x0020)
#define TIM_TRIG_SEL_IN_TR3  ((uint16_t)0x0030)
#define TIM_TRIG_SEL_TI1F_ED ((uint16_t)0x0040)
#define TIM_TRIG_SEL_TI1FP1  ((uint16_t)0x0050)
#define TIM_TRIG_SEL_TI2FP2  ((uint16_t)0x0060)
#define TIM_TRIG_SEL_ETRF    ((uint16_t)0x0070)
#define IsTimTrigSel(SELECTION)                                                                                        \
    (((SELECTION) == TIM_TRIG_SEL_IN_TR0) || ((SELECTION) == TIM_TRIG_SEL_IN_TR1)                                      \
     || ((SELECTION) == TIM_TRIG_SEL_IN_TR2) || ((SELECTION) == TIM_TRIG_SEL_IN_TR3)                                   \
     || ((SELECTION) == TIM_TRIG_SEL_TI1F_ED) || ((SELECTION) == TIM_TRIG_SEL_TI1FP1)                                  \
     || ((SELECTION) == TIM_TRIG_SEL_TI2FP2) || ((SELECTION) == TIM_TRIG_SEL_ETRF))
#define IsTimInterTrigSel(SELECTION)                                                                                   \
    (((SELECTION) == TIM_TRIG_SEL_IN_TR0) || ((SELECTION) == TIM_TRIG_SEL_IN_TR1)                                      \
     || ((SELECTION) == TIM_TRIG_SEL_IN_TR2) || ((SELECTION) == TIM_TRIG_SEL_IN_TR3))
/**
 * @}
 */

/** @addtogroup TIM_TIx_External_Clock_Source
 * @{
 */

#define TIM_EXT_CLK_SRC_TI1   ((uint16_t)0x0050)
#define TIM_EXT_CLK_SRC_TI2   ((uint16_t)0x0060)
#define TIM_EXT_CLK_SRC_TI1ED ((uint16_t)0x0040)
#define IsTimExtClkSrc(SOURCE)                                                                                         \
    (((SOURCE) == TIM_EXT_CLK_SRC_TI1) || ((SOURCE) == TIM_EXT_CLK_SRC_TI2) || ((SOURCE) == TIM_EXT_CLK_SRC_TI1ED))
/**
 * @}
 */

/** @addtogroup TIM_External_Trigger_Polarity
 * @{
 */
#define TIM_EXT_TRIG_POLARITY_INVERTED    ((uint16_t)0x8000)
#define TIM_EXT_TRIG_POLARITY_NONINVERTED ((uint16_t)0x0000)
#define IsTimExtTrigPolarity(POLARITY)                                                                                 \
    (((POLARITY) == TIM_EXT_TRIG_POLARITY_INVERTED) || ((POLARITY) == TIM_EXT_TRIG_POLARITY_NONINVERTED))
/**
 * @}
 */

/** @addtogroup TIM_Prescaler_Reload_Mode
 * @{
 */

#define TIM_PSC_RELOAD_MODE_UPDATE    ((uint16_t)0x0000)
#define TIM_PSC_RELOAD_MODE_IMMEDIATE ((uint16_t)0x0001)
#define IsTimPscReloadMode(RELOAD)                                                                                     \
    (((RELOAD) == TIM_PSC_RELOAD_MODE_UPDATE) || ((RELOAD) == TIM_PSC_RELOAD_MODE_IMMEDIATE))
/**
 * @}
 */

/** @addtogroup TIM_Forced_Action
 * @{
 */

#define TIM_FORCED_ACTION_ACTIVE   ((uint16_t)0x0050)
#define TIM_FORCED_ACTION_INACTIVE ((uint16_t)0x0040)
#define IsTimForceActive(OPERATE)  (((OPERATE) == TIM_FORCED_ACTION_ACTIVE) || ((OPERATE) == TIM_FORCED_ACTION_INACTIVE))
/**
 * @}
 */

/** @addtogroup TIM_Encoder_Mode
 * @{
 */

#define TIM_ENCODE_MODE_TI1  ((uint16_t)0x0001)
#define TIM_ENCODE_MODE_TI2  ((uint16_t)0x0002)
#define TIM_ENCODE_MODE_TI12 ((uint16_t)0x0003)
#define IsTimEncodeMode(MODE)                                                                                          \
    (((MODE) == TIM_ENCODE_MODE_TI1) || ((MODE) == TIM_ENCODE_MODE_TI2) || ((MODE) == TIM_ENCODE_MODE_TI12))
/**
 * @}
 */

/** @addtogroup TIM_Event_Source
 * @{
 */

#define TIM_EVT_SRC_UPDATE  ((uint16_t)0x0001)
#define TIM_EVT_SRC_CC1     ((uint16_t)0x0002)
#define TIM_EVT_SRC_CC2     ((uint16_t)0x0004)
#define TIM_EVT_SRC_CC3     ((uint16_t)0x0008)
#define TIM_EVT_SRC_CC4     ((uint16_t)0x0010)
#define TIM_EVT_SRC_COM     ((uint16_t)0x0020)
#define TIM_EVT_SRC_TRIG    ((uint16_t)0x0040)
#define TIM_EVT_SRC_BREAK   ((uint16_t)0x0080)
#define IsTimEvtSrc(SOURCE) ((((SOURCE) & (uint16_t)0xFF00) == 0x0000) && ((SOURCE) != 0x0000))

/**
 * @}
 */

/** @addtogroup TIM_Update_Source
 * @{
 */

#define TIM_UPDATE_SRC_GLOBAL                                                                                          \
    ((uint16_t)0x0000)                            /*!< Source of update is the counter overflow/underflow              \
                                                         or the setting of UG bit, or an update generation             \
                                                         through the slave mode controller. */
#define TIM_UPDATE_SRC_REGULAr ((uint16_t)0x0001) /*!< Source of update is counter overflow/underflow. */
#define IsTimUpdateSrc(SOURCE) (((SOURCE) == TIM_UPDATE_SRC_GLOBAL) || ((SOURCE) == TIM_UPDATE_SRC_REGULAr))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_Preload_State
 * @{
 */

#define TIM_OC_PRE_LOAD_ENABLE     ((uint16_t)0x0008)
#define TIM_OC_PRE_LOAD_DISABLE    ((uint16_t)0x0000)
#define IsTimOcPreLoadState(STATE) (((STATE) == TIM_OC_PRE_LOAD_ENABLE) || ((STATE) == TIM_OC_PRE_LOAD_DISABLE))
/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_Fast_State
 * @{
 */

#define TIM_OC_FAST_ENABLE      ((uint16_t)0x0004)
#define TIM_OC_FAST_DISABLE     ((uint16_t)0x0000)
#define IsTimOcFastState(STATE) (((STATE) == TIM_OC_FAST_ENABLE) || ((STATE) == TIM_OC_FAST_DISABLE))

/**
 * @}
 */

/** @addtogroup TIM_Output_Compare_Clear_State
 * @{
 */

#define TIM_OC_CLR_ENABLE      ((uint16_t)0x0080)
#define TIM_OC_CLR_DISABLE     ((uint16_t)0x0000)
#define IsTimOcClrState(STATE) (((STATE) == TIM_OC_CLR_ENABLE) || ((STATE) == TIM_OC_CLR_DISABLE))
/**
 * @}
 */

/** @addtogroup TIM_Trigger_Output_Source
 * @{
 */

#define TIM_TRGO_SRC_RESET  ((uint16_t)0x0000)
#define TIM_TRGO_SRC_ENABLE ((uint16_t)0x0010)
#define TIM_TRGO_SRC_UPDATE ((uint16_t)0x0020)
#define TIM_TRGO_SRC_OC1    ((uint16_t)0x0030)
#define TIM_TRGO_SRC_OC1REF ((uint16_t)0x0040)
#define TIM_TRGO_SRC_OC2REF ((uint16_t)0x0050)
#define TIM_TRGO_SRC_OC3REF ((uint16_t)0x0060)
#define TIM_TRGO_SRC_OC4REF ((uint16_t)0x0070)
#define IsTimTrgoSrc(SOURCE)                                                                                           \
    (((SOURCE) == TIM_TRGO_SRC_RESET) || ((SOURCE) == TIM_TRGO_SRC_ENABLE) || ((SOURCE) == TIM_TRGO_SRC_UPDATE)        \
     || ((SOURCE) == TIM_TRGO_SRC_OC1) || ((SOURCE) == TIM_TRGO_SRC_OC1REF) || ((SOURCE) == TIM_TRGO_SRC_OC2REF)       \
     || ((SOURCE) == TIM_TRGO_SRC_OC3REF) || ((SOURCE) == TIM_TRGO_SRC_OC4REF))
/**
 * @}
 */

/** @defgroup ETR selection 
  * @{
  */
#define TIM_ETR_Seletct_ExtGpio                 ((uint16_t)0x0000)
#define TIM_ETR_Seletct_innerTsc                ((uint16_t)0x0100)
/**
  * @}
  */ 

/** @addtogroup TIM_Slave_Mode
 * @{
 */

#define TIM_SLAVE_MODE_RESET ((uint16_t)0x0004)
#define TIM_SLAVE_MODE_GATED ((uint16_t)0x0005)
#define TIM_SLAVE_MODE_TRIG  ((uint16_t)0x0006)
#define TIM_SLAVE_MODE_EXT1  ((uint16_t)0x0007)
#define IsTimSlaveMode(MODE)                                                                                           \
    (((MODE) == TIM_SLAVE_MODE_RESET) || ((MODE) == TIM_SLAVE_MODE_GATED) || ((MODE) == TIM_SLAVE_MODE_TRIG)           \
     || ((MODE) == TIM_SLAVE_MODE_EXT1))
/**
 * @}
 */

/** @addtogroup TIM_Master_Slave_Mode
 * @{
 */

#define TIM_MASTER_SLAVE_MODE_ENABLE  ((uint16_t)0x0080)
#define TIM_MASTER_SLAVE_MODE_DISABLE ((uint16_t)0x0000)
#define IsTimMasterSlaveMode(STATE)                                                                                    \
    (((STATE) == TIM_MASTER_SLAVE_MODE_ENABLE) || ((STATE) == TIM_MASTER_SLAVE_MODE_DISABLE))
/**
 * @}
 */

/** @addtogroup TIM_Flags
 * @{
 */

#define TIM_FLAG_UPDATE ((uint32_t)0x0001)
#define TIM_FLAG_CC1    ((uint32_t)0x0002)
#define TIM_FLAG_CC2    ((uint32_t)0x0004)
#define TIM_FLAG_CC3    ((uint32_t)0x0008)
#define TIM_FLAG_CC4    ((uint32_t)0x0010)
#define TIM_FLAG_COM    ((uint32_t)0x0020)
#define TIM_FLAG_TRIG   ((uint32_t)0x0040)
#define TIM_FLAG_BREAK  ((uint32_t)0x0080)
#define TIM_FLAG_CC1OF  ((uint32_t)0x0200)
#define TIM_FLAG_CC2OF  ((uint32_t)0x0400)
#define TIM_FLAG_CC3OF  ((uint32_t)0x0800)
#define TIM_FLAG_CC4OF  ((uint32_t)0x1000)
#define TIM_FLAG_CC5    ((uint32_t)0x010000)
#define TIM_FLAG_CC6    ((uint32_t)0x020000)

#define IsTimGetFlag(FLAG)                                                                                             \
    (((FLAG) == TIM_FLAG_UPDATE) || ((FLAG) == TIM_FLAG_CC1) || ((FLAG) == TIM_FLAG_CC2) || ((FLAG) == TIM_FLAG_CC3)   \
     || ((FLAG) == TIM_FLAG_CC4) || ((FLAG) == TIM_FLAG_COM) || ((FLAG) == TIM_FLAG_TRIG)                              \
     || ((FLAG) == TIM_FLAG_BREAK) || ((FLAG) == TIM_FLAG_CC1OF) || ((FLAG) == TIM_FLAG_CC2OF)                         \
     || ((FLAG) == TIM_FLAG_CC3OF) || ((FLAG) == TIM_FLAG_CC4OF) || ((FLAG) == TIM_FLAG_CC5)                           \
     || ((FLAG) == TIM_FLAG_CC6))

#define IsTimClrFlag(TIM_FLAG) ((((TIM_FLAG) & (uint16_t)0xE100) == 0x0000) && ((TIM_FLAG) != 0x0000))
/**
 * @}
 */

/** @addtogroup TIM_Input_Capture_Filer_Value
 * @{
 */

#define IsTimInCapFilter(ICFILTER) ((ICFILTER) <= 0xF)
/**
 * @}
 */

/** @addtogroup TIM_External_Trigger_Filter
 * @{
 */

#define IsTimExtTrigFilter(EXTFILTER) ((EXTFILTER) <= 0xF)
/**
 * @}
 */

#define TIM_CC1EN        ((uint32_t)1<<0)
#define TIM_CC1NEN       ((uint32_t)1<<2)
#define TIM_CC2EN        ((uint32_t)1<<4)
#define TIM_CC2NEN       ((uint32_t)1<<6)
#define TIM_CC3EN        ((uint32_t)1<<8)
#define TIM_CC3NEN       ((uint32_t)1<<10)
#define TIM_CC4EN        ((uint32_t)1<<12)
#define TIM_CC5EN        ((uint32_t)1<<16)
#define TIM_CC6EN        ((uint32_t)1<<20)

#define IsAdvancedTimCCENFlag(FLAG)                                                                                             \
    (((FLAG) == TIM_CC1EN) || ((FLAG) == TIM_CC1NEN) || ((FLAG) == TIM_CC2EN) || ((FLAG) == TIM_CC2NEN)   \
     || ((FLAG) == TIM_CC3EN) || ((FLAG) == TIM_CC3NEN)                              \
     || ((FLAG) == TIM_CC4EN) || ((FLAG) == TIM_CC5EN) || ((FLAG) == TIM_CC6EN)   )                      
#define IsGeneralTimCCENFlag(FLAG)                                                                                             \
    (((FLAG) == TIM_CC1EN) ||  ((FLAG) == TIM_CC2EN)    \
     || ((FLAG) == TIM_CC3EN)                             \
     || ((FLAG) == TIM_CC4EN)      )               

/** @addtogroup TIM_Legacy
 * @{
 */

#define TIM_DMA_BURST_LEN_1BYTE   TIM_DMABURST_LENGTH_1TRANSFER
#define TIM_DMA_BURST_LEN_2BYTES  TIM_DMABURST_LENGTH_2TRANSFERS
#define TIM_DMA_BURST_LEN_3BYTES  TIM_DMABURST_LENGTH_3TRANSFERS
#define TIM_DMA_BURST_LEN_4BYTES  TIM_DMABURST_LENGTH_4TRANSFERS
#define TIM_DMA_BURST_LEN_5BYTES  TIM_DMABURST_LENGTH_5TRANSFERS
#define TIM_DMA_BURST_LEN_6BYTES  TIM_DMABURST_LENGTH_6TRANSFERS
#define TIM_DMA_BURST_LEN_7BYTES  TIM_DMABURST_LENGTH_7TRANSFERS
#define TIM_DMA_BURST_LEN_8BYTES  TIM_DMABURST_LENGTH_8TRANSFERS
#define TIM_DMA_BURST_LEN_9BYTES  TIM_DMABURST_LENGTH_9TRANSFERS
#define TIM_DMA_BURST_LEN_10BYTES TIM_DMABURST_LENGTH_10TRANSFERS
#define TIM_DMA_BURST_LEN_11BYTES TIM_DMABURST_LENGTH_11TRANSFERS
#define TIM_DMA_BURST_LEN_12BYTES TIM_DMABURST_LENGTH_12TRANSFERS
#define TIM_DMA_BURST_LEN_13BYTES TIM_DMABURST_LENGTH_13TRANSFERS
#define TIM_DMA_BURST_LEN_14BYTES TIM_DMABURST_LENGTH_14TRANSFERS
#define TIM_DMA_BURST_LEN_15BYTES TIM_DMABURST_LENGTH_15TRANSFERS
#define TIM_DMA_BURST_LEN_16BYTES TIM_DMABURST_LENGTH_16TRANSFERS
#define TIM_DMA_BURST_LEN_17BYTES TIM_DMABURST_LENGTH_17TRANSFERS
#define TIM_DMA_BURST_LEN_18BYTES TIM_DMABURST_LENGTH_18TRANSFERS
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup TIM_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup TIM_Exported_Functions
 * @{
 */

void TIM_DeInit(TIM_Module* TIMx);
void TIM_InitTimeBase(TIM_Module* TIMx, TIM_TimeBaseInitType* TIM_TimeBaseInitStruct);
void TIM_InitOc1(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_InitOc2(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_InitOc3(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_InitOc4(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_InitOc5(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_InitOc6(TIM_Module* TIMx, OCInitType* TIM_OCInitStruct);
void TIM_ICInit(TIM_Module* TIMx, TIM_ICInitType* TIM_ICInitStruct);
void TIM_ConfigPwmIc(TIM_Module* TIMx, TIM_ICInitType* TIM_ICInitStruct);
void TIM_ConfigBkdt(TIM_Module* TIMx, TIM_BDTRInitType* TIM_BDTRInitStruct);
void TIM_InitTimBaseStruct(TIM_TimeBaseInitType* TIM_TimeBaseInitStruct);
void TIM_InitOcStruct(OCInitType* TIM_OCInitStruct);
void TIM_InitIcStruct(TIM_ICInitType* TIM_ICInitStruct);
void TIM_InitBkdtStruct(TIM_BDTRInitType* TIM_BDTRInitStruct);
void TIM_Enable(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_EnableCtrlPwmOutputs(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_ConfigInt(TIM_Module* TIMx, uint16_t TIM_IT, FunctionalState Cmd);
void TIM_GenerateEvent(TIM_Module* TIMx, uint16_t TIM_EventSource);
void TIM_ConfigDma(TIM_Module* TIMx, uint16_t TIM_DMABase, uint16_t TIM_DMABurstLength);
void TIM_EnableDma(TIM_Module* TIMx, uint16_t TIM_DMASource, FunctionalState Cmd);
void TIM_ConfigInternalClk(TIM_Module* TIMx);
void TIM_ConfigInternalTrigToExt(TIM_Module* TIMx, uint16_t TIM_InputTriggerSource);
void TIM_ConfigExtTrigAsClk(TIM_Module* TIMx,
                            uint16_t TIM_TIxExternalCLKSource,
                            uint16_t IcPolarity,
                            uint16_t ICFilter);
void TIM_ConfigExtClkMode1(TIM_Module* TIMx,
                           uint16_t TIM_ExtTRGPrescaler,
                           uint16_t TIM_ExtTRGPolarity,
                           uint16_t ExtTRGFilter);
void TIM_ConfigExtClkMode2(TIM_Module* TIMx,
                           uint16_t TIM_ExtTRGPrescaler,
                           uint16_t TIM_ExtTRGPolarity,
                           uint16_t ExtTRGFilter);
void TIM_ConfigExtTrig(TIM_Module* TIMx,
                       uint16_t TIM_ExtTRGPrescaler,
                       uint16_t TIM_ExtTRGPolarity,
                       uint16_t ExtTRGFilter);
void TIM_ConfigPrescaler(TIM_Module* TIMx, uint16_t Prescaler, uint16_t TIM_PSCReloadMode);
void TIM_ConfigCntMode(TIM_Module* TIMx, uint16_t CntMode);
void TIM_SelectInputTrig(TIM_Module* TIMx, uint16_t TIM_InputTriggerSource);
void TIM_ConfigEncoderInterface(TIM_Module* TIMx,
                                uint16_t TIM_EncoderMode,
                                uint16_t TIM_IC1Polarity,
                                uint16_t TIM_IC2Polarity);
void TIM_ConfigForcedOc1(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigForcedOc2(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigForcedOc3(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigForcedOc4(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigForcedOc5(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigForcedOc6(TIM_Module* TIMx, uint16_t TIM_ForcedAction);
void TIM_ConfigArPreload(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_SelectComEvt(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_SelectCapCmpDmaSrc(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_EnableCapCmpPreloadControl(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_ConfigOc1Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc2Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc3Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc4Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc5Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc6Preload(TIM_Module* TIMx, uint16_t TIM_OCPreload);
void TIM_ConfigOc1Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ConfigOc2Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ConfigOc3Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ConfigOc4Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ConfigOc5Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ConfigOc6Fast(TIM_Module* TIMx, uint16_t TIM_OCFast);
void TIM_ClrOc1Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ClrOc2Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ClrOc3Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ClrOc4Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ClrOc5Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ClrOc6Ref(TIM_Module* TIMx, uint16_t TIM_OCClear);
void TIM_ConfigOc1Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_ConfigOc1NPolarity(TIM_Module* TIMx, uint16_t OcNPolarity);
void TIM_ConfigOc2Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_ConfigOc2NPolarity(TIM_Module* TIMx, uint16_t OcNPolarity);
void TIM_ConfigOc3Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_ConfigOc3NPolarity(TIM_Module* TIMx, uint16_t OcNPolarity);
void TIM_ConfigOc4Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_ConfigOc5Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_ConfigOc6Polarity(TIM_Module* TIMx, uint16_t OcPolarity);
void TIM_EnableCapCmpCh(TIM_Module* TIMx, uint16_t Channel, uint32_t TIM_CCx);
void TIM_EnableCapCmpChN(TIM_Module* TIMx, uint16_t Channel, uint32_t TIM_CCxN);
void TIM_SelectOcMode(TIM_Module* TIMx, uint16_t Channel, uint16_t OcMode);
void TIM_EnableUpdateEvt(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_ConfigUpdateEvt(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_ConfigUpdateRequestIntSrc(TIM_Module* TIMx, uint16_t TIM_UpdateSource);
void TIM_SelectHallSensor(TIM_Module* TIMx, FunctionalState Cmd);
void TIM_SelectOnePulseMode(TIM_Module* TIMx, uint16_t TIM_OPMode);
void TIM_SelectOutputTrig(TIM_Module* TIMx, uint16_t TIM_TRGOSource);
void TIM_SelectExtSignalSource(TIM_Module* TIMx, uint16_t ExtSigalSource);
void TIM_SelectSlaveMode(TIM_Module* TIMx, uint16_t TIM_SlaveMode);
void TIM_SelectMasterSlaveMode(TIM_Module* TIMx, uint16_t TIM_MasterSlaveMode);
void TIM_SetCnt(TIM_Module* TIMx, uint16_t Counter);
void TIM_SetAutoReload(TIM_Module* TIMx, uint16_t Autoreload);
void TIM_SetCmp1(TIM_Module* TIMx, uint16_t Compare1);
void TIM_SetCmp2(TIM_Module* TIMx, uint16_t Compare2);
void TIM_SetCmp3(TIM_Module* TIMx, uint16_t Compare3);
void TIM_SetCmp4(TIM_Module* TIMx, uint16_t Compare4);
void TIM_SetCmp5(TIM_Module* TIMx, uint16_t Compare5);
void TIM_SetCmp6(TIM_Module* TIMx, uint16_t Compare6);
void TIM_SetInCap1Prescaler(TIM_Module* TIMx, uint16_t TIM_ICPSC);
void TIM_SetInCap2Prescaler(TIM_Module* TIMx, uint16_t TIM_ICPSC);
void TIM_SetInCap3Prescaler(TIM_Module* TIMx, uint16_t TIM_ICPSC);
void TIM_SetInCap4Prescaler(TIM_Module* TIMx, uint16_t TIM_ICPSC);
void TIM_SetClkDiv(TIM_Module* TIMx, uint16_t TIM_CKD);
uint16_t TIM_GetCap1(TIM_Module* TIMx);
uint16_t TIM_GetCap2(TIM_Module* TIMx);
uint16_t TIM_GetCap3(TIM_Module* TIMx);
uint16_t TIM_GetCap4(TIM_Module* TIMx);
uint16_t TIM_GetCap5(TIM_Module* TIMx);
uint16_t TIM_GetCap6(TIM_Module* TIMx);
uint16_t TIM_GetCnt(TIM_Module* TIMx);
uint16_t TIM_GetPrescaler(TIM_Module* TIMx);
uint16_t TIM_GetAutoReload(TIM_Module* TIMx);
FlagStatus TIM_GetCCENStatus(TIM_Module* TIMx, uint32_t TIM_CCEN);
FlagStatus TIM_GetFlagStatus(TIM_Module* TIMx, uint32_t TIM_FLAG);
void TIM_ClearFlag(TIM_Module* TIMx, uint32_t TIM_FLAG);
INTStatus TIM_GetIntStatus(TIM_Module* TIMx, uint32_t TIM_IT);
void TIM_ClrIntPendingBit(TIM_Module* TIMx, uint32_t TIM_IT);

#ifdef __cplusplus
}
#endif

#endif /*__N32A455_TIM_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
