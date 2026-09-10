/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
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

		if (c_node->state == USBH_CLASS_STATE_RESERVED) {
			c_node->state = USBH_CLASS_STATE_IDLE;
			c_data->udev = NULL;
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

void usbh_class_probe_device(struct usb_device *const udev)
{
	int err;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_RESERVED) {
			continue;
		}

		if (c_data->udev != udev) {
			continue;
		}

		err = usbh_class_probe(c_data, udev, c_data->iface);
		if (err != 0) {
			LOG_WRN("Failed to probe class %s for interface %u",
				c_data->name, c_data->iface);

			c_node->state = USBH_CLASS_STATE_IDLE;
			c_data->udev = NULL;
			c_data->iface = 0;
		} else {
			c_node->state = USBH_CLASS_STATE_BOUND;

			LOG_INF("Class '%s' matches interface %u", c_data->name, c_data->iface);
		}
	}
}

/*
 * Match the first idle class instance matching a USB device function.
 *
 * Iterate through all registered class nodes and find the first one that
 * is in USBH_CLASS_STATE_IDLE state and whose filter rules match the
 * given filter data. On match, the class node transitions to
 * USBH_CLASS_STATE_RESERVED and records the interface number.
 *
 * This is used during configuration matching to simulate driver allocation
 * without actually binding. The reservation can later be:
 *   - Promoted to BOUND by usbh_class_probe_device()
 *   - Rolled back to IDLE by usbh_class_probe_device() if class probes failed
 */
static struct usbh_class_node *
usbh_class_match_function(struct usb_device *const udev,
			  struct usbh_class_filter *const filter_data,
			  const uint8_t iface)
{
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

		c_node->state = USBH_CLASS_STATE_RESERVED;
		c_data->udev = udev;
		c_data->iface = iface;
		return c_node;
	}

	return NULL;
}

bool usbh_class_match_device(struct usb_device *const udev, const void *cfg_desc)
{
	const struct usb_desc_header *desc = cfg_desc;
	struct usbh_class_node *c_node;
	struct usbh_class_filter filter_data;
	bool match = false;
	uint8_t iface;

	/* To support single-function devices, match against the entire device */
	filter_data.vid = udev->dev_desc.idVendor;
	filter_data.pid = udev->dev_desc.idProduct;
	filter_data.class = udev->dev_desc.bDeviceClass;
	filter_data.sub = udev->dev_desc.bDeviceSubClass;
	filter_data.proto = udev->dev_desc.bDeviceProtocol;
	c_node = usbh_class_match_function(udev, &filter_data, USBH_CLASS_IFNUM_DEVICE);
	if (c_node != NULL) {
		return true;
	}

	/* To support multi-function devices, match against each function */
	while (true) {
		desc = usbh_desc_get_next_function(desc);
		if (desc == NULL) {
			break;
		}

		if (usbh_desc_fill_filter(desc, &filter_data, &iface) != 0) {
			LOG_ERR("Failed to collect class codes for matching interface %u",
				iface);
			continue;
		}

		c_node = usbh_class_match_function(udev, &filter_data, iface);
		if (c_node != NULL) {
			match = true;
		}
	}

	return match;
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
