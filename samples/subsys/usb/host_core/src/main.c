/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include "usbh_desc.h"

/*
 * Synthetic configuration descriptor (wTotalLength = 18): config + one interface.
 */
static const uint8_t sample_cfg_desc[] = {
	0x09,
	USB_DESC_CONFIGURATION,
	0x12,
	0x00, /* wTotalLength */
	0x01,
	0x01,
	0x00,
	0x80,
	0x32,
	0x09,
	USB_DESC_INTERFACE,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

int main(void)
{
	const struct usb_cfg_descriptor *cfg = (const struct usb_cfg_descriptor *)sample_cfg_desc;
	const struct usb_desc_header *desc;
	const void *desc_end;

	printk("USB host core sample\n");

	desc_end = usbh_desc_cfg_end(cfg);
	if (desc_end == NULL) {
		printk("usbh_desc_cfg_end failed\n");
		return 1;
	}

	if (!usbh_desc_header_in_bounds(&cfg->bLength, desc_end)) {
		printk("config header out of bounds\n");
		return 1;
	}

	desc = usbh_desc_get_next_function(cfg, desc_end);
	if (desc == NULL || desc->bDescriptorType != USB_DESC_INTERFACE) {
		printk("get_next_function failed\n");
		return 1;
	}

	desc = usbh_desc_get_next_function(desc, desc_end);
	if (desc != NULL) {
		printk("expected end of configuration\n");
		return 1;
	}

	printk("Host descriptor helpers OK\n");

	return 0;
}
