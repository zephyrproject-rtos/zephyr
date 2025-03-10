/*
 * Copyright 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/drivers/usb/usb_buf.h>

static inline uint8_t *usb_pool_data_alloc(struct net_buf *const buf,
					   size_t *const size, k_timeout_t timeout)
{
	struct net_buf_pool *const buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *const pool = buf_pool->alloc->alloc_data;
	void *b;

	*size = USB_BUF_ROUND_UP(*size);
	b = k_heap_aligned_alloc(pool, USB_BUF_ALIGN, *size, timeout);
	if (b == NULL) {
		*size = 0;
		return NULL;
	}

	return b;
}

static inline void usb_pool_data_unref(struct net_buf *buf, uint8_t *const data)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;

	k_heap_free(pool, data);
}

const struct net_buf_data_cb net_buf_dma_cb = {
	.alloc = usb_pool_data_alloc,
	.unref = usb_pool_data_unref,
};
