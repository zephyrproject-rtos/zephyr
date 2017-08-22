/**
  ******************************************************************************
  * @file    stm32f0xx_hal_tsc.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the TSC firmware 
  *          library.
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
#ifndef __STM32F0xx_TSC_H
#define __STM32F0xx_TSC_H

#ifdef __cplusplus
 extern "C" {
#endif

#if defined(STM32F051x8) || defined(STM32F071xB) || defined(STM32F091xC) || \
    defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) ||                         \
    defined(STM32F058xx) || defined(STM32F078xx) || defined(STM32F098xx)

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal_def.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

/** @addtogroup TSC
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/
   
/** @defgroup TSC_Exported_Types TSC Exported Types
  * @{
  */
/** 
  * @brief TSC state structure definition  
  */ 
typedef enum
{
  HAL_TSC_STATE_RESET  = 0x00U, /*!< TSC registers have their reset value */
  HAL_TSC_STATE_READY  = 0x01U, /*!< TSC registers are initialized or acquisition is completed with success */
  HAL_TSC_STATE_BUSY   = 0x02U, /*!< TSC initialization or acquisition is on-going */
  HAL_TSC_STATE_ERROR  = 0x03U  /*!< Acquisition is completed with max count error */
} HAL_TSC_StateTypeDef;

/** 
  * @brief TSC group status structure definition  
  */ 
typedef enum
{
  TSC_GROUP_ONGOING   = 0x00U, /*!< Acquisition on group is on-going or not started */
  TSC_GROUP_COMPLETED = 0x01U  /*!< Acquisition on group is completed with success (no max count error) */
} TSC_GroupStatusTypeDef;

/** 
  * @brief TSC init structure definition  
  */ 
typedef struct
{
  uint32_t CTPulseHighLength;       /*!< Charge-transfer high pulse length */
  uint32_t CTPulseLowLength;        /*!< Charge-transfer low pulse length */
  uint32_t SpreadSpectrum;          /*!< Spread spectrum activation */
  uint32_t SpreadSpectrumDeviation; /*!< Spread spectrum deviation */
  uint32_t SpreadSpectrumPrescaler; /*!< Spread spectrum prescaler */
  uint32_t PulseGeneratorPrescaler; /*!< Pulse generator prescaler */
  uint32_t MaxCountValue;           /*!< Max count value */
  uint32_t IODefaultMode;           /*!< IO default mode */
  uint32_t SynchroPinPolarity;      /*!< Synchro pin polarity */
  uint32_t AcquisitionMode;         /*!< Acquisition mode */
  uint32_t MaxCountInterrupt;       /*!< Max count interrupt activation */
  uint32_t ChannelIOs;              /*!< Channel IOs mask */
  uint32_t ShieldIOs;               /*!< Shield IOs mask */
  uint32_t SamplingIOs;             /*!< Sampling IOs mask */
} TSC_InitTypeDef;

/** 
  * @brief TSC IOs configuration structure definition  
  */ 
typedef struct
{
  uint32_t ChannelIOs;  /*!< Channel IOs mask */
  uint32_t ShieldIOs;   /*!< Shield IOs mask */
  uint32_t SamplingIOs; /*!< Sampling IOs mask */
} TSC_IOConfigTypeDef;

/** 
  * @brief  TSC handle Structure definition  
  */ 
