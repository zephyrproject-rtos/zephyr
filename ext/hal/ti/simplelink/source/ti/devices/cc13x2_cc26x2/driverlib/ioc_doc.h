/******************************************************************************
*  Filename:       ioc_doc.h
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
//! \addtogroup ioc_api
//! @{
//! \section sec_ioc Introduction
//!
//! The Input/Output Controller (IOC) controls the functionality of the pins (called DIO).
//! The IOC consists of two APIs:
//! - MCU IOC API selects which peripheral module is connected to the individual DIO and thus allowed to control it.
//!   It also controls individual drive strength, slew rate, pull-up/pull-down, edge detection, etc.
//! - AON IOC API controls the general drive strength definitions, IO latches, and if the LF clock is
//!   routed to a DIO for external use.
//!
//! For more information on the AON IOC see the [AON IOC API](\ref aonioc_api).
//!
//! \note The output driver of a DIO is not configured by the IOC API (except for drive strength); instead, it is controlled by the
//! peripheral module which is selected to control the DIO.
//!
//! A DIO is considered "software controlled" if it is configured for GPIO control which allows the
//! System CPU to set the value of the DIO via the [GPIO API](\ref gpio_api). Alternatively, a DIO
//! can be "hardware controlled" if it is controlled by other modules than GPIO.
//!
//! \section sec_ioc_api API
//!
//! The API functions can be grouped like this:
//!
//! Configure all settings at the same time:
//! - \ref IOCPortConfigureSet()
//! - \ref IOCPortConfigureGet()
//!
//! Configure individual settings:
//! - \ref IOCIODrvStrengthSet()
//! - \ref IOCIOHystSet()
//! - \ref IOCIOInputSet()
//! - \ref IOCIOIntSet()
//! - \ref IOCIOModeSet()
//! - \ref IOCIOPortIdSet()
//! - \ref IOCIOPortPullSet()
//! - \ref IOCIOShutdownSet()
//! - \ref IOCIOSlewCtrlSet()
//!
//! Handle edge detection events:
//! - \ref IOCIntEnable()
//! - \ref IOCIntDisable()
//! - \ref IOCIntClear()
//! - \ref IOCIntStatus()
//! - \ref IOCIntRegister()
//! - \ref IOCIntUnregister()
//!
//! Configure IOCs for typical use cases (can also be used as example code):
//! - \ref IOCPinTypeAux()
//! - \ref IOCPinTypeGpioInput()
//! - \ref IOCPinTypeGpioOutput()
//! - \ref IOCPinTypeI2c()
//! - \ref IOCPinTypeSsiMaster()
//! - \ref IOCPinTypeSsiSlave()
//! - \ref IOCPinTypeUart()
//!
//! @}
