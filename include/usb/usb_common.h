/***************************************************************************
 *
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

/**
 * @file
 * @brief useful constants and macros for the USB application
 *
 * This file contains useful constants and macros for the USB applications.
 */

#ifndef USB_COMMON_H_
#define USB_COMMON_H_

/* Decriptor size in bytes */
#define USB_DEVICE_DESC_SIZE            18
#define USB_CONFIGURATION_DESC_SIZE     9
#define USB_INTERFACE_DESC_SIZE         9
#define USB_ENDPOINT_DESC_SIZE          7
#define USB_STRING_DESC_SIZE            4
#define USB_HID_DESC_SIZE               9
#define USB_DFU_DESC_SIZE               9

/* Descriptor type */
#define USB_DEVICE_DESC                 0x01
#define USB_CONFIGURATION_DESC          0x02
#define USB_STRING_DESC                 0x03
#define USB_INTERFACE_DESC              0x04
#define USB_ENDPOINT_DESC               0x05
#define USB_HID_DESC                    0x21
#define USB_HID_REPORT_DESC             0x22
#define USB_DFU_FUNCTIONAL_DESC         0x21

/* Useful define */
#define USB_1_1                         0x0110

#define BCDDEVICE_RELNUM                0x0100

/* 100mA max power, per 2mA units */
/* USB 1.1 spec indicates 100mA(max) per unit load, up to 5 loads */
#define MAX_LOW_POWER                   0x32
#define MAX_HIGH_POWER                  0xFA

/* bmAttributes:
 * D7:Reserved, always 1,
 * D6:Self-Powered -> 1,
 * D5:Remote Wakeup -> 0,
 * D4...0:Reserved -> 0
 */
#define USB_CONFIGURATION_ATTRIBUTES    0xC0

/* Classes */
#define COMMUNICATION_DEVICE_CLASS      0x02
#define COMMUNICATION_DEVICE_CLASS_DATA 0x0A
#define HID_CLASS                       0x03
#define MASS_STORAGE_CLASS              0x08
#define WIRELESS_DEVICE_CLASS           0xE0
#define CUSTOM_CLASS                    0xFF
#define DFU_CLASS                       0xFE

/* Sub-classes */
#define ACM_SUBCLASS                    0x02
#define BOOT_INTERFACE_SUBCLASS         0x01
#define SCSI_TRANSPARENT_SUBCLASS       0x06
#define DFU_INTERFACE_SUBCLASS          0x01
#define RF_SUBCLASS                     0x01

/* Protocols */
#define V25TER_PROTOCOL                 0x01
#define MOUSE_PROTOCOL                  0x02
#define BULK_ONLY_PROTOCOL              0x50
#define DFU_RUNTIME_PROTOCOL            0x01
#define DFU_MODE_PROTOCOL               0x02
#define BLUETOOTH_PROTOCOL              0x01

#endif /* USB_COMMON_H_ */
