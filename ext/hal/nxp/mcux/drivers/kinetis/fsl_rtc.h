/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_RTC_H_
#define _FSL_RTC_H_

#include "fsl_common.h"

/*!
 * @addtogroup rtc
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_RTC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
/*@}*/

/*! @brief List of RTC interrupts */
typedef enum _rtc_interrupt_enable
{
    kRTC_TimeInvalidInterruptEnable = (1U << 0U),  /*!< Time invalid interrupt.*/
    kRTC_TimeOverflowInterruptEnable = (1U << 1U), /*!< Time overflow interrupt.*/
    kRTC_AlarmInterruptEnable = (1U << 2U),        /*!< Alarm interrupt.*/
    kRTC_SecondsInterruptEnable = (1U << 3U),      /*!< Seconds interrupt.*/
#if defined(FSL_FEATURE_RTC_HAS_MONOTONIC) && (FSL_FEATURE_RTC_HAS_MONOTONIC)
    kRTC_MonotonicOverflowInterruptEnable = (1U << 4U), /*!< Monotonic Overflow Interrupt Enable */
#endif /* FSL_FEATURE_RTC_HAS_MONOTONIC */
#if (defined(FSL_FEATURE_RTC_HAS_TIR) && FSL_FEATURE_RTC_HAS_TIR)
    kRTC_TestModeInterruptEnable = (1U << 5U),      /* test mode interrupt */
    kRTC_FlashSecurityInterruptEnable = (1U << 6U), /* flash security interrupt */
#if (defined(FSL_FEATURE_RTC_HAS_TIR_TPIE) && FSL_FEATURE_RTC_HAS_TIR_TPIE)
    kRTC_TamperPinInterruptEnable = (1U << 7U),     /* Tamper pin interrupt */
#endif                                                     /* FSL_FEATURE_RTC_HAS_TIR_TPIE */
#if (defined(FSL_FEATURE_RTC_HAS_TIR_SIE) && FSL_FEATURE_RTC_HAS_TIR_SIE)
    kRTC_SecurityModuleInterruptEnable = (1U << 8U), /* security module interrupt */
#endif                                                     /* FSL_FEATURE_RTC_HAS_TIR_SIE */
#if (defined(FSL_FEATURE_RTC_HAS_TIR_LCIE) && FSL_FEATURE_RTC_HAS_TIR_LCIE)
    kRTC_LossOfClockInterruptEnable = (1U << 9U), /* loss of clock interrupt */
#endif /* FSL_FEATURE_RTC_HAS_TIR_LCIE */
#endif /* FSL_FEATURE_RTC_HAS_TIR */
} rtc_interrupt_enable_t;

/*! @brief List of RTC flags */
typedef enum _rtc_status_flags
{
    kRTC_TimeInvalidFlag = (1U << 0U),            /*!< Time invalid flag */
    kRTC_TimeOverflowFlag = (1U << 1U),           /*!< Time overflow flag */
    kRTC_AlarmFlag = (1U << 2U),                   /*!< Alarm flag*/
#if defined(FSL_FEATURE_RTC_HAS_MONOTONIC) && (FSL_FEATURE_RTC_HAS_MONOTONIC)
    kRTC_MonotonicOverflowFlag = (1U << 3U),       /*!< Monotonic Overflow Flag */
#endif /* FSL_FEATURE_RTC_HAS_MONOTONIC */
#if (defined(FSL_FEATURE_RTC_HAS_SR_TIDF) && FSL_FEATURE_RTC_HAS_SR_TIDF)
    kRTC_TamperInterruptDetectFlag = (1U << 4U), /*!< Tamper interrupt detect flag */
#endif                                                 /* FSL_FEATURE_RTC_HAS_SR_TIDF */
#if (defined(FSL_FEATURE_RTC_HAS_TDR) && FSL_FEATURE_RTC_HAS_TDR)
    kRTC_TestModeFlag = (1U << 5U),      /* Test mode flag */
    kRTC_FlashSecurityFlag = (1U << 6U), /* Flash security flag */
#if (defined(FSL_FEATURE_RTC_HAS_TDR_TPF) && FSL_FEATURE_RTC_HAS_TDR_TPF)
    kRTC_TamperPinFlag = (1U << 7U),     /* Tamper pin flag */
#endif                                         /* FSL_FEATURE_RTC_HAS_TDR_TPF */
#if (defined(FSL_FEATURE_RTC_HAS_TDR_STF) && FSL_FEATURE_RTC_HAS_TDR_STF)
    kRTC_SecurityTamperFlag = (1U << 8U), /* Security tamper flag */
#endif                                          /* FSL_FEATURE_RTC_HAS_TDR_STF */
#if (defined(FSL_FEATURE_RTC_HAS_TDR_LCTF) && FSL_FEATURE_RTC_HAS_TDR_LCTF)
    kRTC_LossOfClockTamperFlag = (1U << 9U), /* Loss of clock flag */
#endif                                             /* FSL_FEATURE_RTC_HAS_TDR_LCTF */
#endif /* FSL_FEATURE_RTC_HAS_TDR */
} rtc_status_flags_t;

