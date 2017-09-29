/**
  ******************************************************************************
  * @file    stm32l4xx_hal_comp.h
  * @author  MCD Application Team
  * @brief   Header file of COMP HAL module.
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
#ifndef __STM32L4xx_HAL_COMP_H
#define __STM32L4xx_HAL_COMP_H

#ifdef __cplusplus
 extern "C" {
#endif

#if defined (COMP1) || defined (COMP2)

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_ll_exti.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @addtogroup COMP
  * @{
  */

/* Exported types ------------------------------------------------------------*/ 
/** @defgroup COMP_Exported_Types COMP Exported Types
  * @{
  */

/** 
  * @brief  COMP Init structure definition  
  */
typedef struct
{

  uint32_t WindowMode;         /*!< Set window mode of a pair of comparators instances
                                    (2 consecutive instances odd and even COMP<x> and COMP<x+1>).
                                    Note: HAL COMP driver allows to set window mode from any COMP instance of the pair of COMP instances composing window mode.
                                    This parameter can be a value of @ref COMP_WindowMode */

  uint32_t Mode;               /*!< Set comparator operating mode to adjust power and speed.
                                    Note: For the characteritics of comparator power modes
                                          (propagation delay and power consumption), refer to device datasheet.
                                    This parameter can be a value of @ref COMP_PowerMode */

  uint32_t NonInvertingInput;  /*!< Set comparator input plus (non-inverting input).
                                    This parameter can be a value of @ref COMP_InputPlus */

  uint32_t InvertingInput;     /*!< Set comparator input minus (inverting input).
                                    This parameter can be a value of @ref COMP_InputMinus */

  uint32_t Hysteresis;         /*!< Set comparator hysteresis mode of the input minus.
                                    This parameter can be a value of @ref COMP_Hysteresis */

  uint32_t OutputPol;          /*!< Set comparator output polarity.
                                    This parameter can be a value of @ref COMP_OutputPolarity */

  uint32_t BlankingSrce;       /*!< Set comparator blanking source.
                                    This parameter can be a value of @ref COMP_BlankingSrce */

  uint32_t TriggerMode;        /*!< Set the comparator output triggering External Interrupt Line (EXTI).
                                    This parameter can be a value of @ref COMP_EXTI_TriggerMode */

}COMP_InitTypeDef;

/**
  * @brief  HAL COMP state machine: HAL COMP states definition
  */
#define COMP_STATE_BITFIELD_LOCK  (0x10U)
typedef enum
{
  HAL_COMP_STATE_RESET             = 0x00U,                                             /*!< COMP not yet initialized                             */
  HAL_COMP_STATE_RESET_LOCKED      = (HAL_COMP_STATE_RESET | COMP_STATE_BITFIELD_LOCK), /*!< COMP not yet initialized and configuration is locked */
  HAL_COMP_STATE_READY             = 0x01U,                                             /*!< COMP initialized and ready for use                   */
  HAL_COMP_STATE_READY_LOCKED      = (HAL_COMP_STATE_READY | COMP_STATE_BITFIELD_LOCK), /*!< COMP initialized but configuration is locked         */
  HAL_COMP_STATE_BUSY              = 0x02U,                                             /*!< COMP is running                                      */
  HAL_COMP_STATE_BUSY_LOCKED       = (HAL_COMP_STATE_BUSY | COMP_STATE_BITFIELD_LOCK)   /*!< COMP is running and configuration is locked          */
}HAL_COMP_StateTypeDef;

/** 
  * @brief  COMP Handle Structure definition
  */