typedef struct
{
  TSC_TypeDef               *Instance; /*!< Register base address */
  TSC_InitTypeDef           Init;      /*!< Initialization parameters */
  __IO HAL_TSC_StateTypeDef State;     /*!< Peripheral state */
  HAL_LockTypeDef           Lock;      /*!< Lock feature */
} TSC_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup TSC_Exported_Constants TSC Exported Constants
  * @{
  */ 

/** @defgroup TSC_CTPH_Cycles TSC Charge Transfer Pulse High
  * @{
  */ 
#define TSC_CTPH_1CYCLE   ((uint32_t)( 0U << 28))
#define TSC_CTPH_2CYCLES  ((uint32_t)( 1U << 28))
#define TSC_CTPH_3CYCLES  ((uint32_t)( 2U << 28))
#define TSC_CTPH_4CYCLES  ((uint32_t)( 3U << 28))
#define TSC_CTPH_5CYCLES  ((uint32_t)( 4U << 28))
#define TSC_CTPH_6CYCLES  ((uint32_t)( 5U << 28))
#define TSC_CTPH_7CYCLES  ((uint32_t)( 6U << 28))
#define TSC_CTPH_8CYCLES  ((uint32_t)( 7U << 28))
#define TSC_CTPH_9CYCLES  ((uint32_t)( 8U << 28))
#define TSC_CTPH_10CYCLES ((uint32_t)( 9U << 28))
#define TSC_CTPH_11CYCLES ((uint32_t)(10U << 28))
#define TSC_CTPH_12CYCLES ((uint32_t)(11U << 28))
#define TSC_CTPH_13CYCLES ((uint32_t)(12U << 28))
#define TSC_CTPH_14CYCLES ((uint32_t)(13U << 28))
#define TSC_CTPH_15CYCLES ((uint32_t)(14U << 28))
#define TSC_CTPH_16CYCLES ((uint32_t)(15U << 28))
#define IS_TSC_CTPH(VAL) (((VAL) == TSC_CTPH_1CYCLE) || \
                          ((VAL) == TSC_CTPH_2CYCLES) || \
                          ((VAL) == TSC_CTPH_3CYCLES) || \
                          ((VAL) == TSC_CTPH_4CYCLES) || \
                          ((VAL) == TSC_CTPH_5CYCLES) || \
                          ((VAL) == TSC_CTPH_6CYCLES) || \
                          ((VAL) == TSC_CTPH_7CYCLES) || \
                          ((VAL) == TSC_CTPH_8CYCLES) || \
                          ((VAL) == TSC_CTPH_9CYCLES) || \
                          ((VAL) == TSC_CTPH_10CYCLES) || \
                          ((VAL) == TSC_CTPH_11CYCLES) || \
                          ((VAL) == TSC_CTPH_12CYCLES) || \
                          ((VAL) == TSC_CTPH_13CYCLES) || \
                          ((VAL) == TSC_CTPH_14CYCLES) || \
                          ((VAL) == TSC_CTPH_15CYCLES) || \
                          ((VAL) == TSC_CTPH_16CYCLES))
/**
  * @}
  */

/** @defgroup TSC_CTPL_Cycles TSC Charge Transfer Pulse Low
  * @{
  */  
#define TSC_CTPL_1CYCLE   ((uint32_t)( 0U << 24))
#define TSC_CTPL_2CYCLES  ((uint32_t)( 1U << 24))
#define TSC_CTPL_3CYCLES  ((uint32_t)( 2U << 24))
#define TSC_CTPL_4CYCLES  ((uint32_t)( 3U << 24))
#define TSC_CTPL_5CYCLES  ((uint32_t)( 4U << 24))
#define TSC_CTPL_6CYCLES  ((uint32_t)( 5U << 24))
#define TSC_CTPL_7CYCLES  ((uint32_t)( 6U << 24))
#define TSC_CTPL_8CYCLES  ((uint32_t)( 7U << 24))
#define TSC_CTPL_9CYCLES  ((uint32_t)( 8U << 24))
#define TSC_CTPL_10CYCLES ((uint32_t)( 9U << 24))
#define TSC_CTPL_11CYCLES ((uint32_t)(10U << 24))
#define TSC_CTPL_12CYCLES ((uint32_t)(11U << 24))
#define TSC_CTPL_13CYCLES ((uint32_t)(12U << 24))
#define TSC_CTPL_14CYCLES ((uint32_t)(13U << 24))
#define TSC_CTPL_15CYCLES ((uint32_t)(14U << 24))
#define TSC_CTPL_16CYCLES ((uint32_t)(15U << 24))
#define IS_TSC_CTPL(VAL) (((VAL) == TSC_CTPL_1CYCLE) || \
                          ((VAL) == TSC_CTPL_2CYCLES) || \
                          ((VAL) == TSC_CTPL_3CYCLES) || \
                          ((VAL) == TSC_CTPL_4CYCLES) || \
                          ((VAL) == TSC_CTPL_5CYCLES) || \
                          ((VAL) == TSC_CTPL_6CYCLES) || \
                          ((VAL) == TSC_CTPL_7CYCLES) || \
                          ((VAL) == TSC_CTPL_8CYCLES) || \
                          ((VAL) == TSC_CTPL_9CYCLES) || \
                          ((VAL) == TSC_CTPL_10CYCLES) || \
                          ((VAL) == TSC_CTPL_11CYCLES) || \
                          ((VAL) == TSC_CTPL_12CYCLES) || \
                          ((VAL) == TSC_CTPL_13CYCLES) || \
                          ((VAL) == TSC_CTPL_14CYCLES) || \
                          ((VAL) == TSC_CTPL_15CYCLES) || \
                          ((VAL) == TSC_CTPL_16CYCLES))
