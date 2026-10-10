/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Test and Measurement Class (USBTMC) protocol definitions
 *
 * This file contains the USB Test and Measurement Class definitions and
 * follows the USBTMC Specification Revision 1.0.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_

#include <stdint.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/**
 * @brief USBTMC protocol definitions
 * @defgroup usb_tmc USBTMC protocol definitions
 * @ingroup usb
 * @{
 */

/**
 * @name Interface subclass and protocol codes (USBTMC 1.0, Table 43 and Table 44)
 * @{
 */
/** USBTMC interface subclass code */
#define USBTMC_SUBCLASS				0x03
/** Interface protocol code for USBTMC interfaces without a subclass */
#define USBTMC_PROTOCOL_USBTMC			0x00
/** Interface protocol code for USB488 interfaces */
#define USBTMC_PROTOCOL_USB488			0x01
/** @} */

/** USBTMC Specification Release Number 1.0 in BCD format */
#define USBTMC_BCD_1_0				0x0100

/**
 * @name Bulk-OUT and Bulk-IN message identifiers (USBTMC 1.0, Table 2)
 * @{
 */
/** Device dependent command message */
#define USBTMC_MSGID_DEV_DEP_MSG_OUT		1
/** Command message that requests the device to send a response message */
#define USBTMC_MSGID_REQUEST_DEV_DEP_MSG_IN	2
/** Device dependent response message */
#define USBTMC_MSGID_DEV_DEP_MSG_IN		2
/** Vendor specific command message */
#define USBTMC_MSGID_VENDOR_SPECIFIC_OUT	126
/** Command message that requests the device to send a vendor specific response */
#define USBTMC_MSGID_REQUEST_VENDOR_SPECIFIC_IN	127
/** Vendor specific response message */
#define USBTMC_MSGID_VENDOR_SPECIFIC_IN		127
/** @} */

/**
 * @name bmTransferAttributes bits (USBTMC 1.0, Table 3, Table 4, and Table 9)
 * @{
 */
/** Transfer ends with the last byte of the message */
#define USBTMC_TRANSFER_ATTRIB_EOM		BIT(0)
/** Transfer must terminate on the termination character */
#define USBTMC_TRANSFER_ATTRIB_TERM_CHAR	BIT(1)
/** @} */

/**
 * @brief Bulk-OUT and Bulk-IN message header
 *
 * The message header is always 12 bytes long and placed at the beginning of
 * the first transaction of a transfer, the TransferSize fields are in
 * little-endian format. See USBTMC 1.0, Table 1 and Table 8.
 */
struct usbtmc_msg_header {
	/** Message identifier */
	uint8_t MsgID;
	/** Transfer identifier, 1 to 255 */
	uint8_t bTag;
	/** One's complement of bTag */
	uint8_t bTagInverse;
	/** Reserved, must be zero */
	uint8_t reserved;
	/** Message identifier specific part of the header */
	union {
		/** DEV_DEP_MSG_OUT command specific content, Table 3 */
		struct {
			/** Number of message data bytes in the transfer */
			uint32_t TransferSize;
			/** Transfer attributes, EOM bit only */
			uint8_t bmTransferAttributes;
			/** Reserved, must be zero */
			uint8_t reserved[3];
		} __packed dev_dep_msg_out;
		/** REQUEST_DEV_DEP_MSG_IN command specific content, Table 4 */
		struct {
			/** Maximum number of message data bytes to be sent */
			uint32_t TransferSize;
			/** Transfer attributes, TermChar enable bit only */
			uint8_t bmTransferAttributes;
			/** Termination character */
			uint8_t TermChar;
			/** Reserved, must be zero */
			uint8_t reserved[2];
		} __packed request_dev_dep_msg_in;
		/** DEV_DEP_MSG_IN response specific content, Table 9 */
		struct {
			/** Number of message data bytes in the transfer */
			uint32_t TransferSize;
			/** Transfer attributes, EOM and TermChar match bits */
			uint8_t bmTransferAttributes;
			/** Reserved, must be zero */
			uint8_t reserved[3];
		} __packed dev_dep_msg_in;
	};
} __packed;

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct usbtmc_msg_header) == 12);
/** @endcond */

/**
 * @brief Bulk-OUT transaction alignment
 *
 * Bulk-OUT transactions are required to be a multiple of four bytes long,
 * the host adds up to three alignment bytes to the last transaction of a
 * transfer, USBTMC 1.0, 3.2. There is no such requirement for Bulk-IN
 * transactions.
 */
#define USBTMC_ALIGNMENT			4

/**
 * @name Class specific request codes (USBTMC 1.0, Table 15)
 * @{
 */
