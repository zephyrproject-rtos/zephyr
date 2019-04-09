/******************************************************************************
*  Filename:       aon_event_doc.h
*  Revised:        2017-08-09 16:56:05 +0200 (Wed, 09 Aug 2017)
*  Revision:       49506
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
//! \addtogroup aonevent_api
//! @{
//! \section sec_aonevent Introduction
//!
//! The event fabric consists of two event modules. One in the MCU power domain (MCU event fabric) and
//! the other in the AON power domain (AON event fabric). The MCU event fabric is one of the subscribers
//! to the AON event fabric. For more information on MCU event fabric, see [MCU event API](@ref event_api).
//!
//! The AON event fabric is a configurable combinatorial router between AON event sources and event
//! subscribers in both AON and MCU domains. The API to control the AON event fabric configuration
//! can be grouped based on the event subscriber to configure:
//!
//! - Wake-up events.
//!   - MCU wake-up event
//!     - @ref AONEventMcuWakeUpSet()
//!     - @ref AONEventMcuWakeUpGet()
//! - AON RTC receives a single programmable event line from the AON event fabric. For more information, see [AON RTC API](@ref aonrtc_api).
//!   - @ref AONEventRtcSet()
//!   - @ref AONEventRtcGet()
//! - MCU event fabric receives a number of programmable event lines from the AON event fabric. For more information, see [MCU event API](@ref event_api).
//!   - @ref AONEventMcuSet()
//!   - @ref AONEventMcuGet()
//! @}
