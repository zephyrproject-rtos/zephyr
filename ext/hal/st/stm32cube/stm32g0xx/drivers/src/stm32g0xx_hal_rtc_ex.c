/**
  ******************************************************************************
  * @file    stm32g0xx_hal_rtc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RTC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Real Time Clock (RTC) Extended peripheral:
  *           + RTC Time Stamp functions
  *           + RTC Tamper functions
  *           + RTC Wake-up functions
  *           + Extended Control functions
  *           + Extended RTC features functions
  *
  @verbatim
  ==============================================================================
                  ##### How to use this driver #####
  ==============================================================================
  [..]
    (+) Enable the RTC domain access.
    (+) Configure the RTC Prescaler (Asynchronous and Synchronous) and RTC hour
        format using the HAL_RTC_Init() function.

  *** RTC Wakeup configuration ***
  ================================
  [..]
    (+) To configure the RTC Wakeup Clock source and Counter use the HAL_RTCEx_SetWakeUpTimer()
        function. You can also configure the RTC Wakeup timer with interrupt mode
        using the HAL_RTCEx_SetWakeUpTimer_IT() function.
    (+) To read the RTC WakeUp Counter register, use the HAL_RTCEx_GetWakeUpTimer()
        function.

  *** Outputs configuration ***
  =============================
  [..]  The RTC has 2 different outputs:
    (+) RTC_ALARM: this output is used to manage the RTC Alarm A, Alarm B
        and WaKeUp signals.
        To output the selected RTC signal, use the HAL_RTC_Init() function.
    (+) RTC_CALIB: this output is 512Hz signal or 1Hz.
        To enable the RTC_CALIB, use the HAL_RTCEx_SetCalibrationOutPut() function.
    (+) Two pins can be used as RTC_ALARM or RTC_CALIB (PC13, PB2) managed on
        the RTC_OR register.
    (+) When the RTC_CALIB or RTC_ALARM output is selected, the RTC_OUT pin is
        automatically configured in output alternate function.

  *** Smooth digital Calibration configuration ***
  ================================================
  [..]
    (+) Configure the RTC Original Digital Calibration Value and the corresponding
        calibration cycle period (32s,16s and 8s) using the HAL_RTCEx_SetSmoothCalib()
        function.

  *** TimeStamp configuration ***
  ===============================
  [..]
    (+) Enable the RTC TimeStamp using the HAL_RTCEx_SetTimeStamp() function.
        You can also configure the RTC TimeStamp with interrupt mode using the
        HAL_RTCEx_SetTimeStamp_IT() function.
    (+) To read the RTC TimeStamp Time and Date register, use the HAL_RTCEx_GetTimeStamp()
        function.

  *** Internal TimeStamp configuration ***
  ===============================
  [..]
    (+) Enable the RTC internal TimeStamp using the HAL_RTCEx_SetInternalTimeStamp() function.
        User has to check internal timestamp occurrence using __HAL_RTC_INTERNAL_TIMESTAMP_GET_FLAG.
    (+) To read the RTC TimeStamp Time and Date register, use the HAL_RTCEx_GetTimeStamp()
        function.

   *** Tamper configuration ***
   ============================
   [..]
     (+) Enable the RTC Tamper and configure the Tamper filter count, trigger Edge
         or Level according to the Tamper filter (if equal to 0 Edge else Level)
         value, sampling frequency, NoErase, MaskFlag,  precharge or discharge and
         Pull-UP using the HAL_RTCEx_SetTamper() function. You can configure RTC Tamper
         with interrupt mode using HAL_RTCEx_SetTamper_IT() function.
     (+) The default configuration of the Tamper erases the backup registers. To avoid
         erase, enable the NoErase field on the RTC_TAMPCR register.
     (+) With new RTC tamper configuration, you have to call HAL_RTC_Init() in order to
         perform TAMP base address offset calculation.
     (+) If you do not intend to have tamper using RTC clock, you can bypass its initialization
         by setting ClockEnable inti field to RTC_CLOCK_DISABLE.
     (+) Enable Internal tamper using HAL_RTCEx_SetInternalTamper. IT mode can be chosen using
         setting Interrupt field.

   *** Backup Data Registers configuration ***
   ===========================================
   [..]
     (+) To write to the RTC Backup Data registers, use the HAL_RTCEx_BKUPWrite()
         function.
     (+) To read the RTC Backup Data registers, use the HAL_RTCEx_BKUPRead()
         function.
     (+) Befoer callingthose fucntion you have to call HAL_RTC_Init() in order to
         perform TAMP base address offset calculation.

   @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup RTCEx
  * @brief RTC Extended HAL module driver
  * @{
  */

