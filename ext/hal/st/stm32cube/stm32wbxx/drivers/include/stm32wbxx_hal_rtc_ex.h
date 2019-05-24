/**
  ******************************************************************************
  * @file    stm32wbxx_hal_rtc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RTC HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef STM32WBxx_HAL_RTC_EX_H
#define STM32WBxx_HAL_RTC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup RTCEx RTCEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RTCEx_Exported_Types RTCEx Exported Types
  * @{
  */

/**
  * @brief  RTC Tamper structure definition
  */
typedef struct
{
  uint32_t Tamper;                      /*!< Specifies the Tamper Pin.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Pins_Definitions */

  uint32_t Interrupt;                   /*!< Specifies the Tamper Interrupt.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Interrupt_Definitions */

  uint32_t Trigger;                     /*!< Specifies the Tamper Trigger.
                                             This parameter can be a value of @ref  RTCEx_Tamper_Trigger_Definitions */

  uint32_t NoErase;                     /*!< Specifies the Tamper no erase mode.
                                             This parameter can be a value of @ref  RTCEx_Tamper_EraseBackUp_Definitions */

  uint32_t MaskFlag;                     /*!< Specifies the Tamper Flag masking.
                                             This parameter can be a value of @ref RTCEx_Tamper_MaskFlag_Definitions   */

  uint32_t Filter;                      /*!< Specifies the RTC Filter Tamper.
                                             This parameter can be a value of @ref RTCEx_Tamper_Filter_Definitions */

  uint32_t SamplingFrequency;           /*!< Specifies the sampling frequency.
                                             This parameter can be a value of @ref RTCEx_Tamper_Sampling_Frequencies_Definitions */

  uint32_t PrechargeDuration;           /*!< Specifies the Precharge Duration .
                                             This parameter can be a value of @ref RTCEx_Tamper_Pin_Precharge_Duration_Definitions */

  uint32_t TamperPullUp;                /*!< Specifies the Tamper PullUp .
                                             This parameter can be a value of @ref RTCEx_Tamper_Pull_UP_Definitions */

  uint32_t TimeStampOnTamperDetection;  /*!< Specifies the TimeStampOnTamperDetection.
                                             This parameter can be a value of @ref RTCEx_Tamper_TimeStampOnTamperDetection_Definitions */
}RTC_TamperTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Constants RTCEx Exported Constants
  * @{
  */

/** @defgroup RTCEx_Output_selection_Definitions RTCEx Output Selection Definition
  * @{
  */
#define RTC_OUTPUT_DISABLE             ((uint32_t)0x00000000U)
#define RTC_OUTPUT_ALARMA              ((uint32_t)RTC_CR_OSEL_0)
#define RTC_OUTPUT_ALARMB              ((uint32_t)RTC_CR_OSEL_1)
#define RTC_OUTPUT_WAKEUP              ((uint32_t)RTC_CR_OSEL)

/**
  * @}
  */

/** @defgroup RTCEx_Backup_Registers_Definitions RTCEx Backup Registers Definition
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


/** @defgroup RTCEx_Time_Stamp_Edges_definitions RTCEx Time Stamp Edges definition
  * @{
  */
#define RTC_TIMESTAMPEDGE_RISING        ((uint32_t)0x00000000U)
#define RTC_TIMESTAMPEDGE_FALLING       RTC_CR_TSEDGE

/**
  * @}
  */

/** @defgroup RTCEx_TimeStamp_Pin_Selections RTCEx TimeStamp Pin Selection
  * @{
  */
#define RTC_TIMESTAMPPIN_DEFAULT              ((uint32_t)0x00000000U)

/**
  * @}
  */


/** @defgroup RTCEx_Tamper_Pins_Definitions RTCEx Tamper Pins Definition
  * @{
  */
#if defined(RTC_TAMPER1_SUPPORT)
#define RTC_TAMPER_1                       RTC_TAMPCR_TAMP1E
#endif /* RTC_TAMPER1_SUPPORT */

#define RTC_TAMPER_2                       RTC_TAMPCR_TAMP2E

#if defined(RTC_TAMPER3_SUPPORT)
#define RTC_TAMPER_3                       RTC_TAMPCR_TAMP3E
#endif /* RTC_TAMPER3_SUPPORT */
/**
  * @}
  */


/** @defgroup RTCEx_Tamper_Interrupt_Definitions RTCEx Tamper Interrupt Definitions
  * @{
  */
#if defined(RTC_TAMPER1_SUPPORT)
#define RTC_TAMPER1_INTERRUPT              RTC_TAMPCR_TAMP1IE
#endif /* RTC_TAMPER1_SUPPORT */


#define RTC_TAMPER2_INTERRUPT              RTC_TAMPCR_TAMP2IE

#if defined(RTC_TAMPER3_SUPPORT)
#define RTC_TAMPER3_INTERRUPT              RTC_TAMPCR_TAMP3IE
#endif /* RTC_TAMPER3_SUPPORT */
#define RTC_ALL_TAMPER_INTERRUPT             RTC_TAMPCR_TAMPIE
/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Trigger_Definitions RTCEx Tamper Trigger Definitions
  * @{
  */
#define RTC_TAMPERTRIGGER_RISINGEDGE       ((uint32_t)0x00000000U)
#define RTC_TAMPERTRIGGER_FALLINGEDGE      ((uint32_t)0x00000002U)
#define RTC_TAMPERTRIGGER_LOWLEVEL         RTC_TAMPERTRIGGER_RISINGEDGE
#define RTC_TAMPERTRIGGER_HIGHLEVEL        RTC_TAMPERTRIGGER_FALLINGEDGE

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_EraseBackUp_Definitions RTCEx Tamper EraseBackUp Definitions
* @{
*/
#define RTC_TAMPER_ERASE_BACKUP_ENABLE               ((uint32_t)0x00000000U)
#define RTC_TAMPER_ERASE_BACKUP_DISABLE              ((uint32_t)0x00020000U)
/**
  * @}
  */

/** @defgroup RTCEx_Tamper_MaskFlag_Definitions RTCEx Tamper MaskFlag Definitions
* @{
*/
#define RTC_TAMPERMASK_FLAG_DISABLE               ((uint32_t)0x00000000U)
#define RTC_TAMPERMASK_FLAG_ENABLE                ((uint32_t)0x00040000U)

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Filter_Definitions RTCEx Tamper Filter Definitions
  * @{
  */
#define RTC_TAMPERFILTER_DISABLE   ((uint32_t)0x00000000U)  /*!< Tamper filter is disabled */

#define RTC_TAMPERFILTER_2SAMPLE   RTC_TAMPCR_TAMPFLT_0    /*!< Tamper is activated after 2
                                                                consecutive samples at the active level */
#define RTC_TAMPERFILTER_4SAMPLE   RTC_TAMPCR_TAMPFLT_1    /*!< Tamper is activated after 4
                                                                consecutive samples at the active level */
#define RTC_TAMPERFILTER_8SAMPLE   RTC_TAMPCR_TAMPFLT      /*!< Tamper is activated after 8
                                                                consecutive samples at the active leve. */

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Sampling_Frequencies_Definitions RTCEx Tamper Sampling Frequencies Definitions
  * @{
  */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV32768  ((uint32_t)0x00000000U)                                         /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 32768 */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV16384  RTC_TAMPCR_TAMPFREQ_0                                          /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 16384 */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV8192   RTC_TAMPCR_TAMPFREQ_1                                          /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 8192  */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV4096   ((uint32_t) (RTC_TAMPCR_TAMPFREQ_0 | RTC_TAMPCR_TAMPFREQ_1))   /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 4096  */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV2048   RTC_TAMPCR_TAMPFREQ_2                                          /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 2048  */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV1024   ((uint32_t) (RTC_TAMPCR_TAMPFREQ_0 | RTC_TAMPCR_TAMPFREQ_2))   /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 1024  */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV512    ((uint32_t) (RTC_TAMPCR_TAMPFREQ_1 | RTC_TAMPCR_TAMPFREQ_2))   /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 512   */
#define RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV256    ((uint32_t) (RTC_TAMPCR_TAMPFREQ_0 | RTC_TAMPCR_TAMPFREQ_1 | \
                                                 RTC_TAMPCR_TAMPFREQ_2))                                       /*!< Each of the tamper inputs are sampled
                                                                                                                    with a frequency =  RTCCLK / 256   */

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Pin_Precharge_Duration_Definitions RTCEx Tamper Pin Precharge Duration Definitions
  * @{
  */
#define RTC_TAMPERPRECHARGEDURATION_1RTCCLK  ((uint32_t)0x00000000U)                                     /*!< Tamper pins are pre-charged before
                                                                                                            sampling during 1 RTCCLK cycle  */
#define RTC_TAMPERPRECHARGEDURATION_2RTCCLK  RTC_TAMPCR_TAMPPRCH_0                                      /*!< Tamper pins are pre-charged before
                                                                                                             sampling during 2 RTCCLK cycles */
#define RTC_TAMPERPRECHARGEDURATION_4RTCCLK  RTC_TAMPCR_TAMPPRCH_1                                      /*!< Tamper pins are pre-charged before
                                                                                                             sampling during 4 RTCCLK cycles */
#define RTC_TAMPERPRECHARGEDURATION_8RTCCLK ((uint32_t)(RTC_TAMPCR_TAMPPRCH_0 | RTC_TAMPCR_TAMPPRCH_1)) /*!< Tamper pins are pre-charged before
                                                                                                             sampling during 8 RTCCLK cycles */

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_TimeStampOnTamperDetection_Definitions RTCEx Tamper TimeStampOnTamperDetection Definitions
  * @{
  */
#define RTC_TIMESTAMPONTAMPERDETECTION_ENABLE  RTC_TAMPCR_TAMPTS       /*!< TimeStamp on Tamper Detection event saved        */
#define RTC_TIMESTAMPONTAMPERDETECTION_DISABLE ((uint32_t)0x00000000U)  /*!< TimeStamp on Tamper Detection event is not saved */

/**
  * @}
  */

/** @defgroup RTCEx_Tamper_Pull_UP_Definitions RTCEx Tamper Pull UP Definitions
  * @{
  */
#define RTC_TAMPER_PULLUP_ENABLE  ((uint32_t)0x00000000U)  /*!< Tamper pins are pre-charged before sampling */
#define RTC_TAMPER_PULLUP_DISABLE  RTC_TAMPCR_TAMPPUDIS   /*!< Tamper pins pre-charge is disabled          */

/**
  * @}
  */

/** @defgroup RTCEx_Wakeup_Timer_Definitions RTCEx Wakeup Timer Definitions
  * @{
  */
#define RTC_WAKEUPCLOCK_RTCCLK_DIV16        ((uint32_t)0x00000000U)
#define RTC_WAKEUPCLOCK_RTCCLK_DIV8         RTC_CR_WUCKSEL_0
#define RTC_WAKEUPCLOCK_RTCCLK_DIV4         RTC_CR_WUCKSEL_1
#define RTC_WAKEUPCLOCK_RTCCLK_DIV2         ((uint32_t) (RTC_CR_WUCKSEL_0 | RTC_CR_WUCKSEL_1))
#define RTC_WAKEUPCLOCK_CK_SPRE_16BITS      RTC_CR_WUCKSEL_2
#define RTC_WAKEUPCLOCK_CK_SPRE_17BITS      ((uint32_t) (RTC_CR_WUCKSEL_1 | RTC_CR_WUCKSEL_2))
/**
  * @}
  */

/** @defgroup RTCEx_Smooth_calib_period_Definitions RTCEx Smooth calib period Definitions
  * @{
  */
#define RTC_SMOOTHCALIB_PERIOD_32SEC   ((uint32_t)0x00000000U)   /*!< If RTCCLK = 32768 Hz, Smooth calibation
                                                                     period is 32s,  else 2exp20 RTCCLK pulses */
#define RTC_SMOOTHCALIB_PERIOD_16SEC   RTC_CALR_CALW16          /*!< If RTCCLK = 32768 Hz, Smooth calibation
                                                                     period is 16s, else 2exp19 RTCCLK pulses */
#define RTC_SMOOTHCALIB_PERIOD_8SEC    RTC_CALR_CALW8           /*!< If RTCCLK = 32768 Hz, Smooth calibation
                                                                     period is 8s, else 2exp18 RTCCLK pulses */

/**
  * @}
  */

/** @defgroup RTCEx_Smooth_calib_Plus_pulses_Definitions RTCEx Smooth calib Plus pulses Definitions
  * @{
  */
#define RTC_SMOOTHCALIB_PLUSPULSES_SET    RTC_CALR_CALP            /*!< The number of RTCCLK pulses added
                                                                        during a X -second window = Y - CALM[8:0]
                                                                        with Y = 512, 256, 128 when X = 32, 16, 8 */
#define RTC_SMOOTHCALIB_PLUSPULSES_RESET  ((uint32_t)0x00000000U)   /*!< The number of RTCCLK pulses subbstited
                                                                        during a 32-second window = CALM[8:0] */

/**
  * @}
  */
 /** @defgroup RTCEx_Calib_Output_selection_Definitions RTCEx Calib Output selection Definitions
  * @{
  */
#define RTC_CALIBOUTPUT_512HZ            ((uint32_t)0x00000000U)
#define RTC_CALIBOUTPUT_1HZ              RTC_CR_COSEL

/**
  * @}
  */


/** @defgroup RTCEx_Add_1_Second_Parameter_Definition RTCEx Add 1 Second Parameter Definitions
  * @{
  */
#define RTC_SHIFTADD1S_RESET      ((uint32_t)0x00000000U)
#define RTC_SHIFTADD1S_SET        RTC_SHIFTR_ADD1S
/**
  * @}
  */
  /** @defgroup RTCEx_Interrupts_Definitions RTCEx Interrupts Definitions
  * @{
  */
#if defined(RTC_TAMPER3_SUPPORT)
#define RTC_IT_TAMP3                      ((uint32_t)RTC_TAMPCR_TAMP3IE)  /*!< Enable Tamper 3 Interrupt     */
#endif
/**
  * @}
  */

/** @defgroup RTCEx_Flags_Definitions RTCEx Flags Definitions
  * @{
  */
#if defined(RTC_TAMPER3_SUPPORT)
#define RTC_FLAG_TAMP3F                   ((uint32_t)RTC_ISR_TAMP3F)
#endif
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Macros RTCEx Exported Macros
  * @{
  */

/* ---------------------------------WAKEUPTIMER---------------------------------*/
/** @defgroup RTCEx_WakeUp_Timer RTC WakeUp Timer
  * @{
  */
/**
  * @brief  Enable the RTC WakeUp Timer peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_ENABLE(__HANDLE__)                      ((__HANDLE__)->Instance->CR |= (RTC_CR_WUTE))

/**
  * @brief  Disable the RTC WakeUp Timer peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_DISABLE(__HANDLE__)                     ((__HANDLE__)->Instance->CR &= ~(RTC_CR_WUTE))

/**
  * @brief  Enable the RTC WakeUpTimer interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC WakeUpTimer interrupt sources to be enabled.
  *         This parameter can be:
  *            @arg RTC_IT_WUT: WakeUpTimer interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_ENABLE_IT(__HANDLE__, __INTERRUPT__)    ((__HANDLE__)->Instance->CR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC WakeUpTimer interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC WakeUpTimer interrupt sources to be disabled.
  *         This parameter can be:
  *            @arg RTC_IT_WUT: WakeUpTimer interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_DISABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->CR &= ~(__INTERRUPT__))

/**
  * @brief  Check whether the specified RTC WakeUpTimer interrupt has occurred or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC WakeUpTimer interrupt to check.
  *         This parameter can be:
  *            @arg RTC_IT_WUT:  WakeUpTimer interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_IT(__HANDLE__, __INTERRUPT__)       (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 4U)) != 0U) ? 1U : 0U)

/**
  * @brief  Check whether the specified RTC Wake Up timer interrupt has been enabled or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Wake Up timer interrupt sources to check.
  *         This parameter can be:
  *            @arg RTC_IT_WUT:  WakeUpTimer interrupt
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)   (((((__HANDLE__)->Instance->CR) & (__INTERRUPT__)) != 0U) ? 1U : 0U)

/**
  * @brief  Get the selected RTC WakeUpTimer's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC WakeUpTimer Flag is pending or not.
  *          This parameter can be:
  *             @arg RTC_FLAG_WUTF
  *             @arg RTC_FLAG_WUTWF
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_GET_FLAG(__HANDLE__, __FLAG__)   (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Wake Up timer's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC WakeUpTimer Flag to clear.
  *         This parameter can be:
  *            @arg RTC_FLAG_WUTF
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))

/**
  * @brief  Enable interrupt on the RTC WakeUp Timer associated Exti line of core 1.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT()       (EXTI->IMR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable interrupt on the RTC WakeUp Timer associated Exti line of core 1.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_IT()      (EXTI->IMR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))


/**
  * @brief  Enable interrupt on the RTC WakeUp Timer associated Exti line of core 2.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_ENABLE_IT()       (EXTI->C2IMR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable interrupt on the RTC WakeUp Timer associated Exti line of core 2.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_DISABLE_IT()      (EXTI->C2IMR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))


/**
  * @brief  Enable event on the RTC WakeUp Timer associated Exti line of core 1.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_EVENT()    (EXTI->EMR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Enable event on the RTC WakeUp Timer associated Exti line of core 2.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_ENABLE_EVENT()    (EXTI->C2EMR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)


/**
  * @brief  Disable event on the RTC WakeUp Timer associated Exti line of core 1.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_EVENT()   (EXTI->EMR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Disable event on the RTC WakeUp Timer associated Exti line of core 2.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_DISABLE_EVENT()   (EXTI->C2EMR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))


/*---------------*/
/**
  * @brief  Enable falling edge trigger on the RTC WakeUp Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_FALLING_EDGE()   (EXTI->FTSR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable falling edge trigger on the RTC WakeUp Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_FALLING_EDGE()  (EXTI->FTSR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))

/**
  * @brief  Enable rising edge trigger on the RTC WakeUp Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_EDGE()    (EXTI->RTSR1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief  Disable rising edge trigger on the RTC WakeUp Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_EDGE()   (EXTI->RTSR1 &= ~(RTC_EXTI_LINE_WAKEUPTIMER_EVENT))


/**
  * @brief  Enable rising & falling edge trigger on the RTC WakeUp Timer associated Exti line.
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_FALLING_EDGE()  do { \
                                                                   __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_RISING_EDGE();  \
                                                                   __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_FALLING_EDGE(); \
                                                                 } while(0U)

/**
  * @brief  Disable rising & falling edge trigger on the RTC WakeUp Timer associated Exti line.
  * This parameter can be:
  * @retval None
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_FALLING_EDGE()  do { \
                                                                   __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_RISING_EDGE();  \
                                                                   __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_FALLING_EDGE(); \
                                                                  } while(0U)

/**
  * @brief Check whether the RTC WakeUp Timer associated Exti line interrupt flag is set or not of core 1.
  * @retval Line Status.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_GET_FLAG()              (EXTI->PR1 & RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief Check whether the RTC WakeUp Timer associated Exti line interrupt flag is set or not of core 2.
  * @retval Line Status.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_GET_FLAG()            (EXTI->PR1 & RTC_EXTI_LINE_WAKEUPTIMER_EVENT)



/**
  * @brief Clear the RTC WakeUp Timer associated Exti line flag of core 1.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG()            (EXTI->PR1 = RTC_EXTI_LINE_WAKEUPTIMER_EVENT)

/**
  * @brief Clear the RTC WakeUp Timer associated Exti line flag of core 2.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTIC2_CLEAR_FLAG()          (EXTI->PR1 = RTC_EXTI_LINE_WAKEUPTIMER_EVENT)


/*---------------*/
/**
  * @brief Generate a Software interrupt on the RTC WakeUp Timer associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_WAKEUPTIMER_EXTI_GENERATE_SWIT()         (EXTI->SWIER1 |= RTC_EXTI_LINE_WAKEUPTIMER_EVENT)
/*---------------*/

/**
  * @}
  */

/* ---------------------------------TIMESTAMP---------------------------------*/
/** @defgroup RTCEx_Timestamp RTC Timestamp
  * @{
  */
/**
  * @brief  Enable the RTC TimeStamp peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_ENABLE(__HANDLE__)                       ((__HANDLE__)->Instance->CR |= (RTC_CR_TSE))

/**
  * @brief  Disable the RTC TimeStamp peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_DISABLE(__HANDLE__)                      ((__HANDLE__)->Instance->CR &= ~(RTC_CR_TSE))

/**
  * @brief  Enable the RTC TimeStamp interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC TimeStamp interrupt source to be enabled.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_ENABLE_IT(__HANDLE__, __INTERRUPT__)     ((__HANDLE__)->Instance->CR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC TimeStamp interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC TimeStamp interrupt source to be disabled.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_DISABLE_IT(__HANDLE__, __INTERRUPT__)    ((__HANDLE__)->Instance->CR &= ~(__INTERRUPT__))

/**
  * @brief  Check whether the specified RTC TimeStamp interrupt has occurred or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC TimeStamp interrupt to check.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_IT(__HANDLE__, __INTERRUPT__)        (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 4U)) != 0U) ? 1U : 0U)

/**
  * @brief  Check whether the specified RTC Time Stamp interrupt has been enabled or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Time Stamp interrupt source to check.
  *         This parameter can be:
  *            @arg RTC_IT_TS: TimeStamp interrupt
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)     (((((__HANDLE__)->Instance->CR) & (__INTERRUPT__)) != 0U) ? 1U : 0U)

/**
  * @brief  Get the selected RTC TimeStamp's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC TimeStamp Flag is pending or not.
  *         This parameter can be:
  *            @arg RTC_FLAG_TSF
  *            @arg RTC_FLAG_TSOVF
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_GET_FLAG(__HANDLE__, __FLAG__)     (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Time Stamp's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC TimeStamp Flag to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_TSF
  *             @arg RTC_FLAG_TSOVF
  * @retval None
  */
#define __HAL_RTC_TIMESTAMP_CLEAR_FLAG(__HANDLE__, __FLAG__)   ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))


/**
  * @}
  */

/* ---------------------------------TAMPER------------------------------------*/
/** @defgroup RTCEx_Tamper RTC Tamper
  * @{
  */

#if defined(RTC_TAMPER1_SUPPORT)
/**
  * @brief  Enable the RTC Tamper1 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER1_ENABLE(__HANDLE__)                         ((__HANDLE__)->Instance->TAMPCR |= (RTC_TAMPCR_TAMP1E))

/**
  * @brief  Disable the RTC Tamper1 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER1_DISABLE(__HANDLE__)                        ((__HANDLE__)->Instance->TAMPCR &= ~(RTC_TAMPCR_TAMP1E))
#endif /* RTC_TAMPER1_SUPPORT */

/**
  * @brief  Enable the RTC Tamper2 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER2_ENABLE(__HANDLE__)                         ((__HANDLE__)->Instance->TAMPCR |= (RTC_TAMPCR_TAMP2E))

/**
  * @brief  Disable the RTC Tamper2 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER2_DISABLE(__HANDLE__)                        ((__HANDLE__)->Instance->TAMPCR &= ~(RTC_TAMPCR_TAMP2E))

#if defined(RTC_TAMPER3_SUPPORT)
/**
  * @brief  Enable the RTC Tamper3 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER3_ENABLE(__HANDLE__)                         ((__HANDLE__)->Instance->TAMPCR |= (RTC_TAMPCR_TAMP3E))

/**
  * @brief  Disable the RTC Tamper3 input detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_TAMPER3_DISABLE(__HANDLE__)                        ((__HANDLE__)->Instance->TAMPCR &= ~(RTC_TAMPCR_TAMP3E))
#endif /* RTC_TAMPER3_SUPPORT */


/**************************************************************************************************/

#if defined(RTC_TAMPER1_SUPPORT) && defined(RTC_TAMPER3_SUPPORT)
		
/**
  * @brief  Enable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *             @arg  RTC_IT_TAMP: All tampers interrupts
  *             @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *             @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *             @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_ENABLE_IT(__HANDLE__, __INTERRUPT__)        ((__HANDLE__)->Instance->TAMPCR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be disabled.
  *         This parameter can be any combination of the following values:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_DISABLE_IT(__HANDLE__, __INTERRUPT__)       ((__HANDLE__)->Instance->TAMPCR &= ~(__INTERRUPT__))

#elif defined(RTC_TAMPER1_SUPPORT)

/**
  * @brief  Enable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *             @arg  RTC_IT_TAMP: All tampers interrupts
  *             @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *             @arg  RTC_IT_TAMP2: Tamper2 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_ENABLE_IT(__HANDLE__, __INTERRUPT__)        ((__HANDLE__)->Instance->TAMPCR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be disabled.
  *         This parameter can be any combination of the following values:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_DISABLE_IT(__HANDLE__, __INTERRUPT__)       ((__HANDLE__)->Instance->TAMPCR &= ~(__INTERRUPT__))

#elif defined(RTC_TAMPER3_SUPPORT)


/**
  * @brief  Enable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *             @arg  RTC_IT_TAMP: All tampers interrupts
  *             @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *             @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_ENABLE_IT(__HANDLE__, __INTERRUPT__)        ((__HANDLE__)->Instance->TAMPCR |= (__INTERRUPT__))

/**
  * @brief  Disable the RTC Tamper interrupt.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt sources to be disabled.
  *         This parameter can be any combination of the following values:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_DISABLE_IT(__HANDLE__, __INTERRUPT__)       ((__HANDLE__)->Instance->TAMPCR &= ~(__INTERRUPT__))

#endif

/**************************************************************************************************/

#if  defined(RTC_TAMPER1_SUPPORT) && defined(RTC_TAMPER3_SUPPORT)
	
/**
  * @brief  Check whether the specified RTC Tamper interrupt has occurred or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT(__HANDLE__, __INTERRUPT__)    (((__INTERRUPT__) == RTC_IT_TAMP1) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 3U)) != 0U) ? 1U : 0U) : \
                                                               ((__INTERRUPT__) == RTC_IT_TAMP2) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 5U)) != 0U) ? 1U : 0U) : \
                                                               ((__INTERRUPT__) == RTC_IT_TAMP3) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 7U)) != 0U) ? 1U : 0U))

#elif defined(RTC_TAMPER1_SUPPORT)

/**
  * @brief  Check whether the specified RTC Tamper interrupt has occurred or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT(__HANDLE__, __INTERRUPT__)    (((__INTERRUPT__) == RTC_IT_TAMP1) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 3U)) != 0U) ? 1U : 0U) : \
                                                               ((__INTERRUPT__) == RTC_IT_TAMP2) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 5U)) != 0U) ? 1U : 0U))

#elif defined(RTC_TAMPER3_SUPPORT)

/**
  * @brief  Check whether the specified RTC Tamper interrupt has occurred or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT(__HANDLE__, __INTERRUPT__)    (((__INTERRUPT__) == RTC_IT_TAMP2) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 5U)) != 0U) ? 1U : 0U) : \
                                                               ((__INTERRUPT__) == RTC_IT_TAMP3) ? (((((__HANDLE__)->Instance->ISR) & ((__INTERRUPT__)>> 7U)) != 0U) ? 1U : 0U))


#endif

/**************************************************************************************************/

#if defined(RTC_TAMPER1_SUPPORT) && defined(RTC_TAMPER3_SUPPORT)
	
/**
  * @brief  Check whether the specified RTC Tamper interrupt has been enabled or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt source to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)    (((((__HANDLE__)->Instance->TAMPCR) & (__INTERRUPT__)) != 0U) ? 1U : 0U)


/**
  * @brief  Get the selected RTC Tamper's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag is pending or not.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F: Tamper1 flag
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  *             @arg RTC_FLAG_TAMP3F: Tamper3 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_FLAG(__HANDLE__, __FLAG__)        (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Tamper's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F: Tamper1 flag
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  *             @arg RTC_FLAG_TAMP3F: Tamper3 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_CLEAR_FLAG(__HANDLE__, __FLAG__)      ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))

#elif defined(RTC_TAMPER1_SUPPORT)

/**
  * @brief  Check whether the specified RTC Tamper interrupt has been enabled or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt source to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP1: Tamper1 interrupt
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)    (((((__HANDLE__)->Instance->TAMPCR) & (__INTERRUPT__)) != 0U) ? 1U : 0U)


/**
  * @brief  Get the selected RTC Tamper's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag is pending or not.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F: Tamper1 flag
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_FLAG(__HANDLE__, __FLAG__)        (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Tamper's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP1F: Tamper1 flag
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_CLEAR_FLAG(__HANDLE__, __FLAG__)      ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))

#elif defined(RTC_TAMPER3_SUPPORT)

	
/**
  * @brief  Check whether the specified RTC Tamper interrupt has been enabled or not.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __INTERRUPT__ specifies the RTC Tamper interrupt source to check.
  *         This parameter can be:
  *            @arg  RTC_IT_TAMP: All tampers interrupts
  *            @arg  RTC_IT_TAMP2: Tamper2 interrupt
  *            @arg  RTC_IT_TAMP3: Tamper3 interrupt
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)    (((((__HANDLE__)->Instance->TAMPCR) & (__INTERRUPT__)) != 0U) ? 1U : 0U)


/**
  * @brief  Get the selected RTC Tamper's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag is pending or not.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  *             @arg RTC_FLAG_TAMP3F: Tamper3 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_GET_FLAG(__HANDLE__, __FLAG__)        (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Tamper's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Tamper Flag to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_TAMP2F: Tamper2 flag
  *             @arg RTC_FLAG_TAMP3F: Tamper3 flag
  * @retval None
  */
#define __HAL_RTC_TAMPER_CLEAR_FLAG(__HANDLE__, __FLAG__)      ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))

#endif


/**************************************************************************************************/


#if defined(RTC_INTERNALTS_SUPPORT)
/**
  * @brief  Enable the RTC internal TimeStamp peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_INTERNAL_TIMESTAMP_ENABLE(__HANDLE__)                ((__HANDLE__)->Instance->CR |= (RTC_CR_ITSE))

/**
  * @brief  Disable the RTC internal TimeStamp peripheral.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_INTERNAL_TIMESTAMP_DISABLE(__HANDLE__)               ((__HANDLE__)->Instance->CR &= ~(RTC_CR_ITSE))

/**
  * @brief  Get the selected RTC Internal Time Stamp's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Internal Time Stamp Flag is pending or not.
  *         This parameter can be:
  *            @arg RTC_FLAG_ITSF
  * @retval None
  */
#define __HAL_RTC_INTERNAL_TIMESTAMP_GET_FLAG(__HANDLE__, __FLAG__)    (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U) ? 1U : 0U)

/**
  * @brief  Clear the RTC Internal Time Stamp's pending flags.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC Internal Time Stamp Flag source to clear.
  *          This parameter can be:
  *             @arg RTC_FLAG_ITSF
  * @retval None
  */
#define __HAL_RTC_INTERNAL_TIMESTAMP_CLEAR_FLAG(__HANDLE__, __FLAG__)  ((__HANDLE__)->Instance->ISR) = (~((__FLAG__) | RTC_ISR_INIT)|((__HANDLE__)->Instance->ISR & RTC_ISR_INIT))
#endif
/**************************************************************************************************/

/**
  * @}
  */

/* --------------------------TAMPER/TIMESTAMP---------------------------------*/
/** @defgroup RTCEx_Tamper_Timestamp EXTI RTC Tamper Timestamp EXTI
  * @{
  */

/* TAMPER TIMESTAMP EXTI */
/* --------------------- */

/**
  * @brief  Enable interrupt on the RTC Tamper and Timestamp associated Exti line of core 1.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_IT()        (EXTI->IMR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Enable interrupt on the RTC Tamper and Timestamp associated Exti line of core 2.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_ENABLE_IT()      (EXTI->C2IMR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable interrupt on the RTC Tamper and Timestamp associated Exti line of core 1.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_IT()       (EXTI->IMR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Disable interrupt on the RTC Tamper and Timestamp associated Exti line of core 2.
  * @retval None
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_DISABLE_IT()     (EXTI->C2IMR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable event on the RTC Tamper and Timestamp associated Exti line of core 1.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_EVENT()    (EXTI->EMR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Enable event on the RTC Tamper and Timestamp associated Exti line of core 2.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_ENABLE_EVENT()  (EXTI->C2EMR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable event on the RTC Tamper and Timestamp associated Exti line of core 1.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_EVENT()   (EXTI->EMR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Disable event on the RTC Tamper and Timestamp associated Exti line of core 2.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_DISABLE_EVENT() (EXTI->C2EMR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/*-----------------*/
/**
  * @brief  Enable falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_FALLING_EDGE()   (EXTI->FTSR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_FALLING_EDGE()  (EXTI->FTSR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))

/**
  * @brief  Enable rising edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_EDGE()    (EXTI->RTSR1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief  Disable rising edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_EDGE()   (EXTI->RTSR1 &= ~(RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT))


/**
  * @brief  Enable rising & falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_FALLING_EDGE()  do { \
                                                                        __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_RISING_EDGE();  \
                                                                        __HAL_RTC_TAMPER_TIMESTAMP_EXTI_ENABLE_FALLING_EDGE(); \
                                                                      } while(0U)

/**
  * @brief  Disable rising & falling edge trigger on the RTC Tamper and Timestamp associated Exti line.
  * This parameter can be:
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_FALLING_EDGE()  do { \
                                                                        __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_RISING_EDGE();  \
                                                                        __HAL_RTC_TAMPER_TIMESTAMP_EXTI_DISABLE_FALLING_EDGE(); \
                                                                       } while(0U)

/**
  * @brief Check whether the RTC Tamper and Timestamp associated Exti line interrupt flag is set or not of core 1.
  * @retval Line Status.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_GET_FLAG()         (EXTI->PR1 & RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Check whether the RTC Tamper and Timestamp associated Exti line interrupt flag is set or not of core 2.
  * @retval Line Status.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_GET_FLAG()       (EXTI->PR2 & RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Clear the RTC Tamper and Timestamp associated Exti line flag of core 1.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_CLEAR_FLAG()       (EXTI->PR1 = RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Clear the RTC Tamper and Timestamp associated Exti line flag of core 2.
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTIC2_CLEAR_FLAG()     (EXTI->PR1 = RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)

/**
  * @brief Generate a Software interrupt on the RTC Tamper and Timestamp associated Exti line
  * @retval None.
  */
#define __HAL_RTC_TAMPER_TIMESTAMP_EXTI_GENERATE_SWIT()    (EXTI->SWIER1 |= RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT)



/**
  * @}
  */

/* ------------------------------Calibration----------------------------------*/
/** @defgroup RTCEx_Calibration RTC Calibration
  * @{
  */

/**
  * @brief  Enable the RTC calibration output.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CALIBRATION_OUTPUT_ENABLE(__HANDLE__)               ((__HANDLE__)->Instance->CR |= (RTC_CR_COE))

/**
  * @brief  Disable the calibration output.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CALIBRATION_OUTPUT_DISABLE(__HANDLE__)              ((__HANDLE__)->Instance->CR &= ~(RTC_CR_COE))


/**
  * @brief  Enable the clock reference detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CLOCKREF_DETECTION_ENABLE(__HANDLE__)               ((__HANDLE__)->Instance->CR |= (RTC_CR_REFCKON))

/**
  * @brief  Disable the clock reference detection.
  * @param __HANDLE__ specifies the RTC handle.
  * @retval None
  */
#define __HAL_RTC_CLOCKREF_DETECTION_DISABLE(__HANDLE__)              ((__HANDLE__)->Instance->CR &= ~(RTC_CR_REFCKON))


/**
  * @brief  Get the selected RTC shift operation's flag status.
  * @param __HANDLE__ specifies the RTC handle.
  * @param __FLAG__ specifies the RTC shift operation Flag is pending or not.
  *          This parameter can be:
  *             @arg RTC_FLAG_SHPF
  * @retval None
  */
#define __HAL_RTC_SHIFT_GET_FLAG(__HANDLE__, __FLAG__)                (((((__HANDLE__)->Instance->ISR) & (__FLAG__)) != 0U)? 1U : 0U)
/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup RTCEx_Exported_Functions RTCEx Exported Functions
  * @{
  */

/* RTC TimeStamp and Tamper functions *****************************************/
/** @defgroup RTCEx_Exported_Functions_Group1 Extended RTC TimeStamp and Tamper functions
 * @{
 */

HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin);
HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp_IT(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin);
HAL_StatusTypeDef HAL_RTCEx_DeactivateTimeStamp(RTC_HandleTypeDef *hrtc);
#if defined(RTC_INTERNALTS_SUPPORT)
HAL_StatusTypeDef HAL_RTCEx_SetInternalTimeStamp(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_DeactivateInternalTimeStamp(RTC_HandleTypeDef *hrtc);
#endif
HAL_StatusTypeDef HAL_RTCEx_GetTimeStamp(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTimeStamp, RTC_DateTypeDef *sTimeStampDate, uint32_t Format);

HAL_StatusTypeDef HAL_RTCEx_SetTamper(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper);
HAL_StatusTypeDef HAL_RTCEx_SetTamper_IT(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper);
HAL_StatusTypeDef HAL_RTCEx_DeactivateTamper(RTC_HandleTypeDef *hrtc, uint32_t Tamper);
void              HAL_RTCEx_TamperTimeStampIRQHandler(RTC_HandleTypeDef *hrtc);

#if defined(RTC_TAMPER1_SUPPORT)
void              HAL_RTCEx_Tamper1EventCallback(RTC_HandleTypeDef *hrtc);
#endif /* RTC_TAMPER1_SUPPORT */
void              HAL_RTCEx_Tamper2EventCallback(RTC_HandleTypeDef *hrtc);
#if defined(RTC_TAMPER3_SUPPORT)
void              HAL_RTCEx_Tamper3EventCallback(RTC_HandleTypeDef *hrtc);
#endif /* RTC_TAMPER3_SUPPORT */
void              HAL_RTCEx_TimeStampEventCallback(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_PollForTimeStampEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
#if defined(RTC_TAMPER1_SUPPORT)
HAL_StatusTypeDef HAL_RTCEx_PollForTamper1Event(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
#endif /* RTC_TAMPER1_SUPPORT */
HAL_StatusTypeDef HAL_RTCEx_PollForTamper2Event(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
#if defined(RTC_TAMPER3_SUPPORT)
HAL_StatusTypeDef HAL_RTCEx_PollForTamper3Event(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
#endif /* RTC_TAMPER3_SUPPORT */
/**
  * @}
  */

/* RTC Wake-up functions ******************************************************/
/** @defgroup RTCEx_Exported_Functions_Group2 Extended RTC Wake-up functions
 * @{
 */

HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock);
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock);
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *hrtc);
uint32_t          HAL_RTCEx_GetWakeUpTimer(RTC_HandleTypeDef *hrtc);
void              HAL_RTCEx_WakeUpTimerIRQHandler(RTC_HandleTypeDef *hrtc);
void              HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_PollForWakeUpTimerEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout);
/**
  * @}
  */

/* Extended Control functions ************************************************/
/** @defgroup RTCEx_Exported_Functions_Group3 Extended Peripheral Control functions
 * @{
 */

void HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister, uint32_t Data);
uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister);

HAL_StatusTypeDef HAL_RTCEx_SetSmoothCalib(RTC_HandleTypeDef *hrtc, uint32_t SmoothCalibPeriod, uint32_t SmoothCalibPlusPulses, uint32_t SmoothCalibMinusPulsesValue);
HAL_StatusTypeDef HAL_RTCEx_SetSynchroShift(RTC_HandleTypeDef *hrtc, uint32_t ShiftAdd1S, uint32_t ShiftSubFS);
HAL_StatusTypeDef HAL_RTCEx_SetCalibrationOutPut(RTC_HandleTypeDef *hrtc, uint32_t CalibOutput);
HAL_StatusTypeDef HAL_RTCEx_DeactivateCalibrationOutPut(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_SetRefClock(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_DeactivateRefClock(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_EnableBypassShadow(RTC_HandleTypeDef *hrtc);
HAL_StatusTypeDef HAL_RTCEx_DisableBypassShadow(RTC_HandleTypeDef *hrtc);
/**
  * @}
  */

/* Extended RTC features functions *******************************************/
/** @defgroup RTCEx_Exported_Functions_Group4 Extended features functions
 * @{
 */
void              HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc);
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
/** @defgroup RTCEx_Private_Constants RTCEx Private Constants
  * @{
  */

/* Masks Definition */

#if defined(RTC_TAMPER1_SUPPORT) && defined(RTC_TAMPER3_SUPPORT)

#define RTC_FLAGS_MASK          ((uint32_t) (RTC_FLAG_RECALPF | RTC_FLAG_TAMP3F | RTC_FLAG_TAMP2F | \
                                             RTC_FLAG_TAMP1F| RTC_FLAG_TSOVF | RTC_FLAG_TSF       | \
                                             RTC_FLAG_WUTF | RTC_FLAG_ALRBF | RTC_FLAG_ALRAF      | \
                                             RTC_FLAG_INITF | RTC_FLAG_RSF                        | \
                                             RTC_FLAG_INITS | RTC_FLAG_SHPF | RTC_FLAG_WUTWF      | \
                                             RTC_FLAG_ALRBWF | RTC_FLAG_ALRAWF))

#define RTC_TAMPCR_TAMPXE     ((uint32_t) (RTC_TAMPCR_TAMP3E | RTC_TAMPCR_TAMP2E | RTC_TAMPCR_TAMP1E))
#define RTC_TAMPCR_TAMPXIE    ((uint32_t) (RTC_TAMPER1_INTERRUPT | RTC_TAMPER2_INTERRUPT | \
                                           RTC_TAMPER3_INTERRUPT | RTC_ALL_TAMPER_INTERRUPT))

#elif defined(RTC_TAMPER1_SUPPORT)

#define RTC_FLAGS_MASK          ((uint32_t) (RTC_FLAG_RECALPF | RTC_FLAG_TAMP2F | RTC_FLAG_TAMP1F| \
                                             RTC_FLAG_TSOVF | RTC_FLAG_TSF | RTC_FLAG_WUTF       | \
                                             RTC_FLAG_ALRBF | RTC_FLAG_ALRAF                     | \
                                             RTC_FLAG_INITF | RTC_FLAG_RSF | RTC_FLAG_INITS      | \
                                             RTC_FLAG_SHPF | RTC_FLAG_WUTWF |RTC_FLAG_ALRBWF     | \
                                             RTC_FLAG_ALRAWF))

#define RTC_TAMPCR_TAMPXE     ((uint32_t) (RTC_TAMPCR_TAMP2E | RTC_TAMPCR_TAMP1E))
#define RTC_TAMPCR_TAMPXIE    ((uint32_t) (RTC_TAMPER1_INTERRUPT | RTC_TAMPER2_INTERRUPT | \
                                           RTC_ALL_TAMPER_INTERRUPT))

