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

static int usbd_event_carrier(const struct device *dev,
			      const struct udc_event *const event)
{
	struct usbd_context *const uds_ctx = (void *)udc_get_event_ctx(dev);
	k_spinlock_key_t key;

	if (event->type == UDC_EVT_EP_REQUEST) {
		key = k_spin_lock(&uds_ctx->ep_event_lock);
		sys_slist_append(&uds_ctx->ep_events, &event->buf->node);
		k_spin_unlock(&uds_ctx->ep_event_lock, key);
	}

	k_event_post(&uds_ctx->events, BIT(event->type));

	return 0;
}

static void ep_completion_handler(struct usbd_context *const uds_ctx)
{
	struct udc_buf_info *bi;
	k_spinlock_key_t key;
	struct net_buf *buf;
	sys_snode_t *node;
	int err;

	while (!sys_slist_is_empty(&uds_ctx->ep_events)) {
		key = k_spin_lock(&uds_ctx->ep_event_lock);
		node = sys_slist_get(&uds_ctx->ep_events);
		k_spin_unlock(&uds_ctx->ep_event_lock, key);

		buf = SYS_SLIST_CONTAINER(node, buf, node);
		if (buf == NULL) {
			break;
		}

		bi = udc_get_buf_info(buf);
		if (USB_EP_GET_IDX(bi->ep) == 0) {
			err = usbd_handle_ctrl_xfer(uds_ctx, buf, bi->err);
		} else {
			err = usbd_class_handle_xfer(uds_ctx, buf, bi->err);
		}

		if (err) {
			LOG_ERR("Unrecoverable error %d, ep 0x%02x, buf %p",
				err, bi->ep, (void *)buf);
			usbd_msg_pub_simple(uds_ctx, USBD_MSG_STACK_ERROR, err);
		}
	}
}

static void usbd_class_bcast_event(struct usbd_context *const uds_ctx,
				   enum udc_event_type type)
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
		switch (type) {
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
					     const uint32_t event)
{
	int err = 0;

	if (event == BIT(UDC_EVT_VBUS_REMOVED)) {
		LOG_DBG("VBUS remove event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_VBUS_REMOVED, 0);
	}

	if (event == BIT(UDC_EVT_VBUS_READY)) {
		LOG_DBG("VBUS detected event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_VBUS_READY, 0);
	}

	if (event == BIT(UDC_EVT_RESET)) {
		LOG_DBG("RESET event");
		err = event_handler_bus_reset(uds_ctx);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_RESET, 0);
	}

	if (event == BIT(UDC_EVT_SUSPEND)) {
		LOG_DBG("SUSPEND event");
		usbd_status_suspended(uds_ctx, true);
		usbd_class_bcast_event(uds_ctx, UDC_EVT_SUSPEND);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_SUSPEND, 0);
	}

	if (event == BIT(UDC_EVT_RESUME)) {
		LOG_DBG("RESUME event");
		usbd_status_suspended(uds_ctx, false);
		usbd_class_bcast_event(uds_ctx, UDC_EVT_RESUME);
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_RESUME, 0);
	}

	if (event == BIT(UDC_EVT_SOF)) {
		usbd_class_bcast_event(uds_ctx, UDC_EVT_SOF);
	}

	if (event == BIT(UDC_EVT_ERROR)) {
		LOG_ERR("UDC error event");
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_UDC_ERROR, 0);
	}

	if (err) {
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_STACK_ERROR, err);
	}
}

static void usbd_thread(void *const p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct usbd_context *const uds_ctx = p1;

	while (true) {
		uint32_t events;

		events = k_event_wait(&uds_ctx->events, UINT32_MAX, false, K_FOREVER);
		__ASSERT(usbd_is_initialized(uds_ctx), "USB device is not initialized");
		k_event_clear(&uds_ctx->events, events);

		if (events == BIT(UDC_EVT_EP_REQUEST)) {
			ep_completion_handler(uds_ctx);
			events &= ~BIT(UDC_EVT_EP_REQUEST);
		}

		if (events) {
			usbd_event_handler(uds_ctx, events);
		}
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

	if (USBD_SUPPORTS_HIGH_SPEED) {
		SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->hs_configs, cfg_nd, node) {
			uint8_t cfg_value = usbd_config_get_value(cfg_nd);

			ret = usbd_class_remove_all(uds_ctx, USBD_SPEED_HS, cfg_value);
			if (ret) {
				LOG_ERR("Failed to cleanup registered classes, %d", ret);
			}
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

	STRUCT_SECTION_FOREACH(usbd_context, ctx) {
		k_event_init(&ctx->events);
		k_thread_create(ctx->thread_data, ctx->thread_stack,
				ctx->stack_size,
				usbd_thread,
				ctx, NULL, NULL,
				K_PRIO_COOP(8), 0, K_NO_WAIT);

		k_thread_name_set(ctx->thread_data, ctx->name);
	}

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
