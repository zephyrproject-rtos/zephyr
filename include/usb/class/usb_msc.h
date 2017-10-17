/* usb_msc.h - USB MSC public header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on mass_storage.h
 *
 * This file are derived from material that is
 * Copyright (c) 2010-2011 mbed.org, MIT License
 * Copyright (c) 2016 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


/**
 * @file
 * @brief USB Mass Storage Class public header
 *
 * Header follows the Mass Storage Class Specification
 * (Mass_Storage_Specification_Overview_v1.4_2-19-2010.pdf) and
 * Mass Storage Class Bulk-Only Transport Specification
 * (usbmassbulk_10.pdf).
 * Header is limited to Bulk-Only Transfer protocol.
 */

#ifndef __USB_MSC_H__
#define __USB_MSC_H__

/** MSC Class, Subclass and Protocol Codes */
#define MASS_STORAGE_CLASS		0x08
#define SCSI_TRANSPARENT_SUBCLASS	0x06
#define BULK_ONLY_TRANSPORT_PROTOCOL	0x50

/** MSC Request Codes for Bulk-Only Transport */
#define MSC_REQUEST_GET_MAX_LUN		0xFE
#define MSC_REQUEST_RESET		0xFF

/** MSC Command Block Wrapper (CBW) Signature */
#define CBW_Signature			0x43425355

/** MSC Command Block Wrapper Flags */
#define CBW_DIRECTION_DATA_IN		0x80

/** MSC Bulk-Only Command Block Wrapper (CBW) */
struct CBW {
	u32_t Signature;
	u32_t Tag;
	u32_t DataLength;
	u8_t  Flags;
	u8_t  LUN;
	u8_t  CBLength;
	u8_t  CB[16];
} __packed;

/** MSC Command Status Wrapper (CBW) Signature */
#define CSW_Signature			0x53425355

/** MSC Command Block Status Values */
#define CSW_STATUS_CMD_PASSED		0x00
#define CSW_STATUS_CMD_FAILED		0x01
#define CSW_STATUS_PHASE_ERROR		0x02

/** MSC Bulk-Only Command Status Wrapper (CSW) */
struct CSW {
	u32_t Signature;
	u32_t Tag;
	u32_t DataResidue;
	u8_t  Status;
} __packed;

/** SCSI transparent command set used by MSC */
#define TEST_UNIT_READY			0x00
#define REQUEST_SENSE			0x03
#define FORMAT_UNIT			0x04
#define INQUIRY				0x12
#define MODE_SELECT6			0x15
#define MODE_SENSE6			0x1A
#define START_STOP_UNIT			0x1B
#define MEDIA_REMOVAL			0x1E
#define READ_FORMAT_CAPACITIES		0x23
#define READ_CAPACITY			0x25
#define READ10				0x28
#define WRITE10				0x2A
#define VERIFY10			0x2F
#define READ12				0xA8
#define WRITE12				0xAA
#define MODE_SELECT10			0x55
#define MODE_SENSE10			0x5A

#endif /* __USB_MSC_H__ */