/** Abort a Bulk-OUT transfer */
#define USBTMC_REQ_INITIATE_ABORT_BULK_OUT	1
/** Return the status of a Bulk-OUT transfer abort */
#define USBTMC_REQ_CHECK_ABORT_BULK_OUT_STATUS	2
/** Abort a Bulk-IN transfer */
#define USBTMC_REQ_INITIATE_ABORT_BULK_IN	3
/** Return the status of a Bulk-IN transfer abort */
#define USBTMC_REQ_CHECK_ABORT_BULK_IN_STATUS	4
/** Clear all input and output buffers */
#define USBTMC_REQ_INITIATE_CLEAR		5
/** Return the status of an interface clear */
#define USBTMC_REQ_CHECK_CLEAR_STATUS		6
/** Return the interface and device capabilities */
#define USBTMC_REQ_GET_CAPABILITIES		7
/** Turn on an activity indicator for identification */
#define USBTMC_REQ_INDICATOR_PULSE		64
/** @} */

/**
 * @name USBTMC_status values (USBTMC 1.0, Table 16)
 * @{
 */
/** Success */
#define USBTMC_STATUS_SUCCESS			0x01
/** Split transaction is still being processed */
#define USBTMC_STATUS_PENDING			0x02
/** Failure, unspecified reason */
#define USBTMC_STATUS_FAILED			0x80
/** The specified transfer is not in progress */
#define USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS	0x81
/** No split transaction is in progress */
#define USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS	0x82
/** A split transaction is already in progress */
#define USBTMC_STATUS_SPLIT_IN_PROGRESS		0x83
/** @} */

/**
 * @brief INITIATE_ABORT_BULK_OUT and INITIATE_ABORT_BULK_IN response
 *
 * See USBTMC 1.0, Table 19 and Table 25.
 */
struct usbtmc_initiate_abort_response {
	/** Status of the request */
	uint8_t USBTMC_status;
	/** bTag of the transfer in progress or the most recent transfer */
	uint8_t bTag;
} __packed;

/**
 * @brief CHECK_ABORT_BULK_OUT_STATUS response
 *
 * See USBTMC 1.0, Table 22.
 */
struct usbtmc_check_abort_bulk_out_response {
	/** Status of the abort */
	uint8_t USBTMC_status;
	/** Reserved, must be zero */
	uint8_t reserved[3];
	/** Number of message data bytes received and not discarded */
	uint32_t NBYTES_RXD;
} __packed;

/**
 * @brief CHECK_ABORT_BULK_IN_STATUS response
 *
 * See USBTMC 1.0, Table 28.
 */
struct usbtmc_check_abort_bulk_in_response {
	/** Status of the abort */
	uint8_t USBTMC_status;
	/** Bulk-IN FIFO status, BulkInFifoBytes bit only */
	uint8_t bmAbortBulkIn;
	/** Reserved, must be zero */
	uint8_t reserved[2];
	/** Number of message data bytes sent in the transfer */
	uint32_t NBYTES_TXD;
} __packed;

/**
 * @brief CHECK_CLEAR_STATUS response
 *
 * See USBTMC 1.0, Table 34.
 */
struct usbtmc_check_clear_response {
	/** Status of the clear */
	uint8_t USBTMC_status;
	/** Bulk-IN FIFO status, BulkInFifoBytes bit only */
	uint8_t bmClear;
} __packed;

/**
 * @brief BulkInFifoBytes bit
 *
 * Set when device dependent message data bytes are queued on the Bulk-IN
 * endpoint, valid for both the bmAbortBulkIn and bmClear fields, USBTMC 1.0,
 * Table 28 and Table 34.
 */
#define USBTMC_BULK_IN_FIFO_BYTES		BIT(0)

/**
 * @brief GET_CAPABILITIES response
 *
 * See USBTMC 1.0, Table 37.
 */
struct usbtmc_capabilities {
	/** Status of the request */
	uint8_t USBTMC_status;
	/** Reserved, must be zero */
	uint8_t reserved0;
	/** USBTMC Specification Release Number in BCD format */
	uint16_t bcdUSBTMC;
	/** USBTMC interface capabilities */
	uint8_t bmInterfaceCapabilities;
	/** USBTMC device capabilities */
	uint8_t bmDeviceCapabilities;
	/** Reserved, must be zero */
	uint8_t reserved1[6];
	/** Reserved for use by USBTMC subclass specifications */
	uint8_t reserved2[12];
} __packed;

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct usbtmc_capabilities) == 24);
/** @endcond */

/**
 * @name USBTMC interface capabilities bits (USBTMC 1.0, Table 37)
 * @{
 */
/** The interface accepts the INDICATOR_PULSE request */
#define USBTMC_INTF_CAP_INDICATOR_PULSE		BIT(2)
/** The interface is talk-only */
#define USBTMC_INTF_CAP_TALK_ONLY		BIT(1)
/** The interface is listen-only */
#define USBTMC_INTF_CAP_LISTEN_ONLY		BIT(0)
/** @} */

/**
 * @name USBTMC device capabilities bits (USBTMC 1.0, Table 37)
 * @{
 */
/** The device supports ending a Bulk-IN transfer on the termination character */
#define USBTMC_DEV_CAP_TERM_CHAR		BIT(0)
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_TMC_H_ */
