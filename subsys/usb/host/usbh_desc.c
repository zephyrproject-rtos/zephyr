/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbh_desc.h"

struct usb_desc_header *usbh_desc_get_by_type(const uint8_t *const start_addr,
					      const uint8_t *const end_addr,
					      uint32_t type_mask)
{
	const uint8_t *curr_addr = start_addr;

	while (curr_addr < end_addr) {
		struct usb_desc_header *desc = (void *)curr_addr;

		if (desc->bLength == 0) {
			break;
		}

		if ((BIT(desc->bDescriptorType) & type_mask) != 0) {
			return desc;
		}
		curr_addr += desc->bLength;
	}

	return NULL;
}
