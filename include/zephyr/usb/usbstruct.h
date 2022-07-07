/* SPDX-License-Identifier: BSD-3-Clause */

/*
 *  LPCUSB, an USB device driver for LPC microcontrollers
 *  Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)
 *  Copyright (c) 2016 Intel Corporation
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This header and macros below are deprecated in 2.7 release.
 * Please replace with macros from Chapter 9 header, include/usb/usb_ch9.h
 */

/**
 * @file
 * @brief standard USB packet structures and defines
 *
 * This file contains structures and defines of the standard USB packets
 */

#include <zephyr/usb/usb_ch9.h>

#ifndef ZEPHYR_INCLUDE_USB_USBSTRUCT_H_
#define ZEPHYR_INCLUDE_USB_USBSTRUCT_H_

#warning "<usb/usbstruct.h> header is deprecated, use <usb/usb_ch9.h> instead"

#define REQTYPE_GET_DIR(x)          __DEPRECATED_MACRO USB_REQTYPE_GET_DIR(x)
#define REQTYPE_GET_TYPE(x)         __DEPRECATED_MACRO USB_REQTYPE_GET_TYPE(x)
#define REQTYPE_GET_RECIP(x)        __DEPRECATED_MACRO USB_REQTYPE_GET_RECIPIENT(x)

#define REQTYPE_DIR_TO_DEVICE       __DEPRECATED_MACRO 0
#define REQTYPE_DIR_TO_HOST         __DEPRECATED_MACRO 1

#define REQTYPE_TYPE_STANDARD       __DEPRECATED_MACRO 0
#define REQTYPE_TYPE_CLASS          __DEPRECATED_MACRO 1
#define REQTYPE_TYPE_VENDOR         __DEPRECATED_MACRO 2
#define REQTYPE_TYPE_RESERVED       __DEPRECATED_MACRO 3

#define REQTYPE_RECIP_DEVICE        __DEPRECATED_MACRO 0
#define REQTYPE_RECIP_INTERFACE     __DEPRECATED_MACRO 1
#define REQTYPE_RECIP_ENDPOINT      __DEPRECATED_MACRO 2
#define REQTYPE_RECIP_OTHER         __DEPRECATED_MACRO 3

/* standard requests */
#define REQ_GET_STATUS              __DEPRECATED_MACRO 0x00
#define REQ_CLEAR_FEATURE           __DEPRECATED_MACRO 0x01
#define REQ_SET_FEATURE             __DEPRECATED_MACRO 0x03
#define REQ_SET_ADDRESS             __DEPRECATED_MACRO 0x05
#define REQ_GET_DESCRIPTOR          __DEPRECATED_MACRO 0x06
#define REQ_SET_DESCRIPTOR          __DEPRECATED_MACRO 0x07
#define REQ_GET_CONFIGURATION       __DEPRECATED_MACRO 0x08
#define REQ_SET_CONFIGURATION       __DEPRECATED_MACRO 0x09
#define REQ_GET_INTERFACE           __DEPRECATED_MACRO 0x0A
#define REQ_SET_INTERFACE           __DEPRECATED_MACRO 0x0B
#define REQ_SYNCH_FRAME             __DEPRECATED_MACRO 0x0C

/* feature selectors */
#define FEA_ENDPOINT_HALT           __DEPRECATED_MACRO 0x00
#define FEA_REMOTE_WAKEUP           __DEPRECATED_MACRO 0x01
#define FEA_TEST_MODE               __DEPRECATED_MACRO 0x02

#define DEVICE_STATUS_SELF_POWERED  __DEPRECATED_MACRO 0x01
#define DEVICE_STATUS_REMOTE_WAKEUP __DEPRECATED_MACRO 0x02

#define GET_DESC_TYPE(x)            __DEPRECATED_MACRO USB_GET_DESCRIPTOR_TYPE(x)
#define GET_DESC_INDEX(x)           __DEPRECATED_MACRO USB_GET_DESCRIPTOR_INDEX(x)

#endif /* ZEPHYR_INCLUDE_USB_USBSTRUCT_H_ */