/**
  * @}
  */
  
/** @defgroup TSC_SS_Prescaler_definition TSC Spread spectrum prescaler definition
  * @{
  */
#define TSC_SS_PRESC_DIV1 (0U)  
#define TSC_SS_PRESC_DIV2  (TSC_CR_SSPSC) 
#define IS_TSC_SS_PRESC(VAL) (((VAL) == TSC_SS_PRESC_DIV1) || ((VAL) == TSC_SS_PRESC_DIV2))

/**
  * @}
  */
  
/** @defgroup TSC_PG_Prescaler_definition TSC Pulse Generator prescaler definition
  * @{
  */
#define TSC_PG_PRESC_DIV1   ((uint32_t)(0 << 12))
#define TSC_PG_PRESC_DIV2   ((uint32_t)(1 << 12))
#define TSC_PG_PRESC_DIV4   ((uint32_t)(2 << 12))
#define TSC_PG_PRESC_DIV8   ((uint32_t)(3 << 12))
#define TSC_PG_PRESC_DIV16  ((uint32_t)(4 << 12))
#define TSC_PG_PRESC_DIV32  ((uint32_t)(5 << 12))
#define TSC_PG_PRESC_DIV64  ((uint32_t)(6 << 12))
#define TSC_PG_PRESC_DIV128 ((uint32_t)(7 << 12))
#define IS_TSC_PG_PRESC(VAL) (((VAL) == TSC_PG_PRESC_DIV1) || \
                              ((VAL) == TSC_PG_PRESC_DIV2) || \
                              ((VAL) == TSC_PG_PRESC_DIV4) || \
                              ((VAL) == TSC_PG_PRESC_DIV8) || \
                              ((VAL) == TSC_PG_PRESC_DIV16) || \
                              ((VAL) == TSC_PG_PRESC_DIV32) || \
                              ((VAL) == TSC_PG_PRESC_DIV64) || \
                              ((VAL) == TSC_PG_PRESC_DIV128))
/**
  * @}
  */

/** @defgroup TSC_MCV_definition TSC Max Count Value definition
  * @{
  */  
#define TSC_MCV_255   ((uint32_t)(0 << 5))
#define TSC_MCV_511   ((uint32_t)(1 << 5))
#define TSC_MCV_1023  ((uint32_t)(2 << 5))
#define TSC_MCV_2047  ((uint32_t)(3 << 5))
#define TSC_MCV_4095  ((uint32_t)(4 << 5))
#define TSC_MCV_8191  ((uint32_t)(5 << 5))
#define TSC_MCV_16383 ((uint32_t)(6 << 5))
#define IS_TSC_MCV(VAL) (((VAL) == TSC_MCV_255) || \
                         ((VAL) == TSC_MCV_511) || \
                         ((VAL) == TSC_MCV_1023) || \
                         ((VAL) == TSC_MCV_2047) || \
                         ((VAL) == TSC_MCV_4095) || \
                         ((VAL) == TSC_MCV_8191) || \
                         ((VAL) == TSC_MCV_16383))
/**
  * @}
  */

/** @defgroup TSC_IO_default_mode_definition TSC I/O default mode definition
  * @{
  */  
#define TSC_IODEF_OUT_PP_LOW (0U)
#define TSC_IODEF_IN_FLOAT   (TSC_CR_IODEF)
#define IS_TSC_IODEF(VAL) (((VAL) == TSC_IODEF_OUT_PP_LOW) || ((VAL) == TSC_IODEF_IN_FLOAT))
/**
  * @}
  */

