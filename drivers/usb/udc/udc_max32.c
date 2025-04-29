/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>

#include <wrap_max32_usb.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_max32, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT adi_max32_usbhs

enum udc_max32_event_type {
	/* Shim driver event to trigger transfer */
	UDC_MAX32_EVT_XFER,
	/* Setup event */
	UDC_MAX32_EVT_SETUP,
};

struct udc_max32_evt {
	enum udc_max32_event_type type;
	struct udc_ep_config *ep_cfg;
};

K_MSGQ_DEFINE(drv_msgq, sizeof(struct udc_max32_evt), CONFIG_UDC_MAX32_MAX_QMESSAGES,
	      sizeof(uint32_t));

struct udc_max32_config {
	mxc_usbhs_regs_t *base;
	size_t num_of_in_eps;
	size_t num_of_out_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	int speed_idx;
	const struct device *clock;
	struct max32_perclk perclk;
	void (*irq_func)(void);
};

struct req_cb_data {
	const struct device *dev;
	uint8_t ep;
};

struct udc_max32_data {
	struct k_thread thread_data;
	MXC_USB_Req_t *ep_request;
	struct req_cb_data *req_cb_data;
};

static void udc_event_xfer_ctrl_status(const struct device *dev, struct net_buf *const buf)
{
	if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
		MXC_USB_Ackstat(0);

		/* Status stage finished, notify upper layer */
		udc_ctrl_submit_status(dev, buf);
	}

	if (udc_ctrl_stage_is_data_in(dev)) {
		/*
		 * s-in-[status] finished, release buffer.
		 * Since the controller supports auto-status we cannot use
		 * if (udc_ctrl_stage_is_status_out()) after state update.
		 */
		net_buf_unref(buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);
}

static void udc_event_xfer_in_callback(void *cbdata)
{
	struct req_cb_data *req_cb_data = (struct req_cb_data *)cbdata;
	struct udc_max32_data *priv = udc_get_private(req_cb_data->dev);
	MXC_USB_Req_t *ep_request = &priv->ep_request[USB_EP_GET_IDX(req_cb_data->ep)];
	struct net_buf *buf;

	buf = udc_buf_get(udc_get_ep_cfg(req_cb_data->dev, req_cb_data->ep));

	udc_ep_set_busy(udc_get_ep_cfg(req_cb_data->dev, req_cb_data->ep), false);

	if (ep_request->error_code) {
		LOG_ERR("ep 0x%02x error: %x", req_cb_data->ep, ep_request->error_code);
		udc_submit_ep_event(req_cb_data->dev, buf, ep_request->error_code);
		return;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
	}

	if (req_cb_data->ep == USB_CONTROL_EP_IN) {
		udc_event_xfer_ctrl_status(req_cb_data->dev, buf);
	} else {
		udc_submit_ep_event(req_cb_data->dev, buf, 0);
	}
}

static void udc_event_xfer_out_callback(void *cbdata)
{
	struct req_cb_data *req_cb_data = (struct req_cb_data *)cbdata;
	struct udc_max32_data *priv = udc_get_private(req_cb_data->dev);
	MXC_USB_Req_t *ep_request = &priv->ep_request[USB_EP_GET_IDX(req_cb_data->ep)];
	struct net_buf *buf;

	buf = udc_buf_get(udc_get_ep_cfg(req_cb_data->dev, req_cb_data->ep));
	net_buf_add(buf, ep_request->actlen);

	udc_ep_set_busy(udc_get_ep_cfg(req_cb_data->dev, req_cb_data->ep), false);

	if (ep_request->error_code) {
		LOG_ERR("ep 0x%02x error: %x", req_cb_data->ep, ep_request->error_code);
		udc_submit_ep_event(req_cb_data->dev, buf, ep_request->error_code);
		return;
	}

	if (req_cb_data->ep == USB_CONTROL_EP_OUT) {
		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(req_cb_data->dev, buf);

		udc_ctrl_submit_s_out_status(req_cb_data->dev, buf);
	} else {
		udc_submit_ep_event(req_cb_data->dev, buf, 0);
	}
}

