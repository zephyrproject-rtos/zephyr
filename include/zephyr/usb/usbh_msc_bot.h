/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host Mass Storage Bulk-Only Transport (CBW / CSW) layout
 *
 * Wire formats match MSC BOT CBW/CSW (USB Mass Storage Bulk-Only Transport).
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USB_USBH_MSC_BOT_H_
#define ZEPHYR_INCLUDE_USB_USBH_MSC_BOT_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbh_msc_scsi_cmd.h>
#include <zephyr/usb/usbh_msc_transport_errno.h>

#if defined(CONFIG_USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS)
/** BOT synchronous bulk phase timeout from Kconfig (milliseconds). */
#define USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS CONFIG_USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS
#else
/** Default BOT synchronous bulk phase timeout (milliseconds). */
#define USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS 5000U
#endif

/** Backward-compatible alias for @ref USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS. */
#define MSC_BOT_CBW_TRANSFER_TIMEOUT_MS USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS

#if defined(CONFIG_USBH_MSC_BOT_POST_CBW_DELAY_US)
/** Optional post-CBW delay from Kconfig (microseconds). */
#define USBH_MSC_BOT_POST_CBW_DELAY_US CONFIG_USBH_MSC_BOT_POST_CBW_DELAY_US
#else
/** Default post-CBW delay: none. */
#define USBH_MSC_BOT_POST_CBW_DELAY_US 0U
#endif

/** BOT bulk-only class request: reset (MSC BOT section 5.3.4). */
#define USBH_MSC_BOT_REQ_RESET 0xFFU

/** Backward-compatible aliases for CSW-derived transport statuses. */
#define USBH_MSC_BOT_ERR_CSW_COMMAND_FAILED USBH_MSC_TRANSPORT_ERR_CSW_COMMAND_FAILED
#define USBH_MSC_BOT_ERR_CSW_PHASE_ERROR    USBH_MSC_TRANSPORT_ERR_CSW_PHASE_ERROR

/**
 * @brief MSC Bulk-Only Command Block Wrapper (CBW).
 *
 * Wire layout per USB Mass Storage Bulk-Only Transport specification.
 */
struct usb_msc_bot_cbw {
	/** Signature; must be @c USB_MSC_BOT_CBW_SIGNATURE. */
	uint32_t dCBWSignature;
	/** Tag echoed in the matching CSW. */
	uint32_t dCBWTag;
	/** Expected bulk DATA transfer length in bytes. */
	uint32_t dCBWDataTransferLength;
	/** Direction bit in bmCBWFlags (0 = OUT, @c USB_MSC_BOT_CBW_FLAG_DATA_IN = IN). */
	uint8_t bmCBWFlags;
	/** Target logical unit (lower 4 bits). */
	uint8_t bCBWLUN;
	/** Valid CDB length in bytes (1–16). */
	uint8_t bCBWCBLength;
	/** SCSI command descriptor block. */
	uint8_t CBWCB[16];
} __packed;

BUILD_ASSERT(sizeof(struct usb_msc_bot_cbw) == 31U);
BUILD_ASSERT(__alignof__(struct usb_msc_bot_cbw) == 1U);

#define USB_MSC_BOT_CBW_SIGNATURE 0x43425355U

/** bmCBWFlags direction bit: bulk DATA IN expected. */
#define USB_MSC_BOT_CBW_FLAG_DATA_IN 0x80U

/**
 * @brief MSC Bulk-Only Command Status Wrapper (CSW).
 */
struct usb_msc_bot_csw {
	/** Signature; must be @c USB_MSC_BOT_CSW_SIGNATURE. */
	uint32_t dCSWSignature;
	/** Tag copied from the matching CBW. */
	uint32_t dCSWTag;
	/** Difference between expected and actual DATA bytes transferred. */
	uint32_t dCSWDataResidue;
	/** Command completion status (@c USB_MSC_BOT_CSW_STATUS_*). */
	uint8_t bCSWStatus;
} __packed;

BUILD_ASSERT(sizeof(struct usb_msc_bot_csw) == 13U);

#define USB_MSC_BOT_CSW_SIGNATURE 0x53425355U

/** @name CSW bCSWStatus values */
/**@{*/
#define USB_MSC_BOT_CSW_STATUS_GOOD        0x00U
#define USB_MSC_BOT_CSW_STATUS_FAILED      0x01U
#define USB_MSC_BOT_CSW_STATUS_PHASE_ERROR 0x02U
/**@}*/

/** Default CBW tag for INQUIRY (BOT layer). */
#define USB_MSC_BOT_CBW_INQUIRY_TAG_LE     0x00000001U
/** @name INQUIRY CDB constants — aliases to @ref usbh_msc_scsi_cmd.h */
/**@{*/
#define USB_MSC_BOT_CBW_INQUIRY_DATALEN_LE USB_SCSI_INQUIRY_DATA_LEN
#define USB_MSC_BOT_CBW_INQUIRY_CB_LEN     USB_SCSI_INQUIRY_CDB_LEN
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B0     USB_SCSI_INQUIRY_OPCODE
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B1     0x00U
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B2     0x00U
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B3     0x00U
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B4     USB_SCSI_INQUIRY_ALLOC_LEN_STD
#define USB_MSC_BOT_CBW_INQUIRY_CDB_B5     0x00U
/**@}*/

