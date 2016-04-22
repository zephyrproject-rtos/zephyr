/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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
#ifndef _FSL_PIT_H_
#define _FSL_PIT_H_

#include "fsl_common.h"

/*!
 * @addtogroup pit_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_PIT_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
/*@}*/

/*!
 * @brief List of PIT channels
 * @note Actual number of available channels is SoC dependent
 */
typedef enum _pit_chnl
{
    kPIT_Chnl_0 = 0U, /*!< PIT channel number 0*/
    kPIT_Chnl_1,      /*!< PIT channel number 1 */
    kPIT_Chnl_2,      /*!< PIT channel number 2 */
    kPIT_Chnl_3,      /*!< PIT channel number 3 */
} pit_chnl_t;

/*! @brief List of PIT interrupts */
typedef enum _pit_interrupt_enable
{
    kPIT_TimerInterruptEnable = PIT_TCTRL_TIE_MASK, /*!< Timer interrupt enable*/
} pit_interrupt_enable_t;

/*! @brief List of PIT status flags */
typedef enum _pit_status_flags
{
    kPIT_TimerFlag = PIT_TFLG_TIF_MASK, /*!< Timer flag */
} pit_status_flags_t;

/*!
 * @brief PIT config structure
 *
 * This structure holds the configuration settings for the PIT peripheral. To initialize this
 * structure to reasonable defaults, call the PIT_GetDefaultConfig() function and pass a
 * pointer to your config structure instance.
 *
 * The config struct can be made const so it resides in flash
 */
typedef struct _pit_config
{
    bool enableRunInDebug; /*!< true: Timers run in debug mode; false: Timers stop in debug mode */
} pit_config_t;

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
 * @brief Ungates the PIT clock, enables the PIT module and configures the peripheral for basic operation.
 *
 * @note This API should be called at the beginning of the application using the PIT driver.
 *
 * @param base   PIT peripheral base address
 * @param config Pointer to user's PIT config structure
 */
void PIT_Init(PIT_Type *base, const pit_config_t *config);

/*!
 * @brief Gate the PIT clock and disable the PIT module
 *
 * @param base PIT peripheral base address
 */
void PIT_Deinit(PIT_Type *base);

/*!
 * @brief Fill in the PIT config struct with the default settings
 *
 * The default values are:
 * @code
 *     config->enableRunInDebug = false;
 * @endcode
 * @param config Pointer to user's PIT config structure.
 */
static inline void PIT_GetDefaultConfig(pit_config_t *config)
{
    assert(config);

    /* Timers are stopped in Debug mode */
    config->enableRunInDebug = false;
}

#if defined(FSL_FEATURE_PIT_HAS_CHAIN_MODE) && FSL_FEATURE_PIT_HAS_CHAIN_MODE

/*!
 * @brief Enables or disables chaining a timer with the previous timer.
 *
 * When a timer has a chain mode enabled, it only counts after the previous
 * timer has expired. If the timer n-1 has counted down to 0, counter n
 * decrements the value by one. Each timer is 32-bits, this allows the developers
 * to chain timers together and form a longer timer (64-bits and larger). The first timer
 * (timer 0) cannot be chained to any other timer.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number which is chained with the previous timer
 * @param enable  Enable or disable chain.
 *                true:  Current timer is chained with the previous timer.
 *                false: Timer doesn't chain with other timers.
 */
static inline void PIT_SetTimerChainMode(PIT_Type *base, pit_chnl_t channel, bool enable)
{
    if (enable)
    {
        base->CHANNEL[channel].TCTRL |= PIT_TCTRL_CHN_MASK;
    }
    else
    {
        base->CHANNEL[channel].TCTRL &= ~PIT_TCTRL_CHN_MASK;
    }
}

#endif /* FSL_FEATURE_PIT_HAS_CHAIN_MODE */

/*! @}*/

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enables the selected PIT interrupts.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 * @param mask    The interrupts to enable. This is a logical OR of members of the
 *                enumeration ::pit_interrupt_enable_t
 */
static inline void PIT_EnableInterrupts(PIT_Type *base, pit_chnl_t channel, uint32_t mask)
{
    base->CHANNEL[channel].TCTRL |= mask;
}

/*!
 * @brief Disables the selected PIT interrupts.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 * @param mask    The interrupts to disable. This is a logical OR of members of the
 *                enumeration ::pit_interrupt_enable_t
 */
