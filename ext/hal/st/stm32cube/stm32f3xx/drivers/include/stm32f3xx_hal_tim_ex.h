/**
  ******************************************************************************
  * @file    stm32f3xx_hal_tim_ex.h
  * @author  MCD Application Team
  * @brief   Header file of TIM HAL Extended module.
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
#ifndef __STM32F3xx_HAL_TIM_EX_H
#define __STM32F3xx_HAL_TIM_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @addtogroup TIMEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Types TIMEx Exported Types
  * @{
  */

/**
  * @brief  TIM Hall sensor Configuration Structure definition
  */

typedef struct
{

  uint32_t IC1Polarity;            /*!< Specifies the active edge of the input signal.
                                        This parameter can be a value of @ref TIM_Input_Capture_Polarity */

  uint32_t IC1Prescaler;        /*!< Specifies the Input Capture Prescaler.
                                     This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

  uint32_t IC1Filter;           /*!< Specifies the input capture filter.
                                     This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xFU */
  uint32_t Commutation_Delay;  /*!< Specifies the pulse value to be loaded into the Capture Compare Register.
                                    This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFFU */
} TIM_HallSensor_InitTypeDef;

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  TIM Master configuration Structure definition
  * @note   STM32F373xC and STM32F378xx: timer instances provide a single TRGO
  *         output
  */
typedef struct {
  uint32_t  MasterOutputTrigger;   /*!< Trigger output (TRGO) selection
                                      This parameter can be a value of @ref TIM_Master_Mode_Selection */
  uint32_t  MasterSlaveMode;       /*!< Master/slave mode selection
                                      This parameter can be a value of @ref TIM_Master_Slave_Mode */
}TIM_MasterConfigTypeDef;

/**
  * @brief  TIM Break and Dead time configuration Structure definition
  * @note   STM32F373xC and STM32F378xx: single break input with configurable polarity.
  */
typedef struct
{
  uint32_t OffStateRunMode;        /*!< TIM off state in run mode
                                         This parameter can be a value of @ref TIM_OSSR_Off_State_Selection_for_Run_mode_state */
  uint32_t OffStateIDLEMode;        /*!< TIM off state in IDLE mode
                                         This parameter can be a value of @ref TIM_OSSI_Off_State_Selection_for_Idle_mode_state */
  uint32_t LockLevel;           /*!< TIM Lock level
                                         This parameter can be a value of @ref TIM_Lock_level */
  uint32_t DeadTime;           /*!< TIM dead Time
                                         This parameter can be a number between Min_Data = 0x00 and Max_Data = 0xFFU */
  uint32_t BreakState;           /*!< TIM Break State
                                         This parameter can be a value of @ref TIM_Break_Input_enable_disable */
  uint32_t BreakPolarity;             /*!< TIM Break input polarity
                                         This parameter can be a value of @ref TIM_Break_Polarity */
  uint32_t AutomaticOutput;           /*!< TIM Automatic Output Enable state
                                         This parameter can be a value of @ref TIM_AOE_Bit_Set_Reset */
} TIM_BreakDeadTimeConfigTypeDef;

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  TIM Break input(s) and Dead time configuration Structure definition
  * @note   2 break inputs can be configured (BKIN and BKIN2) with configurable
  *        filter and polarity.
  */
typedef struct
{
  uint32_t OffStateRunMode;        /*!< TIM off state in run mode
                                         This parameter can be a value of @ref TIM_OSSR_Off_State_Selection_for_Run_mode_state */
  uint32_t OffStateIDLEMode;        /*!< TIM off state in IDLE mode
                                         This parameter can be a value of @ref TIM_OSSI_Off_State_Selection_for_Idle_mode_state */
  uint32_t LockLevel;           /*!< TIM Lock level
                                         This parameter can be a value of @ref TIM_Lock_level */
  uint32_t DeadTime;           /*!< TIM dead Time
                                         This parameter can be a number between Min_Data = 0x00 and Max_Data = 0xFFU */
  uint32_t BreakState;           /*!< TIM Break State
                                         This parameter can be a value of @ref TIM_Break_Input_enable_disable */
  uint32_t BreakPolarity;             /*!< TIM Break input polarity
                                         This parameter can be a value of @ref TIM_Break_Polarity */
  uint32_t BreakFilter;               /*!< Specifies the brek input filter.
                                         This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xFU */
  uint32_t Break2State;           /*!< TIM Break2 State
                                         This parameter can be a value of @ref TIMEx_Break2_Input_enable_disable */
  uint32_t Break2Polarity;            /*!< TIM Break2 input polarity
                                         This parameter can be a value of @ref TIMEx_Break2_Polarity */
  uint32_t Break2Filter;              /*!< TIM break2 input filter.
                                         This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xFU */
  uint32_t AutomaticOutput;           /*!< TIM Automatic Output Enable state
                                         This parameter can be a value of @ref TIM_AOE_Bit_Set_Reset */
} TIM_BreakDeadTimeConfigTypeDef;

/**
  * @brief  TIM Master configuration Structure definition
  * @note   Advanced timers provide TRGO2 internal line which is redirected
  *         to the ADC
  */
