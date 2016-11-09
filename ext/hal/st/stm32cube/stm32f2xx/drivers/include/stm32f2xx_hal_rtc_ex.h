/**
  ******************************************************************************
  * @file    stm32f2xx_hal_rtc_ex.h
  * @author  MCD Application Team
  * @version V1.1.3
  * @date    29-June-2016
  * @brief   Header file of RTC HAL Extension module.
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
#ifndef __STM32F2xx_HAL_RTC_EX_H
#define __STM32F2xx_HAL_RTC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_hal_def.h"

/** @addtogroup STM32F2xx_HAL_Driver
  * @{
  */

/** @addtogroup RTCEx
  * @{
  */
 
/* Exported types ------------------------------------------------------------*/ 
/** @defgroup RTCEx_Exported_Types RTC Exported Types
  * @{
  */

/** @brief  RTC Tamper structure definition  
  */
typedef struct 
{
  uint32_t Tamper;                      /*!< Specifies the Tamper Pin.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Pins_Definitions */
  
  uint32_t PinSelection;                /*!< Specifies the Tamper Pin.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Pins_Selection */
                                             
  uint32_t Trigger;                     /*!< Specifies the Tamper Trigger.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Trigger_Definitions */
}RTC_TamperTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Constants RTC Exported Constants
  * @{
  */ 

/** @defgroup RTCEx_Backup_Registers_Definitions RTC Backup Registers Definitions
  * @{
  */
#define RTC_BKP_DR0                       ((uint32_t)0x00000000U)
#define RTC_BKP_DR1                       ((uint32_t)0x00000001U)
#define RTC_BKP_DR2                       ((uint32_t)0x00000002U)
#define RTC_BKP_DR3                       ((uint32_t)0x00000003U)
#define RTC_BKP_DR4                       ((uint32_t)0x00000004U)
#define RTC_BKP_DR5                       ((uint32_t)0x00000005U)
#define RTC_BKP_DR6                       ((uint32_t)0x00000006U)
#define RTC_BKP_DR7                       ((uint32_t)0x00000007U)
#define RTC_BKP_DR8                       ((uint32_t)0x00000008U)
#define RTC_BKP_DR9                       ((uint32_t)0x00000009U)
#define RTC_BKP_DR10                      ((uint32_t)0x0000000AU)
#define RTC_BKP_DR11                      ((uint32_t)0x0000000BU)
#define RTC_BKP_DR12                      ((uint32_t)0x0000000CU)
#define RTC_BKP_DR13                      ((uint32_t)0x0000000DU)
#define RTC_BKP_DR14                      ((uint32_t)0x0000000EU)
#define RTC_BKP_DR15                      ((uint32_t)0x0000000FU)
#define RTC_BKP_DR16                      ((uint32_t)0x00000010U)
#define RTC_BKP_DR17                      ((uint32_t)0x00000011U)
#define RTC_BKP_DR18                      ((uint32_t)0x00000012U)
#define RTC_BKP_DR19                      ((uint32_t)0x00000013U)
/**
  * @}
  */ 

/** @defgroup RTCEx_Time_Stamp_Edges_definitions RTC TimeStamp Edges Definitions
  * @{
  */ 
#define RTC_TIMESTAMPEDGE_RISING          ((uint32_t)0x00000000U)
#define RTC_TIMESTAMPEDGE_FALLING         ((uint32_t)0x00000008U)
/**
  * @}
  */
  
/** @defgroup RTCEx_Tamper_Pins_Definitions RTC Tamper Pins Definitions
  * @{
  */ 
#define RTC_TAMPER_1                    RTC_TAFCR_TAMP1E
/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Pins_Selection RTC tamper Pins Selection
  * @{
  */ 
#define RTC_TAMPERPIN_DEFAULT               ((uint32_t)0x00000000U)
#define RTC_TAMPERPIN_POS1                  ((uint32_t)0x00010000U)

/**
  * @}
  */ 

/** @defgroup RTCEx_TimeStamp_Pin_Selection RTC TimeStamp Pins Selection
  * @{
  */ 
#define RTC_TIMESTAMPPIN_DEFAULT            ((uint32_t)0x00000000U)
#define RTC_TIMESTAMPPIN_POS1               ((uint32_t)0x00020000U)

/**
  * @}
  */ 

/** @defgroup RTCEx_Tamper_Trigger_Definitions RTC Tamper Triggers Definitions
  * @{
  */ 
#define RTC_TAMPERTRIGGER_RISINGEDGE       ((uint32_t)0x00000000U)
#define RTC_TAMPERTRIGGER_FALLINGEDGE      ((uint32_t)0x00000002U)

/**
  * @}
  */  

/** @defgroup RTCEx_Wakeup_Timer_Definitions RTC Wake-up Timer Definitions
  * @{
  */ 
#define RTC_WAKEUPCLOCK_RTCCLK_DIV16        ((uint32_t)0x00000000U)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV8         ((uint32_t)0x00000001U)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV4         ((uint32_t)0x00000002U)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV2         ((uint32_t)0x00000003U)
#define RTC_WAKEUPCLOCK_CK_SPRE_16BITS      ((uint32_t)0x00000004U)
#define RTC_WAKEUPCLOCK_CK_SPRE_17BITS      ((uint32_t)0x00000006U)
/**
  * @}
  */ 

/** @defgroup RTCEx_Digital_Calibration_Definitions RTC Digital Calib Definitions
  * @{
  */ 
#define RTC_CALIBSIGN_POSITIVE            ((uint32_t)0x00000000U) 
#define RTC_CALIBSIGN_NEGATIVE            ((uint32_t)0x00000080U)
/**
  * @}
  */

/**
  * @}
  */ 
  
/* Exported macro ------------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Macros RTC Exported Macros
  * @{
  */

/* ---------------------------------WAKEUPTIMER---------------------------------*/
/** @defgroup RTCEx_WakeUp_Timer RTC WakeUp Timer
  * @{
  */

/**
  * @brief  Enable the RTC WakeUp Timer peripheral.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_ENABLE(__HANDLE__)                      ((__HANDLE__)->Instance->CR |= (RTC_CR_WUTE))

/**
  * @brief  Disable the RTC Wake-up Timer peripheral.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_DISABLE(__HANDLE__)                     ((__HANDLE__)->Instance->CR &= ~(RTC_CR_WUTE))

/**
  * @brief  Enable the RTC WakeUpTimer interrupt.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC WakeUpTimer interrupt sources to be enabled or disabled. 
  *         This parameter can be:
  *            @arg RTC_IT_WUT: WakeUpTimer A interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_ENABLE_IT(__HANDLE__, __INTERRUPT__)    ((__HANDLE__)->Instance->CR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC WakeUpTimer interrupt.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC WakeUpTimer interrupt sources to be enabled or disabled. 
  *         This parameter can be:
  *            @arg RTC_IT_WUT: WakeUpTimer A interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_DISABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->CR &= ~(__INTERRUPT__))

/**
  * @brief  Check whether the specified RTC WakeUpTimer interrupt has occurred or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC WakeUpTimer interrupt to check.
  *         This parameter can be:
  *            @arg RTC_IT_WUT:  WakeUpTimer A interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_IT(__HANDLE__, __INTERRUPT__)            (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 4U)) != RESET)? SET : RESET)

/**
  * @brief  Check whether the specified RTC Wake Up timer interrupt has been enabled or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC Wake Up timer interrupt sources to check.
  *         This parameter can be:
  *            @arg RTC_IT_WUT:  WakeUpTimer interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)   (((((__HANDLE__)->Instance->CR) & (__INTERRUPT__)) != RESET) ? SET : RESET)

/**
  * @brief  Get the selected RTC WakeUpTimer's flag status.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC WakeUpTimer Flag to check.
  *          This parameter can be:
  *             @arg RTC_FLAG_WUTF   
  *             @arg RTC_FLAG_WUTWF     
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_FLAG(__HANDLE__, __FLAG__)          (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != RESET)? SET : RESET)

/**
  * @brief  Clear the RTC Wake Up timer's pending flags.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC Tamper Flag sources to be enabled or disabled.
  *         This parameter can be:
  *            @arg RTC_FLAG_WUTF   
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(__HANDLE__, __FLAG__)            ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))  

/**
  * @brief  Enable interrupt on the RTC Wake-up Timer associated Exti line.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT()       (EXTI->IMR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable interrupt on the RTC Wake-up Timer associated Exti line.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_IT()      (EXTI->IMR &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Enable event on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_EVENT()    (EXTI->EMR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable event on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_EVENT()   (EXTI->EMR &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Enable falling edge trigger on the RTC Wake-up Timer associated Exti line. 
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_FALLING_EDGE()   (EXTI->FTSR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable falling edge trigger on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_FALLING_EDGE()  (EXTI->FTSR &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Enable rising edge trigger on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_EDGE()    (EXTI->RTSR |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable rising edge trigger on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_EDGE()   (EXTI->RTSR &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Enable rising & falling edge trigger on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_FALLING_EDGE() do{ __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_EDGE();\
                                                                    __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_FALLING_EDGE();\
                                                                  }while(0)

/**
  * @brief  Disable rising & falling edge trigger on the RTC Wake-up Timer associated Exti line.
  * This parameter can be:
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_FALLING_EDGE() do{ __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_EDGE();\
                                                                     __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_FALLING_EDGE();\
                                                                   }while(0)

/**
  * @brief Check whether the RTC Wake-up Timer associated Exti line interrupt flag is set or not.
  * @retval Line Status.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_GET_FLAG()              (EXTI->PR & RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief Clear the RTC Wake-up Timer associated Exti line flag.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG()            (EXTI->PR = RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief Generate a Software interrupt on the RTC Wake-up Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_GENERATE_SWIT()         (EXTI->SWIER |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @}
  */

/* ---------------------------------TIMESTAMP---------------------------------*/
/** @defgroup RTCEx_Timestamp RTC Timestamp
  * @{
  */

/**
  * @brief  Enable the RTC TimeStamp peripheral.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_ENABLE(__HANDLE__)                        ((__HANDLE__)->Instance->CR |= (RTC_CR_TSE))

/**
  * @brief  Disable the RTC TimeStamp peripheral.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_DISABLE(__HANDLE__)                       ((__HANDLE__)->Instance->CR &= ~(RTC_CR_TSE))

/**
  * @brief  Enable the RTC TimeStamp interrupt.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC TimeStamp interrupt sources to be enabled or disabled. 
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_ENABLE_IT(__HANDLE__, __INTERRUPT__)      ((__HANDLE__)->Instance->CR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC TimeStamp interrupt.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC TimeStamp interrupt sources to be enabled or disabled. 
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_DISABLE_IT(__HANDLE__, __INTERRUPT__)     ((__HANDLE__)->Instance->CR &= ~(__INTERRUPT__))

/**
  * @brief  Check whether the specified RTC TimeStamp interrupt has occurred or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC TimeStamp interrupt to check.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_IT(__HANDLE__, __INTERRUPT__)         (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 4U)) != RESET)? SET : RESET)

/**
  * @brief  Check whether the specified RTC Time Stamp interrupt has been enabled or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC Time Stamp interrupt source to check.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)     (((((__HANDLE__)->Instance->CR) & (__INTERRUPT__)) != RESET) ? SET : RESET)

/**
  * @brief  Get the selected RTC TimeStamp's flag status.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC TimeStamp flag to check.
  *         This parameter can be:
  *            @arg RTC_FLAG_TSF   
  *            @arg RTC_FLAG_TSOVF     
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_FLAG(__HANDLE__, __FLAG__)            (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != RESET)? SET : RESET)

/**
  * @brief  Clear the RTC Time Stamp's pending flags.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC Alarm Flag sources to be enabled or disabled.
  *          This parameter can be:
  *             @arg RTC_FLAG_TSF  
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_CLEAR_FLAG(__HANDLE__, __FLAG__)          ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))

/**
  * @}
  */

/* ---------------------------------TAMPER------------------------------------*/
/** @defgroup RTCEx_Tamper RTC Tamper
  * @{
  */

/**
  * @brief  Enable the RTC Tamper1 input detection.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER1_ENABLE(__HANDLE__)                         ((__HANDLE__)->Instance->TAFCR |= (RTC_TAFCR_TAMP1E))

/**
  * @brief  Disable the RTC Tamper1 input detection.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER1_DISABLE(__HANDLE__)                        ((__HANDLE__)->Instance->TAFCR &= ~(RTC_TAFCR_TAMP1E))

/**
  * @brief  Check whether the specified RTC Tamper interrupt has occurred or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC Tamper interrupt to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP1
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT(__HANDLE__, __INTERRUPT__)       (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 4U)) != RESET)? SET : RESET)

/**
  * @brief  Check whether the specified RTC Tamper interrupt has been enabled or not.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __INTERRUPT__: specifies the RTC Tamper interrupt source to check.
  *         This parameter can be:
  *            @arg RTC_IT_TAMP: Tamper interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)     (((((__HANDLE__)->Instance->TAFCR) & (__INTERRUPT__)) != RESET) ? SET : RESET)

/**
  * @brief  Get the selected RTC Tamper's flag status.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC Tamper Flag sources to be enabled or disabled.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F 
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_FLAG(__HANDLE__, __FLAG__)               (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != RESET)? SET : RESET)

/**
  * @brief  Clear the RTC Tamper's pending flags.
  * @param  __HANDLE__: specifies the RTC handle.
  * @param  __FLAG__: specifies the RTC Tamper Flag to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F
  * @retval None
  */
#define __HAL_RTC_TAMPER_CLEAR_FLAG(__HANDLE__, __FLAG__)         ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))
/**
  * @}
  */

/* --------------------------TAMPER/TIMESTAMP---------------------------------*/
/** @defgroup RTCEx_Tamper_Timestamp EXTI RTC Tamper Timestamp EXTI
  * @{
  */

/**
  * @brief  Enable interrupt on the RTC Tamper and Timestamp associated Exti line.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_IT()        (EXTI->IMR |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable interrupt on the RTC Tamper and Timestamp associated Exti line.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_IT()       (EXTI->IMR &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable event on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_EVENT()    (EXTI->EMR |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable event on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_EVENT()   (EXTI->EMR &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable falling edge trigger on the RTC Tamper and Timestamp associated Exti line. 
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_FALLING_EDGE()   (EXTI->FTSR |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_FALLING_EDGE()  (EXTI->FTSR &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable rising edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_EDGE()    (EXTI->RTSR |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable rising edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_EDGE()   (EXTI->RTSR &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable rising & falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_FALLING_EDGE() do{ __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_EDGE();\
                                                                         __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_FALLING_EDGE();\
                                                                       }while(0)

/**
  * @brief  Disable rising & falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * This parameter can be:
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_FALLING_EDGE() do{ __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_EDGE();\
                                                                          __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_FALLING_EDGE();\
                                                                        }while(0)

/**
  * @brief Check whether the RTC Tamper and Timestamp associated Exti line interrupt flag is set or not.
  * @retval Line Status.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_GET_FLAG()         (EXTI->PR & RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Clear the RTC Tamper and Timestamp associated Exti line flag.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_CLEAR_FLAG()       (EXTI->PR = RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Generate a Software interrupt on the RTC Tamper and Timestamp associated Exti line
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_GENERATE_SWIT()    (EXTI->SWIER |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)
/**
  * @}
  */

/* ------------------------------Calibration----------------------------------*/
/** @defgroup RTCEx_Calibration RTC Calibration
  * @{
  */

/**
  * @brief  Enable the Coarse calibration process.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_COARSE_CALIB_ENABLE(__HANDLE__)                       ((__HANDLE__)->Instance->CR |= (RTC_CR_DCE))

/**
  * @brief  Disable the Coarse calibration process.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_COARSE_CALIB_DISABLE(__HANDLE__)                      ((__HANDLE__)->Instance->CR &= ~(RTC_CR_DCE))

/**
  * @brief  Enable the RTC calibration output.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CALIBRATION_OUTPUT_ENABLE(__HANDLE__)                 ((__HANDLE__)->Instance->CR |= (RTC_CR_COE))

/**
  * @brief  Disable the calibration output.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CALIBRATION_OUTPUT_DISABLE(__HANDLE__)                ((__HANDLE__)->Instance->CR &= ~(RTC_CR_COE))

/**
  * @brief  Enable the clock reference detection.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CLOCKREF_DETECTION_ENABLE(__HANDLE__)                 ((__HANDLE__)->Instance->CR |= (RTC_CR_REFCKON))

/**
  * @brief  Disable the clock reference detection.
  * @param  __HANDLE__: specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CLOCKREF_DETECTION_DISABLE(__HANDLE__)                ((__HANDLE__)->Instance->CR &= ~(RTC_CR_REFCKON))

/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Functions RTC Exported Functions
  * @{
  */

/** @addtogroup RTCEx_Exported_Functions_Group1
  * @{
  */
/* RTC TimeStamp and Tamper functions *****************************************/
HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin);
HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp_IT(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin);
HAL_StatusTypeDef HAL_RTCEx_DeactivateTimeStamp(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_GetTimeStamp(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTimeStamp, RTC_DateTypeDef *sTimeStampDate, uint32_t Format);

HAL_StatusTypeDef HAL_RTCEx_SetTamper(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper);
HAL_StatusTypeDef HAL_RTCEx_SetTamper_IT(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper);
HAL_StatusTypeDef HAL_RTCEx_DeactivateTamper(RTC_HandleTypeDef *hrtc, uint32_t Tamper);
void HAL_RTCEx_TamperTimeStampIRQHandler(RTC_HandleTypeDef *hrtc);

void HAL_RTCEx_Tamper1EventCallback(RTC_HandleTypeDef *hrtc);
void HAL_RTCEx_TimeStampEventCallback(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_PollForTimeStampEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
HAL_StatusTypeDef HAL_RTCEx_PollForTamper1Event(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group2
  * @{
  */
/* RTC Wake-up functions ******************************************************/
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock);
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock);
uint32_t HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *hrtc);
uint32_t HAL_RTCEx_GetWakeUpTimer(RTC_HandleTypeDef *hrtc);
void HAL_RTCEx_WakeUpTimerIRQHandler(RTC_HandleTypeDef *hrtc);
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_PollForWakeUpTimerEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group3
  * @{
  */
/* Extension Control functions ************************************************/
void HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister, uint32_t Data);
uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister);

HAL_StatusTypeDef HAL_RTCEx_SetCoarseCalib(RTC_HandleTypeDef *hrtc, uint32_t CalibSign, uint32_t Value);
HAL_StatusTypeDef HAL_RTCEx_DeactivateCoarseCalib(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_SetCalibrationOutPut(RTC_HandleTypeDef* hrtc);
HAL_StatusTypeDef HAL_RTCEx_DeactivateCalibrationOutPut(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_SetRefClock(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_DeactivateRefClock(RTC_HandleTypeDef *hrtc);
/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group4
  * @{
  */
/* Extension RTC features functions *******************************************/
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc); 
HAL_StatusTypeDef HAL_RTCEx_PollForAlarmBEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
/**
  * @}
  */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/ 
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup RTCEx_Private_Constants RTC Private Constants
  * @{
  */
#define RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT  ((uint32_t)EXTI_IMR_MR21)  /*!< External interrupt line 21 Connected to the RTC Tamper and Time Stamp events */                                               
#define RTC_EXTI_LINE_WAKEUPTIMER_EVENT       ((uint32_t)EXTI_IMR_MR22)  /*!< External interrupt line 22 Connected to the RTC Wake-up event */  
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup RTCEx_Private_Macros RTC Private Macros
  * @{
  */

/** @defgroup RTCEx_IS_RTC_Definitions Private macros to check input parameters
  * @{
  */ 
#define IS_RTC_BKP(BKP)                   (((BKP) == RTC_BKP_DR0)  || \
                                           ((BKP) == RTC_BKP_DR1)  || \
                                           ((BKP) == RTC_BKP_DR2)  || \
                                           ((BKP) == RTC_BKP_DR3)  || \
                                           ((BKP) == RTC_BKP_DR4)  || \
                                           ((BKP) == RTC_BKP_DR5)  || \
                                           ((BKP) == RTC_BKP_DR6)  || \
                                           ((BKP) == RTC_BKP_DR7)  || \
                                           ((BKP) == RTC_BKP_DR8)  || \
                                           ((BKP) == RTC_BKP_DR9)  || \
                                           ((BKP) == RTC_BKP_DR10) || \
                                           ((BKP) == RTC_BKP_DR11) || \
                                           ((BKP) == RTC_BKP_DR12) || \
                                           ((BKP) == RTC_BKP_DR13) || \
                                           ((BKP) == RTC_BKP_DR14) || \
                                           ((BKP) == RTC_BKP_DR15) || \
                                           ((BKP) == RTC_BKP_DR16) || \
                                           ((BKP) == RTC_BKP_DR17) || \
                                           ((BKP) == RTC_BKP_DR18) || \
                                           ((BKP) == RTC_BKP_DR19))
#define IS_TIMESTAMP_EDGE(EDGE) (((EDGE) == RTC_TIMESTAMPEDGE_RISING) || \
                                 ((EDGE) == RTC_TIMESTAMPEDGE_FALLING))

#define IS_RTC_TAMPER_PIN(PIN) (((PIN) == RTC_TAMPERPIN_DEFAULT) || \
                                ((PIN) == RTC_TAMPERPIN_POS1))

#define IS_RTC_TIMESTAMP_PIN(PIN) (((PIN) == RTC_TIMESTAMPPIN_DEFAULT) || \
                                   ((PIN) == RTC_TIMESTAMPPIN_POS1))

#define IS_RTC_TAMPER_TRIGGER(TRIGGER) (((TRIGGER) == RTC_TAMPERTRIGGER_RISINGEDGE) || \
                                        ((TRIGGER) == RTC_TAMPERTRIGGER_FALLINGEDGE))

#define IS_RTC_WAKEUP_CLOCK(CLOCK) (((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV16)   || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV8)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV4)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV2)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_CK_SPRE_16BITS) || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_CK_SPRE_17BITS))

#define IS_RTC_WAKEUP_COUNTER(COUNTER)  ((COUNTER) <= 0xFFFFU)
#define IS_RTC_CALIB_SIGN(SIGN) (((SIGN) == RTC_CALIBSIGN_POSITIVE) || \
                                 ((SIGN) == RTC_CALIBSIGN_NEGATIVE))

#define IS_RTC_CALIB_VALUE(VALUE) ((VALUE) < 0x20U)
#define IS_RTC_TAMPER(TAMPER) ((((TAMPER) & ((uint32_t)!(RTC_TAFCR_TAMP1E))) == 0x00U) && ((TAMPER) != (uint32_t)RESET)) 
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

#endif /* __STM32F2xx_HAL_RTC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