#ifdef HAL_RTC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @addtogroup RTCEx_Exported_Functions
  * @{
  */


/** @addtogroup RTCEx_Exported_Functions_Group1
 *  @brief   RTC TimeStamp and Tamper functions
  *
@verbatim
 ===============================================================================
                 ##### RTC TimeStamp and Tamper functions #####
 ===============================================================================

 [..] This section provides functions allowing to configure TimeStamp feature

@endverbatim
  * @{
  */

/**
  * @brief  Set TimeStamp.
  * @note   This API must be called before enabling the TimeStamp feature.
  * @param  hrtc RTC handle
  * @param  TimeStampEdge Specifies the pin edge on which the TimeStamp is
  *         activated.
  *          This parameter can be one of the following values:
  *             @arg RTC_TIMESTAMPEDGE_RISING: the Time stamp event occurs on the
  *                                        rising edge of the related pin.
  *             @arg RTC_TIMESTAMPEDGE_FALLING: the Time stamp event occurs on the
  *                                         falling edge of the related pin.
  * @param  RTC_TimeStampPin specifies the RTC TimeStamp Pin.
  *          This parameter can be one of the following values:
  *             @arg RTC_TIMESTAMPPIN_DEFAULT: PC13 is selected as RTC TimeStamp Pin.
  *               The RTC TimeStamp Pin is per default PC13, but for reasons of
  *               compatibility, this parameter is required.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin)
{
  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_TIMESTAMP_EDGE(TimeStampEdge));
  assert_param(IS_RTC_TIMESTAMP_PIN(RTC_TimeStampPin));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Get the RTC_CR register and clear the bits to be configured */
  tmpreg = (uint32_t)(hrtc->Instance->CR & (uint32_t)~(RTC_CR_TSEDGE | RTC_CR_TSE));

  tmpreg|= TimeStampEdge;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Configure the Time Stamp TSEDGE and Enable bits */
  hrtc->Instance->CR = (uint32_t)tmpreg;

  __HAL_RTC_TIMESTAMP_ENABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Set TimeStamp with Interrupt.
  * @note   This API must be called before enabling the TimeStamp feature.
  * @param  hrtc RTC handle
  * @param  TimeStampEdge Specifies the pin edge on which the TimeStamp is
  *         activated.
  *          This parameter can be one of the following values:
  *             @arg RTC_TIMESTAMPEDGE_RISING: the Time stamp event occurs on the
  *                                        rising edge of the related pin.
  *             @arg RTC_TIMESTAMPEDGE_FALLING: the Time stamp event occurs on the
  *                                         falling edge of the related pin.
  * @param  RTC_TimeStampPin Specifies the RTC TimeStamp Pin.
  *          This parameter can be one of the following values:
  *             @arg RTC_TIMESTAMPPIN_DEFAULT: PC13 is selected as RTC TimeStamp Pin.
  *               The RTC TimeStamp Pin is per default PC13, but for reasons of
  *               compatibility, this parameter is required.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetTimeStamp_IT(RTC_HandleTypeDef *hrtc, uint32_t TimeStampEdge, uint32_t RTC_TimeStampPin)
{
  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_TIMESTAMP_EDGE(TimeStampEdge));
  assert_param(IS_RTC_TIMESTAMP_PIN(RTC_TimeStampPin));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Get the RTC_CR register and clear the bits to be configured */
  tmpreg = (uint32_t)(hrtc->Instance->CR & (uint32_t)~(RTC_CR_TSEDGE | RTC_CR_TSE));

  tmpreg |= TimeStampEdge;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Configure the Time Stamp TSEDGE and Enable bits */
  hrtc->Instance->CR = (uint32_t)tmpreg;

  __HAL_RTC_TIMESTAMP_ENABLE(hrtc);

  /* Enable IT timestamp */
  __HAL_RTC_TIMESTAMP_ENABLE_IT(hrtc,RTC_IT_TS);

  /* RTC timestamp Interrupt Configuration: EXTI configuration */
  __HAL_RTC_TIMESTAMP_EXTI_ENABLE_IT();

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Deactivate TimeStamp.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateTimeStamp(RTC_HandleTypeDef *hrtc)
{
  uint32_t tmpreg;

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* In case of interrupt mode is used, the interrupt source must disabled */
  __HAL_RTC_TIMESTAMP_DISABLE_IT(hrtc, RTC_IT_TS);

  /* Get the RTC_CR register and clear the bits to be configured */
  tmpreg = (uint32_t)(hrtc->Instance->CR & (uint32_t)~(RTC_CR_TSEDGE | RTC_CR_TSE));

  /* Configure the Time Stamp TSEDGE and Enable bits */
  hrtc->Instance->CR = (uint32_t)tmpreg;

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Set Internal TimeStamp.
  * @note   This API must be called before enabling the internal TimeStamp feature.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetInternalTimeStamp(RTC_HandleTypeDef *hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Configure the internal Time Stamp Enable bits */
  __HAL_RTC_INTERNAL_TIMESTAMP_ENABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Deactivate Internal TimeStamp.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateInternalTimeStamp(RTC_HandleTypeDef *hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Configure the internal Time Stamp Enable bits */
  __HAL_RTC_INTERNAL_TIMESTAMP_DISABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Get the RTC TimeStamp value.
  * @param  hrtc RTC handle
  * @param  sTimeStamp Pointer to Time structure
  * @param  sTimeStampDate Pointer to Date structure
  * @param  Format specifies the format of the entered parameters.
  *          This parameter can be one of the following values:
  *             @arg RTC_FORMAT_BIN: Binary data format
  *             @arg RTC_FORMAT_BCD: BCD data format
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_GetTimeStamp(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef* sTimeStamp, RTC_DateTypeDef* sTimeStampDate, uint32_t Format)
{
  uint32_t tmptime, tmpdate;

  /* Check the parameters */
  assert_param(IS_RTC_FORMAT(Format));

  /* Get the TimeStamp time and date registers values */
  tmptime = (uint32_t)(hrtc->Instance->TSTR & RTC_TR_RESERVED_MASK);
  tmpdate = (uint32_t)(hrtc->Instance->TSDR & RTC_DR_RESERVED_MASK);

  /* Fill the Time structure fields with the read parameters */
  sTimeStamp->Hours = (uint8_t)((tmptime & (RTC_TR_HT | RTC_TR_HU)) >> 16U);
  sTimeStamp->Minutes = (uint8_t)((tmptime & (RTC_TR_MNT | RTC_TR_MNU)) >> 8U);
  sTimeStamp->Seconds = (uint8_t)(tmptime & (RTC_TR_ST | RTC_TR_SU));
  sTimeStamp->TimeFormat = (uint8_t)((tmptime & (RTC_TR_PM)) >> 16U);
  sTimeStamp->SubSeconds = (uint32_t) hrtc->Instance->TSSSR;

  /* Fill the Date structure fields with the read parameters */
  sTimeStampDate->Year = 0U;
  sTimeStampDate->Month = (uint8_t)((tmpdate & (RTC_DR_MT | RTC_DR_MU)) >> 8U);
  sTimeStampDate->Date = (uint8_t)(tmpdate & (RTC_DR_DT | RTC_DR_DU));
  sTimeStampDate->WeekDay = (uint8_t)((tmpdate & (RTC_DR_WDU)) >> 13U);

  /* Check the input parameters format */
  if(Format == RTC_FORMAT_BIN)
  {
    /* Convert the TimeStamp structure parameters to Binary format */
    sTimeStamp->Hours = (uint8_t)RTC_Bcd2ToByte(sTimeStamp->Hours);
    sTimeStamp->Minutes = (uint8_t)RTC_Bcd2ToByte(sTimeStamp->Minutes);
    sTimeStamp->Seconds = (uint8_t)RTC_Bcd2ToByte(sTimeStamp->Seconds);

    /* Convert the DateTimeStamp structure parameters to Binary format */
    sTimeStampDate->Month = (uint8_t)RTC_Bcd2ToByte(sTimeStampDate->Month);
    sTimeStampDate->Date = (uint8_t)RTC_Bcd2ToByte(sTimeStampDate->Date);
    sTimeStampDate->WeekDay = (uint8_t)RTC_Bcd2ToByte(sTimeStampDate->WeekDay);
  }

  /* Clear the TIMESTAMP Flags */
  __HAL_RTC_INTERNAL_TIMESTAMP_CLEAR_FLAG(hrtc, RTC_FLAG_ITSF);
  __HAL_RTC_TIMESTAMP_CLEAR_FLAG(hrtc, RTC_FLAG_TSF);

  return HAL_OK;
}

/**
  * @brief  TimeStamp callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_TimeStampEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_TimeStampEventCallback could be implemented in the user file
  */
}


/**
  * @brief  Handle TimeStamp interrupt request.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTCEx_TimeStampIRQHandler(RTC_HandleTypeDef *hrtc)
{
  /* Get the TimeStamp interrupt source enable status */
  if(__HAL_RTC_TIMESTAMP_GET_IT_SOURCE(hrtc, RTC_IT_TS) != 0U)
  {
    /* Get the pending status of the TIMESTAMP Interrupt */
    if(__HAL_RTC_TIMESTAMP_GET_FLAG(hrtc, RTC_FLAG_TSF) != 0U)
    {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call TimeStampEvent registered Callback */
      hrtc->TimeStampEventCallback(hrtc);
#else
      /* TIMESTAMP callback */
      HAL_RTCEx_TimeStampEventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */

      /* Clear the TIMESTAMP interrupt pending bit */
      __HAL_RTC_TIMESTAMP_CLEAR_FLAG(hrtc, RTC_FLAG_TSF);
    }
  }

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;
}


/**
  * @brief  Handle TimeStamp polling request.
  * @param  hrtc RTC handle
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_PollForTimeStampEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout)
{
  uint32_t tickstart = HAL_GetTick();

  while(__HAL_RTC_TIMESTAMP_GET_FLAG(hrtc, RTC_FLAG_TSF) == 0U)
  {
    if(__HAL_RTC_TIMESTAMP_GET_FLAG(hrtc, RTC_FLAG_TSOVF) != 0U)
    {
      /* Clear the TIMESTAMP OverRun Flag */
      __HAL_RTC_TIMESTAMP_CLEAR_FLAG(hrtc, RTC_FLAG_TSOVF);

      /* Change TIMESTAMP state */
      hrtc->State = HAL_RTC_STATE_ERROR;

      return HAL_ERROR;
    }

    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart ) > Timeout)||(Timeout == 0U))
      {
        hrtc->State = HAL_RTC_STATE_TIMEOUT;
        return HAL_TIMEOUT;
      }
    }
  }

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  return HAL_OK;
}