#if (defined(FSL_FEATURE_RTC_HAS_OSC_SCXP) && FSL_FEATURE_RTC_HAS_OSC_SCXP)

/*! @brief List of RTC Oscillator capacitor load settings */
typedef enum _rtc_osc_cap_load
{
    kRTC_Capacitor_2p = RTC_CR_SC2P_MASK,  /*!< 2 pF capacitor load */
    kRTC_Capacitor_4p = RTC_CR_SC4P_MASK,  /*!< 4 pF capacitor load */
    kRTC_Capacitor_8p = RTC_CR_SC8P_MASK,  /*!< 8 pF capacitor load */
    kRTC_Capacitor_16p = RTC_CR_SC16P_MASK /*!< 16 pF capacitor load */
} rtc_osc_cap_load_t;

#endif /* FSL_FEATURE_SCG_HAS_OSC_SCXP */

/*! @brief Structure is used to hold the date and time */
typedef struct _rtc_datetime
{
    uint16_t year;  /*!< Range from 1970 to 2099.*/
    uint8_t month;  /*!< Range from 1 to 12.*/
    uint8_t day;    /*!< Range from 1 to 31 (depending on month).*/
    uint8_t hour;   /*!< Range from 0 to 23.*/
    uint8_t minute; /*!< Range from 0 to 59.*/
    uint8_t second; /*!< Range from 0 to 59.*/
} rtc_datetime_t;

#if (defined(FSL_FEATURE_RTC_HAS_PCR) && FSL_FEATURE_RTC_HAS_PCR)

/*!
 * @brief RTC pin config structure
 */
typedef struct _rtc_pin_config
{
    bool inputLogic;       /*!< true: Tamper pin input data is logic one.
                                false: Tamper pin input data is logic zero. */
    bool pinActiveLow;     /*!< true: Tamper pin is active low.
                                false: Tamper pin is active high. */
    bool filterEnable;     /*!< true: Input filter is enabled on the tamper pin.
                                false: Input filter is disabled on the tamper pin. */
    bool pullSelectNegate; /*!< true: Tamper pin pull resistor direction will negate the tamper pin.
                                false: Tamper pin pull resistor direction will assert the tamper pin. */
    bool pullEnable;       /*!< true: Pull resistor is enabled on tamper pin.
                                false: Pull resistor is disabled on tamper pin. */
} rtc_pin_config_t;

#endif /* FSL_FEATURE_RTC_HAS_PCR */

/*!
 * @brief RTC config structure
 *
 * This structure holds the configuration settings for the RTC peripheral. To initialize this
 * structure to reasonable defaults, call the RTC_GetDefaultConfig() function and pass a
 * pointer to your config structure instance.
 *
 * The config struct can be made const so it resides in flash
 */
typedef struct _rtc_config
{
    bool wakeupSelect;             /*!< true: Wakeup pin outputs the 32 KHz clock;
                                        false:Wakeup pin used to wakeup the chip  */
    bool updateMode;               /*!< true: Registers can be written even when locked under certain
                                        conditions, false: No writes allowed when registers are locked */
    bool supervisorAccess;         /*!< true: Non-supervisor accesses are allowed;
                                        false: Non-supervisor accesses are not supported */
    uint32_t compensationInterval; /*!< Compensation interval that is written to the CIR field in RTC TCR Register */
    uint32_t compensationTime;     /*!< Compensation time that is written to the TCR field in RTC TCR Register */
} rtc_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Ungates the RTC clock and configures the peripheral for basic operation.
 *
 * This function issues a software reset if the timer invalid flag is set.
 *
 * @note This API should be called at the beginning of the application using the RTC driver.
 *
 * @param base   RTC peripheral base address
 * @param config Pointer to the user's RTC configuration structure.
 */
