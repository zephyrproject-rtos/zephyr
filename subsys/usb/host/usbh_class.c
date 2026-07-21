/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_host.h"

LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

void usbh_class_init_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_IDLE) {
			LOG_DBG("Skipping '%s' in state %u",
				c_data->name, c_node->state);
			continue;
		}

		ret = usbh_class_init(c_data);
		if (ret != 0) {
			LOG_WRN("Failed to initialize class %s (%d)",
				c_data->name, ret);
			c_node->state = USBH_CLASS_STATE_ERROR;
		}
	}
}

void usbh_class_remove_all(struct usb_device *const udev)
{
	int ret;

	k_mutex_lock(&udev->mutex, K_FOREVER);

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_data->udev != udev) {
			continue;
		}

		ret = usbh_class_removed(c_data);
		if (ret != 0) {
			LOG_ERR("Failed to handle device removal for each class (%d)", ret);
			c_node->state = USBH_CLASS_STATE_ERROR;
			c_data->udev = NULL;
			continue;
		}

		/* The class instance is now free to bind to a new device */
		c_node->state = USBH_CLASS_STATE_IDLE;
		c_data->udev = NULL;
	}

	k_mutex_unlock(&udev->mutex);
}

/*
 * Probe an USB device function against each host class instantiated.
 *
 * Try to match a class from the global list of all system classes, using their
 * filter rules and return status to tell if a class matches or not.
 *
 * The first match will stop the loop, and the status will be updated
 * so that classes only match one function at a time.
 *
 * USB functions will have at most one class matching, and calling
 * usbh_class_probe_function() multiple times consequently has no effect.
 */
static void usbh_class_probe_function(struct usb_device *const udev,
				      struct usbh_class_filter *const filter_data,
				      const uint8_t iface)
{
	int ret;

	/* Assumes that udev->mutex is locked */

	/* First check if any interface is already bound to this */
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state == USBH_CLASS_STATE_BOUND &&
		    c_data->udev == udev && c_data->iface == iface) {
			LOG_DBG("Interface %u bound to '%s', skipping",
				iface, c_data->name);
			return;
		}
	}

	/* Then try to match this function against all interfaces */
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_IDLE) {
			LOG_DBG("Class %s already matched, skipping",
				c_data->name);
			continue;
		}

		if (!usbh_class_is_matching(c_node->filters, filter_data)) {
			LOG_DBG("Class %s not matching interface %u",
				c_data->name, iface);
			continue;
		}

		ret = usbh_class_probe(c_data, udev, iface);
		if (ret == -ENOTSUP) {
			LOG_DBG("Class %s not supporting this function, skipping",
				c_data->name);
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
	const struct usb_desc_header *desc = udev->cfg_desc;
	struct usbh_class_filter filter_data;
	uint8_t iface;
	int ret;

	/* To support single-function devices, match against the entire device */

	filter_data.vid = udev->dev_desc.idVendor;
	filter_data.pid = udev->dev_desc.idProduct;
	filter_data.class = udev->dev_desc.bDeviceClass;
	filter_data.sub = udev->dev_desc.bDeviceSubClass;
	filter_data.proto = udev->dev_desc.bDeviceProtocol;

	usbh_class_probe_function(udev, &filter_data, USBH_CLASS_IFNUM_DEVICE);

	/* To support multi-function devices, match against each function */

	while (true) {
		desc = usbh_desc_get_next_function(desc);
		if (desc == NULL) {
			break;
		}

		ret = usbh_desc_fill_filter(desc, &filter_data, &iface);
		if (ret != 0) {
			LOG_ERR("Failed to collect class codes for matching interface %u",
				iface);
			continue;
		}

		usbh_class_probe_function(udev, &filter_data, iface);
	}
}

bool usbh_class_is_matching(const struct usbh_class_filter *const filter_rules,
			    const struct usbh_class_filter *const filter_data)
{
	/* Make empty filter set match everything (use class_api->probe() only) */
	if (filter_rules == NULL) {
		return true;
	}

	/* Try to find a rule that matches completely */
	for (size_t i = 0; filter_rules[i].flags != 0; i++) {
		const struct usbh_class_filter *rule = &filter_rules[i];

		if (rule->flags & USBH_CLASS_MATCH_VID_PID &&
		    (filter_data->vid != rule->vid || filter_data->pid != rule->pid)) {
			continue;
		}

		if (rule->flags & USBH_CLASS_MATCH_CODE_TRIPLE &&
		    (filter_data->class != rule->class || filter_data->sub != rule->sub ||
		     filter_data->proto != rule->proto)) {
			continue;
		}

		/* All the selected filter_rules did match */
		return true;
	}

	/* At the end of the filter table and still no match */
	return false;
}
