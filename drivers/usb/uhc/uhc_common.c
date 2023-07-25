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

K_MEM_SLAB_DEFINE_STATIC(uhc_xfer_pool, sizeof(struct uhc_transfer),
			 CONFIG_UHC_XFER_COUNT, sizeof(void *));

NET_BUF_POOL_VAR_DEFINE(uhc_ep_pool,
			CONFIG_UHC_BUF_COUNT, CONFIG_UHC_BUF_POOL_SIZE,
			0, NULL);


int uhc_submit_event(const struct device *dev,
		     const enum uhc_event_type type,
		     const int status,
		     struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct uhc_event drv_evt = {
		.type = type,
		.xfer = xfer,
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
	sys_dlist_remove(&xfer->node);
	xfer->queued = 0;
	xfer->claimed = 0;
	uhc_submit_event(dev, UHC_EVT_EP_REQUEST, err, xfer);
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

	sys_dlist_append(&data->ctrl_xfers, &xfer->node);

	return 0;
}

struct uhc_transfer *uhc_xfer_alloc(const struct device *dev,
				    const uint8_t addr,
				    const uint8_t ep,
				    const uint8_t attrib,
				    const uint16_t mps,
				    const uint16_t timeout,
				    void *const owner)
{
	const struct uhc_api *api = dev->api;
	struct uhc_transfer *xfer = NULL;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		goto xfer_alloc_error;
	}

	LOG_DBG("Allocate xfer, ep 0x%02x attrib 0x%02x owner %p",
		ep, attrib, owner);

	if (k_mem_slab_alloc(&uhc_xfer_pool, (void **)&xfer, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate transfer");
		goto xfer_alloc_error;
	}

	memset(xfer, 0, sizeof(struct uhc_transfer));
	k_fifo_init(&xfer->queue);
	k_fifo_init(&xfer->done);
	xfer->addr = addr;
	xfer->ep = ep;
	xfer->attrib = attrib;
	xfer->mps = mps;
	xfer->timeout = timeout;
	xfer->owner = owner;

xfer_alloc_error:
	api->unlock(dev);

	return xfer;
}

int uhc_xfer_free(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_api *api = dev->api;
	struct net_buf *buf;
	int ret = 0;

	api->lock(dev);

	if (xfer->queued || xfer->claimed) {
		ret = -EBUSY;
		LOG_ERR("Transfer is still claimed");
		goto xfer_free_error;
	}

	while (!k_fifo_is_empty(&xfer->queue)) {
		buf = net_buf_get(&xfer->queue, K_NO_WAIT);
		uhc_xfer_buf_free(dev, buf);
	}

	while (!k_fifo_is_empty(&xfer->done)) {
		buf = net_buf_get(&xfer->done, K_NO_WAIT);
		uhc_xfer_buf_free(dev, buf);
	}

	k_mem_slab_free(&uhc_xfer_pool, (void **)&xfer);

xfer_free_error:
	api->unlock(dev);

	return ret;
}

struct net_buf *uhc_xfer_buf_alloc(const struct device *dev,
				   struct uhc_transfer *const xfer,
				   const size_t size)
{
	const struct uhc_api *api = dev->api;
	struct net_buf *buf = NULL;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		goto buf_alloc_error;
	}

	if (xfer->queued || xfer->claimed) {
		goto buf_alloc_error;
	}

	LOG_DBG("Allocate net_buf, ep 0x%02x, size %zd", xfer->ep, size);
	buf = net_buf_alloc_len(&uhc_ep_pool, size, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Failed to allocate net_buf");
		goto buf_alloc_error;
	}

	if (buf->size < size) {
		LOG_ERR("Buffer is smaller than requested");
		net_buf_unref(buf);
		buf = NULL;
		goto buf_alloc_error;
	}

	k_fifo_put(&xfer->queue, &buf->node);

buf_alloc_error:
	api->unlock(dev);

	return buf;
}

int uhc_xfer_buf_free(const struct device *dev, struct net_buf *const buf)
{
	const struct uhc_api *api = dev->api;
	int ret = 0;

	api->lock(dev);
	net_buf_unref(buf);
	api->unlock(dev);

	return ret;
}

int uhc_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_api *api = dev->api;
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_enqueue_error;
	}

	xfer->claimed = 1;
	ret = api->ep_enqueue(dev, xfer);
	if (ret) {
		xfer->claimed = 0;
	}

ep_enqueue_error:
	api->unlock(dev);

	return ret;
}

int uhc_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	const struct uhc_api *api = dev->api;
	int ret;

	api->lock(dev);

	if (!uhc_is_initialized(dev)) {
		ret = -EPERM;
		goto ep_dequeue_error;
	}

	ret = api->ep_dequeue(dev, xfer);

ep_dequeue_error:
	api->unlock(dev);

	return ret;
}

int uhc_enable(const struct device *dev)
{
	const struct uhc_api *api = dev->api;
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
	const struct uhc_api *api = dev->api;
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

int uhc_init(const struct device *dev, uhc_event_cb_t event_cb)
{
	const struct uhc_api *api = dev->api;
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
	const struct uhc_api *api = dev->api;
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
