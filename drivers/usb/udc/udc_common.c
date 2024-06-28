/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/usb/usb_ch9.h>
#include "udc_common.h"

#include <zephyr/logging/log.h>
#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL)
#define UDC_COMMON_LOG_LEVEL CONFIG_UDC_DRIVER_LOG_LEVEL
#else
#define UDC_COMMON_LOG_LEVEL LOG_LEVEL_NONE
#endif
LOG_MODULE_REGISTER(udc, CONFIG_UDC_DRIVER_LOG_LEVEL);

static inline void udc_buf_destroy(struct net_buf *buf);

NET_BUF_POOL_VAR_DEFINE(udc_ep_pool,
			CONFIG_UDC_BUF_COUNT, CONFIG_UDC_BUF_POOL_SIZE,
			sizeof(struct udc_buf_info), udc_buf_destroy);

#define USB_EP_LUT_IDX(ep) (USB_EP_DIR_IS_IN(ep) ? (ep & BIT_MASK(4)) + 16 : \
						   ep & BIT_MASK(4))

void udc_set_suspended(const struct device *dev, const bool value)
{
	struct udc_data *data = dev->data;

	if (value == udc_is_suspended(dev)) {
		LOG_WRN("Spurious suspend/resume event");
	}

	atomic_set_bit_to(&data->status, UDC_STATUS_SUSPENDED, value);
}

struct udc_ep_config *udc_get_ep_cfg(const struct device *dev, const uint8_t ep)
{
	struct udc_data *data = dev->data;

	return data->ep_lut[USB_EP_LUT_IDX(ep)];
}

bool udc_ep_is_busy(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	__ASSERT(ep_cfg != NULL, "ep 0x%02x is not available", ep);

	return ep_cfg->stat.busy;
}

void udc_ep_set_busy(const struct device *dev, const uint8_t ep, const bool busy)
{
	struct udc_ep_config *ep_cfg;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	__ASSERT(ep_cfg != NULL, "ep 0x%02x is not available", ep);
	ep_cfg->stat.busy = busy;
}

int udc_register_ep(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct udc_data *data = dev->data;
	uint8_t idx;

	if (udc_is_initialized(dev)) {
		return -EACCES;
	}

	idx = USB_EP_LUT_IDX(cfg->addr);
	__ASSERT_NO_MSG(idx < ARRAY_SIZE(data->ep_lut));

	data->ep_lut[idx] = cfg;
	k_fifo_init(&cfg->fifo);

	return 0;
}

struct net_buf *udc_buf_get(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (ep_cfg == NULL) {
		return NULL;
	}

	return net_buf_get(&ep_cfg->fifo, K_NO_WAIT);
}

struct net_buf *udc_buf_get_all(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (ep_cfg == NULL) {
		return NULL;
	}

	buf = k_fifo_get(&ep_cfg->fifo, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	LOG_DBG("ep 0x%02x dequeue %p", ep, buf);
	for (struct net_buf *n = buf; !k_fifo_is_empty(&ep_cfg->fifo); n = n->frags) {
		n->frags = k_fifo_get(&ep_cfg->fifo, K_NO_WAIT);
		LOG_DBG("|-> %p ", n->frags);
		if (n->frags == NULL) {
			break;
		}
	}

	return buf;
}

struct net_buf *udc_buf_peek(const struct device *dev, const uint8_t ep)
{
	struct udc_ep_config *ep_cfg;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (ep_cfg == NULL) {
		return NULL;
	}

	return k_fifo_peek_head(&ep_cfg->fifo);
}

void udc_buf_put(struct udc_ep_config *const ep_cfg,
		 struct net_buf *const buf)
{
	net_buf_put(&ep_cfg->fifo, buf);
}

void udc_ep_buf_set_setup(struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	bi->setup = 1;
	bi->data = 0;
	bi->status = 0;
}

bool udc_ep_buf_has_zlp(const struct net_buf *const buf)
{
	const struct udc_buf_info *bi = udc_get_buf_info(buf);

	return bi->zlp;
}

void udc_ep_buf_clear_zlp(const struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	bi->zlp = false;
}

int udc_submit_event(const struct device *dev,
		     const enum udc_event_type type,
		     const int status)
{
	struct udc_data *data = dev->data;
	struct udc_event drv_evt = {
		.type = type,
		.status = status,
		.dev = dev,
	};

	return data->event_cb(dev, &drv_evt);
}

int udc_submit_ep_event(const struct device *dev,
			struct net_buf *const buf,
			const int err)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct udc_data *data = dev->data;
	const struct udc_event drv_evt = {
		.type = UDC_EVT_EP_REQUEST,
		.buf = buf,
		.dev = dev,
	};

	if (!udc_is_initialized(dev)) {
		return -EPERM;
	}

	bi->err = err;

	return data->event_cb(dev, &drv_evt);
}

