/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
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
}

void uhc_xfer_add_to_int(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	sys_dlist_t *int_list = &data->int_xfers;
	struct uhc_transfer *curr;

	SYS_DLIST_FOR_EACH_CONTAINER(int_list, curr, node) {
		if (xfer_seq_cmp(xfer->start_frame, curr->start_frame) < 0) {
			continue;
		}
		sys_dlist_insert(&curr->node, &xfer->node);
		return;
	}

	sys_dlist_append(int_list, &xfer->node);
}

void uhc_xfer_reschedule_periodic(const struct device *dev, struct uhc_transfer *const xfer,
				  uint32_t frame_number)
{
	uint32_t old_start_frame = xfer->start_frame;

	if (unlikely(xfer->interval == 0)) {
		/* Prevent a infinite loop if somehow interval equals 0 */
		xfer->interval = 1;
	}

	do {
		xfer->start_frame += xfer->interval;
	} while (xfer_seq_cmp(xfer->start_frame, frame_number) <= 0);

	sys_dlist_remove(&xfer->node);
	uhc_xfer_add_to_int(dev, xfer);

	LOG_DBG("Interrupt transfer advance frame old s.f. %u new s.f %u f.n. %u interval %u",
		old_start_frame, xfer->start_frame, frame_number, xfer->interval);
}

static enum uhc_xfer_mask uhc_xfer_type_to_mask(const struct uhc_transfer *xfer)
{
	return BIT(xfer->type);
}

static struct uhc_transfer *uhc_xfer_get_next_periodic(const struct device *dev,
						       uint32_t frame_number,
						       enum uhc_xfer_mask mask,
						       uhc_xfer_filter_func_t filter, void *priv)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer = NULL;
	sys_dnode_t *node;

	for (node = sys_dlist_peek_tail(&data->int_xfers); node != NULL;
	     node = sys_dlist_peek_prev(&data->int_xfers, node)) {
		xfer = SYS_DLIST_CONTAINER(node, xfer, node);

		if (xfer_seq_cmp(xfer->start_frame, frame_number) > 0) {
			break;
		}

		if (uhc_xfer_type_to_mask(xfer) & ~mask) {
			continue;
		}

		if (filter == NULL || filter(xfer, priv)) {
			return xfer;
		}
	}

	return NULL;
}

static struct uhc_transfer *uhc_xfer_get_next_non_periodic(const struct device *dev,
						       sys_dlist_t *dlist,
						       uhc_xfer_filter_func_t filter, void *priv)
{
	struct uhc_transfer *xfer = NULL;

	SYS_DLIST_FOR_EACH_CONTAINER(dlist, xfer, node) {
		if (filter == NULL || filter(xfer, priv)) {
			return xfer;
		}
	}

	return NULL;
}

struct uhc_transfer *uhc_xfer_get_next(const struct device *dev, uint32_t frame_number,
				       enum uhc_xfer_mask mask, uhc_xfer_filter_func_t filter,
				       void *priv)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer = NULL;

	if (mask & UHC_XFER_MASK_PERIODIC) {
		xfer = uhc_xfer_get_next_periodic(dev, frame_number, mask, filter, priv);
	}

	if (xfer == NULL && mask & UHC_XFER_MASK_CONTROL) {
		xfer = uhc_xfer_get_next_non_periodic(dev, &data->ctrl_xfers, filter, priv);
	}

	if (xfer == NULL && mask & UHC_XFER_MASK_BULK) {
		xfer = uhc_xfer_get_next_non_periodic(dev, &data->bulk_xfers, filter, priv);
	}

	return xfer;
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
	case USB_EP_TYPE_ISO:
		LOG_WRN("Iso xfers not implemeted yet, treating as interrupt xfer");
	case USB_EP_TYPE_INTERRUPT:
		uhc_xfer_add_to_int(dev, xfer);
		break;
	default:
		LOG_ERR("Invalid xfer type: %d", xfer->type);
	}

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
				    const size_t rec_len,
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
	xfer->ep = ep;
	xfer->mps = mps;
	xfer->interval = uhc_xfer_bInterval_to_microframe(udev, bInterval, type);
	xfer->bInterval = bInterval;
	xfer->type = type;
	xfer->udev = udev;
	xfer->cb = cb;
	xfer->priv = cb_priv;
	xfer->expected_data_len = rec_len;

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

	xfer = uhc_xfer_alloc(dev, ep, udev, size, cb, cb_priv);
	if (xfer == NULL) {
		net_buf_unref(buf);
		return NULL;
	}

	xfer->buf = buf;

	return xfer;
}

int uhc_xfer_free(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	int ret = 0;

	api->lock(dev);

	if (xfer->queued) {
		ret = -EBUSY;
		LOG_ERR("Transfer is still queued");
		goto xfer_free_error;
	}

	k_mem_slab_free(&uhc_xfer_pool, (void *)xfer);

xfer_free_error:
	api->unlock(dev);

	return ret;
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
	ret = api->ep_enqueue(dev, xfer);
	if (ret) {
		xfer->queued = 0;
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
	sys_dlist_init(&data->int_xfers);

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
