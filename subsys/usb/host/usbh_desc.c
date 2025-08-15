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

const void *usbh_desc_get_by_ifnum(const void *const desc_beg, const void *const desc_end,
				   const uint8_t ifnum)
{
	const struct usb_desc_header *desc;

	if (desc_beg == NULL || desc_end == NULL) {
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

const int usbh_desc_get_device_info(struct usbh_class_filter *const device_info,
				    const struct usb_device *const udev, const uint8_t ifnum)
{
	const void *const desc_beg = usbh_desc_get_cfg_beg(udev);
	const void *const desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_cfg_descriptor *const cfg_desc = desc_beg;
	const struct usb_desc_header *desc;

	desc = usbh_desc_get_by_ifnum(desc_beg, desc_end, ifnum);
	if (desc == NULL) {
		return -EBADMSG;
	}

	if (cfg_desc->bDescriptorType != USB_DESC_CONFIGURATION) {
		return -EBADMSG;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		struct usb_association_descriptor *iad_desc = (void *)desc;

		device_info->class = iad_desc->bFunctionClass;
		device_info->sub = iad_desc->bFunctionSubClass;
		device_info->proto = iad_desc->bFunctionProtocol;
		return ifnum + iad_desc->bInterfaceCount;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE) {
		struct usb_if_descriptor *if_desc = (void *)desc;

		device_info->class = if_desc->bInterfaceClass;
		device_info->sub = if_desc->bInterfaceSubClass;
		device_info->proto = if_desc->bInterfaceProtocol;
		return ifnum + 1;
	}

	__ASSERT(false, "invalid implementation of usbh_desc_get_by_ifnum()");

	return -EINVAL;
}
