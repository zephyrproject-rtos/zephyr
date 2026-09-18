/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_ch9.h>
#include "uhc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc, CONFIG_UHC_DRIVER_LOG_LEVEL);

K_MEM_SLAB_DEFINE_STATIC_TYPE(uhc_xfer_pool, struct uhc_transfer,
			      CONFIG_UHC_XFER_COUNT);

USB_BUF_POOL_VAR_DEFINE(uhc_ep_pool,
			CONFIG_UHC_BUF_COUNT, CONFIG_UHC_BUF_POOL_SIZE,
			0, NULL);

int uhc_common_init(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_init(&data->mutex);
}

int uhc_submit_event(const struct device *dev,
		     const enum uhc_event_type type,
		     const int status)
{
	struct uhc_data *data = dev->data;
	struct uhc_event drv_evt = {
		.type = type,
		.status = status,
		.dev = dev,
	};

	if (!uhc_is_initialized(dev)) {
		return -EPERM;
	}

	return data->event_cb(dev, &drv_evt);
}

void uhc_xfer_return(const struct device *dev,
		     struct uhc_transfer *const xfer,
		     const int err)
{
	struct uhc_data *data = dev->data;
	struct uhc_event drv_evt = {
		.type = UHC_EVT_EP_REQUEST,
		.xfer = xfer,
		.dev = dev,
	};

	sys_dlist_remove(&xfer->node);
	xfer->queued = 0;
	xfer->err = err;

	data->event_cb(dev, &drv_evt);
	(void)uhc_xfer_unref(xfer);
}

struct uhc_transfer *uhc_xfer_get_next(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer;
	sys_dnode_t *node;

	/* Draft, WIP */
	node = sys_dlist_peek_head(&data->ctrl_xfers);
	if (node == NULL) {
		node = sys_dlist_peek_head(&data->bulk_xfers);
	}

	return (node == NULL) ? NULL : SYS_DLIST_CONTAINER(node, xfer, node);
}

int uhc_xfer_append(const struct device *dev,
		    struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		sys_dlist_append(&data->ctrl_xfers, &xfer->node);
		break;
	case USB_EP_TYPE_BULK:
		sys_dlist_append(&data->bulk_xfers, &xfer->node);
		break;
	default:
		LOG_ERR("Invalid xfer type: %d", xfer->type);
	}

	return 0;
}

static void uhc_xfer_dlist_cleanup_cancelled(const struct device *dev, sys_dlist_t *dlist,
					   uhc_xfer_cancel_cb_t cb, void *priv)
{
	struct uhc_transfer *current, *next;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(dlist, current, next, node) {
		if (current->err == -ECONNRESET) {
			if (cb == NULL) {
				uhc_xfer_return(dev, current, -ECONNRESET);
			} else {
				cb(dev, current, priv);
			}
		}
	}
}

void uhc_xfer_cleanup_cancelled(const struct device *dev, uhc_xfer_cancel_cb_t cb, void *priv)
{
	struct uhc_data *data = dev->data;

	uhc_xfer_dlist_cleanup_cancelled(dev, &data->ctrl_xfers, cb, priv);
	uhc_xfer_dlist_cleanup_cancelled(dev, &data->bulk_xfers, cb, priv);
}

int uhc_xfer_dequeue(struct uhc_transfer *const xfer)
{
	if (!sys_dnode_is_linked(&xfer->node)) {
		return -EINVAL;
	}

	xfer->err = -ECONNRESET;
	return 0;
}

struct net_buf *uhc_xfer_buf_alloc(const struct device *dev,
				   const size_t size)
{
	return net_buf_alloc_len(&uhc_ep_pool, size, K_NO_WAIT);
}

void uhc_xfer_buf_free(const struct device *dev, struct net_buf *const buf)
{
	net_buf_unref(buf);
}

static uint32_t uhc_xfer_bInterval_to_microframe(const struct usb_device *const udev,
					       uint16_t bInterval, uint8_t ep_type)
{
	if (bInterval == 0) {
		return 0;
	}

	switch (udev->speed) {
	case USB_SPEED_SPEED_SS:
	case USB_SPEED_SPEED_HS:
		bInterval = CLAMP(bInterval, 1, 16);
		return 1U << (bInterval - 1);
	case USB_SPEED_SPEED_FS:
		/*
		 * ISO FS uses 2^(bInterval - 1) * 1ms
		 * unlike ISO HS which uses 2^(bInterval - 1) * 125microseconds
		 * INT FS does it the same as INT LS
		 * See USB 2.0 Specification 5.6.4 and 5.7.4
		 */
		if (ep_type == USB_EP_TYPE_ISO) {
			bInterval = CLAMP(bInterval, 1, 16);
			return (1U << (bInterval - 1)) * 8;
		}
		bInterval = CLAMP(bInterval, 1, 255);
		return bInterval * 8;
	case USB_SPEED_SPEED_LS:
	case USB_SPEED_UNKNOWN:
		bInterval = CLAMP(bInterval, 10, 255);
		return bInterval * 8;
	}

	return 0;
}

