/******************************************************************************
*  Filename:       timer.h
*  Revised:        2017-05-23 12:08:52 +0200 (Tue, 23 May 2017)
*  Revision:       49048
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup timer_api
//! @{
//
//****************************************************************************

#ifndef __GPT_H__
#define __GPT_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_ints.h"
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_gpt.h"
#include "interrupt.h"
#include "debug.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define TimerConfigure                  NOROM_TimerConfigure
    #define TimerLevelControl               NOROM_TimerLevelControl
    #define TimerStallControl               NOROM_TimerStallControl
    #define TimerWaitOnTriggerControl       NOROM_TimerWaitOnTriggerControl
    #define TimerIntRegister                NOROM_TimerIntRegister
    #define TimerIntUnregister              NOROM_TimerIntUnregister
    #define TimerMatchUpdateMode            NOROM_TimerMatchUpdateMode
    #define TimerIntervalLoadMode           NOROM_TimerIntervalLoadMode
#endif

//*****************************************************************************
//
// Values that can be passed to TimerConfigure as the ui32Config parameter.
//
//*****************************************************************************
#define TIMER_CFG_ONE_SHOT        0x00000021  // Full-width one-shot timer
#define TIMER_CFG_ONE_SHOT_UP     0x00000031  // Full-width one-shot up-count timer
#define TIMER_CFG_PERIODIC        0x00000022  // Full-width periodic timer
#define TIMER_CFG_PERIODIC_UP     0x00000032  // Full-width periodic up-count timer
#define TIMER_CFG_SPLIT_PAIR      0x04000000  // Two half-width timers
#define TIMER_CFG_A_ONE_SHOT      0x00000021  // Timer A one-shot timer
#define TIMER_CFG_A_ONE_SHOT_UP   0x00000031  // Timer A one-shot up-count timer
#define TIMER_CFG_A_PERIODIC      0x00000022  // Timer A periodic timer
#define TIMER_CFG_A_PERIODIC_UP   0x00000032  // Timer A periodic up-count timer
#define TIMER_CFG_A_CAP_COUNT     0x00000003  // Timer A event counter
#define TIMER_CFG_A_CAP_COUNT_UP  0x00000013  // Timer A event up-counter
#define TIMER_CFG_A_CAP_TIME      0x00000007  // Timer A event timer
#define TIMER_CFG_A_CAP_TIME_UP   0x00000017  // Timer A event up-count timer
#define TIMER_CFG_A_PWM           0x0000000A  // Timer A PWM output
#define TIMER_CFG_B_ONE_SHOT      0x00002100  // Timer B one-shot timer
#define TIMER_CFG_B_ONE_SHOT_UP   0x00003100  // Timer B one-shot up-count timer
#define TIMER_CFG_B_PERIODIC      0x00002200  // Timer B periodic timer
#define TIMER_CFG_B_PERIODIC_UP   0x00003200  // Timer B periodic up-count timer
#define TIMER_CFG_B_CAP_COUNT     0x00000300  // Timer B event counter
#define TIMER_CFG_B_CAP_COUNT_UP  0x00001300  // Timer B event up-counter
#define TIMER_CFG_B_CAP_TIME      0x00000700  // Timer B event timer
#define TIMER_CFG_B_CAP_TIME_UP   0x00001700  // Timer B event up-count timer
#define TIMER_CFG_B_PWM           0x00000A00  // Timer B PWM output

//*****************************************************************************
//
// Values that can be passed to TimerIntEnable, TimerIntDisable, and
// TimerIntClear as the ui32IntFlags parameter, and returned from
// TimerIntStatus.
//
//*****************************************************************************
#define TIMER_TIMB_DMA            0x00002000  // TimerB DMA Done interrupt
#define TIMER_TIMB_MATCH          0x00000800  // TimerB match interrupt
#define TIMER_CAPB_EVENT          0x00000400  // CaptureB event interrupt
#define TIMER_CAPB_MATCH          0x00000200  // CaptureB match interrupt
#define TIMER_TIMB_TIMEOUT        0x00000100  // TimerB time out interrupt
#define TIMER_TIMA_DMA            0x00000020  // TimerA DMA Done interrupt
#define TIMER_TIMA_MATCH          0x00000010  // TimerA match interrupt
#define TIMER_CAPA_EVENT          0x00000004  // CaptureA event interrupt
#define TIMER_CAPA_MATCH          0x00000002  // CaptureA match interrupt
#define TIMER_TIMA_TIMEOUT        0x00000001  // TimerA time out interrupt

//*****************************************************************************
//
// Values that can be passed to TimerControlEvent as the ui32Event parameter.
//
//*****************************************************************************
#define TIMER_EVENT_POS_EDGE      0x00000000  // Count positive edges
#define TIMER_EVENT_NEG_EDGE      0x00000404  // Count negative edges
#define TIMER_EVENT_BOTH_EDGES    0x00000C0C  // Count both edges

//*****************************************************************************
//
// Values that can be passed to most of the timer APIs as the ui32Timer
// parameter.
//
//*****************************************************************************
#define TIMER_A                   0x000000FF  // Timer A
#define TIMER_B                   0x0000FF00  // Timer B
#define TIMER_BOTH                0x0000FFFF  // Timer Both

//*****************************************************************************
//
// Values that can be passed to GPTSynchronize as the ui32Timers parameter
//
//*****************************************************************************
#define TIMER_0A_SYNC             0x00000001  // Synchronize Timer 0A
#define TIMER_0B_SYNC             0x00000002  // Synchronize Timer 0B
#define TIMER_1A_SYNC             0x00000004  // Synchronize Timer 1A
#define TIMER_1B_SYNC             0x00000008  // Synchronize Timer 1B
#define TIMER_2A_SYNC             0x00000010  // Synchronize Timer 2A
#define TIMER_2B_SYNC             0x00000020  // Synchronize Timer 2B
#define TIMER_3A_SYNC             0x00000040  // Synchronize Timer 3A
#define TIMER_3B_SYNC             0x00000080  // Synchronize Timer 3B

//*****************************************************************************
//
// Values that can be passed to TimerMatchUpdateMode
//
//*****************************************************************************
#define TIMER_MATCHUPDATE_NEXTCYCLE  0x00000000 // Apply match register on next cycle
#define TIMER_MATCHUPDATE_TIMEOUT    0x00000001 // Apply match register on next timeout

//*****************************************************************************
//
// Values that can be passed to TimerIntervalLoad
//
//*****************************************************************************
#define TIMER_INTERVALLOAD_NEXTCYCLE  0x00000000 // Load TxR register with the value in the TxILR register on the next clock cycle
#define TIMER_INTERVALLOAD_TIMEOUT    0x00000001 // Load TxR register with the value in the TxILR register on next timeout

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

#ifdef DRIVERLIB_DEBUG
//*****************************************************************************
//
//! \internal
//!
//! \brief Checks a timer base address.
//!
//! This function determines if a timer module base address is valid.
//!
//! \param ui32Base is the base address of the timer module.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise.
//
//*****************************************************************************
static bool
TimerBaseValid(uint32_t ui32Base)
{
    return((ui32Base == GPT0_BASE) || (ui32Base == GPT1_BASE) ||
           (ui32Base == GPT2_BASE) || (ui32Base == GPT3_BASE));
}
#endif

//*****************************************************************************
//
//! \brief Enables the timer(s).
//!
//! This function enables operation of the timer module. The timer must be
//! configured before it is enabled.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to enable. Must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerEnable(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Enable the timer(s) module.
    HWREG(ui32Base + GPT_O_CTL) |= ui32Timer & (GPT_CTL_TAEN | GPT_CTL_TBEN);
}

//*****************************************************************************
//
//! \brief Disables the timer(s).
//!
//! This function disables operation of the timer module.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to disable. Must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerDisable(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Disable the timer module.
    HWREG(ui32Base + GPT_O_CTL) &= ~(ui32Timer &
                                     (GPT_CTL_TAEN | GPT_CTL_TBEN));
}

//*****************************************************************************
//
//! \brief Configures the timer(s).
//!
//! This function configures the operating mode of the timer(s). The timer
//! module is disabled before being configured and is left in the disabled
//! state.
//!
//! The timers are comprised of two 16-bit timers that can
//! operate independently or be concatenated to form a 32-bit timer.
//!
//! \note If the timers are used independently the length of timer can be
//! extended to 24 bit by use of an 8 bit prescale register set using
//! \ref TimerPrescaleSet().
//!
//! When configuring for full-width timer \c ui32Config is set
//! as one of the following values:
//! - \ref TIMER_CFG_ONE_SHOT    : Full-width one-shot timer.
//! - \ref TIMER_CFG_ONE_SHOT_UP : Full-width one-shot timer that counts up
//!   instead of down.
//! - \ref TIMER_CFG_PERIODIC    : Full-width periodic timer.
//! - \ref TIMER_CFG_PERIODIC_UP : Full-width periodic timer that counts up
//!   instead of down.
//!
//! When configuring for a pair of half-width timers, each timer is separately
//! configured. The timers are configured by setting \c ui32Config to
//! the bitwise OR of one of each of the following three:
//! - Use half-width timers:
//!   - \ref TIMER_CFG_SPLIT_PAIR
//! - Timer A:
//!   - \ref TIMER_CFG_A_ONE_SHOT     : Half-width one-shot timer
//!   - \ref TIMER_CFG_A_ONE_SHOT_UP  : Half-width one-shot timer that counts up
//!     instead of down.
//!   - \ref TIMER_CFG_A_PERIODIC     : Half-width periodic timer
//!   - \ref TIMER_CFG_A_PERIODIC_UP  : Half-width periodic timer that counts up
//!     instead of down.
//!   - \ref TIMER_CFG_A_CAP_COUNT    : Half-width edge count capture
//!   - \ref TIMER_CFG_A_CAP_COUNT_UP : Half-width edge count capture that counts
//!     up instead of down.
//!   - \ref TIMER_CFG_A_CAP_TIME     : Half-width edge time capture
//!   - \ref TIMER_CFG_A_CAP_TIME_UP  : Half-width edge time capture that counts up
//!     instead of down.
//!   - \ref TIMER_CFG_A_PWM          : Half-width PWM output
//! - Timer B:
//!   - Same as Timer A but using TIMER_CFG_B_* instead.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Config is the configuration for the timer.
//!
//! \return None
//
//*****************************************************************************
extern void TimerConfigure(uint32_t ui32Base, uint32_t ui32Config);

//*****************************************************************************
//
//! \brief Controls the output level.
//!
//! This function configures the PWM output level for the specified timer.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to adjust. Must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param bInvert specifies the output level.
//! - \c true  : Timer's output is active low.
//! - \c false : Timer's output is active high.
//!
//! \return None
//
//*****************************************************************************
extern void TimerLevelControl(uint32_t ui32Base, uint32_t ui32Timer,
                              bool bInvert);

//*****************************************************************************
//
//! \brief Controls the event type.
//!
//! This function configures the signal edge(s) that triggers the timer when
//! in capture mode.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to be adjusted; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Event specifies the type of event; must be one of:
//! - \ref TIMER_EVENT_POS_EDGE
//! - \ref TIMER_EVENT_NEG_EDGE
//! - \ref TIMER_EVENT_BOTH_EDGES
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerEventControl(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Event)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the event type.
    ui32Timer &= GPT_CTL_TAEVENT_M | GPT_CTL_TBEVENT_M;
    HWREG(ui32Base + GPT_O_CTL) = ((HWREG(ui32Base + GPT_O_CTL) & ~ui32Timer) |
                                   (ui32Event & ui32Timer));
}

//*****************************************************************************
//
//! \brief Controls the stall handling.
//!
//! This function controls the stall response for the specified timer. If the
//! \e bStall parameter is \b true, then the timer stops counting if the
//! processor enters debug mode; otherwise the timer keeps running while in
//! debug mode.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to be adjusted; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param bStall specifies the response to a stall signal.
//! - \c true  : Timer stops counting if the processor enters debug mode.
//! - \c false : Timer keeps running if the processor enters debug mode.
//!
//! \return None
//
//*****************************************************************************
extern void TimerStallControl(uint32_t ui32Base, uint32_t ui32Timer,
                              bool bStall);

//*****************************************************************************
//
//! \brief Controls the wait on trigger handling.
//!
//! This function controls whether or not a timer waits for a trigger input to
//! start counting. When enabled, the previous timer in the trigger chain must
//! count to its timeout in order for this timer to start counting. Refer to
//! the part's data sheet for a description of the trigger chain.
//!
//! \note This function should not be used for Timer 0A or Wide Timer 0A.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to be adjusted; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param bWait specifies if the timer should wait for a trigger input.
//! - \c true  : Wait for trigger.
//! - \c false : Do not wait for trigger.
//!
//! \return None
//
//*****************************************************************************
extern void TimerWaitOnTriggerControl(uint32_t ui32Base, uint32_t ui32Timer,
                                      bool bWait);

//*****************************************************************************
//
//! \brief Set the timer prescale value.
//!
//! This function configures the value of the timer clock prescaler. The
//! prescaler is only operational when in half-width mode and is used to extend
//! the range of the half-width timer modes.
//!
//! When in one-shot or periodic down count modes, \b ui32Value defines the
//! prescaler for the timer counter. When acting as a true prescaler, the
//! prescaler counts down to 0 before the value in timer registers are incremented.
//!
//! In all other individual/split modes, \b ui32Value is a linear extension of
//! the upper range of the timer counter, holding bits 23:16 in the 16-bit modes
//! of the 16/32-bit timer.
//!
//! \note Because the prescaler counts down to 0 the timer division ratio equals
//! \b ui32Value + 1. E.g. a prescale value of 15 divides the timer rate by 16.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to adjust; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Value is the timer prescale value which must be between 0 and 255
//! (both included).
//! - 0   : Timer division ratio = 1 (disable prescaling).
//! - 1   : Timer division ratio = 2.
//! - ...
//! - 255 : Timer division ratio = 256.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerPrescaleSet(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Value)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));
    ASSERT(ui32Value < 256);

    // Set the timer A prescaler if requested.
    if(ui32Timer & TIMER_A)
    {
        HWREG(ui32Base + GPT_O_TAPR) = ui32Value;
    }

    // Set the timer B prescaler if requested.
    if(ui32Timer & TIMER_B)
    {
        HWREG(ui32Base + GPT_O_TBPR) = ui32Value;
    }
}

//*****************************************************************************
//
//! \brief Get the timer prescale value.
//!
//! This function gets the value of the timer clock prescaler. The
//! prescaler is only operational when in half-width mode and is used to extend
//! the range of the half-width timer modes.
//!
//! When in one-shot or periodic down count modes, \b ui32Value defines the
//! prescaler for the timer counter. When acting as a true prescaler, the
//! prescaler counts down to 0 before the value in timer registers are incremented.
//!
//! In all other individual/split modes, \b ui32Value is a linear extension of
//! the upper range of the timer counter, holding bits 23:16 in the 16-bit modes
//! of the 16/32-bit timer.
//!
//! \note Because the prescaler counts down to 0 the timer division ratio equals
//! \b ui32Value + 1. E.g. a prescale value of 15 divides the timer rate by 16.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//!
//! \return Returns the value of the timer prescaler.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerPrescaleGet(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Return the appropriate prescale value.
    return((ui32Timer == TIMER_A) ? HWREG(ui32Base + GPT_O_TAPR) :
           HWREG(ui32Base + GPT_O_TBPR));
}

//*****************************************************************************
//
//! \brief Set the timer prescale match value.
//!
//! This function configures the value of the input clock prescaler match
//! value. When in a half-width mode that uses the counter match and the
//! prescaler, the prescale match effectively extends the range of the match.
//! The prescaler provides the least significant bits when counting down in
//! periodic and one-shot modes; in all other modes, the prescaler provides the
//! most significant bits.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to adjust; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Value is the timer prescale match value which must be between 0
//! and 255 (both included).
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerPrescaleMatchSet(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Value)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));
    ASSERT(ui32Value < 256);

    // Set the timer A prescale match if requested.
    if(ui32Timer & TIMER_A)
    {
        HWREG(ui32Base + GPT_O_TAPMR) = ui32Value;
    }

    // Set the timer B prescale match if requested.
    if(ui32Timer & TIMER_B)
    {
        HWREG(ui32Base + GPT_O_TBPMR) = ui32Value;
    }
}

//*****************************************************************************
//
//! \brief Get the timer prescale match value.
//!
//! This function gets the value of the input clock prescaler match value.
//! When in a half-width mode that uses the counter match and prescaler, the
//! prescale match effectively extends the range of the match. The prescaler
//! provides the least significant bits when counting down in periodic and
//! one-shot modes; in all other modes, the prescaler provides the most
//! significant bits.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//!
//! \return Returns the value of the timer prescale match.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerPrescaleMatchGet(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B));

    // Return the appropriate prescale match value.
    return((ui32Timer == TIMER_A) ? HWREG(ui32Base + GPT_O_TAPMR) :
           HWREG(ui32Base + GPT_O_TBPMR));
}

//*****************************************************************************
//
//! \brief Sets the timer load value.
//!
//! This function configures the timer load value; if the timer is running then
//! the value is immediately loaded into the timer.
//!
//! \note This function can be used for both full- and half-width modes of
//! 16/32-bit timers.
//!
//! \note Only \ref TIMER_A should be used when the timer is configured for
//! full-width operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to adjust; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Value is the load value.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerLoadSet(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Value)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the timer A load value if requested.
    if(ui32Timer & TIMER_A)
    {
        HWREG(ui32Base + GPT_O_TAILR) = ui32Value;
    }

    // Set the timer B load value if requested.
    if(ui32Timer & TIMER_B)
    {
        HWREG(ui32Base + GPT_O_TBILR) = ui32Value;
    }
}

//*****************************************************************************
//
//! \brief Gets the timer load value.
//!
//! This function gets the currently programmed interval load value for the
//! specified timer.
//!
//! \note This function can be used for both full- and half-width modes of
//! 16/32-bit timers.
//!
//! \note Only \ref TIMER_A should be used when the timer is configured for
//! full-width operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//!
//! \return Returns the load value for the timer
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerLoadGet(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B));

    // Return the appropriate load value.
    return((ui32Timer == TIMER_A) ? HWREG(ui32Base + GPT_O_TAILR) :
           HWREG(ui32Base + GPT_O_TBILR));
}

//*****************************************************************************
//
//! \brief Gets the current timer value.
//!
//! This function reads the current value of the specified timer.
//!
//! \note This function can be used for both full- and half-width modes of
//! 16/32-bit timers.
//!
//! \note Only \ref TIMER_A should be used when the timer is configured for
//! full-width operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//!
//! \return Returns the current value of the timer.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerValueGet(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B));

    // Return the appropriate timer value.
    return((ui32Timer == TIMER_A) ? HWREG(ui32Base + GPT_O_TAR) :
           HWREG(ui32Base + GPT_O_TBR));
}

//*****************************************************************************
//
//! \brief Sets the timer match value.
//!
//! This function configures the match value for a timer. This value is used
//! in capture count mode to determine when to interrupt the processor and in
//! PWM mode to determine the duty cycle of the output signal. Match interrupts
//! can also be generated in periodic and one-shot modes when the value of the
//! counter matches this register.
//!
//! \note This function can be used for both full- and half-width modes of
//! 16/32-bit timers.
//!
//! \note Only \ref TIMER_A should be used when the timer is configured for
//! full-width operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to adjust; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Value is the match value.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerMatchSet(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Value)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the timer A match value if requested.
    if(ui32Timer & TIMER_A)
    {
        HWREG(ui32Base + GPT_O_TAMATCHR) = ui32Value;
    }

    // Set the timer B match value if requested.
    if(ui32Timer & TIMER_B)
    {
        HWREG(ui32Base + GPT_O_TBMATCHR) = ui32Value;
    }
}

//*****************************************************************************
//
//! \brief Gets the timer match value.
//!
//! This function gets the match value for the specified timer.
//!
//! \note This function can be used for both full- and half-width modes of
//! 16/32-bit timers.
//!
//! \note Only \ref TIMER_A should be used when the timer is configured for
//! full-width operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//!
//! \return Returns the match value for the timer
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerMatchGet(uint32_t ui32Base, uint32_t ui32Timer)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B));

    // Return the appropriate match value.
    return((ui32Timer == TIMER_A) ? HWREG(ui32Base + GPT_O_TAMATCHR) :
           HWREG(ui32Base + GPT_O_TBMATCHR));
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for the timer interrupt in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific timer interrupts must be enabled via \ref TimerIntEnable(). It is the
//! interrupt handler's responsibility to clear the interrupt source via
//! \ref TimerIntClear().
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s); must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param pfnHandler is a pointer to the function to be called when the timer
//! interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void TimerIntRegister(uint32_t ui32Base, uint32_t ui32Timer,
                             void (*pfnHandler)(void));

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for the timer interrupt in the dynamic interrupt table.
//!
//! This function unregisters the handler to be called when a timer interrupt
//! occurs. This function also masks off the interrupt in the interrupt
//! controller so that the interrupt handler is no longer called.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s); must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void TimerIntUnregister(uint32_t ui32Base, uint32_t ui32Timer);

//*****************************************************************************
//
//! \brief Enables individual timer interrupt sources.
//!
//! This function enables the indicated timer interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32IntFlags is the bit mask of the interrupt sources to be enabled.
//! The parameter must be the bitwise OR of any combination of
//! the following:
//! - \ref TIMER_CAPB_EVENT    : Capture B event interrupt.
//! - \ref TIMER_CAPB_MATCH    : Capture B match interrupt.
//! - \ref TIMER_TIMB_TIMEOUT  : Timer B timeout interrupt.
//! - \ref TIMER_CAPA_EVENT    : Capture A event interrupt.
//! - \ref TIMER_CAPA_MATCH    : Capture A match interrupt.
//! - \ref TIMER_TIMA_TIMEOUT  : Timer A timeout interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerIntEnable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));

    // Enable the specified interrupts.
    HWREG(ui32Base + GPT_O_IMR) |= ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Disables individual timer interrupt sources.
//!
//! This function disables the indicated timer interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32IntFlags is the bit mask of the interrupt sources to be disabled.
//! The parameter must be the bitwise OR of any combination of
//! the following:
//! - \ref TIMER_CAPB_EVENT    : Capture B event interrupt.
//! - \ref TIMER_CAPB_MATCH    : Capture B match interrupt.
//! - \ref TIMER_TIMB_TIMEOUT  : Timer B timeout interrupt.
//! - \ref TIMER_CAPA_EVENT    : Capture A event interrupt.
//! - \ref TIMER_CAPA_MATCH    : Capture A match interrupt.
//! - \ref TIMER_TIMA_TIMEOUT  : Timer A timeout interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerIntDisable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));

    // Disable the specified interrupts.
    HWREG(ui32Base + GPT_O_IMR) &= ~(ui32IntFlags);
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status.
//!
//! This function returns the interrupt status for the timer module. Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param ui32Base is the base address of the timer module.
//! \param bMasked selects either raw or masked interrupt status:
//! - \c true  : Masked interrupt.
//! - \c false : Raw interrupt.
//!
//! \return The current interrupt status, enumerated as a bit field of values:
//! - \ref TIMER_CAPB_EVENT    : Capture B event interrupt.
//! - \ref TIMER_CAPB_MATCH    : Capture B match interrupt.
//! - \ref TIMER_TIMB_TIMEOUT  : Timer B timeout interrupt.
//! - \ref TIMER_CAPA_EVENT    : Capture A event interrupt.
//! - \ref TIMER_CAPA_MATCH    : Capture A match interrupt.
//! - \ref TIMER_TIMA_TIMEOUT  : Timer A timeout interrupt.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TimerIntStatus(uint32_t ui32Base, bool bMasked)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    return(bMasked ? HWREG(ui32Base + GPT_O_MIS) :
           HWREG(ui32Base + GPT_O_RIS));
}

//*****************************************************************************
//
//! \brief Clears timer interrupt sources.
//!
//! The specified timer interrupt sources are cleared, so that they no longer
//! assert. This function must be called in the interrupt handler to keep the
//! interrupt from being triggered again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in a module and until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//! At the same time, an early event clear allows new events of the same type to be
//! pended instead of ignored if the event is cleared later in the ISR.
//! It is the responsibility of the programmer to make sure that enough time has passed
//! before returning from the ISR to avoid false re-triggering of the cleared event.
//! A simple, although not necessarily optimal, way of clearing an event before
//! returning from the ISR is:
//! -# Write to clear event (interrupt source). (buffered write)
//! -# Dummy read from the event source module. (making sure the write has propagated)
//! -# Wait two system CPU clock cycles (user code or two NOPs). (allowing cleared event to propagate through any synchronizers)
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! The parameter must be the bitwise OR of any combination of
//! the following:
//! - \ref TIMER_CAPB_EVENT    : Capture B event interrupt.
//! - \ref TIMER_CAPB_MATCH    : Capture B match interrupt.
//! - \ref TIMER_TIMB_TIMEOUT  : Timer B timeout interrupt.
//! - \ref TIMER_CAPA_EVENT    : Capture A event interrupt.
//! - \ref TIMER_CAPA_MATCH    : Capture A match interrupt.
//! - \ref TIMER_TIMA_TIMEOUT  : Timer A timeout interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerIntClear(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));

    // Clear the requested interrupt sources.
    HWREG(ui32Base + GPT_O_ICLR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Synchronizes the counters in a set of timers.
//!
//! This function synchronizes the counters in a specified set of timers.
//! When a timer is running in half-width mode, each half can be included or
//! excluded in the synchronization event. When a timer is running in
//! full-width mode, only the A timer can be synchronized (specifying the B
//! timer has no effect).
//!
//! \param ui32Base is the base address of the timer module. This parameter must
//! be the base address of Timer0 (in other words, \b GPT0_BASE).
//! \param ui32Timers is the set of timers to synchronize.
//! The parameter is the bitwise OR of any of the following:
//! - \ref TIMER_0A_SYNC
//! - \ref TIMER_0B_SYNC
//! - \ref TIMER_1A_SYNC
//! - \ref TIMER_1B_SYNC
//! - \ref TIMER_2A_SYNC
//! - \ref TIMER_2B_SYNC
//! - \ref TIMER_3A_SYNC
//! - \ref TIMER_3B_SYNC
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerSynchronize(uint32_t ui32Base, uint32_t ui32Timers)
{
    // Check the arguments.
    ASSERT(ui32Base == GPT0_BASE);

    // Synchronize the specified timers.
    HWREG(ui32Base + GPT_O_SYNC) = ui32Timers;
}

//*****************************************************************************
//
//! \brief Enables AND'ing of the CCP outputs from Timer A and Timer B.
//!
//! \param ui32Base is the base address of the timer module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerCcpCombineEnable(uint32_t ui32Base)
{
    // Check the arguments
    ASSERT(TimerBaseValid(ui32Base));

    // Set the bit
    HWREG(ui32Base + GPT_O_ANDCCP) |= GPT_ANDCCP_CCP_AND_EN;
}

//*****************************************************************************
//
//! \brief Disables AND'ing of the CCP outputs from Timer A and Timer B.
//!
//! \param ui32Base is the base address of the timer module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TimerCcpCombineDisable(uint32_t ui32Base)
{
    // Check the arguments
    ASSERT(TimerBaseValid(ui32Base));

    // Clear the bit
    HWREG(ui32Base + GPT_O_ANDCCP) &= ~(GPT_ANDCCP_CCP_AND_EN);
}

//*****************************************************************************
//
//! \brief Sets the Match Register Update mode.
//!
//! This function controls when the Match Register value and Prescale Register value
//! are applied after writing these registers while a timer is enabled.
//!
//! \note If the timer is disabled when setting the update mode the Match Register
//! and Prescale Register values are applied immediately when enabling the timer.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to configure; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Mode sets the mode:
//! - \ref TIMER_MATCHUPDATE_NEXTCYCLE : Apply Match Register and Prescale Register on next clock
//!        cycle after writing any of these registers.
//! - \ref TIMER_MATCHUPDATE_TIMEOUT   : Apply Match Register and Prescale Register on next timeout
//!        after writing any of these registers.
//!
//! \return None
//
//*****************************************************************************
extern void TimerMatchUpdateMode(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Mode);

//*****************************************************************************
//
//! \brief Sets the Interval Load mode.
//!
//! This function controls when the Timer Register and Prescale Snap-shot (if used)
//! are updated.
//!
//! Timer Register (TAR/TBR) is updated when Interval Load Register (TAILR/TBILR) is written
//! and the Prescale Snap-shot (TAPS/TBPS) is updated when Prescale Register (TAPR/TBPR) is
//! written depending on the mode of operation.
//!
//! \param ui32Base is the base address of the timer module.
//! \param ui32Timer specifies the timer(s) to configure; must be one of:
//! - \ref TIMER_A
//! - \ref TIMER_B
//! - \ref TIMER_BOTH
//! \param ui32Mode sets the mode:
//! - \ref TIMER_INTERVALLOAD_NEXTCYCLE : Update Timer Register and Prescale Snap-shot on next clock
//!        cycle after writing Interval Load Register or Prescale Register, respectively.
//! - \ref TIMER_INTERVALLOAD_TIMEOUT   : Update Timer Register and Prescale Snap-shot on next timeout
//!        after writing Interval Load Register or Prescale Register, respectively.
//!
//! \return None
//
//*****************************************************************************
extern void TimerIntervalLoadMode(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Mode);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_TimerConfigure
        #undef  TimerConfigure
        #define TimerConfigure                  ROM_TimerConfigure
    #endif
    #ifdef ROM_TimerLevelControl
        #undef  TimerLevelControl
        #define TimerLevelControl               ROM_TimerLevelControl
    #endif
    #ifdef ROM_TimerStallControl
        #undef  TimerStallControl
        #define TimerStallControl               ROM_TimerStallControl
    #endif
    #ifdef ROM_TimerWaitOnTriggerControl
        #undef  TimerWaitOnTriggerControl
        #define TimerWaitOnTriggerControl       ROM_TimerWaitOnTriggerControl
    #endif
    #ifdef ROM_TimerIntRegister
        #undef  TimerIntRegister
        #define TimerIntRegister                ROM_TimerIntRegister
    #endif
    #ifdef ROM_TimerIntUnregister
        #undef  TimerIntUnregister
        #define TimerIntUnregister              ROM_TimerIntUnregister
    #endif
    #ifdef ROM_TimerMatchUpdateMode
        #undef  TimerMatchUpdateMode
        #define TimerMatchUpdateMode            ROM_TimerMatchUpdateMode
    #endif
    #ifdef ROM_TimerIntervalLoadMode
        #undef  TimerIntervalLoadMode
        #define TimerIntervalLoadMode           ROM_TimerIntervalLoadMode
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __GPT_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
