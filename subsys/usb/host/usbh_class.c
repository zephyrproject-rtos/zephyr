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

void usbh_class_init_all(struct usbh_context *const uhs_ctx)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_IDLE) {
			LOG_DBG("Skipping '%s' in state %u", c_data->name, c_node->state);
			continue;
		}

		ret = usbh_class_init(c_data, uhs_ctx);
		if (ret != 0) {
			LOG_WRN("Failed to initialize class %s (%d)", c_data->name, ret);
			c_node->state = USBH_CLASS_STATE_ERROR;
		}
	}
}

void usbh_class_remove_all(struct usb_device *const udev)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		k_mutex_lock(&udev->mutex, K_FOREVER);

		if (c_data->udev == udev) {
			ret = usbh_class_removed(c_data);
			if (ret != 0) {
				LOG_ERR("Failed to handle device removal for each class (%d)", ret);
				c_node->state = USBH_CLASS_STATE_ERROR;
				continue;
			}

			/* The class instance is now free to bind to a new device */
			c_data->udev = NULL;
			c_node->state = USBH_CLASS_STATE_IDLE;
		}

		k_mutex_unlock(&udev->mutex);
	}
}

void usbh_class_probe_function(struct usb_device *const udev,
			       struct usbh_class_filter *const info, uint8_t iface)
{
	int ret;

	k_mutex_lock(&udev->mutex, K_FOREVER);

	/* First check if any interface is already bound to this */
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state == USBH_CLASS_STATE_BOUND &&
		    c_data->udev == udev && c_data->iface == iface) {
			LOG_DBG("Interface %u bound to '%s', skipping", iface, c_data->name);
			return;
		}
	}

	/* Then try to match this function against all interfaces */
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_IDLE) {
			LOG_DBG("Class %s already matched, skipping", c_data->name);
			continue;
		}

		if (!usbh_class_is_matching(c_node->filters, c_node->num_filters, info)) {
			LOG_DBG("Class %s not matching interface %u", c_data->name, iface);
			continue;
		}

		ret = usbh_class_probe(c_data, udev, iface);
		if (ret == -ENOTSUP) {
			LOG_DBG("Class %s not supporting this device, skipping", c_data->name);
			continue;
		}

		LOG_INF("Class '%s' matches interface %u", c_data->name, iface);
		c_node->state = USBH_CLASS_STATE_BOUND;
		c_data->udev = udev;
		c_data->iface = iface;
		break;
	}

	k_mutex_unlock(&udev->mutex);
}

void usbh_class_probe_device(struct usb_device *const udev)
{
	const void *const desc_beg = usbh_desc_get_cfg_beg(udev);
	const void *const desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_desc_header *desc = desc_beg;
	struct usbh_class_filter info = {
		.vid = udev->dev_desc.idVendor,
		.pid = udev->dev_desc.idProduct,
	};
	uint8_t iface;
	int ret;

	while (true) {
		desc = usbh_desc_get_next_function(desc, desc_end);
		if (desc == NULL) {
			break;
		}

		ret = usbh_desc_get_iface_info(desc, &info, &iface);
		if (ret < 0) {
			LOG_ERR("Failed to collect class codes for matching interface %u", iface);
			continue;
		}

		usbh_class_probe_function(udev, &info, iface);
	}
}

bool usbh_class_is_matching(struct usbh_class_filter *const filters, size_t num_filters,
			    struct usbh_class_filter *const info)
{
	/* Make empty filter set match everything (use class_api->probe() only) */
	if (num_filters == 0) {
		return true;
	}

	/* Try to find a rule that matches completely */
	for (int i = 0; i < num_filters; i++) {
		const struct usbh_class_filter *filt = &filters[i];

		if ((filt->flags & USBH_CLASS_MATCH_VID) &&
		    info->vid != filt->vid) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_PID) &&
		    info->pid != filt->pid) {
			continue;
		}

		if (filt->flags & USBH_CLASS_MATCH_CLASS &&
		    info->class != filt->class) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_SUB) &&
		    info->sub != filt->sub) {
			continue;
		}

		if ((filt->flags & USBH_CLASS_MATCH_PROTO) &&
		    info->proto != filt->proto) {
			continue;
		}

		/* All the selected filters did match */
		return true;
	}

	/* At the end of the filter table and still no match */
	return false;
}