/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group2
  * @brief    RTC Wake-up functions
  *
@verbatim
 ===============================================================================
                        ##### RTC Wake-up functions #####
 ===============================================================================

 [..] This section provides functions allowing to configure Wake-up feature

@endverbatim
  * @{
  */

/**
  * @brief  Set wake up timer.
  * @param  hrtc RTC handle
  * @param  WakeUpCounter Wake up counter
  * @param  WakeUpClock Wake up clock
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RTC_WAKEUP_CLOCK(WakeUpClock));
  assert_param(IS_RTC_WAKEUP_COUNTER(WakeUpCounter));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /*Check RTC WUTWF flag is reset only when wake up timer enabled*/
  if((hrtc->Instance->CR & RTC_CR_WUTE) != 0U)
  {
    tickstart = HAL_GetTick();

   /* Wait till RTC WUTWF flag is reset and if Time out is reached exit */
   while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTWF) == 1U)
   {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
      {
       /* Enable the write protection for RTC registers */
       __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

       hrtc->State = HAL_RTC_STATE_TIMEOUT;

       /* Process Unlocked */
       __HAL_UNLOCK(hrtc);

       return HAL_TIMEOUT;
      }
    }
  }

  __HAL_RTC_WAKEUPTIMER_DISABLE(hrtc);

  tickstart = HAL_GetTick();

  /* Wait till RTC WUTWF flag is set and if Time out is reached exit */
  while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTWF) == 0U)
  {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
    {
      /* Enable the write protection for RTC registers */
      __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

      hrtc->State = HAL_RTC_STATE_TIMEOUT;

      /* Process Unlocked */
      __HAL_UNLOCK(hrtc);

      return HAL_TIMEOUT;
    }
  }

  /* Clear the Wakeup Timer clock source bits in CR register */
  hrtc->Instance->CR &= (uint32_t)~RTC_CR_WUCKSEL;

  /* Configure the clock source */
  hrtc->Instance->CR |= (uint32_t)WakeUpClock;

  /* Configure the Wakeup Timer counter */
  hrtc->Instance->WUTR = (uint32_t)WakeUpCounter;

   /* Enable the Wakeup Timer */
  __HAL_RTC_WAKEUPTIMER_ENABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Set wake up timer with interrupt.
  * @param  hrtc RTC handle
  * @param  WakeUpCounter Wake up counter
  * @param  WakeUpClock Wake up clock
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RTC_WAKEUP_CLOCK(WakeUpClock));
  assert_param(IS_RTC_WAKEUP_COUNTER(WakeUpCounter));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /*Check RTC WUTWF flag is reset only when wake up timer enabled*/
  if((hrtc->Instance->CR & RTC_CR_WUTE) != 0U)
  {
    tickstart = HAL_GetTick();

   /* Wait till RTC WUTWF flag is reset and if Time out is reached exit */
   while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTWF) == 1U)
   {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
      {
       /* Enable the write protection for RTC registers */
       __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

       hrtc->State = HAL_RTC_STATE_TIMEOUT;

       /* Process Unlocked */
       __HAL_UNLOCK(hrtc);

       return HAL_TIMEOUT;
      }
    }
  }
  /* Disable the Wake-Up timer */
  __HAL_RTC_WAKEUPTIMER_DISABLE(hrtc);

  /* Clear flag Wake-Up */
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc, RTC_FLAG_WUTF);

  tickstart = HAL_GetTick();

  /* Wait till RTC WUTWF flag is set and if Time out is reached exit */
  while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTWF) == 0U)
  {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
    {
      /* Enable the write protection for RTC registers */
      __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

      hrtc->State = HAL_RTC_STATE_TIMEOUT;

      /* Process Unlocked */
      __HAL_UNLOCK(hrtc);

      return HAL_TIMEOUT;
    }
  }

  /* Configure the Wakeup Timer counter */
  hrtc->Instance->WUTR = (uint32_t)WakeUpCounter;

  /* Clear the Wakeup Timer clock source bits in CR register */
  hrtc->Instance->CR &= (uint32_t)~RTC_CR_WUCKSEL;

  /* Configure the clock source */
  hrtc->Instance->CR |= (uint32_t)WakeUpClock;

  /* RTC WakeUpTimer Interrupt Configuration: EXTI configuration */
  __HAL_RTC_WAKEUPTIMER_EXTI_ENABLE_IT();

  /* Configure the Interrupt in the RTC_CR register */
  __HAL_RTC_WAKEUPTIMER_ENABLE_IT(hrtc,RTC_IT_WUT);

  /* Enable the Wakeup Timer */
  __HAL_RTC_WAKEUPTIMER_ENABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Deactivate wake up timer counter.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *hrtc)
{
  uint32_t tickstart;

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Disable the Wakeup Timer */
  __HAL_RTC_WAKEUPTIMER_DISABLE(hrtc);

  /* In case of interrupt mode is used, the interrupt source must disabled */
  __HAL_RTC_WAKEUPTIMER_DISABLE_IT(hrtc,RTC_IT_WUT);

  tickstart = HAL_GetTick();
  /* Wait till RTC WUTWF flag is set and if Time out is reached exit */
  while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTWF) == 0U)
  {
    if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
    {
      /* Enable the write protection for RTC registers */
      __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

      hrtc->State = HAL_RTC_STATE_TIMEOUT;

      /* Process Unlocked */
      __HAL_UNLOCK(hrtc);

      return HAL_TIMEOUT;
    }
  }

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Get wake up timer counter.
  * @param  hrtc RTC handle
  * @retval Counter value
  */
