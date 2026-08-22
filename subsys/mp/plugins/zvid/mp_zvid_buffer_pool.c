/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/zvid/mp_zvid_buffer_pool.h>
#include <zephyr/mp/zvid/mp_zvid_object.h>

LOG_MODULE_REGISTER(mp_zvid_buffer_pool, CONFIG_MP_LOG_LEVEL);

static int mp_zvid_buffer_pool_start(struct mp_buffer_pool *pool)
{
	int ret = 0;
	struct mp_zvid_buffer_pool *zvid_pool = (struct mp_zvid_buffer_pool *)pool;

	if (pool->config.min_buffers > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		LOG_ERR("min_buffers=%u exceeds CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=%u",
			pool->config.min_buffers, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
		return -EINVAL;
	}

	zvid_pool->vbuf_count = pool->config.min_buffers;

	for (uint8_t i = 0; i < zvid_pool->vbuf_count; i++) {
		struct video_buffer *vbuf = video_buffer_aligned_alloc(
			pool->config.size, pool->config.align, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Failed to alloc video buffer %u", i);
			return -ENOBUFS;
		}

		zvid_pool->vbufs[i] = vbuf;

		if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_INPUT) {
			k_fifo_put(&zvid_pool->free_fifo, vbuf);
			continue;
		}

		vbuf->type = zvid_pool->zvid_obj->type;
		ret = video_enqueue(zvid_pool->zvid_obj->vdev, vbuf);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue video buffer %u", i);
			(void)video_buffer_release(vbuf);
			return ret;
		}
	}

	if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_OUTPUT) {
		ret = video_stream_start(zvid_pool->zvid_obj->vdev, zvid_pool->zvid_obj->type);
		if (ret != 0) {
			LOG_ERR("Failed to start video streaming");
			return ret;
		}
	}

	LOG_INF("Started video %s buffer pool",
		zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_OUTPUT ? "output" : "input");

	return ret;
}

static int mp_zvid_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	int ret = 0;
	struct mp_zvid_buffer_pool *zvid_pool = (struct mp_zvid_buffer_pool *)pool;

	if (zvid_pool == NULL || zvid_pool->zvid_obj == NULL || zvid_pool->zvid_obj->vdev == NULL) {
		return -EINVAL;
	}

	if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_OUTPUT) {
		ret = video_stream_stop(zvid_pool->zvid_obj->vdev, zvid_pool->zvid_obj->type);
		if (ret != 0) {
			LOG_ERR("Failed to stop video streaming");
			return ret;
		}
	}

	for (uint8_t i = 0; i < zvid_pool->vbuf_count; i++) {
		ret = video_buffer_release(zvid_pool->vbufs[i]);
		if (ret != 0) {
			LOG_ERR("Failed to release video buffer %u", i);
			return ret;
		}

		zvid_pool->vbufs[i] = NULL;
	}

	zvid_pool->vbuf_count = 0;

	return ret;
}

static int mp_zvid_buffer_pool_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct mp_zvid_buffer_pool *zvid_pool = (struct mp_zvid_buffer_pool *)pool;
	struct video_buffer *vbuf = &(struct video_buffer){0};
	struct mp_buffer_meta *bm;
	int ret = 0;

	if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_INPUT) {
		vbuf = k_fifo_get(&zvid_pool->free_fifo, K_FOREVER);
		if (vbuf == NULL) {
			LOG_ERR("Failed to get a free input buffer");
			return -ENOBUFS;
		}
	} else {
		vbuf->type = zvid_pool->zvid_obj->type;
		ret = video_dequeue(zvid_pool->zvid_obj->vdev, &vbuf, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to dequeue a video buffer");
			return ret;
		}
	}

	*buf = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (*buf == NULL) {
		LOG_ERR("Failed to allocate a net_buf wrapper for the video buffer");
		/* Re-enqueue the video buffer */
		if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_INPUT) {
			k_fifo_put(&zvid_pool->free_fifo, vbuf);
		} else {
			(void)video_enqueue(zvid_pool->zvid_obj->vdev, vbuf);
		}

		return -ENOBUFS;
	}

	/* Set buffer metadata */
	bm = mp_buffer_get_meta(*buf);
	bm->pool = pool;
	bm->priv = vbuf;
	bm->bytes_used = vbuf->bytesused;
	bm->timestamp = vbuf->timestamp;

	return ret;
}

static int mp_zvid_buffer_pool_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	struct mp_zvid_buffer_pool *zvid_pool = (struct mp_zvid_buffer_pool *)pool;
	struct video_buffer *vbuf = mp_buffer_get_meta(buf)->priv;
	int ret = 0;

	if (vbuf == NULL) {
		return -EINVAL;
	}

	if (zvid_pool->zvid_obj->type == VIDEO_BUF_TYPE_INPUT) {
		k_fifo_put(&zvid_pool->free_fifo, vbuf);
		return 0;
	}

	vbuf->type = zvid_pool->zvid_obj->type;
	ret = video_enqueue(zvid_pool->zvid_obj->vdev, vbuf);
	if (ret != 0) {
		LOG_ERR("Failed to re-enqueue the video buffer");
	}

	return ret;
}

void mp_zvid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_zvid_object *obj)
{
	struct mp_zvid_buffer_pool *zvid_pool = (struct mp_zvid_buffer_pool *)pool;

	k_fifo_init(&zvid_pool->free_fifo);
	zvid_pool->zvid_obj = obj;

	mp_buffer_pool_init(pool);

	pool->start = mp_zvid_buffer_pool_start;
	pool->stop = mp_zvid_buffer_pool_stop;
	pool->acquire_buffer = mp_zvid_buffer_pool_acquire_buffer;
	pool->release_buffer = mp_zvid_buffer_pool_release_buffer;
}
