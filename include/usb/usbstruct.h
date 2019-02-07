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

/**
 * @file
 * @brief standard USB packet structures and defines
 *
 * This file contains structures and defines of the standard USB packets
 */

#ifndef ZEPHYR_INCLUDE_USB_USBSTRUCT_H_
#define ZEPHYR_INCLUDE_USB_USBSTRUCT_H_

#define REQTYPE_GET_DIR(x)          (((x)>>7)&0x01)
#define REQTYPE_GET_TYPE(x)         (((x)>>5)&0x03)
#define REQTYPE_GET_RECIP(x)        ((x)&0x1F)

#define REQTYPE_DIR_TO_DEVICE       0
#define REQTYPE_DIR_TO_HOST         1

#define REQTYPE_TYPE_STANDARD       0
#define REQTYPE_TYPE_CLASS          1
#define REQTYPE_TYPE_VENDOR         2
#define REQTYPE_TYPE_RESERVED       3

#define REQTYPE_RECIP_DEVICE        0
#define REQTYPE_RECIP_INTERFACE     1
#define REQTYPE_RECIP_ENDPOINT      2
#define REQTYPE_RECIP_OTHER         3

/* standard requests */
#define REQ_GET_STATUS              0x00
#define REQ_CLEAR_FEATURE           0x01
#define REQ_SET_FEATURE             0x03
#define REQ_SET_ADDRESS             0x05
#define REQ_GET_DESCRIPTOR          0x06
#define REQ_SET_DESCRIPTOR          0x07
#define REQ_GET_CONFIGURATION       0x08
#define REQ_SET_CONFIGURATION       0x09
#define REQ_GET_INTERFACE           0x0A
#define REQ_SET_INTERFACE           0x0B
#define REQ_SYNCH_FRAME             0x0C

/* feature selectors */
#define FEA_ENDPOINT_HALT           0x00
#define FEA_REMOTE_WAKEUP           0x01
#define FEA_TEST_MODE               0x02

#define DEVICE_STATUS_SELF_POWERED  0x01
#define DEVICE_STATUS_REMOTE_WAKEUP 0x02

/*
 *  USB descriptors
 */

/** USB descriptor header */
struct usb_desc_header {
	u8_t bLength;               /**< descriptor length */
	u8_t bDescriptorType;       /**< descriptor type */
};

#define DESC_DEVICE                 1
#define DESC_CONFIGURATION          2
#define DESC_STRING                 3
#define DESC_INTERFACE              4
#define DESC_ENDPOINT               5
#define DESC_DEVICE_QUALIFIER       6
#define DESC_OTHER_SPEED            7
#define DESC_INTERFACE_POWER        8

#define CS_INTERFACE                0x24
#define CS_ENDPOINT                 0x25

#define GET_DESC_TYPE(x)            (((x)>>8)&0xFF)
#define GET_DESC_INDEX(x)           ((x)&0xFF)

#endif /* ZEPHYR_INCLUDE_USB_USBSTRUCT_H_ */