typedef struct {
  uint32_t  MasterOutputTrigger;   /*!< Trigger output (TRGO) selection
                                      This parameter can be a value of @ref TIM_Master_Mode_Selection */
  uint32_t  MasterOutputTrigger2;  /*!< Trigger output2 (TRGO2) selection
                                      This parameter can be a value of @ref TIMEx_Master_Mode_Selection_2 */
  uint32_t  MasterSlaveMode;       /*!< Master/slave mode selection
                                      This parameter can be a value of @ref TIM_Master_Slave_Mode */
}TIM_MasterConfigTypeDef;
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Constants TIMEx Exported Constants
  * @{
  */

#if defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup TIMEx_Channel TIMEx Channel
  * @{
  */
#define TIM_CHANNEL_1                      (0x0000U)
#define TIM_CHANNEL_2                      (0x0004U)
#define TIM_CHANNEL_3                      (0x0008U)
#define TIM_CHANNEL_4                      (0x000CU)
#define TIM_CHANNEL_ALL                    (0x0018U)
/**
  * @}
  */

/** @defgroup TIMEx_Output_Compare_and_PWM_modes TIMEx Output Compare and PWM Modes
  * @{
  */
#define TIM_OCMODE_TIMING                   (0x0000U)
#define TIM_OCMODE_ACTIVE                   ((uint32_t)TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_INACTIVE                 ((uint32_t)TIM_CCMR1_OC1M_1)
#define TIM_OCMODE_TOGGLE                   ((uint32_t)TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1)
#define TIM_OCMODE_PWM1                     ((uint32_t)TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)
#define TIM_OCMODE_PWM2                     ((uint32_t)TIM_CCMR1_OC1M)
#define TIM_OCMODE_FORCED_ACTIVE            ((uint32_t)TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2)
#define TIM_OCMODE_FORCED_INACTIVE          ((uint32_t)TIM_CCMR1_OC1M_2)
/**
  * @}
  */

/** @defgroup TIMEx_ClearInput_Source TIMEx Clear Input Source
  * @{
  */
#define TIM_CLEARINPUTSOURCE_ETR           (0x0001U)
#define TIM_CLEARINPUTSOURCE_NONE          (0x0000U)
/**
  * @}
  */

/** @defgroup TIMEx_Slave_Mode TIMEx Slave Mode
  * @{
  */
#define TIM_SLAVEMODE_DISABLE                (0x0000U)
#define TIM_SLAVEMODE_RESET                  ((uint32_t)(TIM_SMCR_SMS_2))
#define TIM_SLAVEMODE_GATED                  ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_0))
#define TIM_SLAVEMODE_TRIGGER                ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1))
#define TIM_SLAVEMODE_EXTERNAL1              ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0))
/**
  * @}
  */

/** @defgroup TIMEx_Event_Source TIMEx Event Source
  * @{
  */
#define TIM_EVENTSOURCE_UPDATE              TIM_EGR_UG     /*!< Reinitialize the counter and generates an update of the registers */
#define TIM_EVENTSOURCE_CC1                 TIM_EGR_CC1G   /*!< A capture/compare event is generated on channel 1U */
#define TIM_EVENTSOURCE_CC2                 TIM_EGR_CC2G   /*!< A capture/compare event is generated on channel 2U */
#define TIM_EVENTSOURCE_CC3                 TIM_EGR_CC3G   /*!< A capture/compare event is generated on channel 3U */
#define TIM_EVENTSOURCE_CC4                 TIM_EGR_CC4G   /*!< A capture/compare event is generated on channel 4U */
#define TIM_EVENTSOURCE_COM                 TIM_EGR_COMG   /*!< A commutation event is generated */
#define TIM_EVENTSOURCE_TRIGGER             TIM_EGR_TG     /*!< A trigger event is generated */
#define TIM_EVENTSOURCE_BREAK               TIM_EGR_BG     /*!< A break event is generated */
/**
  * @}
  */

/** @defgroup TIMEx_DMA_Base_address TIMEx DMA BAse Address
  * @{
  */
#define TIM_DMABASE_CR1                    (0x00000000U)
#define TIM_DMABASE_CR2                    (0x00000001U)
#define TIM_DMABASE_SMCR                   (0x00000002U)
#define TIM_DMABASE_DIER                   (0x00000003U)
#define TIM_DMABASE_SR                     (0x00000004U)
#define TIM_DMABASE_EGR                    (0x00000005U)
#define TIM_DMABASE_CCMR1                  (0x00000006U)
#define TIM_DMABASE_CCMR2                  (0x00000007U)
#define TIM_DMABASE_CCER                   (0x00000008U)
#define TIM_DMABASE_CNT                    (0x00000009U)
#define TIM_DMABASE_PSC                    (0x0000000AU)
#define TIM_DMABASE_ARR                    (0x0000000BU)
#define TIM_DMABASE_RCR                    (0x0000000CU)
#define TIM_DMABASE_CCR1                   (0x0000000DU)
#define TIM_DMABASE_CCR2                   (0x0000000EU)
#define TIM_DMABASE_CCR3                   (0x0000000FU)
#define TIM_DMABASE_CCR4                   (0x00000010U)
#define TIM_DMABASE_BDTR                   (0x00000011U)
#define TIM_DMABASE_DCR                    (0x00000012U)
#define TIM_DMABASE_OR                     (0x00000013U)
/**
  * @}
  */
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup TIMEx_Channel TIMEx Channel
  * @{
  */
#define TIM_CHANNEL_1                      (0x0000U)
#define TIM_CHANNEL_2                      (0x0004U)
#define TIM_CHANNEL_3                      (0x0008U)
#define TIM_CHANNEL_4                      (0x000CU)
#define TIM_CHANNEL_5                      (0x0010U)
#define TIM_CHANNEL_6                      (0x0014U)
#define TIM_CHANNEL_ALL                    (0x003CU)
/**
  * @}
  */

/** @defgroup TIMEx_Output_Compare_and_PWM_modes TIMEx Output Compare and PWM Modes
  * @{
  */
#define TIM_OCMODE_TIMING                   (0x0000U)
#define TIM_OCMODE_ACTIVE                   ((uint32_t)TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_INACTIVE                 ((uint32_t)TIM_CCMR1_OC1M_1)
#define TIM_OCMODE_TOGGLE                   ((uint32_t)TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_PWM1                     ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1)
#define TIM_OCMODE_PWM2                     ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_FORCED_ACTIVE            ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_FORCED_INACTIVE          ((uint32_t)TIM_CCMR1_OC1M_2)

#define TIM_OCMODE_RETRIGERRABLE_OPM1      ((uint32_t)TIM_CCMR1_OC1M_3)
#define TIM_OCMODE_RETRIGERRABLE_OPM2      ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_COMBINED_PWM1           ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2)
#define TIM_OCMODE_COMBINED_PWM2           ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2)
#define TIM_OCMODE_ASSYMETRIC_PWM1         ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)
#define TIM_OCMODE_ASSYMETRIC_PWM2         ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M)

/**
  * @}
  */

/** @defgroup TIMEx_ClearInput_Source TIMEx Clear Input Source
  * @{
  */
#define TIM_CLEARINPUTSOURCE_ETR            (0x0001U)
#define TIM_CLEARINPUTSOURCE_OCREFCLR       (0x0002U)
#define TIM_CLEARINPUTSOURCE_NONE           (0x0000U)
/**
  * @}
  */

/** @defgroup TIMEx_Break2_Input_enable_disable  TIMEX Break input 2 Enable
  * @{
  */
#define TIM_BREAK2_DISABLE         (0x00000000U)
#define TIM_BREAK2_ENABLE          ((uint32_t)TIM_BDTR_BK2E)
/**
  * @}
  */

/** @defgroup TIMEx_Break2_Polarity TIMEx Break Input 2 Polarity
  * @{
  */
#define TIM_BREAK2POLARITY_LOW        (0x00000000U)
#define TIM_BREAK2POLARITY_HIGH       ((uint32_t)TIM_BDTR_BK2P)
/**
  * @}
  */

/** @defgroup TIMEx_Master_Mode_Selection_2 TIMEx Master Mode Selection 2 (TRGO2)
  * @{
  */
#define  TIM_TRGO2_RESET                          (0x00000000U)
#define  TIM_TRGO2_ENABLE                         ((uint32_t)(TIM_CR2_MMS2_0))
#define  TIM_TRGO2_UPDATE                         ((uint32_t)(TIM_CR2_MMS2_1))
#define  TIM_TRGO2_OC1                            ((uint32_t)(TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC1REF                         ((uint32_t)(TIM_CR2_MMS2_2))
#define  TIM_TRGO2_OC2REF                         ((uint32_t)(TIM_CR2_MMS2_2 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC3REF                         ((uint32_t)(TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1))
#define  TIM_TRGO2_OC4REF                         ((uint32_t)(TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC5REF                         ((uint32_t)(TIM_CR2_MMS2_3))
#define  TIM_TRGO2_OC6REF                         ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC4REF_RISINGFALLING           ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_1))
#define  TIM_TRGO2_OC6REF_RISINGFALLING           ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC4REF_RISING_OC6REF_RISING    ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2))
#define  TIM_TRGO2_OC4REF_RISING_OC6REF_FALLING   ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 | TIM_CR2_MMS2_0))
#define  TIM_TRGO2_OC5REF_RISING_OC6REF_RISING    ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 |TIM_CR2_MMS2_1))
#define  TIM_TRGO2_OC5REF_RISING_OC6REF_FALLING   ((uint32_t)(TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0))
/**
  * @}
  */

/** @defgroup TIMEx_Slave_Mode TIMEx Slave mode
  * @{
  */
#define TIM_SLAVEMODE_DISABLE                (0x0000U)
#define TIM_SLAVEMODE_RESET                  ((uint32_t)(TIM_SMCR_SMS_2))
#define TIM_SLAVEMODE_GATED                  ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_0))
#define TIM_SLAVEMODE_TRIGGER                ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1))
#define TIM_SLAVEMODE_EXTERNAL1              ((uint32_t)(TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0))
#define TIM_SLAVEMODE_COMBINED_RESETTRIGGER  ((uint32_t)(TIM_SMCR_SMS_3))
/**
  * @}
  */

/** @defgroup TIM_Event_Source TIMEx Event Source
  * @{
  */
#define TIM_EVENTSOURCE_UPDATE              TIM_EGR_UG     /*!< Reinitialize the counter and generates an update of the registers */
#define TIM_EVENTSOURCE_CC1                 TIM_EGR_CC1G   /*!< A capture/compare event is generated on channel 1U */
#define TIM_EVENTSOURCE_CC2                 TIM_EGR_CC2G   /*!< A capture/compare event is generated on channel 2U */
#define TIM_EVENTSOURCE_CC3                 TIM_EGR_CC3G   /*!< A capture/compare event is generated on channel 3U */
#define TIM_EVENTSOURCE_CC4                 TIM_EGR_CC4G   /*!< A capture/compare event is generated on channel 4U */
#define TIM_EVENTSOURCE_COM                 TIM_EGR_COMG   /*!< A commutation event is generated */
#define TIM_EVENTSOURCE_TRIGGER             TIM_EGR_TG     /*!< A trigger event is generated */
#define TIM_EVENTSOURCE_BREAK               TIM_EGR_BG     /*!< A break event is generated */
#define TIM_EVENTSOURCE_BREAK2              TIM_EGR_B2G    /*!< A break 2 event is generated */
/**
  * @}
  */

/** @defgroup TIM_DMA_Base_address TIMEx DMA Base Address
  * @{
  */
#define TIM_DMABASE_CR1                    (0x00000000U)
#define TIM_DMABASE_CR2                    (0x00000001U)
#define TIM_DMABASE_SMCR                   (0x00000002U)
#define TIM_DMABASE_DIER                   (0x00000003U)
#define TIM_DMABASE_SR                     (0x00000004U)
#define TIM_DMABASE_EGR                    (0x00000005U)
#define TIM_DMABASE_CCMR1                  (0x00000006U)
#define TIM_DMABASE_CCMR2                  (0x00000007U)
#define TIM_DMABASE_CCER                   (0x00000008U)
#define TIM_DMABASE_CNT                    (0x00000009U)
#define TIM_DMABASE_PSC                    (0x0000000AU)
#define TIM_DMABASE_ARR                    (0x0000000BU)
#define TIM_DMABASE_RCR                    (0x0000000CU)
#define TIM_DMABASE_CCR1                   (0x0000000DU)
#define TIM_DMABASE_CCR2                   (0x0000000EU)
#define TIM_DMABASE_CCR3                   (0x0000000FU)
#define TIM_DMABASE_CCR4                   (0x00000010U)
#define TIM_DMABASE_BDTR                   (0x00000011U)
#define TIM_DMABASE_DCR                    (0x00000012U)
#define TIM_DMABASE_CCMR3                  (0x00000015U)
#define TIM_DMABASE_CCR5                   (0x00000016U)
#define TIM_DMABASE_CCR6                   (0x00000017U)
#define TIM_DMABASE_OR                     (0x00000018U)
/**
  * @}
  */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE)                                                 || \
    defined(STM32F302xC)                                                 || \
    defined(STM32F303x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup TIMEx_Remap TIMEx Remapping
  * @{
  */
#define TIM_TIM1_ADC1_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC1_AWD1                     (0x00000001U) /*!< TIM1_ETR is connected to ADC1 AWD1 */
#define TIM_TIM1_ADC1_AWD2                     (0x00000002U) /*!< TIM1_ETR is connected to ADC1 AWD2 */
#define TIM_TIM1_ADC1_AWD3                     (0x00000003U) /*!< TIM1_ETR is connected to ADC1 AWD3 */
#define TIM_TIM16_GPIO                         (0x00000000U) /*!< TIM16 TI1 is connected to GPIO */
#define TIM_TIM16_RTC                          (0x00000001U) /*!< TIM16 TI1 is connected to RTC_clock */
#define TIM_TIM16_HSE                          (0x00000002U) /*!< TIM16 TI1 is connected to HSE/32U */
#define TIM_TIM16_MCO                          (0x00000003U) /*!< TIM16 TI1 is connected to MCO */
/**
  * @}
  */
#endif /* STM32F302xE                               || */
       /* STM32F302xC                               || */
       /* STM32F303x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */


#if defined(STM32F334x8)
/** @defgroup TIMEx_Remap TIMEx Remapping 1
  * @{
  */
#define TIM_TIM1_ADC1_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC1_AWD1                     (0x00000001U) /*!< TIM1_ETR is connected to ADC1 AWD1 */
#define TIM_TIM1_ADC1_AWD2                     (0x00000002U) /*!< TIM1_ETR is connected to ADC1 AWD2 */
#define TIM_TIM1_ADC1_AWD3                     (0x00000003U) /*!< TIM1_ETR is connected to ADC1 AWD3 */
#define TIM_TIM16_GPIO                         (0x00000000U) /*!< TIM16 TI1 is connected to GPIO */
#define TIM_TIM16_RTC                          (0x00000001U) /*!< TIM16 TI1 is connected to RTC_clock */
#define TIM_TIM16_HSE                          (0x00000002U) /*!< TIM16 TI1 is connected to HSE/32U */
#define TIM_TIM16_MCO                          (0x00000003U) /*!< TIM16 TI1 is connected to MCO */
/**
  * @}
  */

/** @defgroup TIMEx_Remap2 TIMEx Remapping 2
  * @{
  */
#define TIM_TIM1_ADC2_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC2_AWD1                     (0x00000004U) /*!< TIM1_ETR is connected to ADC2 AWD1 */
#define TIM_TIM1_ADC2_AWD2                     (0x00000008U) /*!< TIM1_ETR is connected to ADC2 AWD2 */
#define TIM_TIM1_ADC2_AWD3                     (0x0000000CU) /*!< TIM1_ETR is connected to ADC2 AWD3 */
#define TIM_TIM16_NONE                         (0x00000000U) /*!< Non significant value for TIM16U */
/**
  * @}
  */
#endif /* STM32F334x8 */

#if defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup TIMEx_Remap TIMEx Remapping 1
  * @{
  */
#define TIM_TIM1_ADC1_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC1_AWD1                     (0x00000001U) /*!< TIM1_ETR is connected to ADC1 AWD1 */
#define TIM_TIM1_ADC1_AWD2                     (0x00000002U) /*!< TIM1_ETR is connected to ADC1 AWD2 */
#define TIM_TIM1_ADC1_AWD3                     (0x00000003U) /*!< TIM1_ETR is connected to ADC1 AWD3 */
#define TIM_TIM8_ADC2_NONE                     (0x00000000U) /*!< TIM8_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM8_ADC2_AWD1                     (0x00000001U) /*!< TIM8_ETR is connected to ADC2 AWD1 */
#define TIM_TIM8_ADC2_AWD2                     (0x00000002U) /*!< TIM8_ETR is connected to ADC2 AWD2 */
#define TIM_TIM8_ADC2_AWD3                     (0x00000003U) /*!< TIM8_ETR is connected to ADC2 AWD3 */
#define TIM_TIM16_GPIO                         (0x00000000U) /*!< TIM16 TI1 is connected to GPIO */
#define TIM_TIM16_RTC                          (0x00000001U) /*!< TIM16 TI1 is connected to RTC_clock */
#define TIM_TIM16_HSE                          (0x00000002U) /*!< TIM16 TI1 is connected to HSE/32U */
#define TIM_TIM16_MCO                          (0x00000003U) /*!< TIM16 TI1 is connected to MCO */
/**
  * @}
  */

/** @defgroup TIMEx_Remap2 TIMEx Remapping 2
  * @{
  */
#define TIM_TIM1_ADC4_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC4_AWD1                     (0x00000004U) /*!< TIM1_ETR is connected to ADC4 AWD1 */
#define TIM_TIM1_ADC4_AWD2                     (0x00000008U) /*!< TIM1_ETR is connected to ADC4 AWD2 */
#define TIM_TIM1_ADC4_AWD3                     (0x0000000CU) /*!< TIM1_ETR is connected to ADC4 AWD3 */
#define TIM_TIM8_ADC3_NONE                     (0x00000000U) /*!< TIM8_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM8_ADC3_AWD1                     (0x00000004U) /*!< TIM8_ETR is connected to ADC3 AWD1 */
#define TIM_TIM8_ADC3_AWD2                     (0x00000008U) /*!< TIM8_ETR is connected to ADC3 AWD2 */
#define TIM_TIM8_ADC3_AWD3                     (0x0000000CU) /*!< TIM8_ETR is connected to ADC3 AWD3 */
#define TIM_TIM16_NONE                         (0x00000000U) /*!< Non significant value for TIM16U */
/**
  * @}
  */
#endif /* STM32F303xC || STM32F358xx */

#if defined(STM32F303xE) || defined(STM32F398xx)
/** @defgroup TIMEx_Remap TIMEx Remapping 1
  * @{
  */
#define TIM_TIM1_ADC1_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC1_AWD1                     (0x00000001U) /*!< TIM1_ETR is connected to ADC1 AWD1 */
#define TIM_TIM1_ADC1_AWD2                     (0x00000002U) /*!< TIM1_ETR is connected to ADC1 AWD2 */
#define TIM_TIM1_ADC1_AWD3                     (0x00000003U) /*!< TIM1_ETR is connected to ADC1 AWD3 */
#define TIM_TIM8_ADC2_NONE                     (0x00000000U) /*!< TIM8_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM8_ADC2_AWD1                     (0x00000001U) /*!< TIM8_ETR is connected to ADC2 AWD1 */
#define TIM_TIM8_ADC2_AWD2                     (0x00000002U) /*!< TIM8_ETR is connected to ADC2 AWD2 */
#define TIM_TIM8_ADC2_AWD3                     (0x00000003U) /*!< TIM8_ETR is connected to ADC2 AWD3 */
#define TIM_TIM16_GPIO                         (0x00000000U) /*!< TIM16 TI1 is connected to GPIO */
#define TIM_TIM16_RTC                          (0x00000001U) /*!< TIM16 TI1 is connected to RTC_clock */
#define TIM_TIM16_HSE                          (0x00000002U) /*!< TIM16 TI1 is connected to HSE/32U */
#define TIM_TIM16_MCO                          (0x00000003U) /*!< TIM16 TI1 is connected to MCO */
#define TIM_TIM20_ADC3_NONE                    (0x00000000U) /*!< TIM20_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM20_ADC3_AWD1                    (0x00000001U) /*!< TIM20_ETR is connected to ADC3 AWD1 */
#define TIM_TIM20_ADC3_AWD2                    (0x00000002U) /*!< TIM20_ETR is connected to ADC3 AWD2 */
#define TIM_TIM20_ADC3_AWD3                    (0x00000003U) /*!< TIM20_ETR is connected to ADC3 AWD3 */
/**
  * @}
  */

/** @defgroup TIMEx_Remap2 TIMEx Remapping 2
  * @{
  */
#define TIM_TIM1_ADC4_NONE                     (0x00000000U) /*!< TIM1_ETR is not connected to any AWD (analog watchdog)*/
#define TIM_TIM1_ADC4_AWD1                     (0x00000004U) /*!< TIM1_ETR is connected to ADC4 AWD1 */
#define TIM_TIM1_ADC4_AWD2                     (0x00000008U) /*!< TIM1_ETR is connected to ADC4 AWD2 */
#define TIM_TIM1_ADC4_AWD3                     (0x0000000CU) /*!< TIM1_ETR is connected to ADC4 AWD3 */
#define TIM_TIM8_ADC3_NONE                     (0x00000000U) /*!< TIM8_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM8_ADC3_AWD1                     (0x00000004U) /*!< TIM8_ETR is connected to ADC3 AWD1 */
#define TIM_TIM8_ADC3_AWD2                     (0x00000008U) /*!< TIM8_ETR is connected to ADC3 AWD2 */
#define TIM_TIM8_ADC3_AWD3                     (0x0000000CU) /*!< TIM8_ETR is connected to ADC3 AWD3 */
#define TIM_TIM16_NONE                         (0x00000000U) /*!< Non significant value for TIM16U */
#define TIM_TIM20_ADC4_NONE                    (0x00000000U) /*!< TIM20_ETR is not connected to any AWD (analog watchdog) */
#define TIM_TIM20_ADC4_AWD1                    (0x00000004U) /*!< TIM20_ETR is connected to ADC4 AWD1 */
#define TIM_TIM20_ADC4_AWD2                    (0x00000008U) /*!< TIM20_ETR is connected to ADC4 AWD2 */
#define TIM_TIM20_ADC4_AWD3                    (0x0000000CU) /*!< TIM20_ETR is connected to ADC4 AWD3 */
/**
  * @}
  */
#endif /* STM32F303xE || STM32F398xx */


#if defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup TIMEx_Remap TIMEx remapping
  * @{
  */
#define TIM_TIM2_TIM8_TRGO      (0x00000000U)  /*!< TIM8 TRGOUT is connected to TIM2_ITR1 */
#define TIM_TIM2_ETH_PTP        (0x00000400U)  /*!< PTP trigger output is connected to TIM2_ITR1 */
#define TIM_TIM2_USBFS_SOF      (0x00000800U)  /*!< OTG FS SOF is connected to the TIM2_ITR1 input */
#define TIM_TIM2_USBHS_SOF      (0x00000C00U)  /*!< OTG HS SOF is connected to the TIM2_ITR1 input */
#define TIM_TIM14_GPIO          (0x00000000U) /*!< TIM14 TI1 is connected to GPIO */
#define TIM_TIM14_RTC           (0x00000001U) /*!< TIM14 TI1 is connected to RTC_clock */
#define TIM_TIM14_HSE           (0x00000002U) /*!< TIM14 TI1 is connected to HSE/32U */
#define TIM_TIM14_MCO           (0x00000003U) /*!< TIM14 TI1 is connected to MCO */
/**
  * @}
  */
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup TIMEx_Group_Channel5 Group Channel 5 and Channel 1U, 2 or 3
  * @{
  */
#define TIM_GROUPCH5_NONE       0x00000000  /*!< No effect of OC5REF on OC1REFC, OC2REFC and OC3REFC */
#define TIM_GROUPCH5_OC1REFC    (TIM_CCR5_GC5C1)      /*!< OC1REFC is the logical AND of OC1REFC and OC5REF */
#define TIM_GROUPCH5_OC2REFC    (TIM_CCR5_GC5C2)      /*!< OC2REFC is the logical AND of OC2REFC and OC5REF */
#define TIM_GROUPCH5_OC3REFC    (TIM_CCR5_GC5C3)       /*!< OC3REFC is the logical AND of OC3REFC and OC5REF */
/**
  * @}
  */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

/**
  * @}
  */


/* Private Macros -----------------------------------------------------------*/
/** @defgroup TIM_Private_Macros TIM Private Macros
  * @{
  */
#if defined(STM32F373xC) || defined(STM32F378xx)

#define IS_TIM_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                  ((CHANNEL) == TIM_CHANNEL_2) || \
                                  ((CHANNEL) == TIM_CHANNEL_3) || \
                                  ((CHANNEL) == TIM_CHANNEL_4) || \
                                  ((CHANNEL) == TIM_CHANNEL_ALL))

#define IS_TIM_OPM_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                      ((CHANNEL) == TIM_CHANNEL_2))

#define IS_TIM_COMPLEMENTARY_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                                ((CHANNEL) == TIM_CHANNEL_2) || \
                                                ((CHANNEL) == TIM_CHANNEL_3))

#define IS_TIM_PWM_MODE(MODE) (((MODE) == TIM_OCMODE_PWM1) || \
	                       ((MODE) == TIM_OCMODE_PWM2))

#define IS_TIM_OC_MODE(MODE) (((MODE) == TIM_OCMODE_TIMING)           || \
                              ((MODE) == TIM_OCMODE_ACTIVE)           || \
                              ((MODE) == TIM_OCMODE_INACTIVE)         || \
                              ((MODE) == TIM_OCMODE_TOGGLE)           || \
                              ((MODE) == TIM_OCMODE_FORCED_ACTIVE)    || \
                              ((MODE) == TIM_OCMODE_FORCED_INACTIVE))

#define IS_TIM_CLEARINPUT_SOURCE(SOURCE)  (((SOURCE) == TIM_CLEARINPUTSOURCE_NONE) || \
                                           ((SOURCE) == TIM_CLEARINPUTSOURCE_ETR))

#define IS_TIM_SLAVE_MODE(MODE) (((MODE) == TIM_SLAVEMODE_DISABLE) || \
                                 ((MODE) == TIM_SLAVEMODE_RESET) || \
                                 ((MODE) == TIM_SLAVEMODE_GATED) || \
                                 ((MODE) == TIM_SLAVEMODE_TRIGGER) || \
                                 ((MODE) == TIM_SLAVEMODE_EXTERNAL1))

#define IS_TIM_EVENT_SOURCE(SOURCE) ((((SOURCE) & 0xFFFFFF00U) == 0x00000000U) && ((SOURCE) != 0x00000000U))

#define IS_TIM_DMA_BASE(BASE) (((BASE) == TIM_DMABASE_CR1) || \
                               ((BASE) == TIM_DMABASE_CR2) || \
                               ((BASE) == TIM_DMABASE_SMCR) || \
                               ((BASE) == TIM_DMABASE_DIER) || \
                               ((BASE) == TIM_DMABASE_SR) || \
                               ((BASE) == TIM_DMABASE_EGR) || \
                               ((BASE) == TIM_DMABASE_CCMR1) || \
                               ((BASE) == TIM_DMABASE_CCMR2) || \
                               ((BASE) == TIM_DMABASE_CCER) || \
                               ((BASE) == TIM_DMABASE_CNT) || \
                               ((BASE) == TIM_DMABASE_PSC) || \
                               ((BASE) == TIM_DMABASE_ARR) || \
                               ((BASE) == TIM_DMABASE_RCR) || \
                               ((BASE) == TIM_DMABASE_CCR1) || \
                               ((BASE) == TIM_DMABASE_CCR2) || \
                               ((BASE) == TIM_DMABASE_CCR3) || \
                               ((BASE) == TIM_DMABASE_CCR4) || \
                               ((BASE) == TIM_DMABASE_BDTR) || \
                               ((BASE) == TIM_DMABASE_DCR) || \
                               ((BASE) == TIM_DMABASE_OR))

