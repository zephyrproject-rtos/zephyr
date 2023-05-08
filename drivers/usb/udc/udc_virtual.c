/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  udc_virtual.c
 * @brief Virtual USB device controller (UDC) driver
 *
 * Virtual device controller does not emulate any hardware
 * and can only communicate with the virtual host controller
 * through virtual bus.
 */

#include "udc_common.h"
#include "../uvb/uvb.h"

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_vrt, CONFIG_UDC_DRIVER_LOG_LEVEL);

struct udc_vrt_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	struct uvb_node *dev_node;
	int speed_idx;
	const char *uhc_name;
};

struct udc_vrt_data {
	struct k_fifo fifo;
	struct k_thread thread_data;
	uint8_t addr;
};

struct udc_vrt_event {
	sys_snode_t node;
	enum uvb_event_type type;
	struct uvb_packet *pkt;
};

K_MEM_SLAB_DEFINE(udc_vrt_slab, sizeof(struct udc_vrt_event),
		  16, sizeof(void *));

/* Reuse request packet for reply */
static int vrt_request_reply(const struct device *dev,
			     struct uvb_packet *const pkt,
			     const enum uvb_reply reply)
{
	const struct udc_vrt_config *config = dev->config;

	pkt->reply = reply;

	return uvb_reply_pkt(config->dev_node, pkt);
}

static void ctrl_ep_clear_halt(const struct device *dev)
{
	struct udc_ep_config *cfg;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	cfg->stat.halted = false;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	cfg->stat.halted = false;
}

static int vrt_ctrl_feed_dout(const struct device *dev,
			      const size_t length)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return 0;
}

static int vrt_handle_setup(const struct device *dev,
			    struct uvb_packet *const pkt)
{
	struct net_buf *buf;
	int err, ret;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, pkt->data, pkt->length);
	udc_ep_buf_set_setup(buf);
	ctrl_ep_clear_halt(dev);

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s: %p | feed for -out-", buf);
		err = vrt_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			/*
			 * Pass it on to the higher level which will
			 * halt control OUT endpoint.
			 */
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s: %p | submit for -in-", buf);
		/* Allocate buffer for data IN and submit to upper layer */
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p | submit for -status", buf);
		/*
		 * For all other cases we feed with a buffer
		 * large enough for setup packet.
		 */
		err = udc_ctrl_submit_s_status(dev);
	}

	ret = vrt_request_reply(dev, pkt, UVB_REPLY_ACK);

	return ret ? ret : err;
}

static int vrt_handle_ctrl_out(const struct device *dev,
			       struct net_buf *const buf)
{
	int err = 0;

	if (udc_ctrl_stage_is_status_out(dev)) {
		/* Status stage finished, notify upper layer */
		err = udc_ctrl_submit_status(dev, buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_in(dev)) {
		return udc_ctrl_submit_s_out_status(dev, buf);
	}

	return err;
}

static int vrt_handle_out(const struct device *dev,
			  struct uvb_packet *const pkt)
{
	struct udc_ep_config *ep_cfg;
	const uint8_t ep = pkt->ep;
	struct net_buf *buf;
	size_t min_len;
	int err = 0;
	int ret;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (ep_cfg->stat.halted) {
		LOG_DBG("reply STALL ep 0x%02x", ep);
		return vrt_request_reply(dev, pkt, UVB_REPLY_STALL);
	}

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		LOG_DBG("reply NACK ep 0x%02x", ep);
		return vrt_request_reply(dev, pkt, UVB_REPLY_NACK);
	}

	min_len = MIN(pkt->length, net_buf_tailroom(buf));
	net_buf_add_mem(buf, pkt->data, min_len);

	LOG_DBG("Handle data OUT, %zu | %zu", pkt->length, net_buf_tailroom(buf));

	if (net_buf_tailroom(buf) == 0 || pkt->length < ep_cfg->mps) {
		buf = udc_buf_get(dev, ep);

		if (ep == USB_CONTROL_EP_OUT) {
			err = vrt_handle_ctrl_out(dev, buf);
		} else {
			err = udc_submit_ep_event(dev, buf, 0);
		}
	}

