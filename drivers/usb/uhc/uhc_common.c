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
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	struct uhc_event drv_evt = {
		.type = UHC_EVT_EP_REQUEST,
		.xfer = xfer,
		.dev = dev,
	};

	api->lock(dev);

	sys_dlist_remove(&xfer->node);
	xfer->queued = 0;
	xfer->err = err;

	api->unlock(dev);

	data->event_cb(dev, &drv_evt);
	(void)uhc_xfer_unref(xfer);
}

/* Already locked by caller */
static struct uhc_transfer *uhc_xfer_get_next_periodic(const struct device *dev,
						       uint32_t microframe_number,
						       enum uhc_xfer_mask mask,
						       uhc_xfer_filter_func_t filter, void *priv)
{
	struct uhc_data *data = dev->data;
	sys_dnode_t *node;
	struct uhc_transfer *xfer;

	for (node = sys_dlist_peek_tail(&data->periodic_xfers); node != NULL;
	     node = sys_dlist_peek_prev(&data->periodic_xfers, node)) {
		xfer = SYS_DLIST_CONTAINER(node, xfer, node);

		if (xfer_seq_cmp(xfer->start_microframe, microframe_number) > 0) {
			break;
		}

		if ((BIT(xfer->type) & mask) == 0) {
			continue;
		}

		if (filter == NULL || filter(xfer, priv)) {
			return xfer;
		}
	}

	return NULL;
}

/* Already locked by caller */
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

/* Already locked by caller */
static void xfer_schedule_periodic(const struct device *dev,
				  struct uhc_transfer *const xfer,
				  const uint32_t current_microframe)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *item;

	xfer->start_microframe = current_microframe;

	/* Find the last position in the schedule for this address and endpoint */
	SYS_DLIST_FOR_EACH_CONTAINER(&data->periodic_xfers, item, node) {
		if (xfer->udev->addr == item->udev->addr &&
		    xfer->ep == item->ep &&
		    xfer_seq_cmp(xfer->start_microframe, item->start_microframe) < 0) {
			xfer->start_microframe = item->start_microframe;
		}
	}

	/* Schedule it on the next interval after it */
	xfer->start_microframe++;
	xfer->start_microframe = ROUND_UP(xfer->start_microframe, xfer->microframe_interval);

	/* Insert it at correct offset in the list */
	SYS_DLIST_FOR_EACH_CONTAINER(&data->periodic_xfers, item, node) {
		if (xfer_seq_cmp(item->start_microframe, xfer->start_microframe) < 0) {
			continue;
		}
		sys_dlist_insert(&item->node, &xfer->node);
		return;
	}

	sys_dlist_append(&data->periodic_xfers, &xfer->node);
}

struct uhc_transfer *uhc_xfer_get_next(const struct device *dev, uint32_t microframe_number,
				       enum uhc_xfer_mask mask, uhc_xfer_filter_func_t filter,
				       void *priv)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer = NULL;

	api->lock(dev);

	if (mask & UHC_XFER_MASK_PERIODIC) {
		xfer = uhc_xfer_get_next_periodic(dev, microframe_number, mask, filter, priv);
	}

	if (xfer == NULL && mask & UHC_XFER_MASK_CONTROL) {
		xfer = uhc_xfer_get_next_non_periodic(dev, &data->ctrl_xfers, filter, priv);
	}

	if (xfer == NULL && mask & UHC_XFER_MASK_BULK) {
		xfer = uhc_xfer_get_next_non_periodic(dev, &data->bulk_xfers, filter, priv);
	}

	if (xfer != NULL) {
		sys_dlist_remove(&xfer->node);
		sys_dlist_append(&data->active_xfers, &xfer->node);
	}

	api->unlock(dev);

	return xfer;
}

int uhc_xfer_append(const struct device *dev, struct uhc_transfer *const xfer,
		    uint32_t current_microframe_number)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_data *data = dev->data;
	int ret = 0;

	api->lock(dev);

	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		sys_dlist_append(&data->ctrl_xfers, &xfer->node);
		break;
	case USB_EP_TYPE_BULK:
		sys_dlist_append(&data->bulk_xfers, &xfer->node);
		break;
	case USB_EP_TYPE_ISO:
	case USB_EP_TYPE_INTERRUPT:
		xfer_schedule_periodic(dev, xfer, current_microframe_number);
		break;
	default:
		LOG_ERR("Invalid xfer type: %d", xfer->type);
		ret = -EINVAL;
		goto end;
	}

end:
	api->unlock(dev);

	return ret;
}

struct net_buf *uhc_xfer_buf_alloc(const struct device *dev,
				   const size_t size,
				   const k_timeout_t timeout)
{
	return net_buf_alloc_len(&uhc_ep_pool, size, timeout);
}

void uhc_xfer_buf_free(const struct device *dev, struct net_buf *const buf)
{
	net_buf_unref(buf);
}