#elif defined(RTC_TAMPER3_SUPPORT)

#define RTC_FLAGS_MASK          ((uint32_t) (RTC_FLAG_RECALPF | RTC_FLAG_TAMP3F | RTC_FLAG_TAMP2F | \
                                             RTC_FLAG_TSOVF | RTC_FLAG_TSF                        | \
                                             RTC_FLAG_WUTF | RTC_FLAG_ALRBF | RTC_FLAG_ALRAF      | \
                                             RTC_FLAG_INITF | RTC_FLAG_RSF                        | \
                                             RTC_FLAG_INITS | RTC_FLAG_SHPF | RTC_FLAG_WUTWF      | \
                                             RTC_FLAG_ALRBWF | RTC_FLAG_ALRAWF))

#define RTC_TAMPCR_TAMPXE     ((uint32_t) (RTC_TAMPCR_TAMP3E | RTC_TAMPCR_TAMP2E))
#define RTC_TAMPCR_TAMPXIE    ((uint32_t) (RTC_TAMPER2_INTERRUPT | \
                                           RTC_TAMPER3_INTERRUPT | RTC_ALL_TAMPER_INTERRUPT))
#endif

#define RTC_EXTI_LINE_TAMPER_TIMESTAMP_EVENT  (EXTI_IMR1_IM18)    /*!< External interrupt line 18 Connected to the RTC Tamper and Time Stamp events */
#define RTC_EXTI_LINE_WAKEUPTIMER_EVENT       (EXTI_IMR1_IM19)    /*!< External interrupt line 19 Connected to the RTC Wakeup event */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup RTCEx_Private_Macros RTCEx Private Macros
  * @{
  */

/** @defgroup RTCEx_IS_RTC_Definitions Private macros to check input parameters
  * @{
  */

#define IS_RTC_OUTPUT(OUTPUT) (((OUTPUT) == RTC_OUTPUT_DISABLE) || \
                               ((OUTPUT) == RTC_OUTPUT_ALARMA)  || \
                               ((OUTPUT) == RTC_OUTPUT_ALARMB)  || \
                               ((OUTPUT) == RTC_OUTPUT_WAKEUP))

