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

/* Time to wait for transfers to drain during removal */
#define USBH_CLASS_DRAIN_TIMEOUT	K_MSEC(1000)

void usbh_class_init_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		sys_dlist_init(&c_data->xfer_anchor_list);
		k_mutex_init(&c_data->mutex);
		k_condvar_init(&c_data->drained);

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

		/* Stop the instance from accepting new transfers */
		k_mutex_lock(&c_data->mutex, K_FOREVER);
		c_data->bound = false;
		k_mutex_unlock(&c_data->mutex);

		/*
		 * Cancel queued transfers and wait for their completion before
		 * calling usbh_class_removed().
		 */
		usbh_class_xfer_dequeue_all_anchored(c_data);
		if (usbh_class_xfer_drain(c_data, USBH_CLASS_DRAIN_TIMEOUT) != 0) {
			LOG_ERR("%s transfers did not drain on removal", c_data->name);
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

		k_mutex_lock(&c_data->mutex, K_FOREVER);
		c_data->bound = true;
		k_mutex_unlock(&c_data->mutex);
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

void usbh_class_xfer_anchor(struct usbh_class_data *const c_data,
			    struct uhc_transfer *const xfer)
{
	sys_dnode_init(&xfer->anchor_node);
	k_mutex_lock(&c_data->mutex, K_FOREVER);
	xfer->anchor = c_data;
	sys_dlist_append(&c_data->xfer_anchor_list, &xfer->anchor_node);
	uhc_xfer_ref(xfer);
	k_mutex_unlock(&c_data->mutex);
}

int usbh_class_xfer_acquire(struct usbh_class_data *const c_data)
{
	int ret = 0;

	k_mutex_lock(&c_data->mutex, K_FOREVER);

	if (!c_data->bound) {
		ret = -ESHUTDOWN;
	} else {
		c_data->xfer_count++;
	}

	k_mutex_unlock(&c_data->mutex);

	return ret;
}

struct usbh_class_data *usbh_class_xfer_unanchor(struct uhc_transfer *const xfer)
{
	struct usbh_class_data *const c_data = xfer->anchor;
	bool linked = false;

	if (c_data == NULL) {
		return NULL;
	}

	k_mutex_lock(&c_data->mutex, K_FOREVER);

	if (sys_dnode_is_linked(&xfer->anchor_node)) {
		linked = true;
		sys_dlist_remove(&xfer->anchor_node);
	}

	xfer->anchor = NULL;
	k_mutex_unlock(&c_data->mutex);

	if (linked) {
		(void)uhc_xfer_unref(xfer);
	}

	return c_data;
}

void usbh_class_xfer_release(struct usbh_class_data *const c_data)
{
	k_mutex_lock(&c_data->mutex, K_FOREVER);

	if (c_data->xfer_count > 0) {
		c_data->xfer_count--;
	}

	if (c_data->xfer_count == 0) {
		k_condvar_broadcast(&c_data->drained);
	}

	k_mutex_unlock(&c_data->mutex);
}

int usbh_class_xfer_drain(struct usbh_class_data *const c_data, k_timeout_t timeout)
{
	int ret = 0;

	k_mutex_lock(&c_data->mutex, K_FOREVER);

	while (c_data->xfer_count != 0) {
		if (k_condvar_wait(&c_data->drained, &c_data->mutex, timeout) != 0) {
			ret = -ETIMEDOUT;
			break;
		}
	}

	k_mutex_unlock(&c_data->mutex);

	return ret;
}

void usbh_class_xfer_dequeue_anchored(struct usbh_class_data *const c_data,
				      const uint8_t ep)
{
	const struct usbh_context *uhs_ctx;
	struct uhc_transfer *xfer;

	if (c_data->udev == NULL) {
		LOG_ERR("Class instance is not bound");
		return;
	}

	uhs_ctx = c_data->udev->ctx;
	k_mutex_lock(&c_data->mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_CONTAINER(&c_data->xfer_anchor_list, xfer, anchor_node) {
		if (xfer->ep == ep) {
			(void)uhc_ep_dequeue(uhs_ctx->dev, xfer);
		}
	}

	k_mutex_unlock(&c_data->mutex);
}

void usbh_class_xfer_dequeue_all_anchored(struct usbh_class_data *const c_data)
{
	const struct usbh_context *uhs_ctx;
	struct uhc_transfer *xfer;

	if (c_data->udev == NULL) {
		LOG_ERR("Class instance is not bound");
		return;
	}

	uhs_ctx = c_data->udev->ctx;
	k_mutex_lock(&c_data->mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_CONTAINER(&c_data->xfer_anchor_list, xfer, anchor_node) {
		(void)uhc_ep_dequeue(uhs_ctx->dev, xfer);
	}

	k_mutex_unlock(&c_data->mutex);
}
