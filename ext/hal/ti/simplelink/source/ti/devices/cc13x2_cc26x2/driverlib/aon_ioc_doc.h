/******************************************************************************
*  Filename:       aon_ioc_doc.h
*  Revised:        2016-03-30 11:01:30 +0200 (Wed, 30 Mar 2016)
*  Revision:       45969
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
//! \addtogroup aonioc_api
//! @{
//! \section sec_aonioc Introduction
//!
//! The Input/Output Controller (IOC) controls the functionality of the pins (called DIO).
//! The IOC consists of two APIs:
//! - MCU IOC API selects which peripheral module is connected to the individual DIO and thus allowed to control it.
//!   It also controls individual drive strength, slew rate, pull-up/pull-down, edge detection, etc.
//! - AON IOC API controls the general drive strength definitions, IO latches, and if the LF clock is
//!   routed to a DIO for external use.
//!
//! For more information on the MCU IOC see the [IOC API](\ref ioc_api).
//!
//! \section sec_aonioc_api API
//!
//! The API functions can be grouped like this:
//!
//! Freeze IOs while MCU domain is powered down:
//! - \ref AONIOCFreezeEnable()
//! - \ref AONIOCFreezeDisable()
//!
//! Output LF clock to a DIO:
//! - \ref AONIOC32kHzOutputEnable()
//! - \ref AONIOC32kHzOutputDisable()
//!
//! Configure the value of drive strength for the three manual MCU IOC settings (MIN, MED, MAX):
//! - \ref AONIOCDriveStrengthSet()
//! - \ref AONIOCDriveStrengthGet()
//!
//! @}
