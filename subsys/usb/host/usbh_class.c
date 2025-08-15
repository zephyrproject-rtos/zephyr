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

static int usbh_class_probe_each_iface(struct usbh_class_data *const c_data,
				       struct usb_device *const udev)
{
	const struct usb_cfg_descriptor *const cfg_desc = usbh_desc_get_cfg_beg(udev);
	int ret;

	for (uint8_t next, ifnum = 0; ifnum < cfg_desc->bNumInterfaces; ifnum = next) {
		struct usbh_class_filter device_info = {};

		ret = usbh_desc_get_device_info(&device_info, udev, ifnum);
		if (ret < 0) {
			LOG_ERR("Failed to collect class codes for matching interface %u", ifnum);
			return ret;
		}
		next = ret;

		if (!usbh_class_is_matching(c_data->filters, c_data->num_filters, &device_info)) {
			LOG_DBG("Class %s not matching interface %u", c_data->name, ifnum);
			continue;
		}

		ret = usbh_class_probe(c_data, udev, ifnum);
		if (ret == -ENOTSUP) {
			LOG_DBG("Class %s not supporting this device, skipping", c_data->name);
			continue;
		}
		if (ret != 0) {
			LOG_ERR("Error while probing the class %s for interface %u",
				c_data->name, ifnum);
			return ret;
		}

		LOG_INF("Class %s matches this device's interface %u", c_data->name, ifnum);
		c_data->udev = udev;
		c_data->ifnum = ifnum;

		return 0;
	}

	return -ENOTSUP;
}

int usbh_class_probe_all(struct usb_device *const udev)
{
	bool matching = false;
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		/* If the class is not already having a device */
		if (c_data->udev == NULL) {
			ret = usbh_class_probe_each_iface(c_data, udev);
			if (ret == -ENOTSUP) {
				LOG_DBG("This device has no %s class matching", c_data->name);
				continue;
			}
			if (ret != 0) {
				return ret;
			}
			matching = true;
		}
	}

	if (!matching) {
		LOG_WRN("Could not find any class for this device");
		return -ENOTSUP;
	}

	return 0;
}

bool usbh_class_is_matching(struct usbh_class_filter *const filters, size_t num_filters,
			    struct usbh_class_filter *const device_info)
{
	/* Make empty filter set match everything (use class_api->probe() only) */
	if (num_filters == 0) {
		return true;
	}

	/* Try to find a rule that matches completely */
	for (int i = 0; i < num_filters; i++) {
		const struct usbh_class_filter *filt = &filters[i];

		if ((filt->flags & USBH_CLASS_MATCH_VID) &&
		    device_info->vid != filt->vid) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_PID) &&
		    device_info->pid != filt->pid) {
			continue;
		}

		if (filt->flags & USBH_CLASS_MATCH_CLASS &&
		    device_info->class != filt->class) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_SUB) &&
		    device_info->sub != filt->sub) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_PROTO) &&
		    device_info->proto != filt->proto) {
			continue;
		}

		/* All the selected filters did match */
		return true;
	}

	/* At the end of the filter table and still no match */
	return false;
}
