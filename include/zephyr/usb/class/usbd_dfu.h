/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Device Firmware Upgrade (DFU) public header
 *
 * Header exposes API for registering DFU images.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_DFU_H
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_DFU_H

#include <stdint.h>

/* DFU Class Subclass */
#define USB_DFU_SUBCLASS		0x01

/* DFU Class runtime Protocol */
#define USB_DFU_PROTOCOL_RUNTIME	0x01

/* DFU Class DFU mode Protocol */
#define USB_DFU_PROTOCOL_DFU		0x02

/* DFU Class Specific Requests */
#define USB_DFU_REQ_DETACH			0x00
#define USB_DFU_REQ_DNLOAD			0x01
#define USB_DFU_REQ_UPLOAD			0x02
#define USB_DFU_REQ_GETSTATUS			0x03
#define USB_DFU_REQ_CLRSTATUS			0x04
#define USB_DFU_REQ_GETSTATE			0x05
#define USB_DFU_REQ_ABORT			0x06

/* Run-Time DFU Functional Descriptor */
struct usb_dfu_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bmAttributes;
	uint16_t wDetachTimeOut;
	uint16_t wTransferSize;
	uint16_t bcdDFUVersion;
} __packed;

/* DFU Functional Descriptor Type */
#define USB_DESC_DFU_FUNCTIONAL			0x21

/* DFU attributes DFU Functional Descriptor */
#define USB_DFU_ATTR_WILL_DETACH		BIT(3)
#define USB_DFU_ATTR_MANIFESTATION_TOLERANT	BIT(2)
#define USB_DFU_ATTR_CAN_UPLOAD			BIT(1)
#define USB_DFU_ATTR_CAN_DNLOAD			BIT(0)

/* DFU Specification release */
#define USB_DFU_VERSION				0x0110

/* DFU device status */
enum usb_dfu_status {
	ERR_OK			= 0x00,
	ERR_TARGET		= 0x01,
	ERR_FILE		= 0x02,
	ERR_WRITE		= 0x03,
	ERR_ERASE		= 0x04,
	ERR_CHECK_ERASED	= 0x05,
	ERR_PROG		= 0x06,
	ERR_VERIFY		= 0x07,
	ERR_ADDRESS		= 0x08,
	ERR_NOTDONE		= 0x09,
	ERR_FIRMWARE		= 0x0A,
	ERR_VENDOR		= 0x0B,
	ERR_USBR		= 0x0C,
	ERR_POR			= 0x0D,
	ERR_UNKNOWN		= 0x0E,
	ERR_STALLEDPKT		= 0x0F,
};

/* DFU device states */
enum usb_dfu_state {
	APP_IDLE		= 0,
	APP_DETACH		= 1,
	DFU_IDLE		= 2,
	DFU_DNLOAD_SYNC		= 3,
	DFU_DNBUSY		= 4,
	DFU_DNLOAD_IDLE		= 5,
	DFU_MANIFEST_SYNC	= 6,
	DFU_MANIFEST		= 7,
	DFU_MANIFEST_WAIT_RST	= 8,
	DFU_UPLOAD_IDLE		= 9,
	DFU_ERROR		= 10,
	DFU_STATE_MAX		= 11,
};

struct usbd_dfu_image {
	const char *name;
	struct usb_if_descriptor *const if_desc;
	void *const priv;
	struct usbd_desc_node *const sd_nd;
	bool (*next_cb)(void *const priv,
			const enum usb_dfu_state state, const enum usb_dfu_state next);
	int (*read_cb)(void *const priv,
		       const uint32_t block, const uint16_t size,
		       uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE]);
	int (*write_cb)(void *const priv,
			const uint32_t block, const uint16_t size,
			const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE]);
};

/**
 * @brief USB DFU device update API
 * @defgroup usbd_dfu USB DFU device update API
 * @ingroup usb
 * @since 4.1
 * @version 0.1.0
 * @{
 */

/**
 * @brief Define USB DFU image
 *
 * Use this macro to create USB DFU image
 *
 * The callbacks must be in form:
 *
 * @code{.c}
 * static int read(void *const priv, const uint32_t block, const uint16_t size,
 *                 uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
 * {
 *         int len;
 *
 *         return len;
 * }
 *
 * static int write(void *const priv, const uint32_t block, const uint16_t size,
 *                  const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
 * {
 *         return 0;
 * }
 *
 * static bool next(void *const priv,
 *                  const enum usb_dfu_state state, const enum usb_dfu_state next)
 * {
 *         return true;
 * }
 * @endcode
 *
 * @param id     Identifier by which the linker sorts registered images
 * @param iname  Image name as used in interface descriptor
 * @param iread  Image read callback
 * @param iwrite Image write callback
 * @param inext  Notify/confirm next state
 */
#define USBD_DFU_DEFINE_IMG(id, iname, ipriv, iread, iwrite, inext)			\
	static __noinit struct usb_if_descriptor usbd_dfu_iface_##id;			\
											\
	USBD_DESC_STRING_DEFINE(usbd_dfu_str_##id, iname, USBD_DUT_STRING_INTERFACE);	\
											\
	static const STRUCT_SECTION_ITERABLE(usbd_dfu_image, usbd_dfu_image_##id) = {	\
		.name = iname,								\
		.if_desc = &usbd_dfu_iface_##id,					\
		.priv = ipriv,								\
		.sd_nd = &usbd_dfu_str_##id,						\
		.read_cb = iread,							\
		.write_cb = iwrite,							\
		.next_cb = inext,							\
	}

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_DFU_H */
