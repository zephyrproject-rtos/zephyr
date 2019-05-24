/******************************************************************************
*  Filename:       timer_doc.h
*  Revised:        2016-03-30 13:03:59 +0200 (Wed, 30 Mar 2016)
*  Revision:       45971
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
//! \addtogroup timer_api
//! @{
//! \section sec_timer Introduction
//!
//! The timer API provides a set of functions for using the general-purpose timer module.
//!
//! The timer module contains four timer blocks with the following functional options:
//! - Operating modes:
//!   - 16-bit with 8-bit prescaler or 32-bit programmable one-shot timer.
//!   - 16-bit with 8-bit prescaler or 32-bit programmable periodic timer.
//!   - Two capture compare PWM pins (CCP) for each 32-bit timer.
//!   - 24-bit input-edge count or 24-bit time-capture modes.
//!   - 24-bit PWM mode with software-programmable output inversion of the PWM signal.
//!   - Count up or down.
//! - Daisy chaining of timer modules allows a single timer to initiate multiple timing events.
//! - Timer synchronization allows selected timers to start counting on the same clock cycle.
//! - User-enabled stalling when the System CPU asserts a CPU Halt flag during debug.
//! - Ability to determine the elapsed time between the assertion of the timer interrupt and
//!   entry into the interrupt service routine.
//!
//! Each timer block provides two half-width timers/counters that can be configured
//! to operate independently as timers or event counters or to operate as a combined
//! full-width timer.
//! The timers provide 16-bit half-width timers and a 32-bit full-width timer.
//! For the purposes of this API, the two
//! half-width timers provided by a timer block are referred to as TimerA and
//! TimerB, and the full-width timer is referred to as TimerA.
//!
//! When in half-width mode, the timer can also be configured for event capture or
//! as a pulse width modulation (PWM) generator. When configured for event
//! capture, the timer acts as a counter. It can be configured to count either the
//! time between events or the events themselves. The type of event
//! being counted can be configured as a positive edge, a negative edge, or both
//! edges. When a timer is configured as a PWM generator, the input signal used to
//! capture events becomes an output signal, and the timer drives an
//! edge-aligned pulse onto that signal.
//!
//! Control is also provided over interrupt sources and events. Interrupts can be
//! generated to indicate that an event has been captured, or that a certain number
//! of events have been captured. Interrupts can also be generated when the timer
//! has counted down to 0 or when the timer matches a certain value.
//!
//! Timer configuration is handled by \ref TimerConfigure(), which performs the high
//! level setup of the timer module; that is, it is used to set up full- or
//! half-width modes, and to select between PWM, capture, and timer operations.
//!
//! \section sec_timer_api API
//!
//! The API functions can be grouped like this:
//!
//! Functions to perform timer control:
//! - \ref TimerConfigure()
//! - \ref TimerEnable()
//! - \ref TimerDisable()
//! - \ref TimerLevelControl()
//! - \ref TimerWaitOnTriggerControl()
//! - \ref TimerEventControl()
//! - \ref TimerStallControl()
//! - \ref TimerIntervalLoadMode()
//! - \ref TimerMatchUpdateMode()
//! - \ref TimerCcpCombineDisable()
//! - \ref TimerCcpCombineEnable()
//!
//! Functions to manage timer content:
//! - \ref TimerLoadSet()
//! - \ref TimerLoadGet()
//! - \ref TimerPrescaleSet()
//! - \ref TimerPrescaleGet()
//! - \ref TimerMatchSet()
//! - \ref TimerMatchGet()
//! - \ref TimerPrescaleMatchSet()
//! - \ref TimerPrescaleMatchGet()
//! - \ref TimerValueGet()
//! - \ref TimerSynchronize()
//!
//! Functions to manage the interrupt handler for the timer interrupt:
//! - \ref TimerIntRegister()
//! - \ref TimerIntUnregister()
//!
//! The individual interrupt sources within the timer module are managed with:
//! - \ref TimerIntEnable()
//! - \ref TimerIntDisable()
//! - \ref TimerIntStatus()
//! - \ref TimerIntClear()
//!
//! @}
