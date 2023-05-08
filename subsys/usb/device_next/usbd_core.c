/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/sys/slist.h>

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_config.h"
#include "usbd_init.h"
#include "usbd_ch9.h"
#include "usbd_class.h"
#include "usbd_class_api.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_core, CONFIG_USBD_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbd_stack, CONFIG_USBD_THREAD_STACK_SIZE);
static struct k_thread usbd_thread_data;

K_MSGQ_DEFINE(usbd_msgq, sizeof(struct udc_event),
	      CONFIG_USBD_MAX_UDC_MSG, sizeof(uint32_t));

static int usbd_event_carrier(const struct device *dev,
			      const struct udc_event *const event)
{
	return k_msgq_put(&usbd_msgq, event, K_NO_WAIT);
}

static int event_handler_ep_request(struct usbd_contex *const uds_ctx,
				    const struct udc_event *const event)
{
	struct udc_buf_info *bi;
	int ret;

	bi = udc_get_buf_info(event->buf);

	if (USB_EP_GET_IDX(bi->ep) == 0) {
		ret = usbd_handle_ctrl_xfer(uds_ctx, event->buf, bi->err);
	} else {
		ret = usbd_class_handle_xfer(uds_ctx, event->buf, bi->err);
	}

	if (ret) {
		LOG_ERR("unrecoverable error %d, ep 0x%02x, buf %p",
			ret, bi->ep, event->buf);
		/* TODO: Shutdown USB device gracefully */
		k_panic();
	}

	return ret;
}

static void usbd_class_bcast_event(struct usbd_contex *const uds_ctx,
				   struct udc_event *const event)
{
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;

	if (!usbd_state_is_configured(uds_ctx)) {
		return;
	}

	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		LOG_ERR("Failed to get cfg_nd, despite configured state");
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		switch (event->type) {
		case UDC_EVT_SUSPEND:
			usbd_class_suspended(c_nd);
			break;
		case UDC_EVT_RESUME:
			usbd_class_resumed(c_nd);
			break;
		default:
			break;
		}
	}
}

static int event_handler_bus_reset(struct usbd_contex *const uds_ctx)
{
	int ret;

	LOG_WRN("Bus reset event");

	usbd_status_suspended(uds_ctx, false);
	ret = udc_set_address(uds_ctx->dev, 0);
	if (ret) {
		LOG_ERR("Failed to set default address after bus reset");
		return ret;
	}

	ret = usbd_config_set(uds_ctx, 0);
	if (ret) {
		LOG_ERR("Failed to set default state after bus reset");
		return ret;
	}

	/* There might be pending data stage transfer */
	if (usbd_ep_dequeue(uds_ctx, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to dequeue control IN");
	}

	LOG_INF("Actual device speed %d", udc_device_speed(uds_ctx->dev));
	uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;

	return 0;
}

/* TODO: Add event broadcaster to user application */
static int ALWAYS_INLINE usbd_event_handler(struct usbd_contex *const uds_ctx,
					    struct udc_event *const event)
{
	int ret = 0;

	switch (event->type) {
	case UDC_EVT_VBUS_REMOVED:
		LOG_WRN("VBUS remove event");
		break;
	case UDC_EVT_VBUS_READY:
		LOG_WRN("VBUS detected event");
		break;
	case UDC_EVT_SUSPEND:
		LOG_WRN("SUSPEND event");
		usbd_status_suspended(uds_ctx, true);
		usbd_class_bcast_event(uds_ctx, event);
		break;
	case UDC_EVT_RESUME:
		LOG_WRN("RESUME event");
		usbd_status_suspended(uds_ctx, false);
		usbd_class_bcast_event(uds_ctx, event);
		break;
	case UDC_EVT_SOF:
		usbd_class_bcast_event(uds_ctx, event);
		break;
	case UDC_EVT_RESET:
		LOG_WRN("RESET event");
		ret = event_handler_bus_reset(uds_ctx);
		break;
	case UDC_EVT_EP_REQUEST:
		ret = event_handler_ep_request(uds_ctx, event);
		break;
	case UDC_EVT_ERROR:
		LOG_ERR("Error event");
		break;
	default:
		break;
	};

	return ret;
}

static void usbd_thread(void)
{
	struct udc_event event;

	while (true) {
		k_msgq_get(&usbd_msgq, &event, K_FOREVER);

		STRUCT_SECTION_FOREACH(usbd_contex, uds_ctx) {
			if (uds_ctx->dev == event.dev) {
				usbd_event_handler(uds_ctx, &event);
			}
		}
	}
}

int usbd_device_init_core(struct usbd_contex *const uds_ctx)
{
	int ret;

	ret = udc_init(uds_ctx->dev, usbd_event_carrier);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	usbd_set_config_value(uds_ctx, 0);

	ret = usbd_init_configurations(uds_ctx);
	if (ret != 0) {
		udc_shutdown(uds_ctx->dev);
		return ret;
	}

	return ret;
}

int usbd_device_shutdown_core(struct usbd_contex *const uds_ctx)
{
	return udc_shutdown(uds_ctx->dev);
}

static int usbd_pre_init(void)
{
	k_thread_create(&usbd_thread_data, usbd_stack,
			K_KERNEL_STACK_SIZEOF(usbd_stack),
			(k_thread_entry_t)usbd_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&usbd_thread_data, "usbd");

	LOG_DBG("Available USB class nodes:");
	STRUCT_SECTION_FOREACH(usbd_class_node, node) {
		atomic_set(&node->data->state, 0);
		LOG_DBG("\t%p, name %s", node, node->name);
	}

	return 0;
}

SYS_INIT(usbd_pre_init, POST_KERNEL, CONFIG_USBD_THREAD_INIT_PRIO);