/** @defgroup TSC_Synchronization_pin_polarity TSC Synchronization pin polarity
  * @{
  */    
#define TSC_SYNC_POLARITY_FALLING      (0U)
#define TSC_SYNC_POLARITY_RISING (TSC_CR_SYNCPOL)
#define IS_TSC_SYNC_POL(VAL) (((VAL) == TSC_SYNC_POLARITY_FALLING) || ((VAL) == TSC_SYNC_POLARITY_RISING))
/**
  * @}
  */
  
/** @defgroup TSC_Acquisition_mode TSC Acquisition mode
  * @{
  */   
#define TSC_ACQ_MODE_NORMAL  (0U)
#define TSC_ACQ_MODE_SYNCHRO (TSC_CR_AM)
#define IS_TSC_ACQ_MODE(VAL) (((VAL) == TSC_ACQ_MODE_NORMAL) || ((VAL) == TSC_ACQ_MODE_SYNCHRO))
/**
  * @}
  */
  
/** @defgroup TSC_IO_mode_definition TSC I/O mode definition
  * @{
  */
#define TSC_IOMODE_UNUSED   (0U)
#define TSC_IOMODE_CHANNEL  (1U)
#define TSC_IOMODE_SHIELD   (2U)
#define TSC_IOMODE_SAMPLING (3U)
#define IS_TSC_IOMODE(VAL) (((VAL) == TSC_IOMODE_UNUSED) || \
                            ((VAL) == TSC_IOMODE_CHANNEL) || \
                            ((VAL) == TSC_IOMODE_SHIELD) || \
                            ((VAL) == TSC_IOMODE_SAMPLING))
/**
  * @}
  */

/** @defgroup TSC_interrupts_definition TSC interrupts definition
  * @{
  */
#define TSC_IT_EOA ((uint32_t)TSC_IER_EOAIE)  
#define TSC_IT_MCE ((uint32_t)TSC_IER_MCEIE) 
#define IS_TSC_MCE_IT(VAL) (((VAL) == DISABLE) || ((VAL) == ENABLE))
/**
  * @}
  */ 

/** @defgroup TSC_flags_definition TSC Flags Definition
  * @{
  */ 
#define TSC_FLAG_EOA ((uint32_t)TSC_ISR_EOAF)
#define TSC_FLAG_MCE ((uint32_t)TSC_ISR_MCEF)
/**
  * @}
  */

/** @defgroup TSC_groups_definition TSC groups definition
  * @{
  */ 
#define TSC_NB_OF_GROUPS (8)

#define TSC_GROUP1 (0x00000001U)
#define TSC_GROUP2 (0x00000002U)
#define TSC_GROUP3 (0x00000004U)
#define TSC_GROUP4 (0x00000008U)
#define TSC_GROUP5 (0x00000010U)
#define TSC_GROUP6 (0x00000020U)
#define TSC_GROUP7 (0x00000040U)
#define TSC_GROUP8 (0x00000080U)
#define TSC_ALL_GROUPS (0x000000FFU)

#define TSC_GROUP1_IDX (0U)
#define TSC_GROUP2_IDX (1U)
#define TSC_GROUP3_IDX (2U)
#define TSC_GROUP4_IDX (3U)
#define TSC_GROUP5_IDX (4U)
#define TSC_GROUP6_IDX (5U)
#define TSC_GROUP7_IDX (6U)
#define TSC_GROUP8_IDX (7U)
#define IS_GROUP_INDEX(VAL) (((VAL) == 0U) || (((VAL) > 0U) && ((VAL) < TSC_NB_OF_GROUPS)))

#define TSC_GROUP1_IO1 (0x00000001U)
#define TSC_GROUP1_IO2 (0x00000002U)
#define TSC_GROUP1_IO3 (0x00000004U)
#define TSC_GROUP1_IO4 (0x00000008U)
#define TSC_GROUP1_ALL_IOS (0x0000000FU)

#define TSC_GROUP2_IO1 (0x00000010U)
#define TSC_GROUP2_IO2 (0x00000020U)
#define TSC_GROUP2_IO3 (0x00000040U)
#define TSC_GROUP2_IO4 (0x00000080U)
#define TSC_GROUP2_ALL_IOS (0x000000F0U)