static void udc_event_xfer_in(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_max32_data *priv = udc_get_private(dev);
	MXC_USB_Req_t *ep_request = &priv->ep_request[USB_EP_GET_IDX(ep_cfg->addr)];
	struct req_cb_data *req_cb_data = &priv->req_cb_data[USB_EP_GET_IDX(ep_cfg->addr)];
	struct net_buf *buf;
	int ret;

	if (udc_ep_is_busy(ep_cfg)) {
		return;
	}

	memset(ep_request, 0, sizeof(MXC_USB_Req_t));

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("Failed to peek net_buf for ep 0x%02x", ep_cfg->addr);
		return;
	}

	if (buf->len == 0 && ep_cfg->addr == USB_CONTROL_EP_IN) {
		buf = udc_buf_get(ep_cfg);
		udc_event_xfer_ctrl_status(dev, buf);
		return;
	}

	req_cb_data->dev = dev;
	req_cb_data->ep = ep_cfg->addr;

	ep_request->ep = USB_EP_GET_IDX(ep_cfg->addr);
	ep_request->data = buf->data;
	ep_request->reqlen = buf->len;
	ep_request->actlen = 0;
	ep_request->error_code = 0;
	ep_request->callback = udc_event_xfer_in_callback;
	ep_request->cbdata = req_cb_data;
	if (udc_ep_buf_has_zlp(buf)) {
		ep_request->has_zlp = true;
	}

	udc_ep_set_busy(ep_cfg, true);
	ret = MXC_USB_WriteEndpoint(ep_request);
	if (ret != 0) {
		udc_ep_set_busy(ep_cfg, false);
		LOG_ERR("ep 0x%02x error: %x", ep_cfg->addr, ret);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	}
}

static void udc_event_xfer_out(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_max32_data *priv = udc_get_private(dev);
	MXC_USB_Req_t *ep_request = &priv->ep_request[USB_EP_GET_IDX(ep_cfg->addr)];
	struct req_cb_data *req_cb_data = &priv->req_cb_data[USB_EP_GET_IDX(ep_cfg->addr)];
	struct net_buf *buf;
	int ret;

	if (udc_ep_is_busy(ep_cfg)) {
		return;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("Failed to peek net_buf for ep 0x%02x", ep_cfg->addr);
		return;
	}

	req_cb_data->dev = dev;
	req_cb_data->ep = ep_cfg->addr;

	ep_request->ep = USB_EP_GET_IDX(ep_cfg->addr);
	ep_request->data = buf->data;
	ep_request->reqlen = buf->size;
	ep_request->actlen = 0;
	ep_request->error_code = 0;
	ep_request->callback = udc_event_xfer_out_callback;
	ep_request->cbdata = req_cb_data;
	ep_request->type = MAXUSB_TYPE_PKT;

	udc_ep_set_busy(ep_cfg, true);
	ret = MXC_USB_ReadEndpoint(ep_request);
	if (ret != 0) {
		udc_ep_set_busy(ep_cfg, false);
		LOG_ERR("ep 0x%02x error: %x", ep_cfg->addr, ret);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	}
}

static int udc_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_max32_data *priv = udc_get_private(dev);
	const struct udc_max32_config *config = dev->config;
	MXC_USB_Req_t *ep_request = &priv->ep_request[USB_EP_GET_IDX(USB_CONTROL_EP_OUT)];
	struct req_cb_data *req_cb_data = &priv->req_cb_data[USB_EP_GET_IDX(USB_CONTROL_EP_OUT)];
	struct net_buf *buf;
	int ret;

	/* Allocate buffer for data stage OUT */
	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}
	memset(buf->data, 0, length);
	udc_buf_put(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT), buf);

	req_cb_data->dev = dev;
	req_cb_data->ep = USB_CONTROL_EP_OUT;

	ep_request->ep = USB_EP_GET_IDX(USB_CONTROL_EP_OUT);
	ep_request->data = buf->data;
	ep_request->reqlen = length;
	ep_request->actlen = 0;
	ep_request->error_code = 0;
	ep_request->callback = udc_event_xfer_out_callback;
	ep_request->cbdata = req_cb_data;
	ep_request->type = MAXUSB_TYPE_TRANS;

	ret = MXC_USB_ReadEndpoint(ep_request);
	if (ret != 0) {
		LOG_ERR("ep 0x%02x error: %x", USB_CONTROL_EP_OUT, ret);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	}

	/*
	 * Set the SERV_OUTPKTRDY bit to trigger the interrupt after creating a read request.
	 * Otherwise program miss this interrupt because of race condition.
	 */
	config->base->csr0 |= MXC_F_USBHS_CSR0_SERV_OUTPKTRDY;

	return ret;
}

