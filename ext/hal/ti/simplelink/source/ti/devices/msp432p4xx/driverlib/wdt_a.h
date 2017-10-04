/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

//*****************************************************************************
//
//! \addtogroup wdt_api
//! @{
//
//*****************************************************************************

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

/* Includes */
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <stdint.h>

#ifdef __MCU_HAS_SYSCTL_A__

#define WDT_A_HARD_RESET SYSCTL_A_HARD_RESET
#define WDT_A_SOFT_RESET SYSCTL_A_SOFT_RESET

#else

#define WDT_A_HARD_RESET SYSCTL_HARD_RESET
#define WDT_A_SOFT_RESET SYSCTL_SOFT_RESET

#endif

//*****************************************************************************
//
// The following are values that can be passed to the clockSelect parameter for
// functions: WDT_A_watchdogTimerInit(), and WDT_A_intervalTimerInit().
//
//*****************************************************************************
#define WDT_A_CLOCKSOURCE_SMCLK                                     (WDT_A_CTL_SSEL_0)
#define WDT_A_CLOCKSOURCE_ACLK                                      (WDT_A_CTL_SSEL_1)
#define WDT_A_CLOCKSOURCE_VLOCLK                                    (WDT_A_CTL_SSEL_2)
#define WDT_A_CLOCKSOURCE_BCLK                                      (WDT_A_CTL_SSEL_3)

//*****************************************************************************
//
// The following are values that can be passed to the clockDivider parameter
// for functions: WDT_A_watchdogTimerInit(), and WDT_A_intervalTimerInit().
//
//*****************************************************************************
#define WDT_A_CLOCKDIVIDER_2G                                         (WDT_A_CTL_IS_0)
#define WDT_A_CLOCKDIVIDER_128M                                       (WDT_A_CTL_IS_1)
#define WDT_A_CLOCKDIVIDER_8192K                                      (WDT_A_CTL_IS_2)
#define WDT_A_CLOCKDIVIDER_512K                                       (WDT_A_CTL_IS_3)
#define WDT_A_CLOCKDIVIDER_32K                                        (WDT_A_CTL_IS_4)
#define WDT_A_CLOCKDIVIDER_8192                                       (WDT_A_CTL_IS_5)
#define WDT_A_CLOCKDIVIDER_512                                        (WDT_A_CTL_IS_6)
#define WDT_A_CLOCKDIVIDER_64                                         (WDT_A_CTL_IS_7)
#define WDT_A_CLOCKITERATIONS_2G    WDT_A_CLOCKDIVIDER_2G
#define WDT_A_CLOCKITERATIONS_128M  WDT_A_CLOCKDIVIDER_128M
#define WDT_A_CLOCKITERATIONS_8192K WDT_A_CLOCKDIVIDER_8192K
#define WDT_A_CLOCKITERATIONS_512K  WDT_A_CLOCKDIVIDER_512K
#define WDT_A_CLOCKITERATIONS_32K   WDT_A_CLOCKDIVIDER_32K
#define WDT_A_CLOCKITERATIONS_8192  WDT_A_CLOCKDIVIDER_8192
#define WDT_A_CLOCKITERATIONS_512   WDT_A_CLOCKDIVIDER_512
#define WDT_A_CLOCKITERATIONS_64    WDT_A_CLOCKDIVIDER_64

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Holds the Watchdog Timer.
//!
//! This function stops the watchdog timer from running. This way no interrupt
//! or PUC is asserted.
//!
//! \return None
//
//*****************************************************************************
extern void WDT_A_holdTimer(void);

//*****************************************************************************
//
//! Starts the Watchdog Timer.
//!
//! This function starts the watchdog timer functionality to start counting.
//!
//! \return None
//
//*****************************************************************************
extern void WDT_A_startTimer(void);

//*****************************************************************************
//
//! Clears the timer counter of the Watchdog Timer.
//!
//! This function clears the watchdog timer count to 0x0000h. This function
//! is used to "service the dog" when operating in watchdog mode.
//!
//! \return None
//
//*****************************************************************************
extern void WDT_A_clearTimer(void);

