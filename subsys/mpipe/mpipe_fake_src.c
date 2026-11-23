/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_fake_src.h>

LOG_MODULE_REGISTER(mpipe_fake_src, CONFIG_MPIPE_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mpipe_fake_src_pool, 1, CONFIG_MPIPE_FAKE_SRC_BUF_SZ,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

static int mpipe_fake_src_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *nb;
	struct mpipe_buffer_meta *meta;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	if (pool->nb_pool == NULL) {
		return -EINVAL;
	}

	nb = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
	if (nb == NULL) {
		LOG_ERR("Failed to acquire buffer from the pool");
		return -ENOBUFS;
	}

	nb->len = pool->config.size;

	meta = mpipe_buffer_get_meta(nb);
	meta->pool = pool;
	meta->bytes_used = pool->config.size;
	meta->timestamp = k_uptime_get_32();

	*buf = nb;

	return 0;
}

static int mpipe_fake_src_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	__ASSERT_NO_MSG(buf != NULL);

	struct mpipe_buffer_meta *meta = mpipe_buffer_get_meta(buf);

	if (meta != NULL) {
		meta->bytes_used = 0;
		meta->timestamp = 0;
		meta->driver_buf = NULL;
		meta->priv = NULL;
	}

	buf->len = 0;

	return 0;
}

int mpipe_fake_src_init(struct mpipe_fake_src *fsrc, uint8_t id)
{
	__ASSERT_NO_MSG(fsrc != NULL);

	struct mpipe_element *self = &fsrc->src.element;
	int ret = mpipe_src_init(&fsrc->src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "fake_src");

	const struct mpipe_buffer_pool_config pool_req = {
		.size = CONFIG_MPIPE_FAKE_SRC_BUF_SZ,
		.min_buffers = 1,
	};

	mpipe_buffer_pool_init(&fsrc->pool);
	fsrc->pool.nb_pool = &mpipe_fake_src_pool;
	(void)mpipe_buffer_pool_set_req_config(&fsrc->pool, &pool_req);
	fsrc->pool.acquire_buffer = mpipe_fake_src_pool_acquire;
	fsrc->pool.release_buffer = mpipe_fake_src_pool_release;

	fsrc->src.pool = &fsrc->pool;

	return 0;
}
