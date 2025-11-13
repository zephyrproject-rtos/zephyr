/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usb_ch9.h>

#include "usbh_class.h"
#include "usbh_desc.h"

const void *usbh_desc_get_next(const void *const desc_beg, const void *const desc_end)
{
	const struct usb_desc_header *const desc = desc_beg;
	const struct usb_desc_header *next;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	next = (const struct usb_desc_header *)((uint8_t *)desc + desc->bLength);

	/* Validate the bLength field to make sure it does not point past the end */
	if ((uint8_t *)next >= (uint8_t *)desc_end ||
	    (uint8_t *)next + next->bLength > (uint8_t *)desc_end ||
	    desc->bLength == 0) {
		return NULL;
	}

	return next;
}

const bool usbh_desc_size_is_valid(const void *const desc, size_t expected_size,
				   const void *const desc_end)
{
	return desc != NULL && desc_end != NULL &&
	       (uint8_t *)desc + expected_size <= (uint8_t *)desc_end;
}

const struct usb_desc_header *usbh_desc_get_by_type(const void *const desc_beg,
						    const void *const desc_end,
						    uint32_t type_mask)
{
	const struct usb_desc_header *desc;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	for (desc = desc_beg; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if ((BIT(desc->bDescriptorType) & type_mask) != 0) {
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
		struct usb_association_descriptor *iad_desc = (void *)desc;

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
	uint32_t type_mask = BIT(USB_DESC_INTERFACE_ASSOC) | BIT(USB_DESC_INTERFACE);
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
		return usbh_desc_get_by_type(desc_beg, desc_end, type_mask);
	}

	desc = usbh_desc_get_next(desc_beg, desc_end);
	if (desc == NULL) {
		return NULL;
	}

	while (skip_num > 0) {
		desc = usbh_desc_get_by_type(desc, desc_end, type_mask);
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
				if(skip_num == 0) {
					return desc;
				}
			}

			desc = usbh_desc_get_next(desc, desc_end);
			if (desc == NULL) {
				return NULL;
			}
		} else {
			__ASSERT(false, "invalid implementation of usbh_desc_get_by_iface()");
			return NULL;
		}
	}

	return desc;
}
