/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/barrier.h>

#include "usbh_internal.h"
#include "usbh_device.h"
#include "usbh_host.h"
#include "usbh_class.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs, CONFIG_USBH_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbh_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_thread_data;

static K_KERNEL_STACK_DEFINE(usbh_bus_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_bus_thread_data;

K_MSGQ_DEFINE(usbh_msgq, sizeof(struct uhc_event), CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

K_MSGQ_DEFINE(usbh_bus_msgq, sizeof(struct uhc_event), CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

static int usbh_event_carrier(const struct device *dev, const struct uhc_event *const event)
{
	int err;

	if (event->type == UHC_EVT_EP_REQUEST) {
		err = k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
	} else {
		err = k_msgq_put(&usbh_bus_msgq, event, K_NO_WAIT);
	}

	if (err != 0) {
		LOG_ERR("USB host event queue full (type=%d)", (int)event->type);
	}

	return err;
}

static void usbh_root_publish(struct usbh_context *const ctx, struct usb_device *udev)
{
	usbh_host_lock(ctx);
	ctx->root = udev;
	usbh_host_unlock(ctx);
}

static struct usb_device *usbh_root_take(struct usbh_context *const ctx)
{
	struct usb_device *udev;

	usbh_host_lock(ctx);
	udev = ctx->root;
	ctx->root = NULL;
	usbh_host_unlock(ctx);

	return udev;
}

static void usbh_device_detach(struct usbh_context *const ctx)
{
	struct usb_device *udev = usbh_root_take(ctx);

	if (udev == NULL) {
		return;
	}

	barrier_dmem_fence_full();
	usbh_device_removed_notify(udev);
	usbh_class_remove_all(udev);
	usbh_device_free(udev);
}

static void dev_connected_handler(struct usbh_context *const ctx,
				  const struct uhc_event *const event)
{
	const char *const tname = k_thread_name_get(k_current_get());
	struct usb_device *udev;
	struct usb_device *prev;
	int init_err;

	LOG_DBG("trace: dev_connected thread=%s prio=%d speed_evt=%d", tname != NULL ? tname : "?",
		k_thread_priority_get(k_current_get()), (int)event->type);

	prev = usbh_root_take(ctx);
	if (prev != NULL) {
		LOG_WRN("Replacing connected USB device");
		barrier_dmem_fence_full();
		usbh_device_removed_notify(prev);
		usbh_class_remove_all(prev);
		usbh_device_free(prev);
		uhc_free_dev(ctx->dev);
	}

	udev = usbh_device_alloc(ctx);
	if (udev == NULL) {
		LOG_ERR("Failed allocate new device");
		uhc_free_dev(ctx->dev);
		return;
	}

	udev->state = USB_STATE_DEFAULT;

	if (event->type == UHC_EVT_DEV_CONNECTED_HS) {
		udev->speed = USB_SPEED_SPEED_HS;
	} else if (event->type == UHC_EVT_DEV_CONNECTED_LS) {
		udev->speed = USB_SPEED_SPEED_LS;
	} else if (event->type == UHC_EVT_DEV_CONNECTED_SS) {
		udev->speed = USB_SPEED_SPEED_SS;
	} else {
		udev->speed = USB_SPEED_SPEED_FS;
	}

	usbh_root_publish(ctx, udev);

	init_err = usbh_device_init(udev);
	LOG_DBG("trace: usbh_device_init done err=%d thread=%s root=%p", init_err,
		tname != NULL ? tname : "?", (void *)udev);

	if (init_err != 0) {
		struct usb_device *failed = usbh_root_take(ctx);

		LOG_ERR("Failed to reset new USB device");
		if (failed != NULL) {
			usbh_device_free(failed);
		}
		uhc_free_dev(ctx->dev);
	}
}

static void dev_removed_handler(struct usbh_context *const ctx)
{
	if (ctx->root != NULL) {
		usbh_device_detach(ctx);
		LOG_DBG("Device removed");
	} else {
		LOG_DBG("Spurious device removed event");
	}

	uhc_free_dev(ctx->dev);
}

static int discard_ep_request(struct usbh_context *const ctx, struct uhc_transfer *const xfer)
{
	const struct device *dev = ctx->dev;

	if (xfer->buf) {
		LOG_HEXDUMP_DBG(xfer->buf->data, xfer->buf->len, "buf");
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
	case UHC_EVT_DEV_CONNECTED_SS:
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

	struct usbh_context *uhs_ctx;
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
	k_thread_create(&usbh_thread_data, usbh_stack, K_KERNEL_STACK_SIZEOF(usbh_stack),
			usbh_thread, NULL, NULL, NULL, K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh");

	k_thread_create(&usbh_bus_thread_data, usbh_bus_stack,
			K_KERNEL_STACK_SIZEOF(usbh_bus_stack), usbh_bus_thread, NULL, NULL, NULL,
			K_PRIO_COOP(9), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_bus_thread_data, "usbh_bus");

	usbh_class_init_all();

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
