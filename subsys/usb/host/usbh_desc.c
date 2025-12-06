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

const bool usbh_desc_is_valid(const void *const desc_ptr, const void *const desc_end,
			      size_t expected_size)
{
	const struct usb_desc_header *desc = desc_ptr;

	/* Block invalid input */
	if (desc == NULL || desc_end == NULL || expected_size < sizeof(struct usb_desc_header)) {
		return false;
	}

	/* Avoid out of order pointers */
	if ((uint8_t *)desc > (uint8_t *)desc_end) {
		return false;
	}

	/* Avoid too short descriptors */
	if ((uint8_t *)desc + expected_size > (uint8_t *)desc_end) {
		return false;
	}

	/* Avoid too short bLength field */
	if (desc->bLength < expected_size) {
		return false;
	}

	return true;
}

bool usbh_desc_is_valid_interface(const void *const desc, const void *const desc_end)
{
	return usbh_desc_is_valid(desc, desc_end, sizeof(struct usb_if_descriptor)) &&
	       ((struct usb_desc_header *)desc)->bDescriptorType == USB_DESC_INTERFACE;
}

bool usbh_desc_is_valid_association(const void *const desc, const void *const desc_end)
{
	return usbh_desc_is_valid(desc, desc_end, sizeof(struct usb_association_descriptor)) &&
	       ((struct usb_desc_header *)desc)->bDescriptorType == USB_DESC_INTERFACE_ASSOC;
}

bool usbh_desc_is_valid_configuration(const void *const desc, const void *const desc_end)
{
	return usbh_desc_is_valid(desc, desc_end, sizeof(struct usb_cfg_descriptor)) &&
	       ((struct usb_desc_header *)desc)->bDescriptorType == USB_DESC_CONFIGURATION;
}

const void *usbh_desc_get_next(const void *const desc_beg, const void *const desc_end)
{
	const struct usb_desc_header *const desc = desc_beg;
	const struct usb_desc_header *next;

	if (!usbh_desc_is_valid(desc, desc_end, sizeof(const struct usb_desc_header))) {
		return NULL;
	}

	next = (const struct usb_desc_header *)((uint8_t *)desc + desc->bLength);

	if (!usbh_desc_is_valid(next, desc_end, sizeof(const struct usb_desc_header))) {
		return NULL;
	}

	return next;
}

const void *usbh_desc_get_next_alt_setting(const void *const desc_beg, const void *const desc_end)
{
	const struct usb_desc_header *desc = desc_beg;

	/* Expect desc to already be pointing at an interface descriptor */
	if (desc_beg == NULL || desc_end == NULL || desc->bDescriptorType != USB_DESC_INTERFACE) {
		return NULL;
	}

	/* Skip the current interface descriptor */
	desc = usbh_desc_get_next(desc, desc_end);

	/* Seek to the nextAlternate Setting for this interface */
	for (; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			/* Non-zero Alternate Setting */
			if (usbh_desc_is_valid_interface(desc, desc_end) &&
			    ((struct usb_if_descriptor *)desc)->bAlternateSetting != 0) {
				return desc;
			}

			/* Do not continue to the next interface */
			return NULL;
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
		if (usbh_desc_is_valid_interface(desc, desc_end) &&
		    ((struct usb_if_descriptor *)desc)->bInterfaceNumber == iface) {
			return desc;
		}
		if (usbh_desc_is_valid_association(desc, desc_end) &&
		    ((struct usb_association_descriptor *)desc)->bFirstInterface == iface) {
			return desc;
		}
	}

	return NULL;
}

const void *usbh_desc_get_cfg_beg(const struct usb_device *const udev)
{
	return udev->cfg_desc;
}

const void *usbh_desc_get_cfg_end(const struct usb_device *const udev)
{
	const struct usb_cfg_descriptor *const cfg_desc = udev->cfg_desc;

	__ASSERT_NO_MSG(cfg_desc != NULL);

	/* Relies on wTotalLength being validated in usbh_device_set_configuration() */
	return (uint8_t *)cfg_desc + cfg_desc->wTotalLength;
}

int usbh_desc_get_iface_info(const struct usb_desc_header *const desc,
			     struct usbh_class_filter *const iface_code, uint8_t *const iface_num)
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
		const struct usb_if_descriptor *if_desc =
			(const struct usb_if_descriptor *)desc;

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
	uint8_t skip_num = 1;

	if (desc_beg == NULL || desc_end == NULL) {
		return NULL;
	}

	/* Skip all interfaces the Association descriptor contains */
	if (usbh_desc_is_valid_association(desc, desc_end)) {
		skip_num = ((struct usb_association_descriptor *)desc)->bInterfaceCount;
	}

	while (desc != NULL && skip_num > 0) {
		/* If already on an Interface Association or Interface, this will skip it */
		desc = usbh_desc_get_next(desc, desc_end);

		/* Association descriptor: this is always a new function */
		if (usbh_desc_is_valid_association(desc, desc_end)) {
			return desc;
		}

		/* Only count the first Alternate Setting of an Interface */
		if (usbh_desc_is_valid_interface(desc, desc_end) &&
		    ((struct usb_if_descriptor *)desc)->bAlternateSetting == 0) {
			skip_num--;
		}
	}

	return desc;
}