#endif /* STM32F373xC || STM32F378xx */



#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

#define IS_TIM_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                  ((CHANNEL) == TIM_CHANNEL_2) || \
                                  ((CHANNEL) == TIM_CHANNEL_3) || \
                                  ((CHANNEL) == TIM_CHANNEL_4) || \
                                  ((CHANNEL) == TIM_CHANNEL_5) || \
                                  ((CHANNEL) == TIM_CHANNEL_6) || \
                                  ((CHANNEL) == TIM_CHANNEL_ALL))

#define IS_TIM_OPM_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                      ((CHANNEL) == TIM_CHANNEL_2))

#define IS_TIM_COMPLEMENTARY_CHANNELS(CHANNEL) (((CHANNEL) == TIM_CHANNEL_1) || \
                                                ((CHANNEL) == TIM_CHANNEL_2) || \
                                                ((CHANNEL) == TIM_CHANNEL_3))

#define IS_TIM_PWM_MODE(MODE) (((MODE) == TIM_OCMODE_PWM1)               || \
	                       ((MODE) == TIM_OCMODE_PWM2)               || \
                               ((MODE) == TIM_OCMODE_COMBINED_PWM1)      || \
                               ((MODE) == TIM_OCMODE_COMBINED_PWM2)      || \
                               ((MODE) == TIM_OCMODE_ASSYMETRIC_PWM1)    || \
                               ((MODE) == TIM_OCMODE_ASSYMETRIC_PWM2))