static uint8_t ep_attrib_get_transfer(uint8_t attributes)
{
	return attributes & USB_EP_TRANSFER_TYPE_MASK;
}

static bool ep_check_config(const struct device *dev,
			    const struct udc_ep_config *const cfg,
			    const uint8_t ep,
			    const uint8_t attributes,
			    const uint16_t mps,
			    const uint8_t interval)
{
	bool dir_is_in = USB_EP_DIR_IS_IN(ep);
	bool dir_is_out = USB_EP_DIR_IS_OUT(ep);

	LOG_DBG("cfg d:%c|%c t:%c|%c|%c|%c, mps %u",
		cfg->caps.in ? 'I' : '-',
		cfg->caps.out ? 'O' : '-',
		cfg->caps.iso ? 'S' : '-',
		cfg->caps.bulk ? 'B' : '-',
		cfg->caps.interrupt ? 'I' : '-',
		cfg->caps.control ? 'C' : '-',
		cfg->caps.mps);

	if (dir_is_out && !cfg->caps.out) {
		return false;
	}

	if (dir_is_in && !cfg->caps.in) {
		return false;
	}

	if (mps > cfg->caps.mps) {
		return false;
	}

	switch (ep_attrib_get_transfer(attributes)) {
	case USB_EP_TYPE_BULK:
		if (!cfg->caps.bulk) {
			return false;
		}
		break;
	case USB_EP_TYPE_INTERRUPT:
		if (!cfg->caps.interrupt) {
			return false;
		}
		break;
	case USB_EP_TYPE_ISO:
		if (!cfg->caps.iso) {
			return false;
		}
		break;
	case USB_EP_TYPE_CONTROL:
		if (!cfg->caps.control) {
			return false;
		}
		break;
	default:
		return false;
	}

	return true;
}

static void ep_update_mps(const struct device *dev,
			  const struct udc_ep_config *const cfg,
			  const uint8_t attributes,
			  uint16_t *const mps)
{
	struct udc_device_caps caps = udc_caps(dev);
	const uint16_t spec_int_mps = caps.hs ? 1024 : 64;
	const uint16_t spec_bulk_mps = caps.hs ? 512 : 64;

	/*
	 * TODO: It does not take into account the actual speed of the
	 * bus after the RESET. Should be fixed/improved when the driver
	 * for high speed controller are ported.
	 */
	switch (ep_attrib_get_transfer(attributes)) {
	case USB_EP_TYPE_BULK:
		*mps = MIN(cfg->caps.mps, spec_bulk_mps);
		break;
	case USB_EP_TYPE_INTERRUPT:
		*mps = MIN(cfg->caps.mps, spec_int_mps);
		break;
	case USB_EP_TYPE_CONTROL:
		__fallthrough;
	case USB_EP_TYPE_ISO:
		__fallthrough;
	default:
		return;
	}
}

