/******************************************************************************
*  Filename:       watchdog_doc.h
*  Revised:        2018-02-09 15:45:36 +0100 (Fri, 09 Feb 2018)
*  Revision:       51470
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
//! \addtogroup wdt_api
//! @{
//! \section sec_wdt Introduction
//!
//! The Watchdog Timer (WDT) allows the application to regain control if the system stalls due to
//! unexpected software behavior. The WDT can generate a normal interrupt or a non-maskable interrupt
//! on the first time-out and a system reset on the following time-out if the application fails to
//! restart the WDT.
//!
//! WDT has the following features:
//! - 32-bit down counter with a configurable load register.
//! - Configurable interrupt generation logic with interrupt masking and optional NMI function.
//! - Optional reset generation.
//! - Register protection from runaway software (lock).
//! - User-enabled stalling when the system CPU asserts the CPU Halt flag during debug.
//!
//! The WDT runs at system HF clock divided by 32; however, when in powerdown it runs at
//! LF clock (32 kHz) - if the LF clock to the MCU domain is enabled.
//!
//! If application does not restart the WDT, using \ref WatchdogIntClear(), before a time-out:
//! - At the first time-out the WDT asserts the interrupt, reloads the 32-bit counter with the load
//!   value, and resumes counting down from that value.
//! - If the WDT counts down to zero again before the application clears the interrupt, and the
//!   reset signal has been enabled, the WDT asserts its reset signal to the system.
//!
//! \note By default, a "warm reset" triggers a pin reset and thus reboots the device.
//!
//! A reset caused by the WDT can be detected as a "warm reset" using \ref SysCtrlResetSourceGet().
//! However, it is not possible to detect which of the warm reset sources that caused the reset.
//!
//! Typical use case:
//! - Use \ref WatchdogIntTypeSet() to select either standard interrupt or non-maskable interrupt on
//!   first time-out.
//!   - The application must implement an interrupt handler for the selected interrupt type. If
//!     application uses the \e static vector table (see startup_<compiler>.c) the interrupt
//!     handlers for standard interrupt and non-maskable interrupt are named WatchdogIntHandler()
//!     and NmiSR() respectively. For more information about \e static and \e dynamic vector table,
//!     see \ref sec_interrupt_table.
//! - Use \ref WatchdogResetEnable() to enable reset on second time-out.
//! - Use \ref WatchdogReloadSet() to set (re)load value of the counter.
//! - Use \ref WatchdogEnable() to start the WDT counter. The WDT counts down from the load value.
//! - Use \ref WatchdogLock() to lock WDT configuration to prevent unintended re-configuration.
//! - Application must use \ref WatchdogIntClear() to restart the counter before WDT times out.
//! - If application does not restart the counter before it reaches zero (times out) the WDT asserts
//!   the selected type of interrupt, reloads the counter, and starts counting down again.
//!   - The interrupt handler triggered by the first time-out can be used to log debug information
//!     or try to enter a safe "pre-reset" state in order to have a more graceful reset when the WDT
//!     times out the second time.
//!   - It is \b not recommended that the WDT interrupt handler clears the WDT interrupt and thus
//!     reloads the WDT counter. This means that the WDT interrupt handler never returns.
//! - If the application does not clear the WDT interrupt and the WDT times out when the interrupt
//!   is still asserted then WDT triggers a reset (if enabled).
//!
//! \section sec_wdt_api API
//!
//! The API functions can be grouped like this:
//!
//! Watchdog configuration:
//! - \ref WatchdogIntTypeSet()
//! - \ref WatchdogResetEnable()
//! - \ref WatchdogResetDisable()
//! - \ref WatchdogReloadSet()
//! - \ref WatchdogEnable()
//!
//! Status:
//! - \ref WatchdogRunning()
//! - \ref WatchdogValueGet()
//! - \ref WatchdogReloadGet()
//! - \ref WatchdogIntStatus()
//!
//! Interrupt configuration:
//! - \ref WatchdogIntEnable()
//! - \ref WatchdogIntClear()
//! - \ref WatchdogIntRegister()
//! - \ref WatchdogIntUnregister()
//!
//! Register protection:
//! - \ref WatchdogLock()
//! - \ref WatchdogLockState()
//! - \ref WatchdogUnlock()
//!
//! Stall configuration for debugging:
//! - \ref WatchdogStallDisable()
//! - \ref WatchdogStallEnable()
//!
//! @}
