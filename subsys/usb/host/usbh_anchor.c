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
	k_condvar_init(&anchor->drained);
	anchor->count = 0;
	anchor->accept = false;
}

void usbh_xfer_anchor_set_accept(struct usbh_xfer_anchor *const anchor,
				 const bool accept)
{
	k_mutex_lock(&anchor->mutex, K_FOREVER);
	anchor->accept = accept;
	k_mutex_unlock(&anchor->mutex);
}

int usbh_xfer_anchor(struct usbh_xfer_anchor *const anchor,
		     struct uhc_transfer *const xfer)
{
	int ret = 0;

	k_mutex_lock(&anchor->mutex, K_FOREVER);

	if (!anchor->accept) {
		ret = -ESHUTDOWN;
	} else {
		sys_dnode_init(&xfer->anchor_node);
		xfer->anchor = anchor;
		sys_dlist_append(&anchor->list, &xfer->anchor_node);
		uhc_xfer_ref(xfer);
		anchor->count++;
	}

	k_mutex_unlock(&anchor->mutex);

	return ret;
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

void usbh_xfer_anchor_release(struct usbh_xfer_anchor *const anchor)
{
	k_mutex_lock(&anchor->mutex, K_FOREVER);

	anchor->count--;
	if (anchor->count == 0) {
		k_condvar_broadcast(&anchor->drained);
	}

	k_mutex_unlock(&anchor->mutex);
}

void usbh_xfer_anchor_abort(struct uhc_transfer *const xfer)
{
	struct usbh_xfer_anchor *const anchor = usbh_xfer_unanchor(xfer);

	if (anchor != NULL) {
		usbh_xfer_anchor_release(anchor);
	}
}

int usbh_xfer_anchor_drain(struct usbh_xfer_anchor *const anchor,
			   k_timeout_t timeout)
{
	int ret = 0;

	k_mutex_lock(&anchor->mutex, K_FOREVER);

	while (anchor->count != 0) {
		if (k_condvar_wait(&anchor->drained, &anchor->mutex, timeout) != 0) {
			ret = -ETIMEDOUT;
			break;
		}
	}

	k_mutex_unlock(&anchor->mutex);

	return ret;
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