typedef struct
{
  COMP_TypeDef       *Instance;       /*!< Register base address    */
  COMP_InitTypeDef   Init;            /*!< COMP required parameters */
  HAL_LockTypeDef    Lock;            /*!< Locking object           */
  __IO HAL_COMP_StateTypeDef  State;  /*!< COMP communication state */
} COMP_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup COMP_Exported_Constants COMP Exported Constants
  * @{
  */

/** @defgroup COMP_WindowMode COMP Window Mode
  * @{
  */
#define COMP_WINDOWMODE_DISABLE                 (0x00000000U)          /*!< Window mode disable: Comparators instances pair COMP1 and COMP2 are independent */
#define COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON (COMP_CSR_WINMODE)     /*!< Window mode enable: Comparators instances pair COMP1 and COMP2 have their input plus connected together. The common input is COMP1 input plus (COMP2 input plus is no more accessible). */
/**
  * @}
  */

/** @defgroup COMP_PowerMode COMP power mode
  * @{
  */
/* Note: For the characteritics of comparator power modes                     */
/*       (propagation delay and power consumption),                           */
/*       refer to device datasheet.                                           */
#define COMP_POWERMODE_HIGHSPEED       (0x00000000U)          /*!< High Speed */
#define COMP_POWERMODE_MEDIUMSPEED     (COMP_CSR_PWRMODE_0)   /*!< Medium Speed */
#define COMP_POWERMODE_ULTRALOWPOWER   (COMP_CSR_PWRMODE)     /*!< Ultra-low power mode */
/**
  * @}
  */

/** @defgroup COMP_InputPlus COMP input plus (non-inverting input)
  * @{
  */
#define COMP_INPUT_PLUS_IO1            (0x00000000U)          /*!< Comparator input plus connected to IO1 (pin PC5 for COMP1, pin PB4 for COMP2) */
#define COMP_INPUT_PLUS_IO2            (COMP_CSR_INPSEL_0)    /*!< Comparator input plus connected to IO2 (pin PB2 for COMP1, pin PB6 for COMP2) */
#if defined(COMP_CSR_INPSEL_1)
#define COMP_INPUT_PLUS_IO3            (COMP_CSR_INPSEL_1)    /*!< Comparator input plus connected to IO3 (pin PA1 for COMP1, pin PA3 for COMP2) */
#endif
/**
  * @}
  */

/** @defgroup COMP_InputMinus COMP input minus (inverting input)
  * @{
  */
#define COMP_INPUT_MINUS_1_4VREFINT    (                                                            COMP_CSR_SCALEN | COMP_CSR_BRGEN)        /*!< Comparator input minus connected to 1/4 VrefInt */
#define COMP_INPUT_MINUS_1_2VREFINT    (                                        COMP_CSR_INMSEL_0 | COMP_CSR_SCALEN | COMP_CSR_BRGEN)        /*!< Comparator input minus connected to 1/2 VrefInt */
#define COMP_INPUT_MINUS_3_4VREFINT    (                    COMP_CSR_INMSEL_1                     | COMP_CSR_SCALEN | COMP_CSR_BRGEN)        /*!< Comparator input minus connected to 3/4 VrefInt */
#define COMP_INPUT_MINUS_VREFINT       (                    COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0 | COMP_CSR_SCALEN                 )        /*!< Comparator input minus connected to VrefInt */
#define COMP_INPUT_MINUS_DAC1_CH1      (COMP_CSR_INMSEL_2                                        )                                           /*!< Comparator input minus connected to DAC1 channel 1 (DAC_OUT1) */
#if defined(DAC_CHANNEL2_SUPPORT)
#define COMP_INPUT_MINUS_DAC1_CH2      (COMP_CSR_INMSEL_2                     | COMP_CSR_INMSEL_0)                                           /*!< Comparator input minus connected to DAC1 channel 2 (DAC_OUT2) */
#endif
#define COMP_INPUT_MINUS_IO1           (COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1                    )                                           /*!< Comparator input minus connected to IO1 (pin PB1 for COMP1, pin PB3 for COMP2) */ 
#define COMP_INPUT_MINUS_IO2           (COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0)                                           /*!< Comparator input minus connected to IO2 (pin PC4 for COMP1, pin PB7 for COMP2) */
#if defined(COMP_CSR_INMESEL_1)
#define COMP_INPUT_MINUS_IO3           (                     COMP_CSR_INMESEL_0 | COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to IO3 (pin PA0 for COMP1, pin PA2 for COMP2) */
#define COMP_INPUT_MINUS_IO4           (COMP_CSR_INMESEL_1                      | COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to IO4 (pin PA4 for COMP1, pin PA4 for COMP2) */
#define COMP_INPUT_MINUS_IO5           (COMP_CSR_INMESEL_1 | COMP_CSR_INMESEL_0 | COMP_CSR_INMSEL_2 | COMP_CSR_INMSEL_1 | COMP_CSR_INMSEL_0) /*!< Comparator input minus connected to IO5 (pin PA5 for COMP1, pin PA5 for COMP2) */
#endif
/**
  * @}
  */

/** @defgroup COMP_Hysteresis COMP hysteresis
  * @{
  */
#define COMP_HYSTERESIS_NONE           (0x00000000U)          /*!< No hysteresis */
#define COMP_HYSTERESIS_LOW            (COMP_CSR_HYST_0)      /*!< Hysteresis level low */
#define COMP_HYSTERESIS_MEDIUM         (COMP_CSR_HYST_1)      /*!< Hysteresis level medium */
#define COMP_HYSTERESIS_HIGH           (COMP_CSR_HYST)        /*!< Hysteresis level high */
/**
  * @}
  */

/** @defgroup COMP_OutputPolarity COMP output Polarity
  * @{
  */
#define COMP_OUTPUTPOL_NONINVERTED     (0x00000000U)          /*!< COMP output level is not inverted (comparator output is high when the input plus is at a higher voltage than the input minus) */
#define COMP_OUTPUTPOL_INVERTED        (COMP_CSR_POLARITY)    /*!< COMP output level is inverted     (comparator output is low  when the input plus is at a higher voltage than the input minus) */
/**
  * @}
  */

/** @defgroup COMP_BlankingSrce  COMP blanking source
  * @{
  */
#define COMP_BLANKINGSRC_NONE            (0x00000000U)           /*!<Comparator output without blanking */
#define COMP_BLANKINGSRC_TIM1_OC5_COMP1  (COMP_CSR_BLANKING_0)   /*!< Comparator output blanking source TIM1 OC5 (specific to COMP instance: COMP1) */
#define COMP_BLANKINGSRC_TIM2_OC3_COMP1  (COMP_CSR_BLANKING_1)   /*!< Comparator output blanking source TIM2 OC3 (specific to COMP instance: COMP1) */
#define COMP_BLANKINGSRC_TIM3_OC3_COMP1  (COMP_CSR_BLANKING_2)   /*!< Comparator output blanking source TIM3 OC3 (specific to COMP instance: COMP1) */
#define COMP_BLANKINGSRC_TIM3_OC4_COMP2  (COMP_CSR_BLANKING_0)   /*!< Comparator output blanking source TIM3 OC4 (specific to COMP instance: COMP2) */
#define COMP_BLANKINGSRC_TIM8_OC5_COMP2  (COMP_CSR_BLANKING_1)   /*!< Comparator output blanking source TIM8 OC5 (specific to COMP instance: COMP2) */
#define COMP_BLANKINGSRC_TIM15_OC1_COMP2 (COMP_CSR_BLANKING_2)   /*!< Comparator output blanking source TIM15 OC1 (specific to COMP instance: COMP2) */
/**
  * @}
  */

/** @defgroup COMP_OutputLevel COMP Output Level
  * @{
  */
/* Note: Comparator output level values are fixed to "0" and "1",             */
/* corresponding COMP register bit is managed by HAL function to match        */
/* with these values (independently of bit position in register).             */

/* When output polarity is not inverted, comparator output is low when
   the input plus is at a lower voltage than the input minus */
#define COMP_OUTPUT_LEVEL_LOW              (0x00000000U)
/* When output polarity is not inverted, comparator output is high when
   the input plus is at a higher voltage than the input minus */
#define COMP_OUTPUT_LEVEL_HIGH             (0x00000001U)
/**
  * @}
  */

/** @defgroup COMP_EXTI_TriggerMode COMP output to EXTI
  * @{
  */
#define COMP_TRIGGERMODE_NONE                 (0x00000000U)                                             /*!< Comparator output triggering no External Interrupt Line */
#define COMP_TRIGGERMODE_IT_RISING            (COMP_EXTI_IT | COMP_EXTI_RISING)                         /*!< Comparator output triggering External Interrupt Line event with interruption, on rising edge */
#define COMP_TRIGGERMODE_IT_FALLING           (COMP_EXTI_IT | COMP_EXTI_FALLING)                        /*!< Comparator output triggering External Interrupt Line event with interruption, on falling edge */
#define COMP_TRIGGERMODE_IT_RISING_FALLING    (COMP_EXTI_IT | COMP_EXTI_RISING | COMP_EXTI_FALLING)     /*!< Comparator output triggering External Interrupt Line event with interruption, on both rising and falling edges */
#define COMP_TRIGGERMODE_EVENT_RISING         (COMP_EXTI_EVENT | COMP_EXTI_RISING)                      /*!< Comparator output triggering External Interrupt Line event only (without interruption), on rising edge */
#define COMP_TRIGGERMODE_EVENT_FALLING        (COMP_EXTI_EVENT | COMP_EXTI_FALLING)                     /*!< Comparator output triggering External Interrupt Line event only (without interruption), on falling edge */
#define COMP_TRIGGERMODE_EVENT_RISING_FALLING (COMP_EXTI_EVENT | COMP_EXTI_RISING | COMP_EXTI_FALLING)  /*!< Comparator output triggering External Interrupt Line event only (without interruption), on both rising and falling edges */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup COMP_Exported_Macros COMP Exported Macros
  * @{
  */

/** @defgroup COMP_Handle_Management  COMP Handle Management
  * @{
  */

/** @brief  Reset COMP handle state.
  * @param  __HANDLE__  COMP handle
  * @retval None
  */
#define __HAL_COMP_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_COMP_STATE_RESET)

/**
  * @brief  Enable the specified comparator.
  * @param  __HANDLE__  COMP handle
  * @retval None
  */
#define __HAL_COMP_ENABLE(__HANDLE__)              SET_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_EN)

/**
  * @brief  Disable the specified comparator.
  * @param  __HANDLE__  COMP handle
  * @retval None
  */
#define __HAL_COMP_DISABLE(__HANDLE__)             CLEAR_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_EN)

/**
  * @brief  Lock the specified comparator configuration.
  * @note   Using this macro induce HAL COMP handle state machine being no
  *         more in line with COMP instance state.
  *         To keep HAL COMP handle state machine updated, it is recommended
  *         to use function "HAL_COMP_Lock')".
  * @param  __HANDLE__  COMP handle
  * @retval None
  */
#define __HAL_COMP_LOCK(__HANDLE__)                SET_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_LOCK)

/**
  * @brief  Check whether the specified comparator is locked.
  * @param  __HANDLE__  COMP handle
  * @retval Value 0 if COMP instance is not locked, value 1 if COMP instance is locked
  */
#define __HAL_COMP_IS_LOCKED(__HANDLE__)           (READ_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_LOCK) == COMP_CSR_LOCK)

/**
  * @}
  */

/** @defgroup COMP_Exti_Management  COMP external interrupt line management
  * @{
  */

/**
  * @brief  Enable the COMP1 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_RISING_EDGE()    LL_EXTI_EnableRisingTrig_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_RISING_EDGE()   LL_EXTI_DisableRisingTrig_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_FALLING_EDGE()   LL_EXTI_EnableFallingTrig_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableFallingTrig_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               LL_EXTI_EnableRisingTrig_0_31(COMP_EXTI_LINE_COMP1); \
                                                               LL_EXTI_EnableFallingTrig_0_31(COMP_EXTI_LINE_COMP1); \
                                                             } while(0)

/**
  * @brief  Disable the COMP1 EXTI line rising & falling edge trigger.
  * @retval None
  */                                         
#define __HAL_COMP_COMP1_EXTI_DISABLE_RISING_FALLING_EDGE()  do { \
                                                               LL_EXTI_DisableRisingTrig_0_31(COMP_EXTI_LINE_COMP1); \
                                                               LL_EXTI_DisableFallingTrig_0_31(COMP_EXTI_LINE_COMP1); \
                                                             } while(0)

/**
  * @brief  Enable the COMP1 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_IT()             LL_EXTI_EnableIT_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_IT()            LL_EXTI_DisableIT_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Generate a software interrupt on the COMP1 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_GENERATE_SWIT()         LL_EXTI_GenerateSWI_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_EVENT()          LL_EXTI_EnableEvent_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_EVENT()         LL_EXTI_DisableEvent_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Check whether the COMP1 EXTI line flag is set.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP1_EXTI_GET_FLAG()              LL_EXTI_IsActiveFlag_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Clear the COMP1 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_CLEAR_FLAG()            LL_EXTI_ClearFlag_0_31(COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP2 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_RISING_EDGE()    LL_EXTI_EnableRisingTrig_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_RISING_EDGE()   LL_EXTI_DisableRisingTrig_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_FALLING_EDGE()   LL_EXTI_EnableFallingTrig_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_FALLING_EDGE()  LL_EXTI_DisableFallingTrig_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               LL_EXTI_EnableRisingTrig_0_31(COMP_EXTI_LINE_COMP2); \
                                                               LL_EXTI_EnableFallingTrig_0_31(COMP_EXTI_LINE_COMP2); \
                                                             } while(0)

/**
  * @brief  Disable the COMP2 EXTI line rising & falling edge trigger.
  * @retval None
  */                                         
#define __HAL_COMP_COMP2_EXTI_DISABLE_RISING_FALLING_EDGE()  do { \
                                                               LL_EXTI_DisableRisingTrig_0_31(COMP_EXTI_LINE_COMP2); \
                                                               LL_EXTI_DisableFallingTrig_0_31(COMP_EXTI_LINE_COMP2); \
                                                             } while(0)