uint32_t HAL_RTCEx_GetWakeUpTimer(RTC_HandleTypeDef *hrtc)
{
  /* Get the counter value */
  return ((uint32_t)(hrtc->Instance->WUTR & RTC_WUTR_WUT));
}

/**
  * @brief  Handle Wake Up Timer interrupt request.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTCEx_WakeUpTimerIRQHandler(RTC_HandleTypeDef *hrtc)
{
    /* Get the pending status of the WAKEUPTIMER Interrupt */
    if(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTF) != 0U)
    {
      /* Clear the WAKEUPTIMER interrupt pending bit */
      __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc, RTC_FLAG_WUTF);

#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call WakeUpTimerEvent registered Callback */
      hrtc->WakeUpTimerEventCallback(hrtc);
#else
      /* WAKEUPTIMER callback */
      HAL_RTCEx_WakeUpTimerEventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
    }

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;
}

/**
  * @brief  Wake Up Timer callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_WakeUpTimerEventCallback could be implemented in the user file
   */
}


/**
  * @brief  Handle Wake Up Timer Polling.
  * @param  hrtc RTC handle
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_PollForWakeUpTimerEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout)
{
  uint32_t tickstart = HAL_GetTick();

  while(__HAL_RTC_WAKEUPTIMER_GET_FLAG(hrtc, RTC_FLAG_WUTF) == 0U)
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart ) > Timeout)||(Timeout == 0U))
      {
        hrtc->State = HAL_RTC_STATE_TIMEOUT;
        return HAL_TIMEOUT;
      }
    }
  }

  /* Clear the WAKEUPTIMER Flag */
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc, RTC_FLAG_WUTF);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  return HAL_OK;
}

/**
  * @}
  */


/** @addtogroup RTCEx_Exported_Functions_Group3
  * @brief    Extended Peripheral Control functions
  *
@verbatim
 ===============================================================================
              ##### Extended Peripheral Control functions #####
 ===============================================================================
    [..]
    This subsection provides functions allowing to
      (+) Write a data in a specified RTC Backup data register
      (+) Read a data in a specified RTC Backup data register
      (+) Set the Coarse calibration parameters.
      (+) Deactivate the Coarse calibration parameters
      (+) Set the Smooth calibration parameters.
      (+) Configure the Synchronization Shift Control Settings.
      (+) Configure the Calibration Pinout (RTC_CALIB) Selection (1Hz or 512Hz).
      (+) Deactivate the Calibration Pinout (RTC_CALIB) Selection (1Hz or 512Hz).
      (+) Enable the RTC reference clock detection.
      (+) Disable the RTC reference clock detection.
      (+) Enable the Bypass Shadow feature.
      (+) Disable the Bypass Shadow feature.

@endverbatim
  * @{
  */



