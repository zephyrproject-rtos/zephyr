/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2026 Renesas Electronics Corporation
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
#include <zephyr/usb/class/usb_dfu_common.h>

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
 * @param ipriv  Pointer to private data passed to the callbacks
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
