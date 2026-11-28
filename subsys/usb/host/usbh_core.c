/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/usb/usbh.h>

#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs, CONFIG_USBH_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbh_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_thread_data;

K_MSGQ_DEFINE_STATIC_TYPE(usbh_msgq, struct uhc_event, CONFIG_USBH_MAX_UHC_MSG);

static int usbh_event_carrier(const struct device *dev,
			      const struct uhc_event *const event)
{
	struct usbh_context *const uhs_ctx = (void *)uhc_get_event_ctx(dev);
	k_spinlock_key_t key;

	if (event->type == UHC_EVT_EP_REQUEST) {
		/*
		 * Always add completed transfer requests to the list, so they
		 * do not get lost.
		 */
		key = k_spin_lock(&uhs_ctx->xfer_event_lock);
		sys_dlist_append(&uhs_ctx->xfer_events, &event->xfer->node);
		k_spin_unlock(&uhs_ctx->xfer_event_lock, key);
	}

	return k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
}

static int discard_ep_request(struct usbh_context *const ctx,
			      struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_INF(xfer->buf->data, xfer->buf->len, "buf");
		uhc_xfer_buf_free(dev, xfer->buf);
	}

	(void)uhc_xfer_unref(xfer);

	return 0;
}

static void event_handler_xfer_request(struct usbh_context *const uhs_ctx)
{
	struct uhc_transfer *xfer;
	k_spinlock_key_t key;
	usbh_udev_cb_t cb;
	sys_dnode_t *node;
	int ret;

	while (true) {
		key = k_spin_lock(&uhs_ctx->xfer_event_lock);
		node = sys_dlist_get(&uhs_ctx->xfer_events);
		k_spin_unlock(&uhs_ctx->xfer_event_lock, key);

		if (node == NULL) {
			break;
		}

		xfer = SYS_DLIST_CONTAINER(node, xfer, node);

		if (xfer->cb != NULL) {
			cb = xfer->cb;
			ret = cb(xfer->udev, xfer);
		} else {
			ret = discard_ep_request(uhs_ctx, xfer);
		}

		if (ret) {
			LOG_ERR("Failed to handle request completion callback");
		}
	}
}

static void usbh_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_context *uhs_ctx;
	struct uhc_event event;

	while (true) {
		k_msgq_get(&usbh_msgq, &event, K_FOREVER);

		if (event.type == UHC_EVT_ERROR) {
			LOG_DBG("UHC error event %d", event.status);
		}

		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		event_handler_xfer_request(uhs_ctx);
	}
}

int usbh_init_device_intl(struct usbh_context *const uhs_ctx)
{
	int ret;

	ret = uhc_init(uhs_ctx->dev, usbh_event_carrier, uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	sys_dlist_init(&uhs_ctx->udevs);

	return 0;
}

static int uhs_pre_init(void)
{
	k_thread_create(&usbh_thread_data, usbh_stack,
			K_KERNEL_STACK_SIZEOF(usbh_stack),
			usbh_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh");

	usbh_class_init_all();

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