#define IS_RTC_BKP(BKP)                   ((BKP) < (uint32_t) RTC_BKP_NUMBER)

#define IS_TIMESTAMP_EDGE(EDGE) (((EDGE) == RTC_TIMESTAMPEDGE_RISING) || \
                                 ((EDGE) == RTC_TIMESTAMPEDGE_FALLING))

#define  IS_RTC_TAMPER(TAMPER)  ((((TAMPER) & ((uint32_t)(0xFFFFFFFFU ^ RTC_TAMPCR_TAMPXE))) == 0x00U) && ((TAMPER) != 0U))

#define IS_RTC_TAMPER_INTERRUPT(INTERRUPT) ((((INTERRUPT) & (uint32_t)(0xFFFFFFFFU ^ RTC_TAMPCR_TAMPXIE)) == 0x00U) && ((INTERRUPT) != 0U))

#define IS_RTC_TIMESTAMP_PIN(PIN)  (((PIN) == RTC_TIMESTAMPPIN_DEFAULT))

#define IS_RTC_TAMPER_TRIGGER(TRIGGER) (((TRIGGER) == RTC_TAMPERTRIGGER_RISINGEDGE) || \
                                        ((TRIGGER) == RTC_TAMPERTRIGGER_FALLINGEDGE) || \
                                        ((TRIGGER) == RTC_TAMPERTRIGGER_LOWLEVEL) || \
                                        ((TRIGGER) == RTC_TAMPERTRIGGER_HIGHLEVEL))