#define TSC_GROUP3_IO1 (0x00000100U)
#define TSC_GROUP3_IO2 (0x00000200U)
#define TSC_GROUP3_IO3 (0x00000400U)
#define TSC_GROUP3_IO4 (0x00000800U)
#define TSC_GROUP3_ALL_IOS (0x00000F00U)

#define TSC_GROUP4_IO1 (0x00001000U)
#define TSC_GROUP4_IO2 (0x00002000U)
#define TSC_GROUP4_IO3 (0x00004000U)
#define TSC_GROUP4_IO4 (0x00008000U)
#define TSC_GROUP4_ALL_IOS (0x0000F000U)

#define TSC_GROUP5_IO1 (0x00010000U)
#define TSC_GROUP5_IO2 (0x00020000U)
#define TSC_GROUP5_IO3 (0x00040000U)
#define TSC_GROUP5_IO4 (0x00080000U)
#define TSC_GROUP5_ALL_IOS (0x000F0000U)

#define TSC_GROUP6_IO1 (0x00100000U)
#define TSC_GROUP6_IO2 (0x00200000U)
#define TSC_GROUP6_IO3 (0x00400000U)
#define TSC_GROUP6_IO4 (0x00800000U)
#define TSC_GROUP6_ALL_IOS (0x00F00000U)

#define TSC_GROUP7_IO1 (0x01000000U)
#define TSC_GROUP7_IO2 (0x02000000U)
#define TSC_GROUP7_IO3 (0x04000000U)
#define TSC_GROUP7_IO4 (0x08000000U)
#define TSC_GROUP7_ALL_IOS (0x0F000000U)

#define TSC_GROUP8_IO1 (0x10000000U)
#define TSC_GROUP8_IO2 (0x20000000U)
#define TSC_GROUP8_IO3 (0x40000000U)
#define TSC_GROUP8_IO4 (0x80000000U)
#define TSC_GROUP8_ALL_IOS (0xF0000000U)

#define TSC_ALL_GROUPS_ALL_IOS (0xFFFFFFFFU)
/**
  * @}
  */
  
/**
  * @}
  */ 

/* Private macros -----------------------------------------------------------*/
/** @defgroup TSC_Private_Macros TSC Private Macros
 * @{
 */
/** @defgroup TSC_Spread_Spectrum  TSC Spread Spectrum
  * @{
  */  
#define IS_TSC_SS(VAL) (((VAL) == DISABLE) || ((VAL) == ENABLE))

#define IS_TSC_SSD(VAL) (((VAL) == 0U) || (((VAL) > 0U) && ((VAL) < 128U)))
/**
  * @}
  */

/**
  * @}
  */  

/* Exported macros -----------------------------------------------------------*/
/** @defgroup TSC_Exported_Macros TSC Exported Macros
 * @{
 */

/** @brief  Reset TSC handle state
  * @param  __HANDLE__ TSC handle.
  * @retval None
  */
#define __HAL_TSC_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_TSC_STATE_RESET)

/**
  * @brief Enable the TSC peripheral.
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->CR |= TSC_CR_TSCE)

/**
  * @brief Disable the TSC peripheral.
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &= (uint32_t)(~TSC_CR_TSCE))

/**
  * @brief Start acquisition
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_START_ACQ(__HANDLE__) ((__HANDLE__)->Instance->CR |= TSC_CR_START)

/**
  * @brief Stop acquisition
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_STOP_ACQ(__HANDLE__) ((__HANDLE__)->Instance->CR &= (uint32_t)(~TSC_CR_START))

/**
  * @brief Set IO default mode to output push-pull low
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_SET_IODEF_OUTPPLOW(__HANDLE__) ((__HANDLE__)->Instance->CR &= (uint32_t)(~TSC_CR_IODEF))

/**
  * @brief Set IO default mode to input floating
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_SET_IODEF_INFLOAT(__HANDLE__) ((__HANDLE__)->Instance->CR |= TSC_CR_IODEF)

/**
  * @brief Set synchronization polarity to falling edge
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_SET_SYNC_POL_FALL(__HANDLE__) ((__HANDLE__)->Instance->CR &= (uint32_t)(~TSC_CR_SYNCPOL))

/**
  * @brief Set synchronization polarity to rising edge and high level
  * @param  __HANDLE__ TSC handle
  * @retval None
  */