	ret = vrt_request_reply(dev, pkt, UVB_REPLY_ACK);

	return ret ? ret : err;
}

static int isr_handle_ctrl_in(const struct device *dev,
			      struct net_buf *const buf)
{
	int err = 0;

	if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
		/* Status stage finished, notify upper layer */
		err = udc_ctrl_submit_status(dev, buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_out(dev)) {
		/*
		 * IN transfer finished, release buffer,
		 * Feed control OUT buffer for status stage.
		 */
		net_buf_unref(buf);
		return vrt_ctrl_feed_dout(dev, 0);
	}

	return err;
}

static int vrt_handle_in(const struct device *dev,
			 struct uvb_packet *const pkt)
{
	struct udc_ep_config *ep_cfg;
	const uint8_t ep = pkt->ep;
	struct net_buf *buf;
	size_t min_len;
	int err = 0;
	int ret;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (ep_cfg->stat.halted) {
		LOG_DBG("reply STALL ep 0x%02x", ep);
		return vrt_request_reply(dev, pkt, UVB_REPLY_STALL);
	}

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		LOG_DBG("reply NACK ep 0x%02x", ep);
		return vrt_request_reply(dev, pkt, UVB_REPLY_NACK);
	}

	LOG_DBG("Handle data IN, %zu | %u | %u",
		pkt->length, buf->len, ep_cfg->mps);
	min_len = MIN(pkt->length, buf->len);
	memcpy(pkt->data, buf->data, min_len);
	net_buf_pull(buf, min_len);
	pkt->length = min_len;

	if (buf->len == 0 || pkt->length < ep_cfg->mps) {
		if (udc_ep_buf_has_zlp(buf)) {
			udc_ep_buf_clear_zlp(buf);
			goto continue_in;
		}

		LOG_DBG("Finish data IN %zu | %u", pkt->length, buf->len);
		buf = udc_buf_get(dev, ep);

		if (ep == USB_CONTROL_EP_IN) {
			err = isr_handle_ctrl_in(dev, buf);
		} else {
			err = udc_submit_ep_event(dev, buf, 0);
		}
	}

continue_in:
	ret = vrt_request_reply(dev, pkt, UVB_REPLY_ACK);

	return ret ? ret : err;
}

static int vrt_handle_request(const struct device *dev,
			      struct uvb_packet *const pkt)
{
	LOG_DBG("REQUEST event for %p pkt %p", dev, pkt);

	if (USB_EP_GET_IDX(pkt->ep) == 0 && pkt->request == UVB_REQUEST_SETUP) {
		return vrt_handle_setup(dev, pkt);
	}

	if (USB_EP_DIR_IS_OUT(pkt->ep) && pkt->request == UVB_REQUEST_DATA) {
		return vrt_handle_out(dev, pkt);
	}

	if (USB_EP_DIR_IS_IN(pkt->ep) && pkt->request == UVB_REQUEST_DATA) {
		return vrt_handle_in(dev, pkt);
	}

	return -ENOTSUP;
}

static ALWAYS_INLINE void udc_vrt_thread_handler(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct udc_vrt_data *priv = udc_get_private(dev);

	while (true) {
		struct udc_vrt_event *vrt_ev;
		int err = 0;

		vrt_ev = k_fifo_get(&priv->fifo, K_FOREVER);

		switch (vrt_ev->type) {
		case UVB_EVT_VBUS_REMOVED:
			err = udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
			break;
		case UVB_EVT_VBUS_READY:
			err = udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
			break;
		case UVB_EVT_SUSPEND:
			err = udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
			break;
		case UVB_EVT_RESUME:
			err = udc_submit_event(dev, UDC_EVT_RESUME, 0);
			break;
		case UVB_EVT_RESET:
			err = udc_submit_event(dev, UDC_EVT_RESET, 0);
			break;
		case UVB_EVT_REQUEST:
			err = vrt_handle_request(dev, vrt_ev->pkt);
			break;
		default:
			break;
		};

		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}

		k_mem_slab_free(&udc_vrt_slab, (void **)&vrt_ev);
	}
}