static inline void PIT_DisableInterrupts(PIT_Type *base, pit_chnl_t channel, uint32_t mask)
{
    base->CHANNEL[channel].TCTRL &= ~mask;
}

/*!
 * @brief Gets the enabled PIT interrupts.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 *
 * @return The enabled interrupts. This is the logical OR of members of the
 *         enumeration ::pit_interrupt_enable_t
 */
static inline uint32_t PIT_GetEnabledInterrupts(PIT_Type *base, pit_chnl_t channel)
{
    return (base->CHANNEL[channel].TCTRL & PIT_TCTRL_TIE_MASK);
}

/*! @}*/

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Gets the PIT status flags
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::pit_status_flags_t
 */
static inline uint32_t PIT_GetStatusFlags(PIT_Type *base, pit_chnl_t channel)
{
    return (base->CHANNEL[channel].TFLG & PIT_TFLG_TIF_MASK);
}

/*!
 * @brief  Clears the PIT status flags.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 * @param mask    The status flags to clear. This is a logical OR of members of the
 *                enumeration ::pit_status_flags_t
 */
static inline void PIT_ClearStatusFlags(PIT_Type *base, pit_chnl_t channel, uint32_t mask)
{
    base->CHANNEL[channel].TFLG = mask;
}

/*! @}*/

/*!
 * @name Read and Write the timer period
 * @{
 */

/*!
 * @brief Sets the timer period in units of count.
 *
 * Timers begin counting from the value set by this function until it reaches 0,
 * then it will generate an interrupt and load this regiter value again.
 * Writing a new value to this register will not restart the timer; instead the value
 * will be loaded after the timer expires.
 *
 * @note User can call the utility macros provided in fsl_common.h to convert to ticks
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 * @param count   Timer period in units of ticks
 */
static inline void PIT_SetTimerPeriod(PIT_Type *base, pit_chnl_t channel, uint32_t count)
{
    base->CHANNEL[channel].LDVAL = count;
}

/*!
 * @brief Reads the current timer counting value.
 *
 * This function returns the real-time timer counting value, in a range from 0 to a
 * timer period.
 *
 * @note User can call the utility macros provided in fsl_common.h to convert ticks to usec or msec
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number
 *
 * @return Current timer counting value in ticks
 */
static inline uint32_t PIT_GetCurrentTimerCount(PIT_Type *base, pit_chnl_t channel)
{
    return base->CHANNEL[channel].CVAL;
}

/*! @}*/

/*!
 * @name Timer Start and Stop
 * @{
 */

/*!
 * @brief Starts the timer counting.
 *
 * After calling this function, timers load period value, count down to 0 and
 * then load the respective start value again. Each time a timer reaches 0,
 * it generates a trigger pulse and sets the timeout interrupt flag.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number.
 */
static inline void PIT_StartTimer(PIT_Type *base, pit_chnl_t channel)
{
    base->CHANNEL[channel].TCTRL |= PIT_TCTRL_TEN_MASK;
}

/*!
 * @brief Stops the timer counting.
 *
 * This function stops every timer counting. Timers reload their periods
 * respectively after the next time they call the PIT_DRV_StartTimer.
 *
 * @param base    PIT peripheral base address
 * @param channel Timer channel number.
 */
static inline void PIT_StopTimer(PIT_Type *base, pit_chnl_t channel)
{
    base->CHANNEL[channel].TCTRL &= ~PIT_TCTRL_TEN_MASK;
}

/*! @}*/

#if defined(FSL_FEATURE_PIT_HAS_LIFETIME_TIMER) && FSL_FEATURE_PIT_HAS_LIFETIME_TIMER

/*!
 * @brief Reads the current lifetime counter value.
 *
 * The lifetime timer is a 64-bit timer which chains timer 0 and timer 1 together.
 * Timer 0 and 1 are chained by calling the PIT_SetTimerChainMode before using this timer.
 * The period of lifetime timer is equal to the "period of timer 0 * period of timer 1".
 * For the 64-bit value, the higher 32-bit has the value of timer 1, and the lower 32-bit
 * has the value of timer 0.
 *
 * @param base PIT peripheral base address
 *
 * @return Current lifetime timer value
 */
uint64_t PIT_GetLifetimeTimerCount(PIT_Type *base);

#endif /* FSL_FEATURE_PIT_HAS_LIFETIME_TIMER */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_PIT_H_ */