#define __HAL_TSC_SET_SYNC_POL_RISE_HIGH(__HANDLE__) ((__HANDLE__)->Instance->CR |= TSC_CR_SYNCPOL)

/**
  * @brief Enable TSC interrupt.
  * @param  __HANDLE__ TSC handle
  * @param  __INTERRUPT__ TSC interrupt
  * @retval None
  */
#define __HAL_TSC_ENABLE_IT(__HANDLE__, __INTERRUPT__) ((__HANDLE__)->Instance->IER |= (__INTERRUPT__))

/**
  * @brief Disable TSC interrupt.
  * @param  __HANDLE__ TSC handle
  * @param  __INTERRUPT__ TSC interrupt
  * @retval None
  */
#define __HAL_TSC_DISABLE_IT(__HANDLE__, __INTERRUPT__) ((__HANDLE__)->Instance->IER &= (uint32_t)(~(__INTERRUPT__)))

/** @brief Check if the specified TSC interrupt source is enabled or disabled.
  * @param  __HANDLE__ TSC Handle
  * @param  __INTERRUPT__ TSC interrupt
  * @retval SET or RESET
  */
#define __HAL_TSC_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((((__HANDLE__)->Instance->IER & (__INTERRUPT__)) == (__INTERRUPT__)) ? SET : RESET)

/**
  * @brief Get the selected TSC's flag status.
  * @param  __HANDLE__ TSC handle
  * @param  __FLAG__ TSC flag
  * @retval SET or RESET
  */
#define __HAL_TSC_GET_FLAG(__HANDLE__, __FLAG__) ((((__HANDLE__)->Instance->ISR & (__FLAG__)) == (__FLAG__)) ? SET : RESET)

/**
  * @brief Clear the TSC's pending flag.
  * @param  __HANDLE__ TSC handle
  * @param  __FLAG__ TSC flag
  * @retval None
  */
#define __HAL_TSC_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->ICR = (__FLAG__))

/**
  * @brief Enable schmitt trigger hysteresis on a group of IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_ENABLE_HYSTERESIS(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOHCR |= (__GX_IOY_MASK__))

/**
  * @brief Disable schmitt trigger hysteresis on a group of IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_DISABLE_HYSTERESIS(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOHCR &= (uint32_t)(~(__GX_IOY_MASK__)))

/**
  * @brief Open analog switch on a group of IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_OPEN_ANALOG_SWITCH(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOASCR &= (uint32_t)(~(__GX_IOY_MASK__)))

/**
  * @brief Close analog switch on a group of IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_CLOSE_ANALOG_SWITCH(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOASCR |= (__GX_IOY_MASK__))

/**
  * @brief Enable a group of IOs in channel mode
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_ENABLE_CHANNEL(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOCCR |= (__GX_IOY_MASK__))

/**
  * @brief Disable a group of channel IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_DISABLE_CHANNEL(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOCCR &= (uint32_t)(~(__GX_IOY_MASK__)))

/**
  * @brief Enable a group of IOs in sampling mode
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_ENABLE_SAMPLING(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOSCR |= (__GX_IOY_MASK__))

/**
  * @brief Disable a group of sampling IOs
  * @param  __HANDLE__ TSC handle
  * @param  __GX_IOY_MASK__ IOs mask
  * @retval None
  */
#define __HAL_TSC_DISABLE_SAMPLING(__HANDLE__, __GX_IOY_MASK__) ((__HANDLE__)->Instance->IOSCR &= (uint32_t)(~(__GX_IOY_MASK__)))

/**
  * @brief Enable acquisition groups
  * @param  __HANDLE__ TSC handle
  * @param  __GX_MASK__ Groups mask
  * @retval None
  */
#define __HAL_TSC_ENABLE_GROUP(__HANDLE__, __GX_MASK__) ((__HANDLE__)->Instance->IOGCSR |= (__GX_MASK__))

/**
  * @brief Disable acquisition groups
  * @param  __HANDLE__ TSC handle
  * @param  __GX_MASK__ Groups mask
  * @retval None
  */