uint32_t uhc_xfer_bInterval_to_microframe(const struct usb_device *const udev,
					  uint16_t bInterval, uint8_t ep_type)
{
	/* See USB 2.0 Specification Table 9-13 */
	switch (udev->speed) {
	case USB_SPEED_SPEED_SS:
	case USB_SPEED_SPEED_HS:
		if (!IN_RANGE(bInterval, 1, 16)) {
			LOG_ERR("Invalid bInterval %u, not in range [1, 16]", bInterval);
			bInterval = CLAMP(bInterval, 1, 16);
		}

		/* 2^(bInterval - 1) * 125 us */
		return 1U << (bInterval - 1);

	case USB_SPEED_SPEED_FS:
		if (ep_type == USB_EP_TYPE_ISO) {
			if (!IN_RANGE(bInterval, 1, 16)) {
				LOG_ERR("Invalid bInterval %u, not in range [1, 16]", bInterval);
				bInterval = CLAMP(bInterval, 1, 16);
			}

			/* 2^(bInterval - 1) * 1 ms */
			return (1U << (bInterval - 1)) * 8;
		} else {
			if (!IN_RANGE(bInterval, 10, 255)) {
				LOG_WRN("Invalid bInterval %u, not in range [10, 255]", bInterval);
				bInterval = CLAMP(bInterval, 10, 255);
			}

			return bInterval * 8;
		}
		break;
	case USB_SPEED_SPEED_LS:
	case USB_SPEED_UNKNOWN:
		if (!IN_RANGE(bInterval, 10, 255)) {
			LOG_WRN("Invalid bInterval %u, not in range [10, 255]", bInterval);
			bInterval = CLAMP(bInterval, 10, 255);
		}

		return bInterval * 8;
	}

	return 0;
}

uint32_t uhc_xfer_microframe_to_bInterval(const struct usb_device *const udev,
					  uint32_t microframe_interval, uint8_t ep_type)
{
	/* See USB 2.0 Specification Table 9-13 */
	switch (udev->speed) {
	case USB_SPEED_SPEED_SS:
	case USB_SPEED_SPEED_HS:
		/* inverse of 2^(bInterval - 1) * 125 us */
		return LOG2(microframe_interval) + 1;
	case USB_SPEED_SPEED_FS:
		if (ep_type == USB_EP_TYPE_ISO) {
			/* inverse of 2^(bInterval - 1) * 1 ms */
			return LOG2(microframe_interval / 8) - 1;
		}
		return microframe_interval / 8;
	case USB_SPEED_SPEED_LS:
	case USB_SPEED_UNKNOWN:
		return microframe_interval / 8;
	}

	return 0;
}

struct uhc_transfer *uhc_xfer_alloc(const struct device *dev,
				    const uint8_t ep,
				    struct usb_device *const udev,
				    void *const cb,
				    void *const cb_priv,
				    const k_timeout_t timeout)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	struct uhc_transfer *xfer = NULL;
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		LOG_ERR("Cannot allocate for %s, it is not initialized", dev->name);
		goto end;
	}

	LOG_DBG("Allocate xfer, ep 0x%02x cb %p", ep, cb);

	if (k_mem_slab_alloc(&uhc_xfer_pool, (void **)&xfer, timeout)) {
		LOG_ERR("Failed to allocate transfer from memory slab");
		goto end;
	}

	memset(xfer, 0, sizeof(struct uhc_transfer));

	ret = uhc_get_ep_properties(dev, udev, ep, &xfer->mps,
				    &xfer->microframe_interval, &xfer->type);
	if (ret != 0) {
		LOG_ERR("Failed to get endpoint properties");
		uhc_xfer_unref(xfer);
		xfer = NULL;
		goto end;
	}

	sys_ref_init(&xfer->ref);

	xfer->ep = ep;
	xfer->udev = udev;
	xfer->cb = cb;
	xfer->priv = cb_priv;

end:
	api->unlock(dev);

	return xfer;
}

int uhc_get_ep_properties(const struct device *dev,
			  struct usb_device *const udev,
			  const uint8_t ep,
			  uint16_t *const mps_p,
			  uint32_t *const microframe_interval_p,
			  uint8_t *const type_p)
{
	const struct uhc_driver_api *api = DEVICE_API_GET(uhc, dev);
	const uint8_t ep_idx = USB_EP_GET_IDX(ep) & 0xF;
	uint16_t mps;
	uint32_t microframe_interval;
	uint8_t type;
	int ret = 0;

	api->lock(dev);

	if (ep_idx == 0) {
		microframe_interval = 0;
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
			ret = -ENOENT;
			goto end;
		}

		mps = ep_desc->wMaxPacketSize;
		type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
		microframe_interval =
			uhc_xfer_bInterval_to_microframe(udev, ep_desc->bInterval, type);
	}

	if (mps_p != NULL) {
		*mps_p = mps;
	}
	if (microframe_interval_p != NULL) {
		*microframe_interval_p = microframe_interval;
	}
	if (type_p != NULL) {
		*type_p = type;
	}

end:
	api->unlock(dev);

	return ret;
}

struct uhc_transfer *uhc_xfer_alloc_with_buf(const struct device *dev,
					     const uint8_t ep,
					     struct usb_device *const udev,
					     void *const cb,
					     void *const cb_priv,
					     size_t size,
					     const k_timeout_t timeout)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;

	buf = uhc_xfer_buf_alloc(dev, size, timeout);
	if (buf == NULL) {
		return NULL;
	}

	xfer = uhc_xfer_alloc(dev, ep, udev, cb, cb_priv, timeout);
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

	__ASSERT(xfer->buf == NULL, "Expecting the previous buffer to be freed");

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
	uhc_xfer_ref(xfer);
	ret = api->ep_enqueue(dev, xfer);
	if (ret != 0) {
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
	sys_dlist_init(&data->periodic_xfers);
	sys_dlist_init(&data->active_xfers);

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
