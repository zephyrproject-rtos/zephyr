/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/vid/mp_vid_buffer_pool_client.h>

LOG_MODULE_REGISTER(mp_vid_buffer_pool_client, CONFIG_MP_LOG_LEVEL);

static int mp_vid_buffer_pool_client_start(struct mp_buffer_pool *pool)
{
	struct mp_vid_buffer_pool_client *zbpc = (struct mp_vid_buffer_pool_client *)pool;

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		struct video_buffer *vbuf = video_buffer_aligned_alloc(
			pool->config.size, pool->config.align, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return -ENOBUFS;
		}

		/*
		 * Wrap the externally allocated payload (video_buffer->buffer) into a
		 * net_buf allocated from mp's net_buf pool.
		 */
		struct net_buf *nb =
			net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
		if (nb == NULL) {
			(void)video_buffer_release(vbuf);
			return -ENOBUFS;
		}

		struct mp_buffer_meta *m = mp_buffer_get_meta(nb);

		m->pool = pool;
		m->driver_buf = vbuf;
		m->bytes_used = vbuf->bytesused;
		m->timestamp = vbuf->timestamp;
		nb->len = m->bytes_used;

		k_fifo_put(&zbpc->fifo, nb);
	}

	LOG_INF("Started buffer pool");
	return 0;
}

static int mp_vid_buffer_pool_client_stop(struct mp_buffer_pool *pool)
{
	ARG_UNUSED(pool);
	return 0;
}

static int mp_vid_buffer_pool_client_acquire_buffer(struct mp_buffer_pool *pool,
						     struct net_buf **buf)
{
	struct mp_vid_buffer_pool_client *zbpc = (struct mp_vid_buffer_pool_client *)pool;
	struct net_buf *nb;

	if (buf == NULL) {
		return -EINVAL;
	}

	nb = k_fifo_get(&zbpc->fifo, K_FOREVER);
	if (nb == NULL) {
		return -ENOBUFS;
	}

	*buf = nb;
	return 0;
}

static int mp_vid_buffer_pool_client_release_buffer(struct mp_buffer_pool *pool,
						     struct net_buf *buf)
{
	struct mp_vid_buffer_pool_client *zbpc = (struct mp_vid_buffer_pool_client *)pool;
	struct mp_buffer_meta *m;
	struct video_buffer *vbuf;

	if (pool == NULL || buf == NULL) {
		return -EINVAL;
	}

	m = mp_buffer_get_meta(buf);
	vbuf = m ? (struct video_buffer *)m->driver_buf : NULL;
	if (vbuf != NULL) {
		m->bytes_used = vbuf->bytesused;
		m->timestamp = vbuf->timestamp;
		buf->len = m->bytes_used;
	}

	k_fifo_put(&zbpc->fifo, buf);
	return 0;
}

void mp_vid_buffer_pool_client_init(struct mp_buffer_pool *pool)
{
	struct mp_vid_buffer_pool_client *vbpc = (struct mp_vid_buffer_pool_client *)pool;

	k_fifo_init(&vbpc->fifo);

	mp_buffer_pool_init(pool);

	pool->start = mp_vid_buffer_pool_client_start;
	pool->stop = mp_vid_buffer_pool_client_stop;
	pool->acquire_buffer = mp_vid_buffer_pool_client_acquire_buffer;
	pool->release_buffer = mp_vid_buffer_pool_client_release_buffer;
}