/**
  * @brief  Set the Smooth calibration parameters.
  * @note   To deactivate the smooth calibration, the field SmoothCalibPlusPulses
  *         must be equal to SMOOTHCALIB_PLUSPULSES_RESET and the field
  *         SmoothCalibMinusPulsesValue must be equal to 0.
  * @param  hrtc RTC handle
  * @param  SmoothCalibPeriod Select the Smooth Calibration Period.
  *          This parameter can be can be one of the following values :
  *             @arg RTC_SMOOTHCALIB_PERIOD_32SEC: The smooth calibration period is 32s.
  *             @arg RTC_SMOOTHCALIB_PERIOD_16SEC: The smooth calibration period is 16s.
  *             @arg RTC_SMOOTHCALIB_PERIOD_8SEC: The smooth calibration period is 8s.
  * @param  SmoothCalibPlusPulses Select to Set or reset the CALP bit.
  *          This parameter can be one of the following values:
  *             @arg RTC_SMOOTHCALIB_PLUSPULSES_SET: Add one RTCCLK pulse every 2*11 pulses.
  *             @arg RTC_SMOOTHCALIB_PLUSPULSES_RESET: No RTCCLK pulses are added.
  * @param  SmoothCalibMinusPulsesValue Select the value of CALM[8:0] bits.
  *          This parameter can be one any value from 0 to 0x000001FF.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetSmoothCalib(RTC_HandleTypeDef* hrtc, uint32_t SmoothCalibPeriod, uint32_t SmoothCalibPlusPulses, uint32_t SmoothCalibMinusPulsesValue)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RTC_SMOOTH_CALIB_PERIOD(SmoothCalibPeriod));
  assert_param(IS_RTC_SMOOTH_CALIB_PLUS(SmoothCalibPlusPulses));
  assert_param(IS_RTC_SMOOTH_CALIB_MINUS(SmoothCalibMinusPulsesValue));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* check if a calibration is pending*/
  if((hrtc->Instance->ICSR & RTC_ICSR_RECALPF) != 0U)
  {
    tickstart = HAL_GetTick();

    /* check if a calibration is pending*/
    while((hrtc->Instance->ICSR & RTC_ICSR_RECALPF) != 0U)
    {
      if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
      {
        /* Enable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

        /* Change RTC state */
        hrtc->State = HAL_RTC_STATE_TIMEOUT;

        /* Process Unlocked */
        __HAL_UNLOCK(hrtc);

        return HAL_TIMEOUT;
      }
    }
  }

  /* Configure the Smooth calibration settings */
  hrtc->Instance->CALR = (uint32_t)((uint32_t)SmoothCalibPeriod | (uint32_t)SmoothCalibPlusPulses | (uint32_t)SmoothCalibMinusPulsesValue);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Configure the Synchronization Shift Control Settings.
  * @note   When REFCKON is set, firmware must not write to Shift control register.
  * @param  hrtc RTC handle
  * @param  ShiftAdd1S Select to add or not 1 second to the time calendar.
  *          This parameter can be one of the following values:
  *             @arg RTC_SHIFTADD1S_SET: Add one second to the clock calendar.
  *             @arg RTC_SHIFTADD1S_RESET: No effect.
  * @param  ShiftSubFS Select the number of Second Fractions to substitute.
  *          This parameter can be one any value from 0 to 0x7FFF.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetSynchroShift(RTC_HandleTypeDef* hrtc, uint32_t ShiftAdd1S, uint32_t ShiftSubFS)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RTC_SHIFT_ADD1S(ShiftAdd1S));
  assert_param(IS_RTC_SHIFT_SUBFS(ShiftSubFS));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

    tickstart = HAL_GetTick();

    /* Wait until the shift is completed*/
    while((hrtc->Instance->ICSR & RTC_ICSR_SHPF) != 0U)
    {
      if((HAL_GetTick() - tickstart ) > RTC_TIMEOUT_VALUE)
      {
        /* Enable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

        hrtc->State = HAL_RTC_STATE_TIMEOUT;

        /* Process Unlocked */
        __HAL_UNLOCK(hrtc);

        return HAL_TIMEOUT;
      }
    }

    /* Check if the reference clock detection is disabled */
    if((hrtc->Instance->CR & RTC_CR_REFCKON) == 0U)
    {
      /* Configure the Shift settings */
      hrtc->Instance->SHIFTR = (uint32_t)(uint32_t)(ShiftSubFS) | (uint32_t)(ShiftAdd1S);

      /* If  RTC_CR_BYPSHAD bit = 0, wait for synchro else this check is not needed */
      if((hrtc->Instance->CR & RTC_CR_BYPSHAD) == 0U)
      {
        if(HAL_RTC_WaitForSynchro(hrtc) != HAL_OK)
        {
          /* Enable the write protection for RTC registers */
          __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

          hrtc->State = HAL_RTC_STATE_ERROR;

          /* Process Unlocked */
          __HAL_UNLOCK(hrtc);

          return HAL_ERROR;
        }
      }
    }
    else
    {
      /* Enable the write protection for RTC registers */
      __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

      /* Change RTC state */
      hrtc->State = HAL_RTC_STATE_ERROR;

      /* Process Unlocked */
      __HAL_UNLOCK(hrtc);

      return HAL_ERROR;
    }

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Configure the Calibration Pinout (RTC_CALIB) Selection (1Hz or 512Hz).
  * @param  hrtc RTC handle
  * @param  CalibOutput Select the Calibration output Selection .
  *          This parameter can be one of the following values:
  *             @arg RTC_CALIBOUTPUT_512HZ: A signal has a regular waveform at 512Hz.
  *             @arg RTC_CALIBOUTPUT_1HZ: A signal has a regular waveform at 1Hz.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetCalibrationOutPut(RTC_HandleTypeDef* hrtc, uint32_t CalibOutput)
{
  /* Check the parameters */
  assert_param(IS_RTC_CALIB_OUTPUT(CalibOutput));

  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Clear flags before config */
  hrtc->Instance->CR &= (uint32_t)~RTC_CR_COSEL;

  /* Configure the RTC_CR register */
  hrtc->Instance->CR |= (uint32_t)CalibOutput;

  __HAL_RTC_CALIBRATION_OUTPUT_ENABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Deactivate the Calibration Pinout (RTC_CALIB) Selection (1Hz or 512Hz).
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateCalibrationOutPut(RTC_HandleTypeDef* hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  __HAL_RTC_CALIBRATION_OUTPUT_DISABLE(hrtc);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Enable the RTC reference clock detection.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetRefClock(RTC_HandleTypeDef* hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Set Initialization mode */
  if(RTC_EnterInitMode(hrtc) != HAL_OK)
  {
    /* Enable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

    /* Set RTC state*/
    hrtc->State = HAL_RTC_STATE_ERROR;

    /* Process Unlocked */
    __HAL_UNLOCK(hrtc);

    return HAL_ERROR;
  }
  else
  {
    __HAL_RTC_CLOCKREF_DETECTION_ENABLE(hrtc);

    /* Exit Initialization mode */
    hrtc->Instance->ICSR &= (uint32_t)~RTC_ICSR_INIT;
  }

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

   /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Disable the RTC reference clock detection.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateRefClock(RTC_HandleTypeDef* hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Set Initialization mode */
  if(RTC_EnterInitMode(hrtc) != HAL_OK)
  {
    /* Enable the write protection for RTC registers */
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

    /* Set RTC state*/
    hrtc->State = HAL_RTC_STATE_ERROR;

    /* Process Unlocked */
    __HAL_UNLOCK(hrtc);

    return HAL_ERROR;
  }
  else
  {
    __HAL_RTC_CLOCKREF_DETECTION_DISABLE(hrtc);

    /* Exit Initialization mode */
    hrtc->Instance->ICSR &= (uint32_t)~RTC_ICSR_INIT;
  }

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Enable the Bypass Shadow feature.
  * @note   When the Bypass Shadow is enabled the calendar value are taken
  *         directly from the Calendar counter.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_EnableBypassShadow(RTC_HandleTypeDef* hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Set the BYPSHAD bit */
  hrtc->Instance->CR |= (uint8_t)RTC_CR_BYPSHAD;

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @brief  Disable the Bypass Shadow feature.
  * @note   When the Bypass Shadow is enabled the calendar value are taken
  *         directly from the Calendar counter.
  * @param  hrtc RTC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DisableBypassShadow(RTC_HandleTypeDef* hrtc)
{
  /* Process Locked */
  __HAL_LOCK(hrtc);

  hrtc->State = HAL_RTC_STATE_BUSY;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  /* Reset the BYPSHAD bit */
  hrtc->Instance->CR &= ((uint8_t)~RTC_CR_BYPSHAD);

  /* Enable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hrtc);

  return HAL_OK;
}

/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group4
  * @brief    Extended features functions
  *
@verbatim
 ===============================================================================
                 ##### Extended features functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) RTC Alarm B callback
      (+) RTC Poll for Alarm B request

@endverbatim
  * @{
  */

/**
  * @brief  Alarm B callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_AlarmBEventCallback could be implemented in the user file
   */
}

/**
  * @brief  Handle Alarm B Polling request.
  * @param  hrtc RTC handle
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_PollForAlarmBEvent(RTC_HandleTypeDef *hrtc, uint32_t Timeout)
{
  uint32_t tickstart = HAL_GetTick();

  while(__HAL_RTC_ALARM_GET_FLAG(hrtc, RTC_FLAG_ALRBF) == 0U)
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart ) > Timeout)||(Timeout == 0U))
      {
        hrtc->State = HAL_RTC_STATE_TIMEOUT;
        return HAL_TIMEOUT;
      }
    }
  }

  /* Clear the Alarm Flag */
  __HAL_RTC_ALARM_CLEAR_FLAG(hrtc, RTC_FLAG_ALRBF);

  /* Change RTC state */
  hrtc->State = HAL_RTC_STATE_READY;

  return HAL_OK;
}

/**
  * @}
  */

/** @addtogroup RTCEx_Exported_Functions_Group5
  * @brief      Extended RTC Tamper functions
  *
@verbatim
  ==============================================================================
                         ##### Tamper functions #####
  ==============================================================================
  [..]
   (+) Before calling any tamper or internal tamper function, you have to call first
       HAL_RTC_Init() function.
   (+) In that ine you can select to output tamper event on RTC pin.
  [..]
   (+) Enable the Tamper and configure the Tamper filter count, trigger Edge
       or Level according to the Tamper filter (if equal to 0 Edge else Level)
       value, sampling frequency, NoErase, MaskFlag, precharge or discharge and
       Pull-UP, timestamp using the HAL_RTCEx_SetTamper() function.
       You can configure Tamper with interrupt mode using HAL_RTCEx_SetTamper_IT() function.
   (+) The default configuration of the Tamper erases the backup registers. To avoid
       erase, enable the NoErase field on the TAMP_TAMPCR register.
  [..]
   (+) Enable Internal Tamper and configure it with interrupt, timestamp using
       the HAL_RTCEx_SetInternalTamper() function.

@endverbatim
  * @{
  */


/**
  * @brief  Set Tamper
  * @param  hrtc RTC handle
  * @param  sTamper Pointer to Tamper Structure.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetTamper(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper)
{
  uint32_t tmpreg;
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  /* Check the parameters */
  assert_param(IS_RTC_TAMPER(sTamper->Tamper));
  assert_param(IS_RTC_TAMPER_TRIGGER(sTamper->Trigger));
  assert_param(IS_RTC_TAMPER_ERASE_MODE(sTamper->NoErase));
  assert_param(IS_RTC_TAMPER_MASKFLAG_STATE(sTamper->MaskFlag));
  assert_param(IS_RTC_TAMPER_FILTER(sTamper->Filter));
  assert_param(IS_RTC_TAMPER_SAMPLING_FREQ(sTamper->SamplingFrequency));
  assert_param(IS_RTC_TAMPER_PRECHARGE_DURATION(sTamper->PrechargeDuration));
  assert_param(IS_RTC_TAMPER_PULLUP_STATE(sTamper->TamperPullUp));
  assert_param(IS_RTC_TAMPER_TIMESTAMPONTAMPER_DETECTION(sTamper->TimeStampOnTamperDetection));

  /* Configuration register 2 */
  tmpreg = tamp->CR2;
  tmpreg &= ~((sTamper->Tamper << 24) | (sTamper->Tamper << 16) | (sTamper->Tamper));

  if(sTamper->Trigger != RTC_TAMPERTRIGGER_RISINGEDGE)
  {
    tmpreg |= (sTamper->Tamper << 24);
  }

  if(sTamper->MaskFlag != RTC_TAMPERMASK_FLAG_DISABLE)
  {
    tmpreg |= (sTamper->Tamper << 16);
  }

  if(sTamper->NoErase != RTC_TAMPER_ERASE_BACKUP_ENABLE)
  {
    tmpreg |= sTamper->Tamper;
  }
  tamp->CR2 = tmpreg;

  /* Filter control register */
  tamp->FLTCR = (sTamper->Filter) | (sTamper->SamplingFrequency) | \
                 (sTamper->PrechargeDuration) | (sTamper->TamperPullUp);

  /* timestamp on tamper */
  if((hrtc->Instance->CR & RTC_CR_TAMPTS) != (sTamper->TimeStampOnTamperDetection))
  {
    __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
    tmpreg = (hrtc->Instance->CR & ~RTC_CR_TAMPTS);
    hrtc->Instance->CR = (tmpreg | (sTamper->TimeStampOnTamperDetection));
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  }

  /* Control register 1 */
  tamp->CR1 |= (sTamper->Tamper);

  return HAL_OK;
}


/**
  * @brief  Set Tamper in IT mode
  * @param  hrtc RTC handle
  * @param  sTamper Pointer to Tamper Structure.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetTamper_IT(RTC_HandleTypeDef *hrtc, RTC_TamperTypeDef* sTamper)
{
  uint32_t tmpreg;
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  /* Check the parameters */
  assert_param(IS_RTC_TAMPER(sTamper->Tamper));
  assert_param(IS_RTC_TAMPER_TRIGGER(sTamper->Trigger));
  assert_param(IS_RTC_TAMPER_ERASE_MODE(sTamper->NoErase));
  assert_param(IS_RTC_TAMPER_MASKFLAG_STATE(sTamper->MaskFlag));
  assert_param(IS_RTC_TAMPER_FILTER(sTamper->Filter));
  assert_param(IS_RTC_TAMPER_SAMPLING_FREQ(sTamper->SamplingFrequency));
  assert_param(IS_RTC_TAMPER_PRECHARGE_DURATION(sTamper->PrechargeDuration));
  assert_param(IS_RTC_TAMPER_PULLUP_STATE(sTamper->TamperPullUp));
  assert_param(IS_RTC_TAMPER_TIMESTAMPONTAMPER_DETECTION(sTamper->TimeStampOnTamperDetection));

  /* Configuration register 2 */
  tmpreg = tamp->CR2;
  tmpreg &= ~((sTamper->Tamper << 24) | (sTamper->Tamper << 16) | (sTamper->Tamper));

  if(sTamper->Trigger != RTC_TAMPERTRIGGER_RISINGEDGE)
  {
    tmpreg |= (sTamper->Tamper << 24);
  }

  if(sTamper->MaskFlag != RTC_TAMPERMASK_FLAG_DISABLE)
  {
    tmpreg |= (sTamper->Tamper << 16);
  }

  if(sTamper->NoErase != RTC_TAMPER_ERASE_BACKUP_ENABLE)
  {
    tmpreg |= sTamper->Tamper;
  }
  tamp->CR2 = tmpreg;

  /* Filter control register */
  tamp->FLTCR = (sTamper->Filter) | (sTamper->SamplingFrequency) | \
                 (sTamper->PrechargeDuration) | (sTamper->TamperPullUp);

  /* timestamp on tamper */
  if((hrtc->Instance->CR & RTC_CR_TAMPTS) != (sTamper->TimeStampOnTamperDetection))
  {
    __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
    tmpreg = (hrtc->Instance->CR & ~RTC_CR_TAMPTS);
    hrtc->Instance->CR = (tmpreg | (sTamper->TimeStampOnTamperDetection));
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  }

  /* RTC Tamper Interrupt Configuration: EXTI configuration */
  __HAL_RTC_TAMPER_EXTI_ENABLE_IT();

  /* Interrupt enable register */
  tamp->IER |= sTamper->Tamper;

  /* Control register 1 */
  tamp->CR1 |= (sTamper->Tamper);

  return HAL_OK;
}



/**
  * @brief  Deactivate Tamper.
  * @param  hrtc RTC handle
  * @param  Tamper Selected tamper pin.
  *         This parameter can be a combination of the following values:
  *         @arg RTC_TAMPER_1
  *         @arg RTC_TAMPER_2
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateTamper(RTC_HandleTypeDef *hrtc, uint32_t Tamper)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  assert_param(IS_RTC_TAMPER(Tamper));

  /* Disable the selected Tamper pin */
  tamp->CR1 &= ~Tamper;

  /* Clear tamper mask/noerase/trigger configuration */
  tamp->CR2 &= ~((Tamper << 24) | (Tamper << 16) | Tamper);

  /* Clear tamper interrupt mode configuration */
  tamp->IER &= ~Tamper;

  return HAL_OK;
}


/**
  * @brief  Tamper event polling.
  * @param  hrtc RTC handle
  * @param  Tamper Selected tamper pin.
  *         This parameter can be a combination of the following values:
  *         @arg RTC_TAMPER_1
  *         @arg RTC_TAMPER_2
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_PollForTamperEvent(RTC_HandleTypeDef *hrtc, uint32_t Tamper, uint32_t Timeout)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  assert_param(IS_RTC_TAMPER(Tamper));

  uint32_t tickstart = HAL_GetTick();

  /* Get the status of the Interrupt */
  while((tamp->SR & Tamper) != Tamper)
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart ) > Timeout)||(Timeout == 0U))
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Clear the Tamper Flag */
  tamp->SCR = Tamper;

  return HAL_OK;
}


