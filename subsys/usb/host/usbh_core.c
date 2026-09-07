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

/*
 * The bus events and the work of the class instances that needs synchronous
 * requests, like the hub port handling, are serialized on this work queue.
 */
static K_KERNEL_STACK_DEFINE(usbh_bus_stack, CONFIG_USBH_STACK_SIZE);
static struct k_work_q usbh_bus_wq;
static struct k_work usbh_bus_event_work;

K_MSGQ_DEFINE_STATIC_TYPE(usbh_msgq, struct uhc_event, CONFIG_USBH_MAX_UHC_MSG);
K_MSGQ_DEFINE_STATIC_TYPE(usbh_bus_msgq, struct uhc_event, CONFIG_USBH_MAX_UHC_MSG);

static int usbh_event_carrier(const struct device *dev,
			      const struct uhc_event *const event)
{
	int err;

	if (event->type == UHC_EVT_EP_REQUEST) {
		err = k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
	} else {
		err = k_msgq_put(&usbh_bus_msgq, event, K_NO_WAIT);
		if (err == 0) {
			(void)k_work_submit_to_queue(&usbh_bus_wq, &usbh_bus_event_work);
		}
	}

	return err;
}

int usbh_bus_work_submit(struct k_work *const work)
{
	return k_work_submit_to_queue(&usbh_bus_wq, work);
}

static void dev_connected_handler(struct usbh_context *const ctx,
				  const struct uhc_event *const event)
{
	struct usb_device *udev;

	udev = usbh_device_alloc(ctx);

	if (udev == NULL) {
		LOG_ERR("Failed allocate new device");
		return;
	}

	switch (event->type) {
	case UHC_EVT_DEV_CONNECTED_HS:
		udev->speed = USB_SPEED_SPEED_HS;
		break;
	case UHC_EVT_DEV_CONNECTED_FS:
		udev->speed = USB_SPEED_SPEED_FS;
		break;
	case UHC_EVT_DEV_CONNECTED_LS:
		udev->speed = USB_SPEED_SPEED_LS;
		break;
	default:
		LOG_ERR("USB device speed not supported");
		return;
	}

	(void)usbh_device_connect(ctx, udev);
}

static void dev_removed_handler(struct usbh_context *const ctx)
{
	struct usb_device *udev = NULL;

	udev = usbh_device_get_root(ctx);
	if (udev != NULL) {
		usbh_device_disconnect(ctx, udev);
	} else {
		LOG_DBG("Spurious device removed event");
	}
}

static int discard_ep_request(struct usbh_context *const ctx,
			      struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_INF(xfer->buf->data, xfer->buf->len, "buf");
		uhc_xfer_buf_free(dev, xfer->buf);
	}

	return uhc_xfer_free(dev, xfer);
}

static ALWAYS_INLINE int usbh_event_handler(struct usbh_context *const ctx,
					    struct uhc_event *const event)
{
	int ret = 0;

	switch (event->type) {
	case UHC_EVT_DEV_CONNECTED_LS:
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

static void usbh_bus_event_work_handler(struct k_work *const work)
{
	struct usbh_context *uhs_ctx;
	struct uhc_event event;

	ARG_UNUSED(work);

	while (k_msgq_get(&usbh_bus_msgq, &event, K_NO_WAIT) == 0) {
		uhs_ctx = (void *)uhc_get_event_ctx(event.dev);
		usbh_event_handler(uhs_ctx, &event);
	}
}

static void usbh_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbh_context *uhs_ctx;
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
	static const struct k_work_queue_config usbh_bus_wq_cfg = {.name = "usbh_bus"};

	k_thread_create(&usbh_thread_data, usbh_stack,
			K_KERNEL_STACK_SIZEOF(usbh_stack),
			usbh_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh");

	k_work_init(&usbh_bus_event_work, usbh_bus_event_work_handler);
	k_work_queue_start(&usbh_bus_wq, usbh_bus_stack, K_KERNEL_STACK_SIZEOF(usbh_bus_stack),
			   K_PRIO_COOP(9), &usbh_bus_wq_cfg);

	usbh_class_init_all();

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