struct uhc_transfer *uhc_xfer_alloc(const struct device *dev,
				    const uint8_t ep,
				    struct usb_device *const udev,
				    void *const cb,
				    void *const cb_priv)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep) & 0xF;
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_transfer *xfer = NULL;
	uint16_t mps;
	uint16_t bInterval;
	uint8_t type;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		goto xfer_alloc_error;
	}

	if (ep_idx == 0) {
		bInterval = 0;
		type = USB_EP_TYPE_CONTROL;
		mps = udev->dev_desc.bMaxPacketSize0;
	} else {
		struct usb_ep_descriptor *ep_desc;

		if (USB_EP_DIR_IS_IN(ep)) {
			ep_desc = udev->ep_in[ep_idx].desc;
		} else {
			ep_desc = udev->ep_out[ep_idx].desc;
		}

		if (ep_desc == NULL) {
			LOG_ERR("Endpoint 0x%02x is not configured", ep);
			goto xfer_alloc_error;
		}

		mps = ep_desc->wMaxPacketSize;
		bInterval = ep_desc->bInterval;
		type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
	}

	LOG_DBG("Allocate xfer, ep 0x%02x mps %u cb %p", ep, mps, cb);

	if (k_mem_slab_alloc(&uhc_xfer_pool, (void **)&xfer, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate transfer");
		goto xfer_alloc_error;
	}

	memset(xfer, 0, sizeof(struct uhc_transfer));
	sys_ref_init(&xfer->ref);
	xfer->ep = ep;
	xfer->mps = mps;
	xfer->interval = uhc_xfer_bInterval_to_microframe(udev, bInterval, type);
	xfer->bInterval = bInterval;
	xfer->type = type;
	xfer->udev = udev;
	xfer->cb = cb;
	xfer->priv = cb_priv;

xfer_alloc_error:
	api->unlock(dev);

	return xfer;
}

struct uhc_transfer *uhc_xfer_alloc_with_buf(const struct device *dev,
					     const uint8_t ep,
					     struct usb_device *const udev,
					     void *const cb,
					     void *const cb_priv,
					     size_t size)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;

	buf = uhc_xfer_buf_alloc(dev, size);
	if (buf == NULL) {
		return NULL;
	}

	xfer = uhc_xfer_alloc(dev, ep, udev, cb, cb_priv);
	if (xfer == NULL) {
		net_buf_unref(buf);
		return NULL;
	}

	xfer->buf = buf;

	return xfer;
}

static void uhc_xfer_release(struct sys_ref *const ref)
{
	struct uhc_transfer *const xfer = CONTAINER_OF(ref, struct uhc_transfer, ref);

	k_mem_slab_free(&uhc_xfer_pool, (void *)xfer);
}

void uhc_xfer_ref(struct uhc_transfer *const xfer)
{
	sys_ref_get(&xfer->ref);
}

bool uhc_xfer_unref(struct uhc_transfer *const xfer)
{
	return sys_ref_put(&xfer->ref, uhc_xfer_release);
}

int uhc_xfer_free(const struct device *dev, struct uhc_transfer *const xfer)
{
	ARG_UNUSED(dev);

	uhc_xfer_unref(xfer);

	return 0;
}

int uhc_xfer_buf_add(const struct device *dev,
		     struct uhc_transfer *const xfer,
		     struct net_buf *buf)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	int ret = 0;

	api->lock(dev);
	if (xfer->queued) {
		ret = -EBUSY;
	} else {
		xfer->buf = buf;
	}

	api->unlock(dev);

	return ret;
}

void uhc_xfer_set_interval(struct uhc_transfer *const xfer, uint16_t bInterval) {
	xfer->bInterval = bInterval;
	xfer->interval = uhc_xfer_bInterval_to_microframe(xfer->udev, bInterval, xfer->type);
}

int uhc_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_enqueue_error;
	}

	xfer->queued = 1;
	uhc_xfer_ref(xfer);
	ret = api->ep_enqueue(dev, xfer);
	if (ret) {
		xfer->queued = 0;
		uhc_xfer_unref(xfer);
	}


ep_enqueue_error:
	api->unlock(dev);

	return ret;
}

int uhc_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_dequeue_error;
	}

	ret = api->ep_dequeue(dev, xfer);
	xfer->queued = 0;

ep_dequeue_error:
	api->unlock(dev);

	return ret;
}

int uhc_enable(const struct device *dev)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		ret = -EPERM;
		goto uhc_enable_error;
	}

	if (uhc_is_enabled(dev)) {
		ret = -EALREADY;
		goto uhc_enable_error;
	}

	ret = api->enable(dev);
	if (ret == 0) {
		atomic_set_bit(&data->status, UHC_STATUS_ENABLED);
	}

uhc_enable_error:
	api->unlock(dev);

	return ret;
}

int uhc_disable(const struct device *dev)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (!uhc_is_enabled(dev)) {
		ret = -EALREADY;
		goto uhc_disable_error;
	}

	ret = api->disable(dev);
	atomic_clear_bit(&data->status, UHC_STATUS_ENABLED);

uhc_disable_error:
	api->unlock(dev);

	return ret;
}

int uhc_init(const struct device *dev,
	     uhc_event_cb_t event_cb, const void *const event_ctx)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	int ret;

	if (event_cb == NULL) {
		return -EINVAL;
	}

	api->lock(dev);

	if (uhc_is_initialized(dev)) {
		ret = -EALREADY;
		goto uhc_init_error;
	}

	data->event_cb = event_cb;
	data->event_ctx = event_ctx;
	sys_dlist_init(&data->ctrl_xfers);
	sys_dlist_init(&data->bulk_xfers);

	ret = api->init(dev);
	if (ret == 0) {
		atomic_set_bit(&data->status, UHC_STATUS_INITIALIZED);
	}

uhc_init_error:
	api->unlock(dev);

	return ret;
}

int uhc_shutdown(const struct device *dev)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	int ret;

	api->lock(dev);

	if (uhc_is_enabled(dev)) {
		ret = -EBUSY;
		goto uhc_shutdown_error;
	}

	if (!uhc_is_initialized(dev)) {
		ret = -EALREADY;
		goto uhc_shutdown_error;
	}

	ret = api->shutdown(dev);
	atomic_clear_bit(&data->status, UHC_STATUS_INITIALIZED);

uhc_shutdown_error:
	api->unlock(dev);

	return ret;
}