#define IS_TIM_OC_MODE(MODE) (((MODE) == TIM_OCMODE_TIMING)             || \
                             ((MODE) == TIM_OCMODE_ACTIVE)             || \
                             ((MODE) == TIM_OCMODE_INACTIVE)           || \
                             ((MODE) == TIM_OCMODE_TOGGLE)             || \
                             ((MODE) == TIM_OCMODE_FORCED_ACTIVE)      || \
                             ((MODE) == TIM_OCMODE_FORCED_INACTIVE)    || \
                             ((MODE) == TIM_OCMODE_RETRIGERRABLE_OPM1) || \
                             ((MODE) == TIM_OCMODE_RETRIGERRABLE_OPM2))

#define IS_TIM_CLEARINPUT_SOURCE(MODE) (((MODE) == TIM_CLEARINPUTSOURCE_ETR)      || \
                                        ((MODE) == TIM_CLEARINPUTSOURCE_OCREFCLR)  || \
                                        ((MODE) == TIM_CLEARINPUTSOURCE_NONE))

#define IS_TIM_BREAK_FILTER(BRKFILTER) ((BRKFILTER) <= 0xFU)

#define IS_TIM_BREAK2_STATE(STATE) (((STATE) == TIM_BREAK2_ENABLE) || \
                                    ((STATE) == TIM_BREAK2_DISABLE))

