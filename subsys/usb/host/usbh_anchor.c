/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>

#include "usbh_device.h"

void usbh_xfer_anchor_init(struct usbh_xfer_anchor *const anchor)
{
	sys_dlist_init(&anchor->list);
	k_mutex_init(&anchor->mutex);
}

int usbh_xfer_anchor(struct usbh_xfer_anchor *const anchor,
		     struct uhc_transfer *const xfer)
{
	sys_dnode_init(&xfer->anchor_node);
	k_mutex_lock(&anchor->mutex, K_FOREVER);
	xfer->anchor = anchor;
	sys_dlist_append(&anchor->list, &xfer->anchor_node);
	uhc_xfer_ref(xfer);
	k_mutex_unlock(&anchor->mutex);

	return 0;
}

struct usbh_xfer_anchor *usbh_xfer_unanchor(struct uhc_transfer *const xfer)
{
	struct usbh_xfer_anchor *const anchor = xfer->anchor;
	bool linked = false;

	if (anchor == NULL) {
		return NULL;
	}

	k_mutex_lock(&anchor->mutex, K_FOREVER);

	if (sys_dnode_is_linked(&xfer->anchor_node)) {
		linked = true;
		sys_dlist_remove(&xfer->anchor_node);
	}

	xfer->anchor = NULL;
	k_mutex_unlock(&anchor->mutex);

	if (linked) {
		(void)uhc_xfer_unref(xfer);
	}

	return anchor;
}

void usbh_xfer_anchor_dequeue_all(struct usbh_xfer_anchor *const anchor)
{
	struct uhc_transfer *xfer;

	k_mutex_lock(&anchor->mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_CONTAINER(&anchor->list, xfer, anchor_node) {
		(void)usbh_xfer_dequeue(xfer->udev, xfer);
	}

	k_mutex_unlock(&anchor->mutex);
}
