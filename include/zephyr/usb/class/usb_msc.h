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

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_MSC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_MSC_H_

/**
 * @name MSC Subclass and Protocol Codes
 * @{
 */
#define SCSI_TRANSPARENT_SUBCLASS	0x06	/**< SCSI transparent command set sub-class */
#define BULK_ONLY_TRANSPORT_PROTOCOL	0x50	/**< Bulk-only transport protocol */
/** @} */

/**
 * @name MSC Request Codes for Bulk-Only Transport
 * @{
 */
#define MSC_REQUEST_GET_MAX_LUN		0xFE	/**< Get Max LUN */
#define MSC_REQUEST_RESET		0xFF	/**< Bulk-Only Mass Storage Reset */
/** @} */

/** MSC Command Block Wrapper (CBW) Signature */
#define CBW_Signature			0x43425355

/**
 * @name MSC Command Block Wrapper Flags
 * @{
 */
#define CBW_DIRECTION_DATA_IN		0x80	/**<  Data-In from the device to the host */w
/** @} */

/** MSC Bulk-Only Command Block Wrapper (CBW) */
struct CBW {
	/**
	 * Signature.
	 *
	 * Shall be set to @ref CBW_Signature
	 */
	uint32_t Signature;
	/**
	 * Tag.
	 */
	uint32_t Tag;
	/**
	 * Data Transfer Length.
	 *
	 * Number of bytes of data that host expects to transfer.
	 */
	uint32_t DataLength;
	/**
	 * Flags.
	 *
	 * - Bit 7: Data transfer direction
	 *   - 0: Data-Out from host to device
	 *   - 1: Data-In from device to host
	 * - Bit 6: Obsolete, shall be set to zero
	 * - Bits 5..0: Reserved
	 */
	uint8_t  Flags;
	/**
	 * Logical Unit Number.
	 */
	uint8_t  LUN;
	/**
	 * Command Block Length.
	 */
	uint8_t  CBLength;
	/**
	 * Command Block to be executed.
	 */
	uint8_t  CB[16];
} __packed;

/** MSC Command Status Wrapper (CSW) Signature */
#define CSW_Signature			0x53425355

/**
 * @name MSC Command Block Status Values
 * @{
 */
#define CSW_STATUS_CMD_PASSED		0x00  /**< Command passed */
#define CSW_STATUS_CMD_FAILED		0x01  /**< Command failed */
#define CSW_STATUS_PHASE_ERROR		0x02  /**< Phase error */
/** @} */

/**
 * MSC Bulk-Only Command Status Wrapper (CSW)
 */
struct CSW {
	/**
	 * Signature.
	 *
	 * Shall be set to @ref CSW_Signature
	 */
	uint32_t Signature;
	/**
	 * Tag.
	 *
	 * Shall be set to the value received in the CBW
	 */
	uint32_t Tag;
	/**
	 * Data Residue.
	 *
	 * Difference between the amount of data expected and the amount of data
	 * processed
	 */
	uint32_t DataResidue;
	/**
	 * Status.
	 * - 0x00: Command passed
	 * - 0x01: Command failed
	 * - 0x02: Phase error
	 */
	uint8_t  Status;	/**< status code */
} __packed;

/**
 * @name SCSI transparent command set used by MSC
 * @{
 */
#define TEST_UNIT_READY			0x00	/**< Test unit ready */
#define REQUEST_SENSE			0x03	/**< Request sense */
#define FORMAT_UNIT			0x04	/**< Format unit */
#define INQUIRY				0x12	/**< Inquiry */
#define MODE_SELECT6			0x15	/**< Mode select (6) */
#define MODE_SENSE6			0x1A	/**< Mode sense (6) */
#define START_STOP_UNIT			0x1B	/**< Start stop unit */
#define MEDIA_REMOVAL			0x1E	/**< Media removal */
#define READ_FORMAT_CAPACITIES		0x23	/**< Read format capacities */
#define READ_CAPACITY			0x25	/**< Read capacity */
#define READ10				0x28	/**< Read (10) */
#define WRITE10				0x2A	/**< Write (10) */
#define VERIFY10			0x2F	/**< Verify (10) */
#define READ12				0xA8	/**< Read (12) */
#define WRITE12				0xAA	/**< Write (12) */
#define MODE_SELECT10			0x55	/**< Mode select (10) */
#define MODE_SENSE10			0x5A	/**< Mode sense (10) */
/** @} */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_MSC_H_ */