/**
  * @brief  Set Internal Tamper in interrupt mode
  * @param  hrtc RTC handle
  * @param  sIntTamper Pointer to Internal Tamper Structure.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetInternalTamper(RTC_HandleTypeDef *hrtc, RTC_InternalTamperTypeDef* sIntTamper)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_RTC_INTERNAL_TAMPER(sIntTamper->IntTamper));
  assert_param(IS_RTC_TAMPER_TIMESTAMPONTAMPER_DETECTION(sIntTamper->TimeStampOnTamperDetection));

  /* timestamp on internal tamper */
  if((hrtc->Instance->CR & RTC_CR_TAMPTS) != (sIntTamper->TimeStampOnTamperDetection))
  {
    __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
    tmpreg = (hrtc->Instance->CR & ~RTC_CR_TAMPTS);
    hrtc->Instance->CR = (tmpreg | (sIntTamper->TimeStampOnTamperDetection));
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  }

  /* Control register 1 */
  tamp->CR1 |= (sIntTamper->IntTamper);

  return HAL_OK;
}


/**
  * @brief  Set Internal Tamper
  * @param  hrtc RTC handle
  * @param  sIntTamper Pointer to Internal Tamper Structure.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_SetInternalTamper_IT(RTC_HandleTypeDef *hrtc, RTC_InternalTamperTypeDef* sIntTamper)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  uint32_t tmpreg;

  /* Check the parameters */
  assert_param(IS_RTC_INTERNAL_TAMPER(sIntTamper->IntTamper));
  assert_param(IS_RTC_TAMPER_TIMESTAMPONTAMPER_DETECTION(sIntTamper->TimeStampOnTamperDetection));

  /* timestamp on internal tamper */
  if((hrtc->Instance->CR & RTC_CR_TAMPTS) != (sIntTamper->TimeStampOnTamperDetection))
  {
    __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
    tmpreg = (hrtc->Instance->CR & ~RTC_CR_TAMPTS);
    hrtc->Instance->CR = (tmpreg | (sIntTamper->TimeStampOnTamperDetection));
    __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  }

  /* RTC Tamper Interrupt Configuration: EXTI configuration */
  __HAL_RTC_TAMPER_EXTI_ENABLE_IT();

  /* Interrupt enable register */
  tamp->IER |= sIntTamper->IntTamper;

  /* Control register 1 */
  tamp->CR1 |= (sIntTamper->IntTamper);

  return HAL_OK;
}