//*****************************************************************************
//
//! Sets the clock source for the Watchdog Timer in watchdog mode.
//!
//! \param clockSelect is the clock source that the watchdog timer will use.
//!        Valid values are
//!        - \b WDT_A_CLOCKSOURCE_SMCLK [Default]
//!        - \b WDT_A_CLOCKSOURCE_ACLK
//!        - \b WDT_A_CLOCKSOURCE_VLOCLK
//!        - \b WDT_A_CLOCKSOURCE_BCLK
//! \param clockIterations is the number of clock iterations for a watchdog
//!         timeout.
//!        Valid values are
//!        - \b WDT_A_CLOCKITERATIONS_2G [Default]
//!        - \b WDT_A_CLOCKITERATIONS_128M
//!        - \b WDT_A_CLOCKITERATIONS_8192K
//!        - \b WDT_A_CLOCKITERATIONS_512K
//!        - \b WDT_A_CLOCKITERATIONS_32K
//!        - \b WDT_A_CLOCKITERATIONS_8192
//!        - \b WDT_A_CLOCKITERATIONS_512
//!        - \b WDT_A_CLOCKITERATIONS_64
//!
//! This function sets the watchdog timer in watchdog mode, which will cause a
//! PUC when the timer overflows. When in the mode, a PUC can be avoided with a
//! call to WDT_A_resetTimer() before the timer runs out.
//!
//! \return None
//
//*****************************************************************************
extern void WDT_A_initWatchdogTimer(uint_fast8_t clockSelect,
        uint_fast8_t clockDivider);

//*****************************************************************************
//
//! Sets the clock source for the Watchdog Timer in timer interval mode.
//!
//! \param clockSelect is the clock source that the watchdog timer will use.
//!        Valid values are
//!        - \b WDT_A_CLOCKSOURCE_SMCLK [Default]
//!        - \b WDT_A_CLOCKSOURCE_ACLK
//!        - \b WDT_A_CLOCKSOURCE_VLOCLK
//!        - \b WDT_A_CLOCKSOURCE_BCLK
//! \param clockIterations is the number of clock iterations for a watchdog
//!         interval.
//!        Valid values are
//!        - \b WDT_A_CLOCKITERATIONS_2G [Default]
//!        - \b WDT_A_CLOCKITERATIONS_128M
//!        - \b WDT_A_CLOCKITERATIONS_8192K
//!        - \b WDT_A_CLOCKITERATIONS_512K
//!        - \b WDT_A_CLOCKITERATIONS_32K
//!        - \b WDT_A_CLOCKITERATIONS_8192
//!        - \b WDT_A_CLOCKITERATIONS_512
//!        - \b WDT_A_CLOCKITERATIONS_64
//!
//! This function sets the watchdog timer as timer interval mode, which will
//! assert an interrupt without causing a PUC.
//!
//! \return None
//
//*****************************************************************************
extern void WDT_A_initIntervalTimer(uint_fast8_t clockSelect,
        uint_fast8_t clockDivider);

//*****************************************************************************
//
//! Registers an interrupt handler for the watchdog interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the
//! watchdog interrupt occurs.
//!
//! \return None.
//
//*****************************************************************************
extern void WDT_A_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the watchdog.
//!
//! This function unregisters the handler to be called when a watchdog
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void WDT_A_unregisterInterrupt(void);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog password violation
//! occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!     - \b WDT_A_HARD_RESET
//!     - \b WDT_A_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void WDT_A_setPasswordViolationReset(uint_fast8_t resetType);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog timeout occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!     - \b WDT_A_HARD_RESET
//!     - \b WDT_A_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void WDT_A_setTimeoutReset(uint_fast8_t resetType);

/* Defines for future devices that might have multiple instances */
#define WDT_A_holdTimerMultipleTimer(a)  WDT_A_holdTimer()
#define WDT_A_startTimerMultipleTimer(a) WDT_A_startTimer()
#define WDT_A_resetTimerMultipleTimer(a) WDT_A_clearTimer()
#define WDT_A_initWatchdogTimerMultipleTimer(a,b,c) WDT_A_initWatchdogTimer(b,c)
#define WDT_A_initIntervalTimerMultipleTimer(a,b,c) WDT_A_initIntervalTimer(b,c)
#define WDT_A_registerInterruptMultipleTimer(a,b) WDT_A_registerInterrupt(b)
#define WDT_A_unregisterInterruptMultipleTimer(a) WDT_A_unregisterInterrupt()

/* Backwards compatibility layer */
#define WDT_A_hold WDT_A_holdTimerMultipleTimer
#define WDT_A_start WDT_A_startTimerMultipleTimer
#define WDT_A_resetTimer WDT_A_resetTimerMultipleTimer
#define WDT_A_watchdogTimerInit WDT_A_initWatchdogTimerMultipleTimer
#define WDT_A_intervalTimerInit WDT_A_initIntervalTimerMultipleTimer

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __WATCHDOG_H__
