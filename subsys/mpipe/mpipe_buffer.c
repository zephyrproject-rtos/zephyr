/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mpipe/mpipe_buffer.h>

NET_BUF_POOL_FIXED_DEFINE(mpipe_buf_pool, CONFIG_MPIPE_NET_BUF_POOL_COUNT, 1,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

void mpipe_buffer_destroy(struct net_buf *buf)
{
	struct mpipe_buffer_meta *bm;

	__ASSERT_NO_MSG(buf != NULL);

	bm = mpipe_buffer_get_meta(buf);
	if (bm != NULL && bm->pool != NULL && bm->pool->release_buffer != NULL) {
		bm->pool->release_buffer(bm->pool, buf);
	}

	net_buf_destroy(buf);
}

int mpipe_buffer_pool_configure(struct mpipe_buffer_pool *pool, struct mpipe_structure *config)
{
	__ASSERT_NO_MSG(pool != NULL);

	if (pool->configure == NULL) {
		return -ENOSYS;
	}

	return pool->configure(pool, config);
}

int mpipe_buffer_pool_set_req_config(struct mpipe_buffer_pool *pool,
				     const struct mpipe_buffer_pool_config *cfg)
{
	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	if (pool->started) {
		return -EBUSY;
	}

	pool->req_config = *cfg;
	pool->config = *cfg;

	return 0;
}

int mpipe_buffer_pool_set_config(struct mpipe_buffer_pool *pool,
				 const struct mpipe_buffer_pool_config *cfg)
{
	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	if (pool->started) {
		return -EBUSY;
	}

	if (pool->set_config != NULL) {
		return pool->set_config(pool, cfg);
	}

	pool->config = *cfg;

	return 0;
}

int mpipe_buffer_pool_start(struct mpipe_buffer_pool *pool)
{
	int ret;

	__ASSERT_NO_MSG(pool != NULL);

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

int mpipe_buffer_pool_stop(struct mpipe_buffer_pool *pool)
{
	int ret;

	__ASSERT_NO_MSG(pool != NULL);

	/*
	 * Forget what the run negotiated, before anything else: a pool that was
	 * proposed and turned down never started, and still must not carry its
	 * demands into the next run.
	 */
	pool->config = pool->req_config;

	if (!pool->started) {
		return 0;
	}

	if (pool->stop == NULL) {
		/* Mirror start: track the logical state even with no hook */
		pool->started = false;
		return -ENOSYS;
	}

	ret = pool->stop(pool);
	if (ret == 0) {
		pool->started = false;
	}

	return ret;
}

void mpipe_buffer_pool_init(struct mpipe_buffer_pool *pool)
{
	__ASSERT_NO_MSG(pool != NULL);

	pool->nb_pool = &mpipe_buf_pool;
	pool->started = false;
}