/**
  * @brief  Enable the COMP2 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_IT()             LL_EXTI_EnableIT_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_IT()            LL_EXTI_DisableIT_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Generate a software interrupt on the COMP2 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_GENERATE_SWIT()         LL_EXTI_GenerateSWI_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_EVENT()          LL_EXTI_EnableEvent_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_EVENT()         LL_EXTI_DisableEvent_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Check whether the COMP2 EXTI line flag is set.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP2_EXTI_GET_FLAG()              LL_EXTI_IsActiveFlag_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @brief  Clear the COMP2 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_CLEAR_FLAG()            LL_EXTI_ClearFlag_0_31(COMP_EXTI_LINE_COMP2)

/**
  * @}
  */

/**
  * @}
  */


/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup COMP_Private_Constants COMP Private Constants
  * @{
  */

/** @defgroup COMP_ExtiLine COMP EXTI Lines
  * @{
  */
#define COMP_EXTI_LINE_COMP1           (LL_EXTI_LINE_21)  /*!< EXTI line 21 connected to COMP1 output */
#define COMP_EXTI_LINE_COMP2           (LL_EXTI_LINE_22)  /*!< EXTI line 22 connected to COMP2 output */
/**
  * @}
  */

/** @defgroup COMP_ExtiLine COMP EXTI Lines
  * @{
  */
#define COMP_EXTI_IT                        (0x01U)  /*!< EXTI line event with interruption */
#define COMP_EXTI_EVENT                     (0x02U)  /*!< EXTI line event only (without interruption) */
#define COMP_EXTI_RISING                    (0x10U)  /*!< EXTI line event on rising edge */
#define COMP_EXTI_FALLING                   (0x20U)  /*!< EXTI line event on falling edge */
/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup COMP_Private_Macros COMP Private Macros
  * @{
  */

/** @defgroup COMP_GET_EXTI_LINE COMP private macros to get EXTI line associated with comparators 
  * @{
  */
/**
  * @brief  Get the specified EXTI line for a comparator instance.
  * @param  __INSTANCE__  specifies the COMP instance.
  * @retval value of @ref COMP_ExtiLine
  */
#define COMP_GET_EXTI_LINE(__INSTANCE__)    (((__INSTANCE__) == COMP1) ? COMP_EXTI_LINE_COMP1 \
                                            : COMP_EXTI_LINE_COMP2)
/**
  * @}
  */

/** @defgroup COMP_IS_COMP_Definitions COMP private macros to check input parameters
  * @{
  */
#define IS_COMP_WINDOWMODE(__WINDOWMODE__)  (((__WINDOWMODE__) == COMP_WINDOWMODE_DISABLE)                || \
                                             ((__WINDOWMODE__) == COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON)  )

