/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_bos, CONFIG_USB_DEVICE_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>

#include <zephyr/usb/bos.h>

extern const uint8_t __usb_bos_desc_start[];
extern const uint8_t __usb_bos_desc_end[];

USB_DEVICE_BOS_DESC_DEFINE_HDR struct usb_bos_descriptor bos_hdr = {
	.bLength = sizeof(struct usb_bos_descriptor),
	.bDescriptorType = USB_DESC_BOS,
	.wTotalLength = 0, /* should be corrected with register */
	.bNumDeviceCaps = 0, /* should be set with register */
};

size_t usb_bos_get_length(void)
{
	return __usb_bos_desc_end - __usb_bos_desc_start;
}

const void *usb_bos_get_header(void)
{
	return __usb_bos_desc_start;
}

void usb_bos_fix_total_length(void)
{
	bos_hdr.wTotalLength = usb_bos_get_length();
}

void usb_bos_register_cap(struct usb_bos_platform_descriptor *desc)
{
	/* Has effect only on first register */
	bos_hdr.wTotalLength = usb_bos_get_length();

	bos_hdr.bNumDeviceCaps += 1U;
}

int usb_handle_bos(struct usb_setup_packet *setup,
		   int32_t *len, uint8_t **data)
{
	if (USB_GET_DESCRIPTOR_TYPE(setup->wValue) == USB_DESC_BOS) {
		LOG_DBG("Read BOS descriptor");
		*data = (uint8_t *)usb_bos_get_header();
		*len = usb_bos_get_length();

		return 0;
	}

	return -ENOTSUP;
}