int udc_ep_try_config(const struct device *dev,
		      const uint8_t ep,
		      const uint8_t attributes,
		      uint16_t *const mps,
		      const uint8_t interval)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	bool ret;

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		return -ENODEV;
	}

	api->lock(dev);

	ret = ep_check_config(dev, cfg, ep, attributes, *mps, interval);
	if (ret == true && *mps == 0U) {
		ep_update_mps(dev, cfg, attributes, mps);
	}

	api->unlock(dev);

	return (ret == false) ? -ENOTSUP : 0;
}

int udc_ep_enable_internal(const struct device *dev,
			   const uint8_t ep,
			   const uint8_t attributes,
			   const uint16_t mps,
			   const uint8_t interval)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	int ret;

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		return -ENODEV;
	}

	if (cfg->stat.enabled) {
		LOG_ERR("ep 0x%02x already enabled", cfg->addr);
		return -EALREADY;
	}

	if (!ep_check_config(dev, cfg, ep, attributes, mps, interval)) {
		LOG_ERR("Endpoint 0x%02x validation failed", cfg->addr);
		return -ENODEV;
	}

	cfg->attributes = attributes;
	cfg->mps = mps;
	cfg->interval = interval;

	cfg->stat.odd = 0;
	cfg->stat.halted = 0;
	cfg->stat.data1 = false;
	ret = api->ep_enable(dev, cfg);
	cfg->stat.enabled = ret ? false : true;

	return ret;
}

int udc_ep_enable(const struct device *dev,
		  const uint8_t ep,
		  const uint8_t attributes,
		  const uint16_t mps,
		  const uint8_t interval)
{
	const struct udc_api *api = dev->api;
	int ret;

	if (ep == USB_CONTROL_EP_OUT || ep == USB_CONTROL_EP_IN) {
		return -EINVAL;
	}

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		ret = -EPERM;
		goto ep_enable_error;
	}

	ret = udc_ep_enable_internal(dev, ep, attributes, mps, interval);

ep_enable_error:
	api->unlock(dev);

	return ret;
}

int udc_ep_disable_internal(const struct device *dev, const uint8_t ep)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	int ret;

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		return -ENODEV;
	}

	if (!cfg->stat.enabled) {
		LOG_ERR("ep 0x%02x already disabled", cfg->addr);
		return -EALREADY;
	}

	ret = api->ep_disable(dev, cfg);
	cfg->stat.enabled = ret ? cfg->stat.enabled : false;

	return ret;
}

int udc_ep_disable(const struct device *dev, const uint8_t ep)
{
	const struct udc_api *api = dev->api;
	int ret;

	if (ep == USB_CONTROL_EP_OUT || ep == USB_CONTROL_EP_IN) {
		return -EINVAL;
	}

	api->lock(dev);

	if (!udc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_disable_error;
	}

	ret = udc_ep_disable_internal(dev, ep);

ep_disable_error:
	api->unlock(dev);

	return ret;
}

int udc_ep_set_halt(const struct device *dev, const uint8_t ep)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	int ret;

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		ret = -EPERM;
		goto ep_set_halt_error;
	}

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		ret = -ENODEV;
		goto ep_set_halt_error;
	}

	if (!cfg->stat.enabled) {
		ret = -ENODEV;
		goto ep_set_halt_error;
	}

	if (ep_attrib_get_transfer(cfg->attributes) == USB_EP_TYPE_ISO) {
		ret = -ENOTSUP;
		goto ep_set_halt_error;
	}

	ret = api->ep_set_halt(dev, cfg);

ep_set_halt_error:
	api->unlock(dev);

	return ret;
}

int udc_ep_clear_halt(const struct device *dev, const uint8_t ep)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	int ret;

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		ret = -EPERM;
		goto ep_clear_halt_error;
	}

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		ret = -ENODEV;
		goto ep_clear_halt_error;
	}

	if (!cfg->stat.enabled) {
		ret = -ENODEV;
		goto ep_clear_halt_error;
	}

	if (ep_attrib_get_transfer(cfg->attributes) == USB_EP_TYPE_ISO) {
		ret = -ENOTSUP;
		goto ep_clear_halt_error;
	}

	ret = api->ep_clear_halt(dev, cfg);
	if (ret == 0) {
		cfg->stat.halted = false;
	}

