/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_MSC_H_
#define ZEPHYR_INCLUDE_USBH_CLASS_MSC_H_

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>

#ifdef __cplusplus
extern "C" {
#endif

/* USB Mass Storage Class codes */
#define USB_CLASS_MASS_STORAGE    0x08
#define USB_SUBCLASS_SCSI         0x06
#define USB_PROTOCOL_BOT          0x50

/* BOT Protocol constants */
#define CBW_SIGNATURE             0x43425355
#define CSW_SIGNATURE             0x53425355
#define CBW_FLAGS_DATA_IN         0x80
#define CBW_FLAGS_DATA_OUT        0x00

/* Command Status codes */
#define CSW_STATUS_PASSED         0x00
#define CSW_STATUS_FAILED         0x01
#define CSW_STATUS_PHASE_ERROR    0x02

/* Transfer parameters */
#define USB_MSC_TIMEOUT_MS        5000
#define MAX_RETRY_COUNT           3

/**
 * @brief MSC device state enumeration
 */
enum msc_device_state {
	MSC_STATE_DISCONNECTED,
	MSC_STATE_CONNECTED,
	MSC_STATE_INITIALIZING,
	MSC_STATE_READY,
	MSC_STATE_ERROR
};

/**
 * @brief Command Block Wrapper (CBW) structure
 */
struct cbw {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} __packed;

/**
 * @brief Command Status Wrapper (CSW) structure
 */
struct csw {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_MSC_H_ */