static int udc_event_setup(const struct device *dev)
{
	const struct udc_max32_config *config = dev->config;
	struct net_buf *buf;
	int ret;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return -ENOMEM;
	}

	udc_ep_buf_set_setup(buf);
	memset(buf->data, 0, sizeof(MXC_USB_SetupPkt));
	if (MXC_USB_GetSetup((MXC_USB_SetupPkt *)buf->data) < 0) {
		LOG_ERR("Failed to get setup data");
		return -1;
	}
	net_buf_add(buf, sizeof(MXC_USB_SetupPkt));

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		ret = udc_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (ret == -ENOMEM) {
			ret = udc_submit_ep_event(dev, buf, ret);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		/*
		 * Moved following line from MSDK driver to here because of the solution of
		 * ctrl_data_out stage's problem.
		 */
		config->base->csr0 |= MXC_F_USBHS_CSR0_SERV_OUTPKTRDY;
		LOG_INF("Setup: IN");
		ret = udc_ctrl_submit_s_in_status(dev);
	} else {
		ret = udc_ctrl_submit_s_status(dev);
	}

	return ret;
}

static ALWAYS_INLINE void max32_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;

	LOG_DBG("Driver %p thread started", dev);
	while (true) {
		bool start_xfer = false;
		struct udc_max32_evt evt;

		k_msgq_get(&drv_msgq, &evt, K_FOREVER);

		switch (evt.type) {
		case UDC_MAX32_EVT_SETUP:
			udc_event_setup(dev);
			break;
		case UDC_MAX32_EVT_XFER:
			start_xfer = true;
			break;
		default:
			break;
		}

		if (start_xfer) {
			if (USB_EP_DIR_IS_IN(evt.ep_cfg->addr)) {
				udc_event_xfer_in(dev, evt.ep_cfg);
			} else {
				udc_event_xfer_out(dev, evt.ep_cfg);
			}
		}
	}
}

static int udc_max32_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
				struct net_buf *buf)
{
	struct udc_max32_evt evt = {
		.type = UDC_MAX32_EVT_XFER,
		.ep_cfg = cfg,
	};

	LOG_DBG("%p enqueue %p", dev, buf);
	udc_buf_put(cfg, buf);

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);

	return 0;
}

static int udc_max32_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	} else {
		LOG_INF("ep 0x%02x queue is empty", cfg->addr);
	}

	irq_unlock(lock_key);

	return 0;
}

static int udc_max32_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	maxusb_ep_type_t ep_type;
	int ret;

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	if (cfg->caps.in) {
		ep_type = MAXUSB_EP_TYPE_IN;
	} else if (cfg->caps.out) {
		ep_type = MAXUSB_EP_TYPE_OUT;
	} else {
		LOG_ERR("ep 0x%02x is not IN or OUT", cfg->addr);
		return -ENODEV;
	}

	ret = MXC_USB_ConfigEp(USB_EP_GET_IDX(cfg->addr), ep_type, cfg->mps);
	if (ret != 0) {
		LOG_ERR("Failed to configure ep 0x%02x", cfg->addr);
		return ret;
	}

	return 0;
}

static int udc_max32_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	int ret;

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		return 0;
	}

	ret = MXC_USB_ResetEp(USB_EP_GET_IDX(cfg->addr));

	return ret;
}

static int udc_max32_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	int ret;

	if (cfg->stat.halted == true) {
		LOG_WRN("ep 0x%02x is already as halt", cfg->addr);
		return 0;
	}

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	ret = MXC_USB_Stall(USB_EP_GET_IDX(cfg->addr));
	if (ret != 0) {
		LOG_ERR("Failed to set halt ep 0x%02x", cfg->addr);
		return ret;
	}

	cfg->stat.halted = true;

	return 0;
}

static int udc_max32_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	int ret;

	if (cfg->stat.halted == false) {
		LOG_WRN("ep 0x%02x is not set as halt, no need to clear halt.", cfg->addr);
		return 0;
	}

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	ret = MXC_USB_Unstall(USB_EP_GET_IDX(cfg->addr));
	if (ret != 0) {
		LOG_ERR("Failed to clear halt ep 0x%02x", cfg->addr);
		return ret;
	}

	cfg->stat.halted = false;

	/* If there is a request for this endpoint, enqueue the request. */
	if (udc_buf_peek(cfg) != NULL) {
		struct udc_max32_evt evt = {
			.type = UDC_MAX32_EVT_XFER,
			.ep_cfg = cfg,
		};

		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	return 0;
}

