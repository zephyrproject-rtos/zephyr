/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/vid/mpipe_vid_buffer_pool_client.h>

LOG_MODULE_REGISTER(mpipe_vid_buffer_pool_client, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_vid_buffer_pool_client_start(struct mpipe_buffer_pool *pool)
{
	struct mpipe_vid_buffer_pool_client *zbpc = (struct mpipe_vid_buffer_pool_client *)pool;

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		struct video_buffer *vbuf = video_buffer_aligned_alloc(
			pool->config.size, pool->config.align, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return -ENOBUFS;
		}

		k_fifo_put(&zbpc->fifo, vbuf);
	}

	LOG_INF("Started buffer pool");

	return 0;
}

static int mpipe_vid_buffer_pool_client_stop(struct mpipe_buffer_pool *pool)
{
	ARG_UNUSED(pool);

	return 0;
}

static int mpipe_vid_buffer_pool_client_acquire_buffer(struct mpipe_buffer_pool *pool,
						       struct net_buf **buf)
{
	struct mpipe_vid_buffer_pool_client *zbpc = (struct mpipe_vid_buffer_pool_client *)pool;
	struct video_buffer *vbuf;
	struct net_buf *nb;
	struct mpipe_buffer_meta *m;

	__ASSERT_NO_MSG(buf != NULL);

	vbuf = k_fifo_get(&zbpc->fifo, K_FOREVER);
	if (vbuf == NULL) {
		return -ENOBUFS;
	}

	/*
	 * Wrap the video buffer payload into a net_buf from mpipe's pool. The
	 * net_buf is freed on its last unref and the payload comes back here.
	 */
	nb = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (nb == NULL) {
		k_fifo_put(&zbpc->fifo, vbuf);
		return -ENOBUFS;
	}

	m = mpipe_buffer_get_meta(nb);
	m->pool = pool;
	m->driver_buf = vbuf;
	m->bytes_used = vbuf->bytesused;
	m->timestamp = vbuf->timestamp;
	nb->len = m->bytes_used;

	*buf = nb;

	return 0;
}

static int mpipe_vid_buffer_pool_client_release_buffer(struct mpipe_buffer_pool *pool,
						       struct net_buf *buf)
{
	struct mpipe_vid_buffer_pool_client *zbpc = (struct mpipe_vid_buffer_pool_client *)pool;
	struct mpipe_buffer_meta *m;
	struct video_buffer *vbuf;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	/* The net_buf is about to be freed: only the video buffer is recycled */
	m = mpipe_buffer_get_meta(buf);
	vbuf = (m != NULL) ? (struct video_buffer *)m->driver_buf : NULL;
	if (vbuf == NULL) {
		return -EINVAL;
	}

	vbuf->bytesused = m->bytes_used;
	vbuf->timestamp = m->timestamp;

	k_fifo_put(&zbpc->fifo, vbuf);

	return 0;
}

void mpipe_vid_buffer_pool_client_init(struct mpipe_buffer_pool *pool)
{
	__ASSERT_NO_MSG(pool != NULL);

	struct mpipe_vid_buffer_pool_client *vbpc = (struct mpipe_vid_buffer_pool_client *)pool;

	k_fifo_init(&vbpc->fifo);

	mpipe_buffer_pool_init(pool);

	pool->start = mpipe_vid_buffer_pool_client_start;
	pool->stop = mpipe_vid_buffer_pool_client_stop;
	pool->acquire_buffer = mpipe_vid_buffer_pool_client_acquire_buffer;
	pool->release_buffer = mpipe_vid_buffer_pool_client_release_buffer;
}
