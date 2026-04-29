/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "mp_zvid_buffer_pool_client.h"

LOG_MODULE_REGISTER(mp_zvid_buffer_pool_client, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_zvid_buffer_pool_client_configure(struct mp_buffer_pool *pool,
						 struct mp_structure *config)
{
	/* Allocate just the pool's buffers structure */
	pool->buffers = k_calloc(pool->config.min_buffers, sizeof(struct mp_buffer));

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		pool->buffers[i].object.release = mp_buffer_release;
	}

	return true;
}

static bool mp_zvid_buffer_pool_client_start(struct mp_buffer_pool *pool)
{
	struct mp_zvid_buffer_pool_client *zbpc = MP_ZVID_BUFFERPOOL_CLIENT(pool);

	struct video_buffer_request vbr = {
		.memory = VIDEO_MEMORY_INTERNAL,
		.count = pool->config.min_buffers,
		.size = pool->config.size,
		.align = pool->config.align,
		.timeout = K_NO_WAIT,
	};
	int ret = video_request_buffers(&vbr);

	if (ret) {
		LOG_ERR("Failed to request buffers, errno %d", ret);
		return false;
	}

	for (uint8_t i = 0; i < vbr.count; i++) {
		pool->buffers[i].pool = pool;
		pool->buffers[i].index = vbr.start_index + i;
		pool->buffers[i].data = vbr.start_addr + i * vbr.size;

		k_fifo_put(&zbpc->fifo, &pool->buffers[i]);
	}

	LOG_INF("Started buffer pool");

	return true;
}

static bool mp_zvid_buffer_pool_client_stop(struct mp_buffer_pool *pool)
{
	return true;
}

static bool mp_zvid_buffer_pool_client_acquire_buffer(struct mp_buffer_pool *pool,
					       struct mp_buffer **buffer)
{
	struct mp_zvid_buffer_pool_client *zbpc = MP_ZVID_BUFFERPOOL_CLIENT(pool);

	*buffer = mp_buffer_ref(k_fifo_get(&zbpc->fifo, K_FOREVER));

	return true;
}

static void mp_zvid_buffer_pool_client_release_buffer(struct mp_buffer_pool *pool,
					       struct mp_buffer *buffer)
{
	struct mp_zvid_buffer_pool_client *zbpc = MP_ZVID_BUFFERPOOL_CLIENT(pool);

	k_fifo_put(&zbpc->fifo, buffer);
}

void mp_zvid_buffer_pool_client_init(struct mp_buffer_pool *pool)
{
	struct mp_zvid_buffer_pool_client *vbpc = MP_ZVID_BUFFERPOOL_CLIENT(pool);

	k_fifo_init(&vbpc->fifo);

	pool->configure = mp_zvid_buffer_pool_client_configure;
	pool->start = mp_zvid_buffer_pool_client_start;
	pool->stop = mp_zvid_buffer_pool_client_stop;
	pool->acquire_buffer = mp_zvid_buffer_pool_client_acquire_buffer;
	pool->release_buffer = mp_zvid_buffer_pool_client_release_buffer;
}
