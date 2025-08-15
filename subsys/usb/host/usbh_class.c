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
#include "usbh_host.h"

LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

void usbh_class_init_all(struct usbh_context *const uhs_ctx)
{
	int ret;

	usbh_host_lock(uhs_ctx);

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

	usbh_host_unlock(uhs_ctx);
}

void usbh_class_remove_all(struct usb_device *const udev)
{
	int ret;

	k_mutex_lock(&udev->mutex, K_FOREVER);

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

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
	}

	k_mutex_unlock(&udev->mutex);
}

/*
 * Probe an USB device function against all available classes of the system.
 *
 * Try to match a class from the global list of all system classes, using their filter rules
 * and return status to tell if a class matches or not.
 *
 * The first matched class will stop the loop, and the status will be updated so that classes
 * are only matched for a single USB function at a time.
 *
 * USB functions will only have one class matching, and calling usbh_class_probe_function()
 * multiple times consequently has no effect.
 */
static void usbh_class_probe_function(struct usb_device *const udev,
				      struct usbh_class_filter *const info, const uint8_t iface)
{
	int ret;

	/* Assumes that udev->mutex is locked */

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

		if (!usbh_class_is_matching(c_node->filters, info)) {
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
}

void usbh_class_probe_device(struct usb_device *const udev)
{
	const void *desc_end;
	const struct usb_desc_header *desc;
	struct usbh_class_filter info;
	uint8_t iface;
	int ret;

	k_mutex_lock(&udev->mutex, K_FOREVER);

	desc = usbh_desc_get_cfg(udev);
	desc_end = usbh_desc_get_cfg_end(udev);
	info.vid = udev->dev_desc.idVendor;
	info.pid = udev->dev_desc.idProduct;

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

	k_mutex_unlock(&udev->mutex);
}

bool usbh_class_is_matching(struct usbh_class_filter *const filters,
			    struct usbh_class_filter *const info)
{
	/* Make empty filter set match everything (use class_api->probe() only) */
	if (filters == NULL) {
		return true;
	}

	/* Try to find a rule that matches completely */
	for (size_t i = 0; filters[i].flags != 0; i++) {
		const struct usbh_class_filter *filt = &filters[i];

		if (filt->flags & USBH_CLASS_MATCH_VID_PID &&
		    (info->vid != filt->vid || info->pid != filt->pid)) {
			continue;
		}

		if (filt->flags & USBH_CLASS_MATCH_CODE_TRIPLE &&
		    (info->class != filt->class || info->sub != filt->sub ||
		     info->proto != filt->proto)) {
			continue;
		}

		/* All the selected filters did match */
		return true;
	}

	/* At the end of the filter table and still no match */
	return false;
}