/**
  * @brief  Deactivate Internal Tamper.
  * @param  hrtc RTC handle
  * @param  IntTamper Selected internal tamper event.
  *          This parameter can be any combination of existing internal tampers.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_DeactivateInternalTamper(RTC_HandleTypeDef *hrtc, uint32_t IntTamper)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  assert_param(IS_RTC_INTERNAL_TAMPER(IntTamper));

  /* Disable the selected Tamper pin */
  tamp->CR1 &= ~IntTamper;

  /* Clear internal tamper interrupt mode configuration */
  tamp->IER &= ~IntTamper;

  return HAL_OK;
}


/**
  * @brief  Internal Tamper event polling.
  * @param  hrtc RTC handle
  * @param  IntTamper selected tamper.
  *          This parameter can be any combination of existing internal tampers.
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RTCEx_PollForInternalTamperEvent(RTC_HandleTypeDef *hrtc, uint32_t IntTamper, uint32_t Timeout)
{
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  assert_param(IS_RTC_INTERNAL_TAMPER(IntTamper));

  uint32_t tickstart = HAL_GetTick();

  /* Get the status of the Interrupt */
  while((tamp->SR & IntTamper) != IntTamper)
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart ) > Timeout)||(Timeout == 0U))
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Clear the Tamper Flag */
  tamp->SCR = IntTamper;

  return HAL_OK;
}