#define IS_TIM_BREAK2_POLARITY(POLARITY) (((POLARITY) == TIM_BREAK2POLARITY_LOW) || \
                                          ((POLARITY) == TIM_BREAK2POLARITY_HIGH))

#define IS_TIM_TRGO2_SOURCE(SOURCE) (((SOURCE) == TIM_TRGO2_RESET)                        || \
                                     ((SOURCE) == TIM_TRGO2_ENABLE)                       || \
                                     ((SOURCE) == TIM_TRGO2_UPDATE)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC1)                          || \
                                     ((SOURCE) == TIM_TRGO2_OC1REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC2REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC3REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC3REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC4REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC5REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC6REF)                       || \
                                     ((SOURCE) == TIM_TRGO2_OC4REF_RISINGFALLING)         || \
                                     ((SOURCE) == TIM_TRGO2_OC6REF_RISINGFALLING)         || \
                                     ((SOURCE) == TIM_TRGO2_OC4REF_RISING_OC6REF_RISING)  || \
                                     ((SOURCE) == TIM_TRGO2_OC4REF_RISING_OC6REF_FALLING) || \
                                     ((SOURCE) == TIM_TRGO2_OC5REF_RISING_OC6REF_RISING)  || \
                                     ((SOURCE) == TIM_TRGO2_OC5REF_RISING_OC6REF_FALLING))