#define IS_RTC_TAMPER_ERASE_MODE(MODE)             (((MODE) == RTC_TAMPER_ERASE_BACKUP_ENABLE) || \
                                                    ((MODE) == RTC_TAMPER_ERASE_BACKUP_DISABLE))

#define IS_RTC_TAMPER_MASKFLAG_STATE(STATE)        (((STATE) == RTC_TAMPERMASK_FLAG_ENABLE) || \
                                                    ((STATE) == RTC_TAMPERMASK_FLAG_DISABLE))

#define IS_RTC_TAMPER_FILTER(FILTER)  (((FILTER) == RTC_TAMPERFILTER_DISABLE) || \
                                       ((FILTER) == RTC_TAMPERFILTER_2SAMPLE) || \
                                       ((FILTER) == RTC_TAMPERFILTER_4SAMPLE) || \
                                       ((FILTER) == RTC_TAMPERFILTER_8SAMPLE))

#define IS_RTC_TAMPER_SAMPLING_FREQ(FREQ) (((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV32768)|| \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV16384)|| \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV8192) || \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV4096) || \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV2048) || \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV1024) || \
                                           ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV512)  || \
                                          ((FREQ) == RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV256))

#define IS_RTC_TAMPER_PRECHARGE_DURATION(DURATION) (((DURATION) == RTC_TAMPERPRECHARGEDURATION_1RTCCLK) || \
                                                    ((DURATION) == RTC_TAMPERPRECHARGEDURATION_2RTCCLK) || \
                                                    ((DURATION) == RTC_TAMPERPRECHARGEDURATION_4RTCCLK) || \
                                                   ((DURATION) == RTC_TAMPERPRECHARGEDURATION_8RTCCLK))