ep_clear_halt_error:
	api->unlock(dev);

	return ret;
}

static void udc_debug_ep_enqueue(const struct device *dev,
				 struct udc_ep_config *const cfg)
{
	struct udc_buf_info *bi;
	struct net_buf *buf;
	sys_slist_t list;

	list.head = k_fifo_peek_head(&cfg->fifo);
	list.tail = k_fifo_peek_tail(&cfg->fifo);
	if (list.head == NULL) {
		LOG_DBG("ep 0x%02x queue is empty", cfg->addr);
		return;
	}

	LOG_DBG("[de]queue ep 0x%02x:", cfg->addr);

	SYS_SLIST_FOR_EACH_CONTAINER(&list, buf, node) {
		bi = udc_get_buf_info(buf);
		LOG_DBG("|-> %p (%u) ->", buf, buf->size);
	}
}

int udc_ep_enqueue(const struct device *dev, struct net_buf *const buf)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	struct udc_buf_info *bi;
	int ret;

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		ret = -EPERM;
		goto ep_enqueue_error;
	}

	bi = udc_get_buf_info(buf);
	if (bi->ep == USB_CONTROL_EP_OUT) {
		ret = -EPERM;
		goto ep_enqueue_error;
	}

	cfg = udc_get_ep_cfg(dev, bi->ep);
	if (cfg == NULL) {
		ret = -ENODEV;
		goto ep_enqueue_error;
	}

	LOG_DBG("Queue ep 0x%02x %p len %u", cfg->addr, buf,
		USB_EP_DIR_IS_IN(cfg->addr) ? buf->len : buf->size);

	bi->setup = 0;
	ret = api->ep_enqueue(dev, cfg, buf);

ep_enqueue_error:
	api->unlock(dev);

	return ret;
}

int udc_ep_dequeue(const struct device *dev, const uint8_t ep)
{
	const struct udc_api *api = dev->api;
	struct udc_ep_config *cfg;
	int ret;

	api->lock(dev);

	if (!udc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_dequeue_error;
	}

	cfg = udc_get_ep_cfg(dev, ep);
	if (cfg == NULL) {
		ret = -ENODEV;
		goto ep_dequeue_error;
	}

	if (cfg->stat.enabled || cfg->stat.halted) {
		LOG_INF("ep 0x%02x is not halted|disabled", cfg->addr);
	}

	if (UDC_COMMON_LOG_LEVEL == LOG_LEVEL_DBG) {
		udc_debug_ep_enqueue(dev, cfg);
	}

	if (k_fifo_is_empty(&cfg->fifo)) {
		ret = 0;
	} else  {
		ret = api->ep_dequeue(dev, cfg);
	}

ep_dequeue_error:
	api->unlock(dev);

	return ret;
}

struct net_buf *udc_ep_buf_alloc(const struct device *dev,
				 const uint8_t ep,
				 const size_t size)
{
	const struct udc_api *api = dev->api;
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	api->lock(dev);

	buf = net_buf_alloc_len(&udc_ep_pool, size, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Failed to allocate net_buf %zd", size);
		goto ep_alloc_error;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;
	LOG_DBG("Allocate net_buf, ep 0x%02x, size %zd", ep, size);

ep_alloc_error:
	api->unlock(dev);

	return buf;
}

struct net_buf *udc_ctrl_alloc(const struct device *dev,
			       const uint8_t ep,
			       const size_t size)
{
	/* TODO: for now just pass to udc_buf_alloc() */
	return udc_ep_buf_alloc(dev, ep, size);
}

static inline void udc_buf_destroy(struct net_buf *buf)
{
	/* Adjust level and use together with the log in udc_ep_buf_alloc() */
	LOG_DBG("destroy %p", buf);
	net_buf_destroy(buf);
}

int udc_ep_buf_free(const struct device *dev, struct net_buf *const buf)
{
	const struct udc_api *api = dev->api;
	int ret = 0;

	api->lock(dev);
	net_buf_unref(buf);
	api->unlock(dev);

	return ret;
}

enum udc_bus_speed udc_device_speed(const struct device *dev)
{
	const struct udc_api *api = dev->api;
	enum udc_bus_speed speed = UDC_BUS_UNKNOWN;

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		goto device_speed_error;
	}

	if (api->device_speed) {
		speed = api->device_speed(dev);
	} else {
		/* TODO: Shall we track connected status in UDC? */
		speed = UDC_BUS_SPEED_FS;
	}

device_speed_error:
	api->unlock(dev);

	return speed;
}

