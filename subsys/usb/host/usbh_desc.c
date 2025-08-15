/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_desc, CONFIG_USBH_LOG_LEVEL);

const bool usbh_desc_size_is_valid(const void *const desc_ptr, size_t expected_size,
				   const void *const desc_end)
{
	const struct usb_desc_header *desc = desc_ptr;

	/* Block invalid input */
	if (desc == NULL || desc_end == NULL || expected_size == 0) {
		LOG_DBG("Invalid input: beg %p, end %p, size %u", desc, desc_end, expected_size);
		return false;
	}

	/* Avoid out of order pointers */
	if ((uint8_t *)desc > (uint8_t *)desc_end) {
		LOG_DBG("Invalid order: beg %p > end %p", desc, desc_end);
		return false;
	}

	/* Avoid invalid descriptor size */
	if (expected_size < sizeof(*desc)) {
		LOG_DBG("Invalid descriptor size: %u < %u", expected_size < sizeof(*desc));
		return false;
	}

	/* Avoid too short descriptors */
	if ((uint8_t *)desc + expected_size > (uint8_t *)desc_end) {
		LOG_DBG("Descriptor too short: %p + %u > %p", desc, expected_size, desc_end);
		return false;
	}

	/* Avoid too short bLength field */
	if (desc->bLength < expected_size) {
		LOG_DBG("Descriptor size (%u) less than %u bytes", desc->bLength, expected_size);
		return false;
	}

	return true;
}

const void *usbh_desc_get_next(const void *const desc_beg, const void *const desc_end)
{
	const struct usb_desc_header *const desc = desc_beg;
	const struct usb_desc_header *next;

	if (!usbh_desc_size_is_valid(desc, sizeof(const struct usb_desc_header), desc_end)) {
		return NULL;
	}

	next = (const struct usb_desc_header *)((uint8_t *)desc + desc->bLength);

	if (!usbh_desc_size_is_valid(next, sizeof(const struct usb_desc_header), desc_end)) {
		return NULL;
	}

	return next;
}

static const void *usbh_desc_get_next_iad_or_iface(const void *const desc_beg,
						   const void *const desc_end)
{
	const struct usb_desc_header *desc = desc_beg;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE ||
	    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		desc = usbh_desc_get_next(desc, desc_end);
	}

	for (; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			return desc;
		}
	}

	return NULL;
}

const void *usbh_desc_get_by_iface(const void *const desc_beg, const void *const desc_end,
				   const uint8_t iface)
{
	const struct usb_desc_header *desc;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	for (desc = desc_beg; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE &&
		    ((struct usb_if_descriptor *)desc)->bInterfaceNumber == iface) {
			return desc;
		}
		if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC &&
		    ((struct usb_association_descriptor *)desc)->bFirstInterface == iface) {
			return desc;
		}
	}

	return NULL;
}

const void *usbh_desc_get_cfg_beg(const struct usb_device *udev)
{
	return udev->cfg_desc;
}

const void *usbh_desc_get_cfg_end(const struct usb_device *udev)
{
	const struct usb_cfg_descriptor *cfg_desc;

	if (udev->cfg_desc == NULL) {
		return NULL;
	}

	cfg_desc = (struct usb_cfg_descriptor *)udev->cfg_desc;

	return (uint8_t *)cfg_desc + cfg_desc->wTotalLength;
}

const int usbh_desc_get_iface_info(const struct usb_desc_header *desc,
				   struct usbh_class_filter *const iface_code,
				   uint8_t *const iface_num)
{
	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		const struct usb_association_descriptor *iad_desc =
			(const struct usb_association_descriptor *)desc;

		iface_code->class = iad_desc->bFunctionClass;
		iface_code->sub = iad_desc->bFunctionSubClass;
		iface_code->proto = iad_desc->bFunctionProtocol;
		if (iface_num != NULL) {
			*iface_num = iad_desc->bFirstInterface;
		}
		return 0;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE) {
		struct usb_if_descriptor *if_desc = (void *)desc;

		iface_code->class = if_desc->bInterfaceClass;
		iface_code->sub = if_desc->bInterfaceSubClass;
		iface_code->proto = if_desc->bInterfaceProtocol;
		if (iface_num != NULL) {
			*iface_num = if_desc->bInterfaceNumber;
		}
		return 0;
	}

	return -EINVAL;
}

const void *usbh_desc_get_next_function(const void *const desc_beg, const void *const desc_end)
{
	const struct usb_desc_header *desc = desc_beg;
	const struct usb_association_descriptor *iad_desc = desc_beg;
	const struct usb_if_descriptor *if_desc = desc_beg;
	size_t skip_num;
	int8_t iface;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		iface = iad_desc->bFirstInterface;
		skip_num = iad_desc->bInterfaceCount;
	} else if (desc->bDescriptorType == USB_DESC_INTERFACE) {
		iface = if_desc->bInterfaceNumber;
		skip_num = 1;
	} else {
		return usbh_desc_get_next_iad_or_iface(desc, desc_end);
	}

	while (skip_num > 0) {
		desc = usbh_desc_get_next_iad_or_iface(desc, desc_end);
		if (desc == NULL) {
			return NULL;
		}

		if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			/* No alternate setting */
			return desc;
		} else if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			if_desc = (struct usb_if_descriptor  *)desc;

			/* Skip all the alternate settings for the same interface number */
			if (if_desc->bInterfaceNumber != iface) {
				iface = if_desc->bInterfaceNumber;
				skip_num--;
			}
		} else {
			__ASSERT(false, "invalid implementation of usbh_desc_get_by_iface()");
			return NULL;
		}
	}

	return desc;
}
