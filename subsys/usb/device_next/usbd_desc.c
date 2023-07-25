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

#include "usbd_desc.h"
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

	dn->utf16le = true;
}

/**
 * @brief Get common USB descriptor
 *
 * Get descriptor from internal descriptor list.
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

static inline bool desc_type_equal(const struct usbd_desc_node *const a,
				   const struct usbd_desc_node *const b)
{
	const struct usb_desc_header *const head_a = a->desc;
	const struct usb_desc_header *const head_b = b->desc;

	return head_a->bDescriptorType == head_b->bDescriptorType;
}

/*
 * Add descriptor node to the descriptor list in ascending order by index
 * and sorted by bDescriptorType. For the string descriptors, the function
 * does not care about index zero for the language string descriptor,
 * so if it is not added first, the device will be non-compliant.
 */
static int desc_add_and_update_idx(struct usbd_contex *const uds_ctx,
				   struct usbd_desc_node *const new_nd)
{
	struct usbd_desc_node *tmp_nd;

	SYS_DLIST_FOR_EACH_CONTAINER(&uds_ctx->descriptors, tmp_nd, node) {
		struct usbd_desc_node *next_nd;

		if (!desc_type_equal(tmp_nd, new_nd)) {
			continue;
		}

		next_nd = SYS_DLIST_PEEK_NEXT_CONTAINER(&uds_ctx->descriptors,
							tmp_nd,
							node);

		if (next_nd == NULL) {
			/* Last node of the same bDescriptorType or tail */
			new_nd->idx = tmp_nd->idx + 1;
			sys_dlist_append(&uds_ctx->descriptors, &new_nd->node);
			LOG_DBG("Add %u behind %u", new_nd->idx, tmp_nd->idx);

			return 0;
		}

		if (!desc_type_equal(next_nd, new_nd)) {
			/* Last node of the same bDescriptorType */
			new_nd->idx = tmp_nd->idx + 1;
			sys_dlist_insert(&next_nd->node, &new_nd->node);
			LOG_DBG("Add %u before %u", new_nd->idx, next_nd->idx);

			return 0;
		}

		if (tmp_nd->idx != (next_nd->idx - 1)) {
			/* Add between nodes of the same bDescriptorType */
			new_nd->idx = tmp_nd->idx + 1;
			sys_dlist_insert(&next_nd->node, &new_nd->node);
			LOG_DBG("Add %u between %u and %u",
				tmp_nd->idx, next_nd->idx, new_nd->idx);
			return 0;
		}
	}

	/* If there are none of same bDescriptorType, node idx is set to 0. */
	new_nd->idx = 0;
	sys_dlist_append(&uds_ctx->descriptors, &new_nd->node);
	LOG_DBG("Added first descriptor node (usage type %u)", new_nd->utype);

	return 0;
}

void *usbd_get_descriptor(struct usbd_contex *const uds_ctx,
			  const uint8_t type, const uint8_t idx)
{
	struct usbd_desc_node *tmp;
	struct usb_desc_header *dh;

	SYS_DLIST_FOR_EACH_CONTAINER(&uds_ctx->descriptors, tmp, node) {
		dh = tmp->desc;
		if (tmp->idx == idx && dh->bDescriptorType == type) {
			return tmp->desc;
		}
	}

	return NULL;
}

int usbd_desc_remove_all(struct usbd_contex *const uds_ctx)
{
	struct usbd_desc_node *tmp;
	sys_dnode_t *node;

	while ((node = sys_dlist_get(&uds_ctx->descriptors))) {
		tmp = CONTAINER_OF(node, struct usbd_desc_node, node);
		LOG_DBG("Remove descriptor node %p", tmp);
	}

	return 0;
}

int usbd_add_descriptor(struct usbd_contex *const uds_ctx,
			struct usbd_desc_node *const desc_nd)
{
	struct usb_device_descriptor *dev_desc = uds_ctx->desc;
	struct usb_desc_header *head;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (dev_desc == NULL || usbd_is_initialized(uds_ctx)) {
		ret = -EPERM;
		goto add_descriptor_error;
	}

	/* Check if descriptor list is initialized */
	if (!sys_dnode_is_linked(&uds_ctx->descriptors)) {
		LOG_DBG("Initialize descriptors list");
		sys_dlist_init(&uds_ctx->descriptors);
	}

	head = desc_nd->desc;
	if (sys_dnode_is_linked(&desc_nd->node)) {
		ret = -EALREADY;
		goto add_descriptor_error;
	}

	ret = desc_add_and_update_idx(uds_ctx, desc_nd);
	if (ret) {
		ret = -EINVAL;
		goto add_descriptor_error;
	}

	if (head->bDescriptorType == USB_DESC_STRING) {
		switch (desc_nd->utype) {
		case USBD_DUT_STRING_LANG:
			break;
		case USBD_DUT_STRING_MANUFACTURER:
			dev_desc->iManufacturer = desc_nd->idx;
			break;
		case USBD_DUT_STRING_PRODUCT:
			dev_desc->iProduct = desc_nd->idx;
			break;
		case USBD_DUT_STRING_SERIAL_NUMBER:
			if (!desc_nd->custom_sn) {
				ret = usbd_get_sn_from_hwid(desc_nd);
				desc_nd->utf16le = false;
			}

			dev_desc->iSerialNumber = desc_nd->idx;
			break;
		default:
			break;
		}

		if (desc_nd->idx && !desc_nd->utf16le) {
			usbd_ascii7_to_utf16le(desc_nd);
		}
	}

add_descriptor_error:
	usbd_device_unlock(uds_ctx);
	return ret;
}