int udc_enable(const struct device *dev)
{
	const struct udc_api *api = dev->api;
	struct udc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (!udc_is_initialized(dev)) {
		ret = -EPERM;
		goto udc_enable_error;
	}

	if (udc_is_enabled(dev)) {
		ret = -EALREADY;
		goto udc_enable_error;
	}

	data->stage = CTRL_PIPE_STAGE_SETUP;

	ret = api->enable(dev);
	if (ret == 0) {
		atomic_set_bit(&data->status, UDC_STATUS_ENABLED);
	}

udc_enable_error:
	api->unlock(dev);

	return ret;
}

int udc_disable(const struct device *dev)
{
	const struct udc_api *api = dev->api;
	struct udc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (!udc_is_enabled(dev)) {
		ret = -EALREADY;
		goto udc_disable_error;
	}

	ret = api->disable(dev);
	atomic_clear_bit(&data->status, UDC_STATUS_ENABLED);

udc_disable_error:
	api->unlock(dev);

	return ret;
}

int udc_init(const struct device *dev,
	     udc_event_cb_t event_cb, const void *const event_ctx)
{
	const struct udc_api *api = dev->api;
	struct udc_data *data = dev->data;
	int ret;

	if (event_cb == NULL || event_ctx == NULL) {
		return -EINVAL;
	}

	api->lock(dev);

	if (udc_is_initialized(dev)) {
		ret = -EALREADY;
		goto udc_init_error;
	}

	data->event_cb = event_cb;
	data->event_ctx = event_ctx;

	ret = api->init(dev);
	if (ret == 0) {
		atomic_set_bit(&data->status, UDC_STATUS_INITIALIZED);
	}

udc_init_error:
	api->unlock(dev);

	return ret;
}

int udc_shutdown(const struct device *dev)
{
	const struct udc_api *api = dev->api;
	struct udc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (udc_is_enabled(dev)) {
		ret = -EBUSY;
		goto udc_shutdown_error;
	}

	if (!udc_is_initialized(dev)) {
		ret = -EALREADY;
		goto udc_shutdown_error;
	}

	ret = api->shutdown(dev);
	atomic_clear_bit(&data->status, UDC_STATUS_INITIALIZED);

udc_shutdown_error:
	api->unlock(dev);

	return ret;
}

static ALWAYS_INLINE
struct net_buf *udc_ctrl_alloc_stage(const struct device *dev,
				     struct net_buf *const parent,
				     const uint8_t ep,
				     const size_t size)
{
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, ep, size);
	if (buf == NULL) {
		return NULL;
	}

	if (parent) {
		net_buf_frag_add(parent, buf);
	}

	return buf;
}

static struct net_buf *udc_ctrl_alloc_data(const struct device *dev,
					   struct net_buf *const setup,
					   const uint8_t ep)
{
	size_t size = udc_data_stage_length(setup);
	struct udc_buf_info *bi;
	struct net_buf *buf;

	buf = udc_ctrl_alloc_stage(dev, setup, ep, size);
	if (buf) {
		bi = udc_get_buf_info(buf);
		bi->data = true;
	}

	return buf;
}