#define IS_COMP_POWERMODE(__POWERMODE__)    (((__POWERMODE__) == COMP_POWERMODE_HIGHSPEED)    || \
                                             ((__POWERMODE__) == COMP_POWERMODE_MEDIUMSPEED)  || \
                                             ((__POWERMODE__) == COMP_POWERMODE_ULTRALOWPOWER)  )

#if defined(COMP_CSR_INPSEL_1)
#define IS_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__) (((__INPUT_PLUS__) == COMP_INPUT_PLUS_IO1) || \
                                                               ((__INPUT_PLUS__) == COMP_INPUT_PLUS_IO2) || \
                                                               ((__INPUT_PLUS__) == COMP_INPUT_PLUS_IO3))
#else
#define IS_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__) (((__INPUT_PLUS__) == COMP_INPUT_PLUS_IO1) || \
                                                               ((__INPUT_PLUS__) == COMP_INPUT_PLUS_IO2))
#endif

/* Note: On this STM32 serie, comparator input minus parameters are           */
/*       the same on all COMP instances.                                      */
/*       However, comparator instance kept as macro parameter for             */
/*       compatibility with other STM32 families.                             */
#if defined(COMP_CSR_INMESEL_1) && defined(DAC_CHANNEL2_SUPPORT)
#define IS_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__) (((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_2VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_3_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_VREFINT)     || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH1)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH2)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO1)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO2)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO3)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO4)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO5))
#elif defined(COMP_CSR_INMESEL_1)
#define IS_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__) (((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_2VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_3_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_VREFINT)     || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH1)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO1)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO2)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO3)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO4)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO5))
#elif defined(DAC_CHANNEL2_SUPPORT)
#define IS_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__) (((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_2VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_3_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_VREFINT)     || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH1)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH2)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO1)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO2))
#else
#define IS_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__) (((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_1_2VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_3_4VREFINT)  || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_VREFINT)     || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_DAC1_CH1)    || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO1)         || \
                                                                 ((__INPUT_MINUS__) == COMP_INPUT_MINUS_IO2))
