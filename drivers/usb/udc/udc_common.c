/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc_buf.h>
#include "udc_common.h"

#include <zephyr/logging/log.h>
#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL)
#define UDC_COMMON_LOG_LEVEL CONFIG_UDC_DRIVER_LOG_LEVEL
#else
#define UDC_COMMON_LOG_LEVEL LOG_LEVEL_NONE
#endif
LOG_MODULE_REGISTER(udc, CONFIG_UDC_DRIVER_LOG_LEVEL);

static inline uint8_t *udc_pool_data_alloc(struct net_buf *const buf,
					   size_t *const size, k_timeout_t timeout)
{
	struct net_buf_pool *const buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *const pool = buf_pool->alloc->alloc_data;
	void *b;

	*size = ROUND_UP(*size, UDC_BUF_GRANULARITY);
	b = k_heap_aligned_alloc(pool, UDC_BUF_ALIGN, *size, timeout);
	if (b == NULL) {
		*size = 0;
		return NULL;
	}

	return b;
}

static inline void udc_pool_data_unref(struct net_buf *buf, uint8_t *const data)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;

	k_heap_free(pool, data);
}

const struct net_buf_data_cb net_buf_dma_cb = {
	.alloc = udc_pool_data_alloc,
	.unref = udc_pool_data_unref,
};

static inline void udc_buf_destroy(struct net_buf *buf);

UDC_BUF_POOL_VAR_DEFINE(udc_ep_pool,
			CONFIG_UDC_BUF_COUNT, CONFIG_UDC_BUF_POOL_SIZE,
			sizeof(struct udc_buf_info), udc_buf_destroy);

#define USB_EP_LUT_IDX(ep) (USB_EP_DIR_IS_IN(ep) ? (ep & BIT_MASK(4)) + 16 : \
						   ep & BIT_MASK(4))

void udc_set_suspended(const struct device *dev, const bool value)
{
	struct udc_data *data = dev->data;

	if (value == udc_is_suspended(dev)) {
		LOG_WRN("Spurious %s event", value ? "suspend" : "resume");
	}

	atomic_set_bit_to(&data->status, UDC_STATUS_SUSPENDED, value);
}

struct udc_ep_config *udc_get_ep_cfg(const struct device *dev, const uint8_t ep)
{
	struct udc_data *data = dev->data;

	return data->ep_lut[USB_EP_LUT_IDX(ep)];
}

bool udc_ep_is_busy(const struct udc_ep_config *const ep_cfg)
{
	return ep_cfg->stat.busy;
}

void udc_ep_set_busy(struct udc_ep_config *const ep_cfg, const bool busy)
{
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

struct net_buf *udc_buf_get(struct udc_ep_config *const ep_cfg)
{
	return k_fifo_get(&ep_cfg->fifo, K_NO_WAIT);
}

struct net_buf *udc_buf_get_all(struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;

	buf = k_fifo_get(&ep_cfg->fifo, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	LOG_DBG("ep 0x%02x dequeue %p", ep_cfg->addr, buf);
	for (struct net_buf *n = buf; !k_fifo_is_empty(&ep_cfg->fifo); n = n->frags) {
		n->frags = k_fifo_get(&ep_cfg->fifo, K_NO_WAIT);
		LOG_DBG("|-> %p ", n->frags);
		if (n->frags == NULL) {
			break;
		}
	}

	return buf;
}

struct net_buf *udc_buf_peek(struct udc_ep_config *const ep_cfg)
{
	return k_fifo_peek_head(&ep_cfg->fifo);
}

void udc_buf_put(struct udc_ep_config *const ep_cfg,
		 struct net_buf *const buf)
{
	k_fifo_put(&ep_cfg->fifo, buf);
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

	if (USB_MPS_EP_SIZE(mps) > USB_MPS_EP_SIZE(cfg->caps.mps)) {
		return false;
	}

	switch (ep_attrib_get_transfer(attributes)) {
	case USB_EP_TYPE_BULK:
		if (!cfg->caps.bulk) {
			return false;
		}
		break;
	case USB_EP_TYPE_INTERRUPT:
		if (!cfg->caps.interrupt ||
		    (USB_MPS_ADDITIONAL_TRANSACTIONS(mps) &&
		     !cfg->caps.high_bandwidth)) {
			return false;
		}
		break;
	case USB_EP_TYPE_ISO:
		if (!cfg->caps.iso ||
		    (USB_MPS_ADDITIONAL_TRANSACTIONS(mps) &&
		     !cfg->caps.high_bandwidth)) {
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
	cfg = udc_get_ep_cfg(dev, bi->ep);
	if (cfg == NULL) {
		ret = -ENODEV;
		goto ep_enqueue_error;
	}

	if (!cfg->stat.enabled) {
		ret = -ENODEV;
		goto ep_enqueue_error;
	}

	LOG_DBG("Queue ep 0x%02x %p len %u", cfg->addr, buf,
		USB_EP_DIR_IS_IN(cfg->addr) ? buf->len : buf->size);

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
		LOG_ERR("Failed to allocate net_buf %zd, ep 0x%02x", size, ep);
		goto ep_alloc_error;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;
	LOG_DBG("Allocate net_buf %p, ep 0x%02x, size %zd", buf, ep, size);

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

struct net_buf *udc_ctrl_setup_alloc(const struct device *dev)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	/* Allocate bMaxPacketSize0 despite SETUP being just 8 bytes */
	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, ep_cfg->mps);
	if (buf) {
		udc_ep_buf_set_setup(buf);
	}

	return buf;
}

struct net_buf *udc_ctrl_data_alloc(const struct device *dev,
				    const uint8_t ep,
				    const size_t size)
{
	struct udc_buf_info *bi;
	struct net_buf *buf;
	size_t alloc_len = size;

	if (ep == USB_CONTROL_EP_OUT) {
		struct udc_ep_config *ep_cfg;

		/* Round up to bMaxPacketSize0 */
		ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		alloc_len = ROUND_UP(size, ep_cfg->mps);
	}

	buf = udc_ctrl_alloc(dev, ep, alloc_len);
	if (buf) {
		bi = udc_get_buf_info(buf);
		bi->data = true;
	}

	return buf;
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