/**
  * @brief  Handle Tamper interrupt request.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTCEx_TamperIRQHandler(RTC_HandleTypeDef *hrtc)
{
  uint32_t tmp;
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  /* Get interrupt status */
  tmp = tamp->MISR;

  /* Immediatly clear flags */
  tamp->SCR = tmp;

  /* Check Tamper1 status */
  if((tmp & RTC_TAMPER_1) == RTC_TAMPER_1)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Tamper 1 Event registered Callback */
      hrtc->Tamper1EventCallback(hrtc);
#else
    /* Tamper1 callback */
    HAL_RTCEx_Tamper1EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }

  /* Check Tamper2 status */
  if((tmp & RTC_TAMPER_2) == RTC_TAMPER_2)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Tamper 2 Event registered Callback */
      hrtc->Tamper2EventCallback(hrtc);
#else
    /* Tamper2 callback */
    HAL_RTCEx_Tamper2EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }


  /* Check Internal Tamper3 status */
  if((tmp & RTC_INT_TAMPER_3) == RTC_INT_TAMPER_3)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Internal Tamper 3 Event registered Callback */
      hrtc->InternalTamper3EventCallback(hrtc);
#else
    /* Internal Tamper3 callback */
    HAL_RTCEx_InternalTamper3EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }

  /* Check Internal Tamper4 status */
  if((tmp & RTC_INT_TAMPER_4) == RTC_INT_TAMPER_4)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Internal Tamper 4 Event registered Callback */
      hrtc->InternalTamper4EventCallback(hrtc);
#else
    /* Internal Tamper4 callback */
    HAL_RTCEx_InternalTamper4EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }

  /* Check Internal Tamper5 status */
  if((tmp & RTC_INT_TAMPER_5) == RTC_INT_TAMPER_5)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Internal Tamper 5 Event registered Callback */
      hrtc->InternalTamper5EventCallback(hrtc);
#else
    /* Internal Tamper5 callback */
    HAL_RTCEx_InternalTamper5EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }

  /* Check Internal Tamper6 status */
  if((tmp & RTC_INT_TAMPER_6) == RTC_INT_TAMPER_6)
  {
#if (USE_HAL_RTC_REGISTER_CALLBACKS == 1)
      /* Call Internal Tamper 6 Event registered Callback */
      hrtc->InternalTamper6EventCallback(hrtc);
#else
    /* Internal Tamper6 callback */
    HAL_RTCEx_InternalTamper6EventCallback(hrtc);
#endif /* USE_HAL_RTC_REGISTER_CALLBACKS */
  }

}


/**
  * @brief  Tamper 1 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_Tamper1EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_Tamper1EventCallback could be implemented in the user file
   */
}


/**
  * @brief  Tamper 2 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_Tamper2EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_Tamper2EventCallback could be implemented in the user file
   */
}




/**
  * @brief  Internal Tamper 3 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_InternalTamper3EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_InternalTamper3EventCallback could be implemented in the user file
   */
}


/**
  * @brief  Internal Tamper 4 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_InternalTamper4EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_InternalTamper4EventCallback could be implemented in the user file
   */
}


/**
  * @brief  Internal Tamper 5 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_InternalTamper5EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_InternalTamper5EventCallback could be implemented in the user file
   */
}


/**
  * @brief  Internal Tamper 6 callback.
  * @param  hrtc RTC handle
  * @retval None
  */
__weak void HAL_RTCEx_InternalTamper6EventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_RTCEx_InternalTamper6EventCallback could be implemented in the user file
   */
}



/**
  * @}
  */


/** @addtogroup RTCEx_Exported_Functions_Group6
  * @brief      Extended RTC Backup register functions
  *
@verbatim
  ===============================================================================
             ##### Extended RTC Backup register functions #####
  ===============================================================================
  [..]
   (+) Before calling any tamper or internal tamper function, you have to call first
       HAL_RTC_Init() function.
   (+) In that ine you can select to output tamper event on RTC pin.
  [..]
   This subsection provides functions allowing to
   (+) Write a data in a specified RTC Backup data register
   (+) Read a data in a specified RTC Backup data register
@endverbatim
  * @{
  */


/**
  * @brief  Write a data in a specified RTC Backup data register.
  * @param  hrtc RTC handle
  * @param  BackupRegister: RTC Backup data Register number.
  *          This parameter can be: RTC_BKP_DRx where x can be from 0 to 4
  *          specify the register.
  * @param  Data Data to be written in the specified Backup data register.
  * @retval None
  */
void HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister, uint32_t Data)
{
  uint32_t tmp;
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  /* Check the parameters */
  assert_param(IS_RTC_BKP(BackupRegister));

  tmp = (uint32_t)&(tamp->BKP0R);
  tmp += (BackupRegister * 4U);

  /* Write the specified register */
  *(__IO uint32_t *)tmp = (uint32_t)Data;
}


/**
  * @brief  Reads data from the specified RTC Backup data Register.
  * @param  hrtc RTC handle
  * @param  BackupRegister  RTC Backup data Register number.
  *         This parameter can be: RTC_BKP_DRx where x can be from 0 to 4
  *         specify the register.
  * @retval Read value
  */
uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef *hrtc, uint32_t BackupRegister)
{
  uint32_t tmp ;
  /* Process TAMP instance pointer */
  TAMP_TypeDef *tamp = (TAMP_TypeDef *)((uint32_t)hrtc->Instance + hrtc->TampOffset);

  /* Check the parameters */
  assert_param(IS_RTC_BKP(BackupRegister));

  tmp = (uint32_t)&(tamp->BKP0R);
  tmp += (BackupRegister * 4U);

  /* Read the specified register */
  return (*(__IO uint32_t *)tmp);
}


/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_RTC_MODULE_ENABLED */
/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