static void vrt_submit_uvb_event(const struct device *dev,
				 const enum uvb_event_type type,
				 struct uvb_packet *const pkt)
{
	struct udc_vrt_data *priv = udc_get_private(dev);
	struct udc_vrt_event *vrt_ev;
	int ret;

	ret = k_mem_slab_alloc(&udc_vrt_slab, (void **)&vrt_ev, K_NO_WAIT);
	__ASSERT(ret == 0, "Failed to allocate slab");

	vrt_ev->type = type;
	vrt_ev->pkt = pkt;
	k_fifo_put(&priv->fifo, vrt_ev);
}

static void udc_vrt_uvb_cb(const void *const vrt_priv,
			   const enum uvb_event_type type,
			   const void *data)
{
	const struct device *dev = vrt_priv;
	struct udc_vrt_data *priv = udc_get_private(dev);
	struct uvb_packet *const pkt = (void *)data;

	switch (type) {
	case UVB_EVT_VBUS_REMOVED:
	case UVB_EVT_VBUS_READY:
		if (udc_is_initialized(dev)) {
			vrt_submit_uvb_event(dev, type, NULL);
		}
		break;
	case UVB_EVT_SUSPEND:
	case UVB_EVT_RESUME:
	case UVB_EVT_RESET:
	case UVB_EVT_REQUEST:
		if (udc_is_enabled(dev) && priv->addr == pkt->addr) {
			vrt_submit_uvb_event(dev, type, pkt);
		}
		break;
	default:
		LOG_ERR("Unknown event for %p", dev);
		break;
	};
}

static int udc_vrt_ep_enqueue(const struct device *dev,
			      struct udc_ep_config *cfg,
			      struct net_buf *buf)
{
	LOG_DBG("%p enqueue %p", dev, buf);
	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	return 0;
}

static int udc_vrt_ep_dequeue(const struct device *dev,
			      struct udc_ep_config *cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();
	/* Draft dequeue implementation */
	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}
	irq_unlock(lock_key);

	return 0;
}

static int udc_vrt_ep_enable(const struct device *dev,
			     struct udc_ep_config *cfg)
{
	return 0;
}

static int udc_vrt_ep_disable(const struct device *dev,
			      struct udc_ep_config *cfg)
{
	return 0;
}

static int udc_vrt_ep_set_halt(const struct device *dev,
			       struct udc_ep_config *cfg)
{
	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	cfg->stat.halted = true;

	return 0;
}

static int udc_vrt_ep_clear_halt(const struct device *dev,
				 struct udc_ep_config *cfg)
{
	cfg->stat.halted = false;

	return 0;
}

static int udc_vrt_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_vrt_data *priv = udc_get_private(dev);

	priv->addr = addr;
	LOG_DBG("Set new address %u for %p", priv->addr, dev);

	return 0;
}

static int udc_vrt_host_wakeup(const struct device *dev)
{

	const struct udc_vrt_config *config = dev->config;

	return uvb_to_host(config->dev_node, UVB_EVT_DEVICE_ACT,
			   INT_TO_POINTER(UVB_DEVICE_ACT_RWUP));
}

static enum udc_bus_speed udc_vrt_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	/* FIXME: get actual device speed */
	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_vrt_enable(const struct device *dev)
{
	const struct udc_vrt_config *config = dev->config;
	enum uvb_device_act act;

	switch (config->speed_idx) {
	case 1:
		act = UVB_DEVICE_ACT_FS;
		break;
	case 2:
		act = UVB_DEVICE_ACT_HS;
		break;
	case 3:
		act = UVB_DEVICE_ACT_SS;
		break;
	case 0:
	default:
		act = UVB_DEVICE_ACT_LS;
		break;
	}

	return uvb_to_host(config->dev_node, UVB_EVT_DEVICE_ACT,
			   INT_TO_POINTER(act));
}

