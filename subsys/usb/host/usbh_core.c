/*
 * Copyright (c) 2022,2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>

#include "usbh_internal.h"
#include "usbh_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs, CONFIG_USBH_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbh_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_thread_data;

static K_KERNEL_STACK_DEFINE(usbh_bus_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_bus_thread_data;

K_MSGQ_DEFINE(usbh_msgq, sizeof(struct uhc_event),
	      CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

K_MSGQ_DEFINE(usbh_bus_msgq, sizeof(struct uhc_event),
	      CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

static int usbh_event_carrier(const struct device *dev,
			      const struct uhc_event *const event)
{
	int err;

	if (event->type == UHC_EVT_EP_REQUEST) {
		err = k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
	} else {
		err = k_msgq_put(&usbh_bus_msgq, event, K_NO_WAIT);
	}

	return err;
}

static void dev_connected_handler(struct usbh_contex *const ctx,
				  const struct uhc_event *const event)
{

	LOG_DBG("Device connected event");
	if (ctx->root != NULL) {
		LOG_ERR("Device already connected");
		usbh_device_free(ctx->root);
		ctx->root = NULL;
	}

	ctx->root = usbh_device_alloc(ctx);
	if (ctx->root == NULL) {
		LOG_ERR("Failed allocate new device");
		return;
	}

	ctx->root->state = USB_STATE_DEFAULT;

	if (event->type == UHC_EVT_DEV_CONNECTED_HS) {
		ctx->root->speed = USB_SPEED_SPEED_HS;
	} else {
		ctx->root->speed = USB_SPEED_SPEED_FS;
	}

	if (usbh_device_init(ctx->root)) {
		LOG_ERR("Failed to reset new USB device");
	}
}

static void dev_removed_handler(struct usbh_contex *const ctx)
{
	if (ctx->root != NULL) {
		usbh_device_free(ctx->root);
		ctx->root = NULL;
		LOG_DBG("Device removed");
	} else {
		LOG_DBG("Spurious device removed event");
	}
}

static int discard_ep_request(struct usbh_contex *const ctx,
			      struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_INF(xfer->buf->data, xfer->buf->len, "buf");
		uhc_xfer_buf_free(dev, xfer->buf);
	}

	return uhc_xfer_free(dev, xfer);
}

static ALWAYS_INLINE int usbh_event_handler(struct usbh_contex *const ctx,
					    struct uhc_event *const event)
{
	int ret = 0;

	switch (event->type) {
	case UHC_EVT_DEV_CONNECTED_LS:
		LOG_ERR("Low speed device not supported (connected event)");
		break;
	case UHC_EVT_DEV_CONNECTED_FS:
	case UHC_EVT_DEV_CONNECTED_HS:
		dev_connected_handler(ctx, event);
		break;
	case UHC_EVT_DEV_REMOVED:
		dev_removed_handler(ctx);
		break;
	case UHC_EVT_RESETED:
		LOG_DBG("Bus reset");
		break;
	case UHC_EVT_SUSPENDED:
		LOG_DBG("Bus suspended");
		break;
	case UHC_EVT_RESUMED:
		LOG_DBG("Bus resumed");
		break;
	case UHC_EVT_RWUP:
		LOG_DBG("RWUP event");
		break;
	case UHC_EVT_ERROR:
		LOG_DBG("Error event %d", event->status);
		break;
	default:
		break;
	};

	return ret;
}

static void usbh_bus_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_contex *uhs_ctx;
	struct uhc_event event;

	while (true) {
		k_msgq_get(&usbh_bus_msgq, &event, K_FOREVER);

		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		usbh_event_handler(uhs_ctx, &event);
	}
}

static void usbh_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_contex *uhs_ctx;
	struct uhc_event event;
	usbh_udev_cb_t cb;
	int ret;

	while (true) {
		k_msgq_get(&usbh_msgq, &event, K_FOREVER);

		__ASSERT(event.type == UHC_EVT_EP_REQUEST, "Wrong event type");
		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		cb = event.xfer->cb;

		if (event.xfer->cb) {
			ret = cb(event.xfer->udev, event.xfer);
		} else {
			ret = discard_ep_request(uhs_ctx, event.xfer);
		}

		if (ret) {
			LOG_ERR("Failed to handle request completion callback");
		}
	}
}

int usbh_init_device_intl(struct usbh_contex *const uhs_ctx)
{
	int ret;

	ret = uhc_init(uhs_ctx->dev, usbh_event_carrier, uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	sys_dlist_init(&uhs_ctx->udevs);

	STRUCT_SECTION_FOREACH(usbh_class_data, cdata) {
		/*
		 * For now, we have not implemented any class drivers,
		 * so just keep it as placeholder.
		 */
		break;
	}

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

	k_thread_create(&usbh_bus_thread_data, usbh_bus_stack,
			K_KERNEL_STACK_SIZEOF(usbh_bus_stack),
			usbh_bus_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh_bus");

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