#endif

#define IS_COMP_HYSTERESIS(__HYSTERESIS__)  (((__HYSTERESIS__) == COMP_HYSTERESIS_NONE)   || \
                                             ((__HYSTERESIS__) == COMP_HYSTERESIS_LOW)    || \
                                             ((__HYSTERESIS__) == COMP_HYSTERESIS_MEDIUM) || \
                                             ((__HYSTERESIS__) == COMP_HYSTERESIS_HIGH))

#define IS_COMP_OUTPUTPOL(__POL__)          (((__POL__) == COMP_OUTPUTPOL_NONINVERTED) || \
                                             ((__POL__) == COMP_OUTPUTPOL_INVERTED))

#define IS_COMP_BLANKINGSRCE(__OUTPUT_BLANKING_SOURCE__)                    \
  (   ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_NONE)               \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM1_OC5_COMP1)     \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM2_OC3_COMP1)     \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM3_OC3_COMP1)     \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM3_OC4_COMP2)     \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM8_OC5_COMP2)     \
   || ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM15_OC1_COMP2)    \
  )

#define IS_COMP_BLANKINGSRC_INSTANCE(__INSTANCE__, __OUTPUT_BLANKING_SOURCE__)  \
   ((((__INSTANCE__) == COMP1) &&                                               \
    (((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_NONE)            ||      \
     ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM1_OC5_COMP1)  ||      \
     ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM2_OC3_COMP1)  ||      \
     ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM3_OC3_COMP1)))        \
    ||                                                                          \
    (((__INSTANCE__) == COMP2) &&                                               \
     (((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_NONE)           ||      \
      ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM3_OC4_COMP2) ||      \
      ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM8_OC5_COMP2) ||      \
      ((__OUTPUT_BLANKING_SOURCE__) == COMP_BLANKINGSRC_TIM15_OC1_COMP2))))