#define IS_TIM_SLAVE_MODE(MODE) (((MODE) == TIM_SLAVEMODE_DISABLE)   || \
                                 ((MODE) == TIM_SLAVEMODE_RESET)     || \
                                 ((MODE) == TIM_SLAVEMODE_GATED)     || \
                                 ((MODE) == TIM_SLAVEMODE_TRIGGER)   || \
                                 ((MODE) == TIM_SLAVEMODE_EXTERNAL1) || \
                                 ((MODE) == TIM_SLAVEMODE_COMBINED_RESETTRIGGER))

#define IS_TIM_EVENT_SOURCE(SOURCE) ((((SOURCE) & 0xFFFFFE00U) == 0x00000000U) && ((SOURCE) != 0x00000000U))

#define IS_TIM_DMA_BASE(BASE) (((BASE) == TIM_DMABASE_CR1)   || \
                               ((BASE) == TIM_DMABASE_CR2)   || \
                               ((BASE) == TIM_DMABASE_SMCR)  || \
                               ((BASE) == TIM_DMABASE_DIER)  || \
                               ((BASE) == TIM_DMABASE_SR)    || \
                               ((BASE) == TIM_DMABASE_EGR)   || \
                               ((BASE) == TIM_DMABASE_CCMR1) || \
                               ((BASE) == TIM_DMABASE_CCMR2) || \
                               ((BASE) == TIM_DMABASE_CCER)  || \
                               ((BASE) == TIM_DMABASE_CNT)   || \
                               ((BASE) == TIM_DMABASE_PSC)   || \
                               ((BASE) == TIM_DMABASE_ARR)   || \
                               ((BASE) == TIM_DMABASE_RCR)   || \
                               ((BASE) == TIM_DMABASE_CCR1)  || \
                               ((BASE) == TIM_DMABASE_CCR2)  || \
                               ((BASE) == TIM_DMABASE_CCR3)  || \
                               ((BASE) == TIM_DMABASE_CCR4)  || \
                               ((BASE) == TIM_DMABASE_BDTR)  || \
                               ((BASE) == TIM_DMABASE_CCMR3) || \
                               ((BASE) == TIM_DMABASE_CCR5)  || \
                               ((BASE) == TIM_DMABASE_CCR6)  || \
                               ((BASE) == TIM_DMABASE_OR))

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE)                                                 || \
    defined(STM32F302xC)                                                 || \
    defined(STM32F303x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

#define IS_TIM_REMAP(REMAP)    (((REMAP) == TIM_TIM1_ADC1_NONE) ||\
                                ((REMAP) == TIM_TIM1_ADC1_AWD1) ||\
                                ((REMAP) == TIM_TIM1_ADC1_AWD2) ||\
                                ((REMAP) == TIM_TIM1_ADC1_AWD3) ||\
                                ((REMAP) == TIM_TIM16_GPIO)     ||\
                                ((REMAP) == TIM_TIM16_RTC)      ||\
                                ((REMAP) == TIM_TIM16_HSE)      ||\
                                ((REMAP) == TIM_TIM16_MCO))