static int udc_vrt_disable(const struct device *dev)
{
	const struct udc_vrt_config *config = dev->config;

	return uvb_to_host(config->dev_node, UVB_EVT_DEVICE_ACT,
			   INT_TO_POINTER(UVB_DEVICE_ACT_REMOVED));
}

static int udc_vrt_init(const struct device *dev)
{
	const struct udc_vrt_config *config = dev->config;

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	return uvb_subscribe(config->uhc_name, config->dev_node);
}

static int udc_vrt_shutdown(const struct device *dev)
{
	const struct udc_vrt_config *config = dev->config;

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return uvb_unsubscribe(config->uhc_name, config->dev_node);
}

static int udc_vrt_driver_preinit(const struct device *dev)
{
	const struct udc_vrt_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_vrt_data *priv = data->priv;
	uint16_t mps = 1023;
	int err;

	k_mutex_init(&data->mutex);
	k_fifo_init(&priv->fifo);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->speed_idx == 2) {
		data->caps.hs = true;
		mps = 1024;
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->dev_node->priv = dev;
	config->make_thread(dev);
	LOG_INF("Device %p (max. speed %d) belongs to %s",
		dev, config->speed_idx, config->uhc_name);

	return 0;
}

static int udc_vrt_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int udc_vrt_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

static const struct udc_api udc_vrt_api = {
	.lock = udc_vrt_lock,
	.unlock = udc_vrt_unlock,
	.device_speed = udc_vrt_device_speed,
	.init = udc_vrt_init,
	.enable = udc_vrt_enable,
	.disable = udc_vrt_disable,
	.shutdown = udc_vrt_shutdown,
	.set_address = udc_vrt_set_address,
	.host_wakeup = udc_vrt_host_wakeup,
	.ep_enable = udc_vrt_ep_enable,
	.ep_disable = udc_vrt_ep_disable,
	.ep_set_halt = udc_vrt_ep_set_halt,
	.ep_clear_halt = udc_vrt_ep_clear_halt,
	.ep_enqueue = udc_vrt_ep_enqueue,
	.ep_dequeue = udc_vrt_ep_dequeue,
};

#define DT_DRV_COMPAT zephyr_udc_virtual

#define UDC_VRT_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(udc_vrt_stack_area_##n,				\
			      CONFIG_UDC_VIRTUAL_STACK_SIZE);			\
										\
	static void udc_vrt_thread_##n(void *dev, void *unused1, void *unused2)	\
	{									\
		while (1) {							\
			udc_vrt_thread_handler(dev);				\
		}								\
	}									\
										\
	static void udc_vrt_make_thread_##n(const struct device *dev)		\
	{									\
		struct udc_vrt_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
			    udc_vrt_stack_area_##n,				\
			    K_THREAD_STACK_SIZEOF(udc_vrt_stack_area_##n),	\
			    udc_vrt_thread_##n,					\
			    (void *)dev, NULL, NULL,				\
			    K_PRIO_COOP(CONFIG_UDC_VIRTUAL_THREAD_PRIORITY),	\
			    K_ESSENTIAL,					\
			    K_NO_WAIT);						\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static struct udc_ep_config						\
		ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];		\
	static struct udc_ep_config						\
		ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];		\
										\
	static struct uvb_node udc_vrt_dev_node##n = {				\
		.name = DT_NODE_FULL_NAME(DT_DRV_INST(n)),			\
		.notify = udc_vrt_uvb_cb,					\
	};									\
										\
	static const struct udc_vrt_config udc_vrt_config_##n = {		\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		\
		.ep_cfg_in = ep_cfg_out,					\
		.ep_cfg_out = ep_cfg_in,					\
		.make_thread = udc_vrt_make_thread_##n,				\
		.dev_node = &udc_vrt_dev_node##n,				\
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),	\
		.uhc_name = DT_NODE_FULL_NAME(DT_INST_PARENT(n)),		\
	};									\
										\
	static struct udc_vrt_data udc_priv_##n = {				\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, udc_vrt_driver_preinit, NULL,			\
			      &udc_data_##n, &udc_vrt_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &udc_vrt_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_VRT_DEVICE_DEFINE)