#define IS_COMP_TRIGGERMODE(__MODE__)       (((__MODE__) == COMP_TRIGGERMODE_NONE)                 || \
                                             ((__MODE__) == COMP_TRIGGERMODE_IT_RISING)            || \
                                             ((__MODE__) == COMP_TRIGGERMODE_IT_FALLING)           || \
                                             ((__MODE__) == COMP_TRIGGERMODE_IT_RISING_FALLING)    || \
                                             ((__MODE__) == COMP_TRIGGERMODE_EVENT_RISING)         || \
                                             ((__MODE__) == COMP_TRIGGERMODE_EVENT_FALLING)        || \
                                             ((__MODE__) == COMP_TRIGGERMODE_EVENT_RISING_FALLING))

#define IS_COMP_OUTPUT_LEVEL(__OUTPUT_LEVEL__) (((__OUTPUT_LEVEL__) == COMP_OUTPUT_LEVEL_LOW)     || \
                                                ((__OUTPUT_LEVEL__) == COMP_OUTPUT_LEVEL_HIGH))

/**
  * @}
  */

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMP_Exported_Functions
  * @{
  */

/** @addtogroup COMP_Exported_Functions_Group1
  * @{
  */

/* Initialization and de-initialization functions  **********************************/
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_DeInit (COMP_HandleTypeDef *hcomp);
void              HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp);
void              HAL_COMP_MspDeInit(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/* IO operation functions  *****************************************************/
/** @addtogroup COMP_Exported_Functions_Group2
  * @{
  */
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_Stop(COMP_HandleTypeDef *hcomp);
void              HAL_COMP_IRQHandler(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/* Peripheral Control functions  ************************************************/
/** @addtogroup COMP_Exported_Functions_Group3
  * @{
  */
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *hcomp);
uint32_t          HAL_COMP_GetOutputLevel(COMP_HandleTypeDef *hcomp);
/* Callback in interrupt mode */
void              HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/* Peripheral State functions  **************************************************/
/** @addtogroup COMP_Exported_Functions_Group4
  * @{
  */
HAL_COMP_StateTypeDef HAL_COMP_GetState(COMP_HandleTypeDef *hcomp);
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

#endif /* COMP1 || COMP2 */

#endif /* __STM32L4xx_HAL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