#endif /* STM32F302xE                               || */
       /* STM32F302xC                               || */
       /* STM32F303x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */

#if defined(STM32F334x8)
#define IS_TIM_REMAP(REMAP1)   (((REMAP1) == TIM_TIM1_ADC1_NONE) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD1) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD2) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD3) ||\
                                ((REMAP1) == TIM_TIM16_GPIO)     ||\
                                ((REMAP1) == TIM_TIM16_RTC)      ||\
                                ((REMAP1) == TIM_TIM16_HSE)      ||\
                                ((REMAP1) == TIM_TIM16_MCO))

#define IS_TIM_REMAP2(REMAP2)  (((REMAP2) == TIM_TIM1_ADC2_NONE) ||\
                                ((REMAP2) == TIM_TIM1_ADC2_AWD1) ||\
                                ((REMAP2) == TIM_TIM1_ADC2_AWD2) ||\
                                ((REMAP2) == TIM_TIM1_ADC2_AWD3) ||\
                                ((REMAP2) == TIM_TIM16_NONE))

#endif /* STM32F334x8 */

#if defined(STM32F303xC) || defined(STM32F358xx)

#define IS_TIM_REMAP(REMAP1)   (((REMAP1) == TIM_TIM1_ADC1_NONE) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD1) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD2) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD3) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_NONE) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD1) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD2) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD3) ||\
                                ((REMAP1) == TIM_TIM16_GPIO)     ||\
                                ((REMAP1) == TIM_TIM16_RTC)      ||\
                                ((REMAP1) == TIM_TIM16_HSE)      ||\
                                ((REMAP1) == TIM_TIM16_MCO))

#define IS_TIM_REMAP2(REMAP2)  (((REMAP2) == TIM_TIM1_ADC4_NONE) ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD1) ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD2) ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD3) ||\
                                ((REMAP2) == TIM_TIM8_ADC3_NONE) ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD1) ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD2) ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD3) ||\
                                ((REMAP2) == TIM_TIM16_NONE))

#endif /* STM32F303xC || STM32F358xx */

#if defined(STM32F303xE) || defined(STM32F398xx)

#define IS_TIM_REMAP(REMAP1)   (((REMAP1) == TIM_TIM1_ADC1_NONE) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD1) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD2) ||\
                                ((REMAP1) == TIM_TIM1_ADC1_AWD3) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_NONE) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD1) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD2) ||\
                                ((REMAP1) == TIM_TIM8_ADC2_AWD3) ||\
                                ((REMAP1) == TIM_TIM16_GPIO)     ||\
                                ((REMAP1) == TIM_TIM16_RTC)      ||\
                                ((REMAP1) == TIM_TIM16_HSE)      ||\
                                ((REMAP1) == TIM_TIM16_MCO)      ||\
                                ((REMAP1) == TIM_TIM20_ADC3_NONE) ||\
                                ((REMAP1) == TIM_TIM20_ADC3_AWD1) ||\
                                ((REMAP1) == TIM_TIM20_ADC3_AWD2) ||\
                                ((REMAP1) == TIM_TIM20_ADC3_AWD3))

#define IS_TIM_REMAP2(REMAP2)  (((REMAP2) == TIM_TIM1_ADC4_NONE)  ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD1)  ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD2)  ||\
                                ((REMAP2) == TIM_TIM1_ADC4_AWD3)  ||\
                                ((REMAP2) == TIM_TIM8_ADC3_NONE)  ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD1)  ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD2)  ||\
                                ((REMAP2) == TIM_TIM8_ADC3_AWD3)  ||\
                                ((REMAP2) == TIM_TIM16_NONE)      ||\
                                ((REMAP2) == TIM_TIM20_ADC4_NONE) ||\
                                ((REMAP2) == TIM_TIM20_ADC4_AWD1) ||\
                                ((REMAP2) == TIM_TIM20_ADC4_AWD2) ||\
                                ((REMAP2) == TIM_TIM20_ADC4_AWD3))

#endif /* STM32F303xE || STM32F398xx */

#if defined(STM32F373xC) || defined(STM32F378xx)

#define IS_TIM_REMAP(REMAP)    (((REMAP) == TIM_TIM2_TIM8_TRGO)  ||\
                                ((REMAP) == TIM_TIM2_ETH_PTP)    ||\
                                ((REMAP) == TIM_TIM2_USBFS_SOF)  ||\
                                ((REMAP) == TIM_TIM2_USBHS_SOF)  ||\
                                ((REMAP) == TIM_TIM14_GPIO)      ||\
                                ((REMAP) == TIM_TIM14_RTC)       ||\
                                ((REMAP) == TIM_TIM14_HSE)       ||\
                                ((REMAP) == TIM_TIM14_MCO))

#endif /* STM32F373xC || STM32F378xx */


#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

#define IS_TIM_GROUPCH5(OCREF) ((((OCREF) & 0x1FFFFFFFU) == 0x00000000U))

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#define IS_TIM_DEADTIME(DEADTIME)      ((DEADTIME) <= 0xFFU)

/**
  * @}
  */
/* End of private macros -----------------------------------------------------*/


/* Exported macro ------------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Macros TIMEx Exported Macros
  * @{
  */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Sets the TIM Capture Compare Register value on runtime without
  *         calling another time ConfigChannel function.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  * @param  __COMPARE__ specifies the Capture Compare register new value.
  * @retval None
  */