void RTC_Init(RTC_Type *base, const rtc_config_t *config);

/*!
 * @brief Stops the timer and gate the RTC clock.
 *
 * @param base RTC peripheral base address
 */
static inline void RTC_Deinit(RTC_Type *base)
{
    /* Stop the RTC timer */
    base->SR &= ~RTC_SR_TCE_MASK;

#if defined(RTC_CLOCKS)
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the module clock */
    CLOCK_DisableClock(kCLOCK_Rtc0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif /* RTC_CLOCKS */
}

/*!
 * @brief Fills in the RTC config struct with the default settings.
 *
 * The default values are as follows.
 * @code
 *    config->wakeupSelect = false;
 *    config->updateMode = false;
 *    config->supervisorAccess = false;
 *    config->compensationInterval = 0;
 *    config->compensationTime = 0;
 * @endcode
 * @param config Pointer to the user's RTC configuration structure.
 */
void RTC_GetDefaultConfig(rtc_config_t *config);

/*! @}*/

/*!
 * @name Current Time & Alarm
 * @{
 */

/*!
 * @brief Sets the RTC date and time according to the given time structure.
 *
 * The RTC counter must be stopped prior to calling this function because writes to the RTC
 * seconds register fail if the RTC counter is running.
 *
 * @param base     RTC peripheral base address
 * @param datetime Pointer to the structure where the date and time details are stored.
 *
 * @return kStatus_Success: Success in setting the time and starting the RTC
 *         kStatus_InvalidArgument: Error because the datetime format is incorrect
 */
status_t RTC_SetDatetime(RTC_Type *base, const rtc_datetime_t *datetime);

/*!
 * @brief Gets the RTC time and stores it in the given time structure.
 *
 * @param base     RTC peripheral base address
 * @param datetime Pointer to the structure where the date and time details are stored.
 */
void RTC_GetDatetime(RTC_Type *base, rtc_datetime_t *datetime);

/*!
 * @brief Sets the RTC alarm time.
 *
 * The function checks whether the specified alarm time is greater than the present
 * time. If not, the function does not set the alarm and returns an error.
 *
 * @param base      RTC peripheral base address
 * @param alarmTime Pointer to the structure where the alarm time is stored.
 *
 * @return kStatus_Success: success in setting the RTC alarm
 *         kStatus_InvalidArgument: Error because the alarm datetime format is incorrect
 *         kStatus_Fail: Error because the alarm time has already passed
 */
status_t RTC_SetAlarm(RTC_Type *base, const rtc_datetime_t *alarmTime);

/*!
 * @brief Returns the RTC alarm time.
 *
 * @param base     RTC peripheral base address
 * @param datetime Pointer to the structure where the alarm date and time details are stored.
 */
void RTC_GetAlarm(RTC_Type *base, rtc_datetime_t *datetime);

/*! @}*/

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enables the selected RTC interrupts.
 *
 * @param base RTC peripheral base address
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::rtc_interrupt_enable_t
 */
void RTC_EnableInterrupts(RTC_Type *base, uint32_t mask);

/*!
 * @brief Disables the selected RTC interrupts.
 *
 * @param base RTC peripheral base address
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::rtc_interrupt_enable_t
 */
void RTC_DisableInterrupts(RTC_Type *base, uint32_t mask);

/*!
 * @brief Gets the enabled RTC interrupts.
 *
 * @param base RTC peripheral base address
 *
 * @return The enabled interrupts. This is the logical OR of members of the
 *         enumeration ::rtc_interrupt_enable_t
 */
uint32_t RTC_GetEnabledInterrupts(RTC_Type *base);

/*! @}*/

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Gets the RTC status flags.
 *
 * @param base RTC peripheral base address
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::rtc_status_flags_t
 */
uint32_t RTC_GetStatusFlags(RTC_Type *base);

/*!
 * @brief  Clears the RTC status flags.
 *
 * @param base RTC peripheral base address
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::rtc_status_flags_t
 */
void RTC_ClearStatusFlags(RTC_Type *base, uint32_t mask);

/*! @}*/

/*!
 * @brief Set RTC clock source.
 * 
 * @param base RTC peripheral base address
 *
 * @note After setting this bit, wait the oscillator startup time before enabling
 *       the time counter to allow the 32.768 kHz clock time to stabilize.
 */
static inline void RTC_SetClockSource(RTC_Type *base)
{
    /* Enable the RTC 32KHz oscillator */
    base->CR |= RTC_CR_OSCE_MASK;
}

#if (defined(FSL_FEATURE_RTC_HAS_TTSR) && FSL_FEATURE_RTC_HAS_TTSR)

/*!
 * @brief Get the RTC tamper time seconds.
 *
 * @param base RTC peripheral base address
 */
static inline uint32_t RTC_GetTamperTimeSeconds(RTC_Type *base)
{
    return base->TTSR;
}

#endif /* FSL_FEATURE_RTC_HAS_TTSR */

/*!
 * @name Timer Start and Stop
 * @{
 */

/*!
 * @brief Starts the RTC time counter.
 *
 * After calling this function, the timer counter increments once a second provided SR[TOF] or
 * SR[TIF] are not set.
 *
 * @param base RTC peripheral base address
 */
static inline void RTC_StartTimer(RTC_Type *base)
{
    base->SR |= RTC_SR_TCE_MASK;
}

/*!
 * @brief Stops the RTC time counter.
 *
 * RTC's seconds register can be written to only when the timer is stopped.
 *
 * @param base RTC peripheral base address
 */
static inline void RTC_StopTimer(RTC_Type *base)
{
    base->SR &= ~RTC_SR_TCE_MASK;
}

/*! @}*/

#if (defined(FSL_FEATURE_RTC_HAS_OSC_SCXP) && FSL_FEATURE_RTC_HAS_OSC_SCXP)

/*!
 * @brief This function sets the specified capacitor configuration for the RTC oscillator.
 *
 * @param base    RTC peripheral base address
 * @param capLoad Oscillator loads to enable. This is a logical OR of members of the
 *                enumeration ::rtc_osc_cap_load_t
 */
static inline void RTC_SetOscCapLoad(RTC_Type *base, uint32_t capLoad)
{
    uint32_t reg = base->CR;

    reg &= ~(RTC_CR_SC2P_MASK | RTC_CR_SC4P_MASK | RTC_CR_SC8P_MASK | RTC_CR_SC16P_MASK);
    reg |= capLoad;

    base->CR = reg;
}

#endif /* FSL_FEATURE_SCG_HAS_OSC_SCXP */

/*!
 * @brief Performs a software reset on the RTC module.
 *
 * This resets all RTC registers except for the SWR bit and the RTC_WAR and RTC_RAR
 * registers. The SWR bit is cleared by software explicitly clearing it.
 *
 * @param base RTC peripheral base address
 */
static inline void RTC_Reset(RTC_Type *base)
{
    base->CR |= RTC_CR_SWR_MASK;
    base->CR &= ~RTC_CR_SWR_MASK;

    /* Set TSR register to 0x1 to avoid the timer invalid (TIF) bit being set in the SR register */
    base->TSR = 1U;
}

#if defined(FSL_FEATURE_RTC_HAS_MONOTONIC) && (FSL_FEATURE_RTC_HAS_MONOTONIC)

/*!
 * @name Monotonic counter functions
 * @{
 */

/*!
 * @brief Reads the values of the Monotonic Counter High and Monotonic Counter Low and returns
 *        them as a single value.
 *
 * @param base    RTC peripheral base address
 * @param counter Pointer to variable where the value is stored.
 */
void RTC_GetMonotonicCounter(RTC_Type *base, uint64_t *counter);

/*!
 * @brief Writes values Monotonic Counter High and Monotonic Counter Low by decomposing
 *        the given single value. The Monotonic Overflow Flag in RTC_SR is cleared due to the API.
 *
 * @param base    RTC peripheral base address
 * @param counter Counter value
 */
void RTC_SetMonotonicCounter(RTC_Type *base, uint64_t counter);

/*!
 * @brief Increments the Monotonic Counter by one.
 *
 * Increments the Monotonic Counter (registers RTC_MCLR and RTC_MCHR accordingly) by setting
 * the monotonic counter enable (MER[MCE]) and then writing to the RTC_MCLR register. A write to the
 * monotonic counter low that causes it to overflow also increments the monotonic counter high.
 *
 * @param base RTC peripheral base address
 *
 * @return kStatus_Success: success
 *         kStatus_Fail: error occurred, either time invalid or monotonic overflow flag was found
 */
status_t RTC_IncrementMonotonicCounter(RTC_Type *base);

/*! @}*/

#endif /* FSL_FEATURE_RTC_HAS_MONOTONIC */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_RTC_H_ */