/**
 * @brief Fill MSC BOT CBW for SCSI READ(10) per SBC READ(10) CDB layout.
 *
 * @param cbw CBW structure to initialize
 * @param tag CBW tag echoed in CSW
 * @param lun Target logical unit (0–15)
 * @param lba Starting logical block address
 * @param blocks Number of logical blocks to read
 * @param block_size Logical block size in bytes
 */
static inline void usb_msc_bot_fill_read10_cbw(struct usb_msc_bot_cbw *cbw, uint32_t tag,
					       uint8_t lun, uint32_t lba, uint16_t blocks,
					       uint32_t block_size)
{
	uint8_t *cdb;

	(void)memset(cbw, 0, sizeof(*cbw));
	cbw->dCBWSignature = sys_cpu_to_le32(USB_MSC_BOT_CBW_SIGNATURE);
	cbw->dCBWTag = sys_cpu_to_le32(tag);
	cbw->dCBWDataTransferLength = sys_cpu_to_le32((uint32_t)blocks * block_size);
	cbw->bmCBWFlags = USB_MSC_BOT_CBW_FLAG_DATA_IN;
	cbw->bCBWLUN = lun & 0x0FU;
	cbw->bCBWCBLength = 10U;

	cdb = cbw->CBWCB;
	cdb[0] = USB_SCSI_READ10;
	cdb[1] = 0U;
	sys_put_be32(lba, &cdb[2]);
	sys_put_be16(blocks, &cdb[7]);
}

/**
 * @brief Fill MSC BOT CBW for SCSI WRITE(10) (bulk DATA OUT).
 *
 * Same layout as @ref usb_msc_bot_fill_read10_cbw except direction is OUT.
 *
 * @param cbw CBW structure to initialize
 * @param tag CBW tag echoed in CSW
 * @param lun Target logical unit (0–15)
 * @param lba Starting logical block address
 * @param blocks Number of logical blocks to write
 * @param block_size Logical block size in bytes
 */
static inline void usb_msc_bot_fill_write10_cbw(struct usb_msc_bot_cbw *cbw, uint32_t tag,
						uint8_t lun, uint32_t lba, uint16_t blocks,
						uint32_t block_size)
{
	uint8_t *cdb;

	(void)memset(cbw, 0, sizeof(*cbw));
	cbw->dCBWSignature = sys_cpu_to_le32(USB_MSC_BOT_CBW_SIGNATURE);
	cbw->dCBWTag = sys_cpu_to_le32(tag);
	cbw->dCBWDataTransferLength = sys_cpu_to_le32((uint32_t)blocks * block_size);
	cbw->bmCBWFlags = 0U;
	cbw->bCBWLUN = lun & 0x0FU;
	cbw->bCBWCBLength = 10U;

	cdb = cbw->CBWCB;
	cdb[0] = USB_SCSI_WRITE10;
	cdb[1] = 0U;
	sys_put_be32(lba, &cdb[2]);
	sys_put_be16(blocks, &cdb[7]);
}

/**
 * @brief Fill MSC BOT CBW for SCSI VERIFY(10), BYTCHK=0 — no DATA phase.
 *
 * Sets @c dCBWDataTransferLength = 0 for a command-only BOT transaction.
 *
 * @param cbw CBW structure to initialize
 * @param tag CBW tag echoed in CSW
 * @param lun Target logical unit (0–15)
 * @param lba Starting logical block address
 * @param blocks Number of logical blocks to verify
 */
static inline void usb_msc_bot_fill_verify10_cbw(struct usb_msc_bot_cbw *cbw, uint32_t tag,
						 uint8_t lun, uint32_t lba, uint16_t blocks)
{
	uint8_t *cdb;

	(void)memset(cbw, 0, sizeof(*cbw));
	cbw->dCBWSignature = sys_cpu_to_le32(USB_MSC_BOT_CBW_SIGNATURE);
	cbw->dCBWTag = sys_cpu_to_le32(tag);
	cbw->dCBWDataTransferLength = sys_cpu_to_le32(0U);
	cbw->bmCBWFlags = 0U;
	cbw->bCBWLUN = lun & 0x0FU;
	cbw->bCBWCBLength = USB_SCSI_VERIFY10_CDB_LEN;

	cdb = cbw->CBWCB;
	cdb[0] = USB_SCSI_VERIFY10;
	cdb[1] = 0U;
	sys_put_be32(lba, &cdb[2]);
	sys_put_be16(blocks, &cdb[7]);
}

#endif /* ZEPHYR_INCLUDE_USB_USBH_MSC_BOT_H_ */
