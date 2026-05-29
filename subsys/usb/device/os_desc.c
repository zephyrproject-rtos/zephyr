/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_os_desc, CONFIG_USB_DEVICE_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include <os_desc.h>

static struct usb_os_descriptor *os_desc;

int usb_handle_os_desc(struct usb_setup_packet *setup,
		       int32_t *len, uint8_t **data)
{
	if (!os_desc) {
		return -ENOTSUP;
	}

	if (USB_GET_DESCRIPTOR_TYPE(setup->wValue) == USB_DESC_STRING &&
	    USB_GET_DESCRIPTOR_INDEX(setup->wValue) == USB_OSDESC_STRING_DESC_INDEX) {
		LOG_DBG("MS OS Descriptor string read");
		*data = os_desc->string;
		*len = os_desc->string_len;

		return 0;
	}

	return -ENOTSUP;
}

int usb_handle_os_desc_feature(struct usb_setup_packet *setup,
			       int32_t *len, uint8_t **data)
{
	LOG_DBG("bRequest 0x%x", setup->bRequest);

	if (!os_desc) {
		return -ENOTSUP;
	}

	if (setup->bRequest == os_desc->vendor_code) {
		switch (setup->wIndex) {
		case USB_OSDESC_EXTENDED_COMPAT_ID:
			LOG_DBG("Handle Compat ID");
			*data = os_desc->compat_id;
			*len = os_desc->compat_id_len;

			return 0;
		default:
			break;
		}
	}

	return -ENOTSUP;
}

/* Register MS OS Descriptors version 1 */
void usb_register_os_desc(struct usb_os_descriptor *desc)
{
	os_desc = desc;
}

bool usb_os_desc_enabled(void)
{
	return !!os_desc;
}