#define IS_RTC_TAMPER_TIMESTAMPONTAMPER_DETECTION(DETECTION) (((DETECTION) == RTC_TIMESTAMPONTAMPERDETECTION_ENABLE) || \
                                                              ((DETECTION) == RTC_TIMESTAMPONTAMPERDETECTION_DISABLE))

#define IS_RTC_TAMPER_PULLUP_STATE(STATE) (((STATE) == RTC_TAMPER_PULLUP_ENABLE) || \
                                           ((STATE) == RTC_TAMPER_PULLUP_DISABLE))

#define IS_RTC_WAKEUP_CLOCK(CLOCK) (((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV16)   || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV8)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV4)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_RTCCLK_DIV2)    || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_CK_SPRE_16BITS) || \
                                    ((CLOCK) == RTC_WAKEUPCLOCK_CK_SPRE_17BITS))

#define IS_RTC_WAKEUP_COUNTER(COUNTER)  ((COUNTER) <= RTC_WUTR_WUT)

#define IS_RTC_SMOOTH_CALIB_PERIOD(PERIOD) (((PERIOD) == RTC_SMOOTHCALIB_PERIOD_32SEC) || \
                                            ((PERIOD) == RTC_SMOOTHCALIB_PERIOD_16SEC) || \
                                            ((PERIOD) == RTC_SMOOTHCALIB_PERIOD_8SEC))

