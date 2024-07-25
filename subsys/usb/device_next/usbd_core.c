/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_desc.h"
#include "usbd_config.h"
#include "usbd_init.h"
#include "usbd_ch9.h"
#include "usbd_class.h"
#include "usbd_class_api.h"
#include "usbd_msg.h"

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

static int event_handler_ep_request(struct usbd_context *const uds_ctx,
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
	}

	return ret;
}

static void usbd_class_bcast_event(struct usbd_context *const uds_ctx,
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
			usbd_class_suspended(c_nd->c_data);
			break;
		case UDC_EVT_RESUME:
			usbd_class_resumed(c_nd->c_data);
			break;
		case UDC_EVT_SOF:
			usbd_class_sof(c_nd->c_data);
			break;
		default:
			break;
		}
	}
}

static int event_handler_bus_reset(struct usbd_context *const uds_ctx)
{
	enum udc_bus_speed udc_speed;
	int ret;

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

	LOG_INF("Actual device speed %u", udc_device_speed(uds_ctx->dev));
	udc_speed = udc_device_speed(uds_ctx->dev);
	switch (udc_speed) {
	case UDC_BUS_SPEED_HS:
		uds_ctx->status.speed = USBD_SPEED_HS;
		break;
	default:
		uds_ctx->status.speed = USBD_SPEED_FS;
	}

	uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;

	uds_ctx->status.rwup = false;

	return 0;
}


static ALWAYS_INLINE void usbd_event_handler(struct usbd_context *const uds_ctx,
					     struct udc_event *const event)
{
	int err = 0;

	switch (event->type) {
	case UDC_EVT_VBUS_REMOVED:
		LOG_DBG("VBUS remove event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_VBUS_REMOVED, 0);
		break;
	case UDC_EVT_VBUS_READY:
		LOG_DBG("VBUS detected event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_VBUS_READY, 0);
		break;
	case UDC_EVT_SUSPEND:
		LOG_DBG("SUSPEND event");
		usbd_status_suspended(uds_ctx, true);
		usbd_class_bcast_event(uds_ctx, event);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_SUSPEND, 0);
		break;
	case UDC_EVT_RESUME:
		LOG_DBG("RESUME event");
		usbd_status_suspended(uds_ctx, false);
		usbd_class_bcast_event(uds_ctx, event);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_RESUME, 0);
		break;
	case UDC_EVT_SOF:
		usbd_class_bcast_event(uds_ctx, event);
		break;
	case UDC_EVT_RESET:
		LOG_DBG("RESET event");
		err = event_handler_bus_reset(uds_ctx);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_RESET, 0);
		break;
	case UDC_EVT_EP_REQUEST:
		err = event_handler_ep_request(uds_ctx, event);
		break;
	case UDC_EVT_ERROR:
		LOG_ERR("UDC error event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_UDC_ERROR, event->status);
		break;
	default:
		break;
	};

	if (err) {
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_STACK_ERROR, err);
	}
}

static void usbd_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct usbd_context *uds_ctx;
	struct udc_event event;

	while (true) {
		k_msgq_get(&usbd_msgq, &event, K_FOREVER);

		uds_ctx = (void *)udc_get_event_ctx(event.dev);
		__ASSERT(uds_ctx != NULL && usbd_is_initialized(uds_ctx),
			 "USB device is not initialized");
		usbd_event_handler(uds_ctx, &event);
	}
}

int usbd_device_init_core(struct usbd_context *const uds_ctx)
{
	int ret;

	ret = udc_init(uds_ctx->dev, usbd_event_carrier, uds_ctx);
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

int usbd_device_shutdown_core(struct usbd_context *const uds_ctx)
{
	struct usbd_config_node *cfg_nd;
	int ret;

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->hs_configs, cfg_nd, node) {
		uint8_t cfg_value = usbd_config_get_value(cfg_nd);

		ret = usbd_class_remove_all(uds_ctx, USBD_SPEED_HS, cfg_value);
		if (ret) {
			LOG_ERR("Failed to cleanup registered classes, %d", ret);
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->fs_configs, cfg_nd, node) {
		uint8_t cfg_value = usbd_config_get_value(cfg_nd);

		ret = usbd_class_remove_all(uds_ctx, USBD_SPEED_FS, cfg_value);
		if (ret) {
			LOG_ERR("Failed to cleanup registered classes, %d", ret);
		}
	}

	ret = usbd_desc_remove_all(uds_ctx);
	if (ret) {
		LOG_ERR("Failed to cleanup descriptors, %d", ret);
	}

	usbd_device_unregister_all_vreq(uds_ctx);

	return udc_shutdown(uds_ctx->dev);
}

static int usbd_pre_init(void)
{
	k_thread_create(&usbd_thread_data, usbd_stack,
			K_KERNEL_STACK_SIZEOF(usbd_stack),
			usbd_thread,
			NULL, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&usbd_thread_data, "usbd");

	LOG_DBG("Available USB class iterators:");
	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		atomic_set(&c_nd->state, 0);
		LOG_DBG("\t%p->%p, name %s", c_nd, c_nd->c_data, c_nd->c_data->name);
	}
	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
		atomic_set(&c_nd->state, 0);
		LOG_DBG("\t%p->%p, name %s", c_nd, c_nd->c_data, c_nd->c_data->name);
	}

	return 0;
}

SYS_INIT(usbd_pre_init, POST_KERNEL, CONFIG_USBD_THREAD_INIT_PRIO);
