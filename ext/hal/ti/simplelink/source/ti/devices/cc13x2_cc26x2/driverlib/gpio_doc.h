/******************************************************************************
*  Filename:       gpio_doc.h
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
//! \addtogroup gpio_api
//! @{
//! \section sec_gpio Introduction
//!
//! The GPIO module allows software to control the pins of the device directly if the IOC module has
//! been configured to route the GPIO signal to a physical pin (called DIO). Alternatively, pins can
//! be hardware controlled by other peripheral modules. For more information about the IOC module,
//! how to configure physical pins, and how to select between software controlled and hardware controlled,
//! see the [IOC API](\ref ioc_api).
//!
//! The System CPU can access the GPIO module to read the value of any DIO of the device and if the IOC
//! module has been configured such that one or more DIOs are GPIO controlled (software controlled) the
//! System CPU can write these DIOs through the GPIO module.
//!
//! The IOC module can also be configured to generate events on edge detection and these events can be
//! read and cleared in the GPIO module by the System CPU.
//!
//! \section sec_gpio_api API
//!
//! The API functions can be grouped like this:
//!
//! Set and get direction of DIO (output enable):
//! - \ref GPIO_setOutputEnableDio()
//! - \ref GPIO_setOutputEnableMultiDio()
//! - \ref GPIO_getOutputEnableDio()
//! - \ref GPIO_getOutputEnableMultiDio()
//!
//! Write DIO (requires IOC to be configured for GPIO usage):
//! - \ref GPIO_writeDio()
//! - \ref GPIO_writeMultiDio()
//!
//! Set, clear, or toggle DIO (requires IOC to be configured for GPIO usage):
//! - \ref GPIO_setDio()
//! - \ref GPIO_setMultiDio()
//! - \ref GPIO_clearDio()
//! - \ref GPIO_clearMultiDio()
//! - \ref GPIO_toggleDio()
//! - \ref GPIO_toggleMultiDio()
//!
//! Read DIO (even if IOC is NOT configured for GPIO usage; however, the DIO must be configured for input enable in IOC):
//! - \ref GPIO_readDio()
//! - \ref GPIO_readMultiDio()
//!
//! Read or clear events (even if IOC is NOT configured for GPIO usage; however, the DIO must be configured for input enable in IOC):
//! - \ref GPIO_getEventDio()
//! - \ref GPIO_getEventMultiDio()
//! - \ref GPIO_clearEventDio()
//! - \ref GPIO_clearEventMultiDio()
//!
//! The [IOC API](\ref ioc_api) provides two functions for easy configuration of DIOs as GPIO enabled using
//! typical settings. They also serve as examples on how to configure the IOC and GPIO modules for GPIO usage:
//! - \ref IOCPinTypeGpioInput()
//! - \ref IOCPinTypeGpioOutput()
//!
//! @}