#define IS_RTC_SMOOTH_CALIB_PLUS(PLUS) (((PLUS) == RTC_SMOOTHCALIB_PLUSPULSES_SET) || \
                                        ((PLUS) == RTC_SMOOTHCALIB_PLUSPULSES_RESET))


/** @defgroup RTCEx_Smooth_calib_Minus_pulses_Definitions RTCEx Smooth calib Minus pulses Definitions
  * @{
  */
#define  IS_RTC_SMOOTH_CALIB_MINUS(VALUE) ((VALUE) <= RTC_CALR_CALM)
/**
  * @}
  */


#define IS_RTC_SHIFT_ADD1S(SEL) (((SEL) == RTC_SHIFTADD1S_RESET) || \
                                 ((SEL) == RTC_SHIFTADD1S_SET))



/** @defgroup RTCEx_Substract_Fraction_Of_Second_Value RTCEx Substract Fraction Of Second Value
  * @{
  */
#define IS_RTC_SHIFT_SUBFS(FS) ((FS) <= RTC_SHIFTR_SUBFS)
/**
  * @}
  */
#define IS_RTC_CALIB_OUTPUT(OUTPUT)  (((OUTPUT) == RTC_CALIBOUTPUT_512HZ) || \
                                      ((OUTPUT) == RTC_CALIBOUTPUT_1HZ))

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

#endif /* STM32WBxx_HAL_RTC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