static int udc_max32_set_address(const struct device *dev, const uint8_t addr)
{
	int ret;

	LOG_DBG("Set new address %u for %p", addr, dev);

	ret = MXC_USB_SetFuncAddr(addr);
	if (ret != 0) {
		LOG_ERR("Failed to set device address %u", addr);
		return -EINVAL;
	}

	return 0;
}

static int udc_max32_host_wakeup(const struct device *dev)
{
	LOG_DBG("Remote wakeup from %p", dev);

	MXC_USB_RemoteWakeup();

	return 0;
}

static enum udc_bus_speed udc_max32_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_max32_event_callback(maxusb_event_t event, void *cbdata)
{
	const struct device *dev = (const struct device *)cbdata;

	switch (event) {
	case MAXUSB_EVENT_NOVBUS:
		LOG_DBG("NOVBUS event occurred");
		udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
		break;
	case MAXUSB_EVENT_VBUS:
		LOG_DBG("VBUS event occurred");
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
		break;
	case MAXUSB_EVENT_SUSP:
		LOG_DBG("SUSP event occurred");
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		break;
	case MAXUSB_EVENT_DPACT:
		LOG_DBG("DPACT event occurred");
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_SOF, 0);
		break;
	case MAXUSB_EVENT_BRST:
		LOG_DBG("BRST event occurred");
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESET, 0);
		break;
	case MAXUSB_EVENT_SUDAV:
		LOG_DBG("SUDAV event occurred");
		struct udc_max32_evt evt = {
			.type = UDC_MAX32_EVT_SETUP,
			.ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT),
		};

		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
		break;
	default:
		break;
	}

	return 0;
}

static int udc_max32_enable(const struct device *dev)
{
	int ret;

	LOG_DBG("Enable device %p", dev);

	MXC_USB_EventClear(MAXUSB_EVENT_SUDAV);
	ret = MXC_USB_EventEnable(MAXUSB_EVENT_SUDAV, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable SUDAV event.");
		return ret;
	}

	MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
	ret = MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable SUSP event.");
		return ret;
	}

	MXC_USB_EventClear(MAXUSB_EVENT_DPACT);
	ret = MXC_USB_EventEnable(MAXUSB_EVENT_DPACT, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable DPACT event.");
		return ret;
	}

	MXC_USB_EventClear(MAXUSB_EVENT_BRST);
	ret = MXC_USB_EventEnable(MAXUSB_EVENT_BRST, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable BRST event.");
		return ret;
	}

	ret = MXC_USB_Connect();

	return ret;
}

static int udc_max32_disable(const struct device *dev)
{
	int ret;

	LOG_DBG("Disable device %p", dev);
	ret = MXC_USB_Disconnect();

	return ret;
}

static void udc_max32_delay_us(unsigned int usec)
{
	k_usleep(usec);
}

static int udc_max32_init(const struct device *dev)
{
	const struct udc_max32_config *config = dev->config;
	struct udc_data *data = dev->data;
	maxusb_cfg_options_t usb_opts;
	int ret;

	if (data->caps.hs) {
		usb_opts.enable_hs = 1;
	}
	usb_opts.delay_us = udc_max32_delay_us;
	usb_opts.init_callback = NULL;
	usb_opts.shutdown_callback = NULL;

	/* Enable clock */
	ret = clock_control_on(config->clock, (clock_control_subsys_t)&config->perclk);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB peripheral clock");
		return ret;
	}

	MXC_SYS_Reset_Periph(MXC_SYS_RESET0_USB);

	ret = Wrap_MXC_USB_Init(&usb_opts);
	if (ret != 0) {
		LOG_ERR("Failed to initialize USB.");
		return ret;
	}

	ret = MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable NOVBUS event.");
		return ret;
	}
	ret = MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, udc_max32_event_callback, (void *)dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable VBUS event.");
		return ret;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	config->irq_func(); /* UDC IRQ enable*/

	return 0;
}

static int udc_max32_shutdown(const struct device *dev)
{
	MXC_USB_Shutdown();
	irq_disable(DT_INST_IRQN(0));

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return 0;
}

