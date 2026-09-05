/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mp/core/mp_buffer.h>

NET_BUF_POOL_FIXED_DEFINE(mp_buf_pool, CONFIG_MP_NET_BUF_POOL_COUNT, 1,
			  sizeof(struct mp_buffer_meta), mp_buffer_destroy);

void mp_buffer_destroy(struct net_buf *buf)
{
	struct mp_buffer_meta *bm;

	if (buf == NULL) {
		return;
	}

	bm = mp_buffer_get_meta(buf);
	if (bm != NULL && bm->pool != NULL && bm->pool->release_buffer != NULL) {
		bm->pool->release_buffer(bm->pool, buf);
	}

	net_buf_destroy(buf);
}

int mp_buffer_pool_configure(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	if (pool == NULL) {
		return -EINVAL;
	}

	if (pool->configure == NULL) {
		return -ENOSYS;
	}

	return pool->configure(pool, config);
}

int mp_buffer_pool_start(struct mp_buffer_pool *pool)
{
	int ret;

	if (pool == NULL) {
		return -EINVAL;
	}

	if (pool->started) {
		return 0;
	}

	if (pool->start == NULL) {
		pool->started = true;
		return -ENOSYS;
	}

	ret = pool->start(pool);
	if (ret == 0) {
		pool->started = true;
	}

	return ret;
}

int mp_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	int ret;

	if (pool == NULL) {
		return -EINVAL;
	}

	if (!pool->started) {
		return 0;
	}

	if (pool->stop == NULL) {
		return -ENOSYS;
	}

	ret = pool->stop(pool);
	if (ret == 0) {
		pool->started = false;
	}

	return ret;
}

void mp_buffer_pool_init(struct mp_buffer_pool *pool)
{
	if (pool == NULL) {
		return;
	}

	pool->nb_pool = &mp_buf_pool;
	pool->started = false;
}
