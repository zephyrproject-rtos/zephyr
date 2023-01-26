/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2017, 2018 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/hwinfo.h>

#include "usbd_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_desc, CONFIG_USBD_LOG_LEVEL);

/*
 * The last index of the initializer_string without null character is:
 *   ascii_idx_max = bLength / 2 - 2
 * Use this macro to determine the last index of ASCII7 string.
 */
#define USB_BSTRING_ASCII_IDX_MAX(n)	(n / 2 - 2)

/*
 * The last index of the bString is:
 *   utf16le_idx_max = sizeof(initializer_string) * 2 - 2 - 1
 *   utf16le_idx_max = bLength - 2 - 1
 * Use this macro to determine the last index of UTF16LE string.
 */
#define USB_BSTRING_UTF16LE_IDX_MAX(n)	(n - 3)

/**
 * @brief Transform ASCII-7 string descriptor to UTF16-LE
 *
 * This function transforms ASCII-7 string descriptor
 * into a UTF16-LE.
 *
 * @param[in] dn     Pointer to descriptor node
 */
static void usbd_ascii7_to_utf16le(struct usbd_desc_node *const dn)
{
	struct usb_string_descriptor *desc = dn->desc;
	int idx_max = USB_BSTRING_UTF16LE_IDX_MAX(desc->bLength);
	int ascii_idx_max = USB_BSTRING_ASCII_IDX_MAX(desc->bLength);
	uint8_t *buf = (uint8_t *)&desc->bString;

	LOG_DBG("idx_max %d, ascii_idx_max %d, buf %p",
		idx_max, ascii_idx_max, buf);

	for (int i = idx_max; i >= 0; i -= 2) {
		LOG_DBG("char %c : %x, idx %d -> %d",
			buf[ascii_idx_max],
			buf[ascii_idx_max],
			ascii_idx_max, i);
		__ASSERT(buf[ascii_idx_max] > 0x1F && buf[ascii_idx_max] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		buf[i] = 0U;
		buf[i - 1] = buf[ascii_idx_max--];
	}
}

/**
 * @brief Get common USB descriptor
 *
 * Get descriptor from internal descrptor list.
 *
 * @param[in] dn     Pointer to descriptor node
 *
 * @return 0 on success, other values on fail.
 */
static int usbd_get_sn_from_hwid(struct usbd_desc_node *const dn)
{
	static const char hex[] = "0123456789ABCDEF";
	struct usb_string_descriptor *desc = dn->desc;
	uint8_t *desc_data = (uint8_t *)&desc->bString;
	uint8_t hwid[16];
	ssize_t hwid_len;
	ssize_t min_len;

	hwid_len = hwinfo_get_device_id(hwid, sizeof(hwid));
	if (hwid_len < 0) {
		if (hwid_len == -ENOSYS) {
			LOG_WRN("hwinfo not implemented");
			return 0;
		}

		return hwid_len;
	}

	min_len = MIN(hwid_len, desc->bLength / 2);
	for (size_t i = 0; i < min_len; i++) {
		desc_data[i * 2] = hex[hwid[i] >> 4];
		desc_data[i * 2 + 1] = hex[hwid[i] & 0xF];
	}

	LOG_HEXDUMP_DBG(&desc->bString, desc->bLength, "SerialNumber");

	return 0;
}

void *usbd_get_descriptor(struct usbd_contex *const uds_ctx,
			  const uint8_t type, const uint8_t idx)
{
	struct usbd_desc_node *tmp;
	struct usb_desc_header *dh;

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->descriptors, tmp, node) {
		dh = tmp->desc;
		if (tmp->idx == idx && dh->bDescriptorType == type) {
			return tmp->desc;
		}
	}

	return NULL;
}

int usbd_add_descriptor(struct usbd_contex *const uds_ctx,
			struct usbd_desc_node *const desc_nd)
{
	struct usb_desc_header *head;
	uint8_t type;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	head = desc_nd->desc;
	type = head->bDescriptorType;
	if (usbd_get_descriptor(uds_ctx, type, desc_nd->idx)) {
		ret = -EALREADY;
		goto add_descriptor_error;
	}

	if (type == USB_DESC_STRING) {
		struct usb_device_descriptor *dev_desc = uds_ctx->desc;

		if (dev_desc == NULL) {
			ret = -EPERM;
			goto add_descriptor_error;
		}

		switch (desc_nd->idx) {
		case USBD_DESC_MANUFACTURER_IDX:
			dev_desc->iManufacturer = desc_nd->idx;
			break;
		case USBD_DESC_PRODUCT_IDX:
			dev_desc->iProduct = desc_nd->idx;
			break;
		case USBD_DESC_SERIAL_NUMBER_IDX:
			/* FIXME, should we force the use of hwid here? */
			ret = usbd_get_sn_from_hwid(desc_nd);
			dev_desc->iSerialNumber = desc_nd->idx;
			break;
		default:
			break;
		}

		if (desc_nd->idx) {
			/* FIXME, should we force ascii7 -> utf16le? */
			usbd_ascii7_to_utf16le(desc_nd);
		}
	}

	sys_slist_append(&uds_ctx->descriptors, &desc_nd->node);

add_descriptor_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}