#define __HAL_TSC_DISABLE_GROUP(__HANDLE__, __GX_MASK__) ((__HANDLE__)->Instance->IOGCSR &= (uint32_t)(~(__GX_MASK__)))

/** @brief Gets acquisition group status
  * @param  __HANDLE__ TSC Handle
  * @param  __GX_INDEX__ Group index
  * @retval SET or RESET
  */
#define __HAL_TSC_GET_GROUP_STATUS(__HANDLE__, __GX_INDEX__) \
((((__HANDLE__)->Instance->IOGCSR & (uint32_t)((uint32_t)1 << ((__GX_INDEX__) + (uint32_t)16))) == (uint32_t)((uint32_t)1 << ((__GX_INDEX__) + (uint32_t)16))) ? TSC_GROUP_COMPLETED : TSC_GROUP_ONGOING)

/**
  * @}
  */
  
/* Exported functions --------------------------------------------------------*/  
/** @addtogroup TSC_Exported_Functions TSC Exported Functions
  * @{
  */

/** @addtogroup TSC_Exported_Functions_Group1 Initialization/de-initialization functions 
 *  @brief    Initialization and Configuration functions 
 *  @{
 */
/* Initialization and de-initialization functions *****************************/
HAL_StatusTypeDef HAL_TSC_Init(TSC_HandleTypeDef* htsc);
HAL_StatusTypeDef HAL_TSC_DeInit(TSC_HandleTypeDef *htsc);
void HAL_TSC_MspInit(TSC_HandleTypeDef* htsc);
void HAL_TSC_MspDeInit(TSC_HandleTypeDef* htsc);
/**
  * @}
  */

/** @addtogroup TSC_Exported_Functions_Group2 IO operation functions
 *  @brief    IO operation functions  *  @{
 */
/* IO operation functions *****************************************************/
HAL_StatusTypeDef HAL_TSC_Start(TSC_HandleTypeDef* htsc);
HAL_StatusTypeDef HAL_TSC_Start_IT(TSC_HandleTypeDef* htsc);
HAL_StatusTypeDef HAL_TSC_Stop(TSC_HandleTypeDef* htsc);
HAL_StatusTypeDef HAL_TSC_Stop_IT(TSC_HandleTypeDef* htsc);
TSC_GroupStatusTypeDef HAL_TSC_GroupGetStatus(TSC_HandleTypeDef* htsc, uint32_t gx_index);
uint32_t HAL_TSC_GroupGetValue(TSC_HandleTypeDef* htsc, uint32_t gx_index);
/**
  * @}
  */

/** @addtogroup TSC_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions 
 *  @{
 */
/* Peripheral Control functions ***********************************************/
HAL_StatusTypeDef HAL_TSC_IOConfig(TSC_HandleTypeDef* htsc, TSC_IOConfigTypeDef* config);
HAL_StatusTypeDef HAL_TSC_IODischarge(TSC_HandleTypeDef* htsc, uint32_t choice);
/**
  * @}
  */

/** @addtogroup TSC_Exported_Functions_Group4 State functions
 *  @brief   State functions 
 *  @{
 */
/* Peripheral State and Error functions ***************************************/
HAL_TSC_StateTypeDef HAL_TSC_GetState(TSC_HandleTypeDef* htsc);
HAL_StatusTypeDef HAL_TSC_PollForAcquisition(TSC_HandleTypeDef* htsc);
void HAL_TSC_IRQHandler(TSC_HandleTypeDef* htsc);
/**
  * @}
  */
  
/** @addtogroup TSC_Exported_Functions_Group5 Callback functions
 *  @brief   Callback functions 
 *  @{
 */
/* Callback functions *********************************************************/
void HAL_TSC_ConvCpltCallback(TSC_HandleTypeDef* htsc);
void HAL_TSC_ErrorCallback(TSC_HandleTypeDef* htsc);
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

#endif /* defined(STM32F051x8) || defined(STM32F071xB) || defined(STM32F091xC) || */
       /* defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) || */
       /* defined(STM32F058xx) || defined(STM32F078xx) || defined(STM32F098xx)    */


#ifdef __cplusplus
}
#endif

#endif /*__STM32F0xx_TSC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