#define __HAL_TIM_SET_COMPARE(__HANDLE__, __CHANNEL__, __COMPARE__) \
(*(__IO uint32_t *)(&((__HANDLE__)->Instance->CCR1) + ((__CHANNEL__) >> 2U)) = (__COMPARE__))

/**
  * @brief  Gets the TIM Capture Compare Register value on runtime
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channel associated with the capture compare register
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: get capture/compare 1 register value
  *            @arg TIM_CHANNEL_2: get capture/compare 2 register value
  *            @arg TIM_CHANNEL_3: get capture/compare 3 register value
  *            @arg TIM_CHANNEL_4: get capture/compare 4 register value
  * @retval None
  */
#define __HAL_TIM_GET_COMPARE(__HANDLE__, __CHANNEL__) \
  (*(__IO uint32_t *)(&((__HANDLE__)->Instance->CCR1) + ((__CHANNEL__) >> 2U)))

/**
  * @brief  Sets the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  * @retval None
  */
#define __HAL_TIM_ENABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
        (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC1PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC2PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC3PE) :\
         ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC4PE))

/**
  * @brief  Resets the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  * @retval None
  */
#define __HAL_TIM_DISABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
        (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 &= (uint16_t)~TIM_CCMR1_OC1PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 &= (uint16_t)~TIM_CCMR1_OC2PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 &= (uint16_t)~TIM_CCMR2_OC3PE) :\
         ((__HANDLE__)->Instance->CCMR2 &= (uint16_t)~TIM_CCMR2_OC4PE))

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Sets the TIM Capture Compare Register value on runtime without
  *         calling another time ConfigChannel function.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @param  __COMPARE__ specifies the Capture Compare register new value.
  * @retval None
  */
#define __HAL_TIM_SET_COMPARE(__HANDLE__, __CHANNEL__, __COMPARE__) \
(((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCR1 = (__COMPARE__)) :\
 ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCR2 = (__COMPARE__)) :\
 ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCR3 = (__COMPARE__)) :\
 ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCR4 = (__COMPARE__)) :\
 ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCR5 = (__COMPARE__)) :\
 ((__HANDLE__)->Instance->CCR6 = (__COMPARE__)))

/**
  * @brief  Gets the TIM Capture Compare Register value on runtime
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channel associated with the capture compare register
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: get capture/compare 1 register value
  *            @arg TIM_CHANNEL_2: get capture/compare 2 register value
  *            @arg TIM_CHANNEL_3: get capture/compare 3 register value
  *            @arg TIM_CHANNEL_4: get capture/compare 4 register value
  *            @arg TIM_CHANNEL_5: get capture/compare 5 register value
  *            @arg TIM_CHANNEL_6: get capture/compare 6 register value
  * @retval None
  */
#define __HAL_TIM_GET_COMPARE(__HANDLE__, __CHANNEL__) \
(((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCR1) :\
 ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCR2) :\
 ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCR3) :\
 ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCR4) :\
 ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCR5) :\
 ((__HANDLE__)->Instance->CCR6))

/**
  * @brief  Sets the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @retval None
  */
#define __HAL_TIM_ENABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
        (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC1PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC2PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC3PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC4PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC5PE) :\
         ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC6PE))

/**
  * @brief  Resets the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @retval None
  */
#define __HAL_TIM_DISABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
        (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 &= (uint16_t)~TIM_CCMR1_OC1PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 &= (uint16_t)~TIM_CCMR1_OC2PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 &= (uint16_t)~TIM_CCMR2_OC3PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 &= (uint16_t)~TIM_CCMR2_OC4PE) :\
         ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 &= (uint16_t)~TIM_CCMR3_OC5PE) :\
         ((__HANDLE__)->Instance->CCMR3 &= (uint16_t)~TIM_CCMR3_OC6PE))

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup TIMEx_Exported_Functions
  * @{
  */

/** @addtogroup TIMEx_Exported_Functions_Group1
 * @{
 */
/*  Timer Hall Sensor functions  **********************************************/
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Init(TIM_HandleTypeDef *htim, TIM_HallSensor_InitTypeDef* sConfig);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_DeInit(TIM_HandleTypeDef *htim);

void HAL_TIMEx_HallSensor_MspInit(TIM_HandleTypeDef *htim);
void HAL_TIMEx_HallSensor_MspDeInit(TIM_HandleTypeDef *htim);

 /* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_IT(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_DMA(TIM_HandleTypeDef *htim, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_DMA(TIM_HandleTypeDef *htim);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group2
 * @{
 */
/*  Timer Complementary Output Compare functions  *****************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group3
 * @{
 */
/*  Timer Complementary PWM functions  ****************************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group4
 * @{
 */
/*  Timer Complementary One Pulse functions  **********************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop(TIM_HandleTypeDef *htim, uint32_t OutputChannel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group5
 * @{
 */
/* Extended Control functions  ************************************************/
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent_IT(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent_DMA(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim, TIM_MasterConfigTypeDef * sMasterConfig);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakDeadTime(TIM_HandleTypeDef *htim, TIM_BreakDeadTimeConfigTypeDef *sBreakDeadTimeConfig);

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx) || defined(STM32F334x8)
HAL_StatusTypeDef HAL_TIMEx_RemapConfig(TIM_HandleTypeDef *htim, uint32_t Remap1, uint32_t Remap2);
#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F302xE)                                                 || \
    defined(STM32F302xC)                                                 || \
    defined(STM32F303x8)  || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || \
    defined(STM32F373xC) || defined(STM32F378xx)
HAL_StatusTypeDef HAL_TIMEx_RemapConfig(TIM_HandleTypeDef *htim, uint32_t Remap);
#endif /* STM32F302xE                               || */
       /* STM32F302xC                               || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
HAL_StatusTypeDef HAL_TIMEx_GroupChannel5(TIM_HandleTypeDef *htim, uint32_t Channels);
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group6
  * @{
  */
/* Extended Callback *********************************************************/
void HAL_TIMEx_CommutationCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_BreakCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_Break2Callback(TIM_HandleTypeDef *htim);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group7
  * @{
  */
/* Extended Peripheral State functions  **************************************/
HAL_TIM_StateTypeDef HAL_TIMEx_HallSensor_GetState(TIM_HandleTypeDef *htim);
/**
  * @}
  */

/**
  * @}
  */
/* End of exported functions -------------------------------------------------*/

/* Private functions----------------------------------------------------------*/
/** @defgroup TIMEx_Private_Functions TIMEx Private Functions
  * @{
  */
void TIMEx_DMACommutationCplt(DMA_HandleTypeDef *hdma);
/**
  * @}
  */
/* End of private functions --------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif


#endif /* __STM32F3xx_HAL_TIM_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
