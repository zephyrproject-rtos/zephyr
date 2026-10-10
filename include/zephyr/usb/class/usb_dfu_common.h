/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Firmware Upgrade (DFU) public header
 *
 * Header exposes DFU protocol definitions
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_DFU_COMMON_H
#define ZEPHYR_INCLUDE_USB_CLASS_USB_DFU_COMMON_H

/* DFU Class Subclass */
#define USB_DFU_SUBCLASS 0x01

/* DFU Class runtime Protocol */
#define USB_DFU_PROTOCOL_RUNTIME 0x01

/* DFU Class DFU mode Protocol */
#define USB_DFU_PROTOCOL_DFU 0x02

/* DFU Class Specific Requests */
#define USB_DFU_REQ_DETACH    0x00
#define USB_DFU_REQ_DNLOAD    0x01
#define USB_DFU_REQ_UPLOAD    0x02
#define USB_DFU_REQ_GETSTATUS 0x03
#define USB_DFU_REQ_CLRSTATUS 0x04
#define USB_DFU_REQ_GETSTATE  0x05
#define USB_DFU_REQ_ABORT     0x06

/* DFU Functional Descriptor Type */
#define USB_DESC_DFU_FUNCTIONAL 0x21

/* DFU attributes DFU Functional Descriptor */
#define USB_DFU_ATTR_WILL_DETACH            BIT(3)
#define USB_DFU_ATTR_MANIFESTATION_TOLERANT BIT(2)
#define USB_DFU_ATTR_CAN_UPLOAD             BIT(1)
#define USB_DFU_ATTR_CAN_DNLOAD             BIT(0)

/* DFU Specification release */
#define USB_DFU_VERSION 0x0110

/* DFU device status */
enum usb_dfu_status {
	ERR_OK = 0x00,
	ERR_TARGET = 0x01,
	ERR_FILE = 0x02,
	ERR_WRITE = 0x03,
	ERR_ERASE = 0x04,
	ERR_CHECK_ERASED = 0x05,
	ERR_PROG = 0x06,
	ERR_VERIFY = 0x07,
	ERR_ADDRESS = 0x08,
	ERR_NOTDONE = 0x09,
	ERR_FIRMWARE = 0x0A,
	ERR_VENDOR = 0x0B,
	ERR_USBR = 0x0C,
	ERR_POR = 0x0D,
	ERR_UNKNOWN = 0x0E,
	ERR_STALLEDPKT = 0x0F,
};

/* DFU device states */
enum usb_dfu_state {
	APP_IDLE = 0,
	APP_DETACH = 1,
	DFU_IDLE = 2,
	DFU_DNLOAD_SYNC = 3,
	DFU_DNBUSY = 4,
	DFU_DNLOAD_IDLE = 5,
	DFU_MANIFEST_SYNC = 6,
	DFU_MANIFEST = 7,
	DFU_MANIFEST_WAIT_RST = 8,
	DFU_UPLOAD_IDLE = 9,
	DFU_ERROR = 10,
	DFU_STATE_MAX = 11,
};

/* Run-Time DFU Functional Descriptor */
struct usb_dfu_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bmAttributes;
	uint16_t wDetachTimeOut;
	uint16_t wTransferSize;
	uint16_t bcdDFUVersion;
} __packed;

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_DFU_COMMON_H */
