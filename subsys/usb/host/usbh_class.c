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
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

int usbh_class_init_all(struct usbh_context *const uhs_ctx)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		ret = usbh_class_init(c_node->c_data, uhs_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to initialize all registered class instances");
			return ret;
		}
	}

	return 0;
}

int usbh_class_remove_all(const struct usb_device *const udev)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		if (c_node->c_data->udev == udev) {
			ret = usbh_class_removed(c_node->c_data);
			if (ret != 0) {
				LOG_ERR("Failed to handle device removal for each classes");
				return ret;
			}

			/* The class instance is now free to bind to a new device */
			c_node->c_data->udev = NULL;
		}
	}

	return 0;
}

static int usbh_class_probe_one(struct usbh_class_data *const c_data, struct usb_device *const udev)
{
	const uint8_t *const desc_beg = usbh_desc_get_cfg_beg(udev);
	const uint8_t *const desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_desc_header *desc;
	const struct usb_cfg_descriptor *cfg_desc;
	int ret;

	desc = usbh_desc_get_by_type(desc_beg, desc_end, USB_DESC_CONFIGURATION);
	if (desc == NULL) {
		LOG_ERR("Unable to find a configuration descriptor");
		return -EBADMSG;
	}

	cfg_desc = (struct usb_cfg_descriptor *)desc;

	for (uint8_t ifnum = 0; ifnum < cfg_desc->bNumInterfaces; ifnum++) {
		ret = usbh_class_probe(c_data, udev, ifnum);
		if (ret == -ENOTSUP) {
			LOG_DBG("Class %s not supporting this device, skipping", c_data->name);
			continue;
		}
		if (ret != 0) {
			LOG_ERR("Failed to register the class %s for interface %u",
				c_data->name, ifnum);
			return ret;
		}
		if (ret == 0) {
			LOG_INF("Class %s is matching this device for interface %u, assigning it",
				c_data->name, ifnum);
			c_data->udev = udev;
			c_data->ifnum = ifnum;
			return 0;
		}
	}

	return -ENOTSUP;
}

int usbh_class_probe_all(struct usb_device *const udev)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		/* If the class is not already having a device */
		if (c_data->udev == NULL) {
			ret = usbh_class_probe_one(c_data, udev);
			if (ret == -ENOTSUP) {
				continue;
			}
			if (ret != 0) {
				return ret;
			}
		}
	}

	LOG_WRN("Could not find any class for this device");
	return -ENODEV;
}

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
