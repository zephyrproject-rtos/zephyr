/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_class_api.h"

LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

bool usbh_class_is_matching(struct usbh_class_filter *const filters, size_t n_filters,
			    struct usb_device_descriptor *const desc)
{
	for (int i = 0; i < n_filters; i++) {
		const struct usbh_class_filter *filt = &filters[i];

		if (filt->flags & USBH_CLASS_MATCH_VID) {
			if (desc->idVendor != filt->vid) {
				continue;
			}
		}

		if (filt->flags & USBH_CLASS_MATCH_PID) {
			if (desc->idProduct != filt->pid) {
				continue;
			}
		}

		if (filt->flags & USBH_CLASS_MATCH_DCLASS) {
			if (desc->bDeviceClass != filt->dclass) {
				continue;
			}
		}

		if (filt->flags & USBH_CLASS_MATCH_SUB) {
			if (desc->bDeviceSubClass != filt->sub) {
				continue;
			}
		}

		if (filt->flags & USBH_CLASS_MATCH_PROTO) {
			if (desc->bDeviceProtocol != filt->proto) {
				continue;
			}
		}

		/* All the selected filters did match */
		return true;
	}

	/* At the end of the filter table and still no match */
	return false;
}
