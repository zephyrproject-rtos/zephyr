/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usb_ch9.h>

#include "usbh_desc.h"

const struct usb_desc_header *usbh_desc_get_next(const struct usb_desc_header *const desc,
						 const void *const desc_end)
{
	const struct usb_desc_header *next;

	if (desc == NULL) {
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

const struct usb_desc_header *usbh_desc_get_by_type(const void *const desc_beg,
						    const void *const desc_end,
						    uint32_t type_mask)
{
	const struct usb_desc_header *desc;

	if (desc_beg == NULL) {
		return NULL;
	}

	for (desc = desc_beg; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if ((BIT(desc->bDescriptorType) & type_mask) != 0) {
			return desc;
		}
	}

	return NULL;
}

const struct usb_desc_header *usbh_desc_get_by_ifnum(const void *const desc_beg,
						     const void *const desc_end,
						     const uint8_t ifnum)
{
	const struct usb_desc_header *desc;

	if (desc_beg == NULL) {
		return NULL;
	}

	for (desc = desc_beg; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE &&
		    ((struct usb_if_descriptor *)desc)->bInterfaceNumber == ifnum) {
			return desc;
		}
		if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC &&
		    ((struct usb_association_descriptor *)desc)->bFirstInterface == ifnum) {
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
