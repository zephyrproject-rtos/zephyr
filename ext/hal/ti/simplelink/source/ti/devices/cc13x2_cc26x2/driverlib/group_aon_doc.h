/******************************************************************************
*  Filename:       group_aon_doc.h
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
//! \addtogroup aon_group
//! @{
//! \section sec_aon Introduction
//!
//! The Always-ON (AON) voltage domain contains the AUX power domain, AON power domain, and JTAG power domain.
//! The AON API includes functions to access the AON power domain. For functions accessing the AUX power domain
//! see the [AUX API](@ref aux_group).
//!
//! The AON power domain contains circuitry that is always enabled, except for the shutdown mode
//! (digital supply is off), and the AON power domain is clocked at 32-kHz.
//!
//! The AON API accesses the AON registers through a common module called AON Interface (AON IF) which handles the
//! actual transactions towards the much slower AON registers. Because accessing AON can cause a significant
//! delay in terms of system CPU clock cycles it is important to understand the basics about how the AON IF
//! operates. The following list describes a few of the most relevant properties of the AON IF seen from the system CPU:
//! - \ti_bold{Shadow registers}: The system CPU actually accesses a set of "shadow registers" which are being synchronized to the AON registers
//!   by the AON IF every AON clock cycle.
//!   - Writing an AON register via AON IF can take up to one AON clock cycle before taking effect in the AON domain. However, the system CPU can
//!     continue executing without waiting for this.
//!   - The AON IF supports multiple writes within the same AON clock cycle thus several registers/bit fields can be synchronized simultaneously.
//!   - Reading from AON IF returns the value from last time the shadow registers were synchronized (if no writes to AON IF have occurred since)
//!     thus the value can be up to one AON clock cycle old.
//!   - Reading from AON IF after a write (but before synchronization has happened) will return the value from the shadow register
//!     and not the last value from the AON register. Thus doing multiple read-modify-writes within one AON clock cycle is supported.
//! - \ti_bold{Read delay}: Due to an asynchronous interface to the AON IF, reading AON registers will generate a few wait cycles thus stalling
//!   the system CPU until the read completes. There is no delay on writes to AON IF if using posted/buffered writes.
//! - \ti_bold{Synchronizing}: If it is required that a write to AON takes effect before continuing code execution it is possible to do a conditional "wait for
//!   synchronization" by calling \ref SysCtrlAonSync(). This will wait for any pending writes to synchronize.
//! - \ti_bold{Updating}: It is also possible to do an unconditional "wait for synchronization", in case a new read
//!   value is required, by calling \ref SysCtrlAonUpdate(). This is typically used after wake-up to make sure the AON IF has been
//!   synchronized at least once before reading the values.
//!
//! Below are a few guidelines to write efficient code for AON access based on the properties of the interface to the AON registers.
//! - Avoid synchronizing unless required by the application. If synchronization is needed then try to group/arrange AON writes to
//!   minimize the number of required synchronizations.
//! - If modifying several bit fields within a single AON register it is slightly faster to do a single read, modify the bit fields,
//!   and then write it back rather than doing multiple independent read-modify-writes (due to the read delay).
//! - Using posted/buffered writes to AON (default) lets the system CPU continue execution immediately. Using non-posted/non-buffered
//!   writes will generate a delay similar to a read access.
//!
//! @}
