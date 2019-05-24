/******************************************************************************
*  Filename:       systick_doc.h
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
//! \addtogroup systick_api
//! @{
//! \section sec_systick Introduction
//!
//! The system CPU includes a system timer, SysTick, integrated in the NVIC which provides a simple, 24-bit,
//! clear-on-write, decrementing, wrap-on-zero counter with a flexible control mechanism.
//! When enabled, the timer counts down on each clock from the reload value to 0, reloads (wraps) on
//! the next clock edge, then decrements on subsequent clocks.
//!
//! The SysTick counter runs on the system clock. If this clock signal is stopped for low-power mode, the
//! SysTick counter stops.
//!
//! When the processor is halted for debugging, the counter does not decrement.
//!
//! \section sec_systick_api API
//!
//! The API functions can be grouped like this:
//!
//! Configuration and status:
//! - \ref SysTickPeriodSet()
//! - \ref SysTickPeriodGet()
//! - \ref SysTickValueGet()
//!
//! Enable and disable:
//! - \ref SysTickEnable()
//! - \ref SysTickDisable()
//!
//! Interrupt configuration:
//! - \ref SysTickIntRegister()
//! - \ref SysTickIntUnregister()
//! - \ref SysTickIntEnable()
//! - \ref SysTickIntDisable()
//! @}