static int udc_max32_driver_preinit(const struct device *dev)
{
	const struct udc_max32_config *config = dev->config;
	struct udc_data *data = dev->data;
	uint16_t mps = 64;
	int ret;

	k_mutex_init(&data->mutex);

	data->caps.rwup = true;
	data->caps.can_detect_vbus = true;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->speed_idx == 2) {
		data->caps.hs = true;
		mps = 512;
	}

	for (int i = 0; i < config->num_of_out_eps; i++) {
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
		ret = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	for (int i = 0; i < config->num_of_in_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
			config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
			/* Set different endpoint indexes from out endpoints */
			config->ep_cfg_in[i].addr =
				USB_EP_DIR_IN | (config->num_of_out_eps + i - 1);
		}

		ret = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	config->make_thread(dev);
	LOG_INF("Device %p (max. speed %d)", dev, config->speed_idx);

	return 0;
}

static void udc_max32_isr(const struct device *dev)
{
	MXC_USB_EventHandler();
}

static void udc_max32_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_max32_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_max32_api = {
	.lock = udc_max32_lock,
	.unlock = udc_max32_unlock,
	.device_speed = udc_max32_device_speed,
	.init = udc_max32_init,
	.enable = udc_max32_enable,
	.disable = udc_max32_disable,
	.shutdown = udc_max32_shutdown,
	.set_address = udc_max32_set_address,
	.host_wakeup = udc_max32_host_wakeup,
	.ep_enable = udc_max32_ep_enable,
	.ep_disable = udc_max32_ep_disable,
	.ep_set_halt = udc_max32_ep_set_halt,
	.ep_clear_halt = udc_max32_ep_clear_halt,
	.ep_enqueue = udc_max32_ep_enqueue,
	.ep_dequeue = udc_max32_ep_dequeue,
};

#define UDC_MAX32_DEVICE_DEFINE(n)                                                                 \
	static void udc_max32_irq_init_##n(void)                                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), udc_max32_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	K_THREAD_STACK_DEFINE(udc_max32_stack_##n, CONFIG_UDC_MAX32_THREAD_STACK_SIZE);            \
                                                                                                   \
	static void udc_max32_thread_##n(void *dev, void *arg1, void *arg2)                        \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		max32_thread_handler(dev);                                                         \
	}                                                                                          \
                                                                                                   \
	static void udc_max32_make_thread_##n(const struct device *dev)                            \
	{                                                                                          \
		struct udc_max32_data *priv = udc_get_private(dev);                                \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_max32_stack_##n,                           \
				K_THREAD_STACK_SIZEOF(udc_max32_stack_##n), udc_max32_thread_##n,  \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_MAX32_THREAD_PRIORITY), K_ESSENTIAL,        \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out_##n[DT_INST_PROP(n, num_out_endpoints)];            \
	static struct udc_ep_config ep_cfg_in_##n[DT_INST_PROP(n, num_in_endpoints)];              \
                                                                                                   \
	static const struct udc_max32_config udc_max32_config_##n = {                              \
		.base = (mxc_usbhs_regs_t *)DT_INST_REG_ADDR(n),                                   \
		.num_of_in_eps = DT_INST_PROP(n, num_out_endpoints),                               \
		.num_of_out_eps = DT_INST_PROP(n, num_in_endpoints),                               \
		.ep_cfg_in = ep_cfg_out_##n,                                                       \
		.ep_cfg_out = ep_cfg_in_##n,                                                       \
		.make_thread = udc_max32_make_thread_##n,                                          \
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),                           \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                    \
		.perclk.bus = DT_INST_CLOCKS_CELL(n, offset),                                      \
		.perclk.bit = DT_INST_CLOCKS_CELL(n, bit),                                         \
		.irq_func = &udc_max32_irq_init_##n,                                               \
	};                                                                                         \
                                                                                                   \
	static MXC_USB_Req_t ep_request_##n[DT_INST_PROP(n, num_out_endpoints) +                   \
					    DT_INST_PROP(n, num_in_endpoints) - 1];                \
                                                                                                   \
	static struct req_cb_data req_cb_data_##n[DT_INST_PROP(n, num_out_endpoints) +             \
						  DT_INST_PROP(n, num_in_endpoints) - 1];          \
                                                                                                   \
	static struct udc_max32_data udc_priv_##n = {                                              \
		.ep_request = ep_request_##n,                                                      \
		.req_cb_data = req_cb_data_##n,                                                    \
	};                                                                                         \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, udc_max32_driver_preinit, NULL, &udc_data_##n,                    \
			      &udc_max32_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_max32_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_MAX32_DEVICE_DEFINE)