static struct net_buf *udc_ctrl_alloc_status(const struct device *dev,
					     struct net_buf *const parent,
					     const uint8_t ep)
{
	size_t size = (ep == USB_CONTROL_EP_OUT) ? 64 : 0;
	struct udc_buf_info *bi;
	struct net_buf *buf;

	buf = udc_ctrl_alloc_stage(dev, parent, ep, size);
	if (buf) {
		bi = udc_get_buf_info(buf);
		bi->status = true;
	}

	return buf;
}

int udc_ctrl_submit_s_out_status(const struct device *dev,
			      struct net_buf *const dout)
{
	struct udc_buf_info *bi = udc_get_buf_info(dout);
	struct udc_data *data = dev->data;
	struct net_buf *buf;
	int ret = 0;

	bi->data = true;
	net_buf_frag_add(data->setup, dout);

	buf = udc_ctrl_alloc_status(dev, dout, USB_CONTROL_EP_IN);
	if (buf == NULL) {
		ret = -ENOMEM;
	}

	return udc_submit_ep_event(dev, data->setup, ret);
}

int udc_ctrl_submit_s_in_status(const struct device *dev)
{
	struct udc_data *data = dev->data;
	struct net_buf *buf;
	int ret = 0;

	if (!udc_ctrl_stage_is_data_in(dev)) {
		return -ENOTSUP;
	}

	/* Allocate buffer for data stage IN */
	buf = udc_ctrl_alloc_data(dev, data->setup, USB_CONTROL_EP_IN);
	if (buf == NULL) {
		ret = -ENOMEM;
	}

	return udc_submit_ep_event(dev, data->setup, ret);
}

int udc_ctrl_submit_s_status(const struct device *dev)
{
	struct udc_data *data = dev->data;
	struct net_buf *buf;
	int ret = 0;

	/* Allocate buffer for possible status IN */
	buf = udc_ctrl_alloc_status(dev, data->setup, USB_CONTROL_EP_IN);
	if (buf == NULL) {
		ret = -ENOMEM;
	}

	return udc_submit_ep_event(dev, data->setup, ret);
}

int udc_ctrl_submit_status(const struct device *dev,
			   struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	bi->status = true;

	return udc_submit_ep_event(dev, buf, 0);
}

bool udc_ctrl_stage_is_data_out(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->stage == CTRL_PIPE_STAGE_DATA_OUT ? true : false;
}

bool udc_ctrl_stage_is_data_in(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->stage == CTRL_PIPE_STAGE_DATA_IN ? true : false;
}

bool udc_ctrl_stage_is_status_out(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->stage == CTRL_PIPE_STAGE_STATUS_OUT ? true : false;
}

bool udc_ctrl_stage_is_status_in(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->stage == CTRL_PIPE_STAGE_STATUS_IN ? true : false;
}

bool udc_ctrl_stage_is_no_data(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->stage == CTRL_PIPE_STAGE_NO_DATA ? true : false;
}

static bool udc_data_stage_to_host(const struct net_buf *const buf)
{
	struct usb_setup_packet *setup = (void *)buf->data;

	return USB_REQTYPE_GET_DIR(setup->bmRequestType);
}

