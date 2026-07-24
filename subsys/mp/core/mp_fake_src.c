/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/core/mp_fake_src.h>

LOG_MODULE_REGISTER(mp_fake_src, CONFIG_MP_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mp_fake_src_pool, 1, CONFIG_MP_FAKE_SRC_BUF_SZ,
			  sizeof(struct mp_buffer_meta), mp_buffer_destroy);

static int mp_fake_src_pool_acquire(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *nb;
	struct mp_buffer_meta *meta;

	if (pool == NULL || buf == NULL || pool->nb_pool == NULL) {
		return -EINVAL;
	}

	nb = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
	if (nb == NULL) {
		LOG_ERR("Failed to acquire buffer from the pool");
		return -ENOBUFS;
	}

	nb->len = pool->config.size;

	meta = mp_buffer_get_meta(nb);
	meta->pool = pool;
	meta->bytes_used = pool->config.size;
	meta->timestamp = k_uptime_get_32();

	*buf = nb;

	return 0;
}

static int mp_fake_src_pool_release(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	if (buf == NULL) {
		return 0;
	}

	struct mp_buffer_meta *meta = mp_buffer_get_meta(buf);

	if (meta != NULL) {
		meta->bytes_used = 0;
		meta->timestamp = 0;
		meta->driver_buf = NULL;
		meta->priv = NULL;
	}

	buf->len = 0;

	return 0;
}

void mp_fake_src_init(struct mp_element *self)
{
	struct mp_fake_src *fsrc = (struct mp_fake_src *)self;

	mp_src_init(self);

	mp_buffer_pool_init(&fsrc->pool);
	fsrc->pool.nb_pool = &mp_fake_src_pool;
	fsrc->pool.config.min_buffers = 1;
	fsrc->pool.config.size = CONFIG_MP_FAKE_SRC_BUF_SZ;
	fsrc->pool.acquire_buffer = mp_fake_src_pool_acquire;
	fsrc->pool.release_buffer = mp_fake_src_pool_release;

	fsrc->src.pool = &fsrc->pool;
}
