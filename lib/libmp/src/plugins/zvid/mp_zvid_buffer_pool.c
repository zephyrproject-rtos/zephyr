/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "mp_zvid_buffer_pool.h"
#include "mp_zvid_object.h"

LOG_MODULE_REGISTER(mp_zvid_buffer_pool, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_zvid_buffer_pool_configure(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	/* Allocate just the pool's buffers structure */
	pool->buffers = k_calloc(pool->config.min_buffers, sizeof(struct mp_buffer));

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		pool->buffers[i].object.release = mp_buffer_release;
	}

	return true;
}

static bool mp_zvid_buffer_pool_start(struct mp_buffer_pool *pool)
{
	struct mp_zvid_buffer_pool *zvid_pool = MP_ZVID_BUFFERPOOL(pool);
	struct video_buffer vbuf = {.type = zvid_pool->zvid_obj->type};
	struct video_buffer_request vbr = {
		.memory = VIDEO_MEMORY_INTERNAL,
		.count = pool->config.min_buffers,
		.size = pool->config.size,
		.align = pool->config.align,
		.timeout = K_FOREVER,
	};
	int ret = video_request_buffers(&vbr);

	if (ret) {
		LOG_ERR("Failed to request buffers, errno %d", ret);
		return false;
	}

	for (uint8_t i = 0; i < vbr.count; i++) {
		/* Wrap the zvid buffer to a generic libMP buffer */
		pool->buffers[i].pool = pool;
		pool->buffers[i].index = vbr.start_index + i;

		/* Enqueue the buffer */
		vbuf.index = pool->buffers[i].index;
		ret = video_enqueue(zvid_pool->zvid_obj->vdev, &vbuf);
		if (ret) {
			LOG_ERR("Failed to enqueue buffer %u, ret = %d", vbuf.index, ret);
			return false;
		}
	}

	if (video_stream_start(zvid_pool->zvid_obj->vdev, zvid_pool->zvid_obj->type)) {
		LOG_ERR("Unable to start buffer pool");
		return false;
	}

	LOG_INF("Started buffer pool");

	return true;
}

static bool mp_zvid_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	return true;
}

static bool mp_zvid_buffer_pool_acquire_buffer(struct mp_buffer_pool *pool,
					       struct mp_buffer **buffer)
{
	struct video_buffer *vbuf = &(struct video_buffer){};
	struct mp_zvid_buffer_pool *zvid_pool = MP_ZVID_BUFFERPOOL(pool);
	int err;

	vbuf->type = zvid_pool->zvid_obj->type;
	err = video_dequeue(zvid_pool->zvid_obj->vdev, &vbuf, K_FOREVER);
	if (err) {
		LOG_ERR("Unable to dequeue video buffer");
		return false;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (pool->buffers[i].index == vbuf->index) {
			pool->buffers[i].data = vbuf->buffer;
			pool->buffers[i].bytes_used = vbuf->bytesused;
			pool->buffers[i].timestamp = vbuf->timestamp;
			pool->buffers[i].line_offset = vbuf->line_offset;

			*buffer = mp_buffer_ref(&pool->buffers[i]);

			return true;
		}
	}

	return false;
}

static void mp_zvid_buffer_pool_release_buffer(struct mp_buffer_pool *pool,
					       struct mp_buffer *buffer)
{
	int err;
	struct mp_zvid_buffer_pool *zvid_pool = MP_ZVID_BUFFERPOOL(pool);
	struct video_buffer vbuf = {.type = zvid_pool->zvid_obj->type, .index = buffer->index};

	err = video_enqueue(zvid_pool->zvid_obj->vdev, &vbuf);
	if (err) {
		LOG_ERR("Unable to re-enqueue video buffer");
		return;
	}
}

void mp_zvid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_zvid_object *obj)
{
	struct mp_zvid_buffer_pool *zvid_pool = MP_ZVID_BUFFERPOOL(pool);

	zvid_pool->zvid_obj = obj;

	pool->configure = mp_zvid_buffer_pool_configure;
	pool->start = mp_zvid_buffer_pool_start;
	pool->stop = mp_zvid_buffer_pool_stop;
	pool->acquire_buffer = mp_zvid_buffer_pool_acquire_buffer;
	pool->release_buffer = mp_zvid_buffer_pool_release_buffer;
}