void udc_ctrl_update_stage(const struct device *dev,
			   struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct udc_device_caps caps = udc_caps(dev);
	uint8_t next_stage = CTRL_PIPE_STAGE_ERROR;
	struct udc_data *data = dev->data;

	__ASSERT(USB_EP_GET_IDX(bi->ep) == 0,
		 "0x%02x is not a control endpoint", bi->ep);

	if (bi->setup && bi->ep == USB_CONTROL_EP_OUT) {
		uint16_t length  = udc_data_stage_length(buf);

		data->setup = buf;

		if (data->stage != CTRL_PIPE_STAGE_SETUP) {
			LOG_INF("Sequence %u not completed", data->stage);
			data->stage = CTRL_PIPE_STAGE_SETUP;
		}

		/*
		 * Setup Stage has been completed (setup packet received),
		 * regardless of the previous stage, this is now being reset.
		 * Next state depends on wLength and the direction bit (D7).
		 */
		if (length == 0) {
			/*
			 * No Data Stage, next is Status Stage
			 * complete sequence: s->status
			 */
			LOG_DBG("s->(status)");
			next_stage = CTRL_PIPE_STAGE_NO_DATA;
		} else if (udc_data_stage_to_host(buf)) {
			/*
			 * Next is Data Stage (to host / IN)
			 * complete sequence: s->in->status
			 */
			LOG_DBG("s->(in)");
			next_stage = CTRL_PIPE_STAGE_DATA_IN;
		} else {
			/*
			 * Next is Data Stage (to device / OUT)
			 * complete sequence: s->out->status
			 */
			LOG_DBG("s->(out)");
			next_stage = CTRL_PIPE_STAGE_DATA_OUT;
		}

	} else if (bi->ep == USB_CONTROL_EP_OUT) {
		if (data->stage == CTRL_PIPE_STAGE_DATA_OUT) {
			/*
			 * Next sequence is Status Stage if request is okay,
			 * (IN ZLP status to host)
			 */
			next_stage = CTRL_PIPE_STAGE_STATUS_IN;
		} else if (data->stage == CTRL_PIPE_STAGE_STATUS_OUT) {
			/*
			 * End of a sequence: s->in->status,
			 * We should check the length here because we always
			 * submit a OUT request with the minimum length
			 * of the control endpoint.
			 */
			if (buf->len == 0) {
				LOG_DBG("s-in-status");
				next_stage = CTRL_PIPE_STAGE_SETUP;
			} else {
				LOG_WRN("ZLP expected");
				next_stage = CTRL_PIPE_STAGE_ERROR;
			}
		} else {
			LOG_ERR("Cannot determine the next stage");
			next_stage = CTRL_PIPE_STAGE_ERROR;
		}

	} else { /* if (bi->ep == USB_CONTROL_EP_IN) */
		if (data->stage == CTRL_PIPE_STAGE_STATUS_IN) {
			/*
			 * End of a sequence: setup->out->in
			 */
			LOG_DBG("s-out-status");
			next_stage = CTRL_PIPE_STAGE_SETUP;
		} else if (data->stage == CTRL_PIPE_STAGE_DATA_IN) {
			/*
			 * Data IN stage completed, next sequence
			 * is Status Stage (OUT ZLP status to device).
			 * over-engineered controllers can send status
			 * on their own, skip this state then.
			 */
			if (caps.out_ack) {
				LOG_DBG("s-in->[status]");
				next_stage = CTRL_PIPE_STAGE_SETUP;
			} else {
				LOG_DBG("s-in->(status)");
				next_stage = CTRL_PIPE_STAGE_STATUS_OUT;
			}
		} else if (data->stage == CTRL_PIPE_STAGE_NO_DATA) {
			/*
			 * End of a sequence (setup->in)
			 * Previous NO Data stage was completed and
			 * we confirmed it with an IN ZLP.
			 */
			LOG_DBG("s-status");
			next_stage = CTRL_PIPE_STAGE_SETUP;
		} else {
			LOG_ERR("Cannot determine the next stage");
			next_stage = CTRL_PIPE_STAGE_ERROR;
		}
	}


	if (next_stage == data->stage) {
		LOG_WRN("State not changed!");
	}

	data->stage = next_stage;
}

#if defined(CONFIG_UDC_WORKQUEUE)
K_KERNEL_STACK_DEFINE(udc_work_q_stack, CONFIG_UDC_WORKQUEUE_STACK_SIZE);

struct k_work_q udc_work_q;

static int udc_work_q_init(void)
{

	k_work_queue_start(&udc_work_q,
			   udc_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(udc_work_q_stack),
			   CONFIG_UDC_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&udc_work_q.thread, "udc_work_q");

	return 0;
}

SYS_INIT(udc_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
