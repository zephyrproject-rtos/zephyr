/******************************************************************************
*  Filename:       event_doc.h
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
//! \addtogroup event_api
//! @{
//! \section sec_event Introduction
//!
//! The event fabric consists of two event modules. One in the MCU power domain (MCU event fabric) and
//! the other in the AON power domain (AON event fabric). The MCU event fabric is one of the subscribers
//! to the AON event fabric. For more information on AON event fabric, see [AON event API](@ref aonevent_api).
//!
//! The MCU event fabric is a combinational router between event sources and event subscribers. Most
//! event subscribers have statically routed event sources but several event subscribers have
//! configurable event sources which is configured in the MCU event fabric through this API. Although
//! configurable only a subset of event sources are available to each of the configurable event subscribers.
//! This is explained in more details in the function @ref EventRegister() which does all the event routing
//! configuration.
//!
//! MCU event fabric also contains four software events which allow software to trigger certain event
//! subscribers. Each of the four software events is an independent event source which must be set and
//! cleared in the MCU event fabric through the functions:
//! - @ref EventSwEventSet()
//! - @ref EventSwEventClear()
//! - @ref EventSwEventGet()
//!
//! @}
