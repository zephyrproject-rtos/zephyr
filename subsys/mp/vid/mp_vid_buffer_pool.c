/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/vid/mp_vid_buffer_pool.h>
#include <zephyr/mp/vid/mp_vid_object.h>

LOG_MODULE_REGISTER(mp_vid_buffer_pool, CONFIG_MP_LOG_LEVEL);

/*
 * Find the slot index of a driver buffer in the pool's vbufs[] array.
 * The whole array is scanned (not just vbuf_count) so a buffer returned late
 * during teardown - after vbuf_count has been reset - is still matched.
 *
 * Returns the index, or -1 if the buffer is not tracked by this pool.
 */
static int mp_vid_buffer_pool_find(struct mp_vid_buffer_pool *vid_pool,
				    struct video_buffer *vbuf)
{
	for (uint8_t i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		if (vid_pool->vbufs[i] == vbuf) {
			return i;
		}
	}

	return -1;
}

static int mp_vid_buffer_pool_start(struct mp_buffer_pool *pool)
{
	int ret = 0;
	struct mp_vid_buffer_pool *vid_pool = (struct mp_vid_buffer_pool *)pool;

	if (pool->config.min_buffers > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		LOG_ERR("min_buffers=%u exceeds CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=%u",
			pool->config.min_buffers, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
		return -EINVAL;
	}

	/* Start fresh: not flushing, no buffers tracked or in-flight yet. */
	atomic_set(&vid_pool->flushing, 0);
	for (uint8_t i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		vid_pool->vbufs[i] = NULL;
		vid_pool->in_flight[i] = false;
	}

	vid_pool->vbuf_count = pool->config.min_buffers;

	for (uint8_t i = 0; i < vid_pool->vbuf_count; i++) {
		struct video_buffer *vbuf = video_buffer_aligned_alloc(
			pool->config.size, pool->config.align, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Failed to alloc video buffer %u", i);
			return -ENOBUFS;
		}

		vid_pool->vbufs[i] = vbuf;

		if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_INPUT) {
			k_fifo_put(&vid_pool->free_fifo, vbuf);
			continue;
		}

		vbuf->type = vid_pool->vid_obj->type;
		ret = video_enqueue(vid_pool->vid_obj->vdev, vbuf);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue video buffer %u", i);
			(void)video_buffer_release(vbuf);
			return ret;
		}
	}

	if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_OUTPUT) {
		ret = video_stream_start(vid_pool->vid_obj->vdev, vid_pool->vid_obj->type);
		if (ret != 0) {
			LOG_ERR("Failed to start video streaming");
			return ret;
		}
	}

	LOG_INF("Started video %s buffer pool",
		vid_pool->vid_obj->type == VIDEO_BUF_TYPE_OUTPUT ? "output" : "input");

	return ret;
}

static int mp_vid_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	int ret = 0;
	struct mp_vid_buffer_pool *vid_pool = (struct mp_vid_buffer_pool *)pool;
	struct video_buffer *to_free[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	uint8_t free_count = 0;
	k_spinlock_key_t key;

	if (vid_pool == NULL || vid_pool->vid_obj == NULL || vid_pool->vid_obj->vdev == NULL) {
		return -EINVAL;
	}

	/*
	 * Enter flushing before anything else. From now on a buffer returned by
	 * release_buffer() is freed instead of re-enqueued into the (about to be
	 * stopped) video device, and a late acquire is refused.
	 */
	atomic_set(&vid_pool->flushing, 1);

	if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_OUTPUT) {
		/*
		 * video_stream_stop() also flushes the device (cancels pending
		 * buffers), which unblocks a pipeline thread waiting in
		 * video_dequeue(). Log on failure but still free our buffers.
		 */
		ret = video_stream_stop(vid_pool->vid_obj->vdev, vid_pool->vid_obj->type);
		if (ret != 0) {
			LOG_ERR("Failed to stop video streaming");
		}
	} else {
		/*
		 * The INPUT acquire path parks in k_fifo_get(K_FOREVER). Unlike
		 * the OUTPUT path there is no device-side flush to wake it, so
		 * cancel the wait here. flushing is already set above, so the
		 * woken thread observes it and bails out of acquire instead of
		 * consuming a buffer.
		 */
		k_fifo_cancel_wait(&vid_pool->free_fifo);
	}

	/*
	 * Free only the buffers the pool still owns. Buffers currently in-flight
	 * (handed to the pipeline and not yet returned) are left alone: each is
	 * freed by release_buffer() when the pipeline returns it, under the
	 * flushing flag set above. The array/flags are read under the spinlock
	 * to stay consistent with a concurrent release_buffer()/acquire_buffer();
	 * the actual free is done after unlocking.
	 */
	key = k_spin_lock(&vid_pool->lock);
	for (uint8_t i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		if (vid_pool->vbufs[i] != NULL && !vid_pool->in_flight[i]) {
			to_free[free_count++] = vid_pool->vbufs[i];
			vid_pool->vbufs[i] = NULL;
		}
	}
	vid_pool->vbuf_count = 0;
	k_spin_unlock(&vid_pool->lock, key);

	for (uint8_t i = 0; i < free_count; i++) {
		int rel = video_buffer_release(to_free[i]);

		if (rel != 0) {
			LOG_ERR("Failed to release video buffer");
			if (ret == 0) {
				ret = rel;
			}
		}
	}

	return ret;
}

static int mp_vid_buffer_pool_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct mp_vid_buffer_pool *vid_pool = (struct mp_vid_buffer_pool *)pool;
	struct video_buffer *vbuf = &(struct video_buffer){0};
	struct mp_buffer_meta *bm;
	k_spinlock_key_t key;
	int idx;
	int ret = 0;

	/* Refuse to hand out a buffer while flushing (teardown in progress). */
	if (atomic_get(&vid_pool->flushing)) {
		return -EPIPE;
	}

	if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_INPUT) {
		vbuf = k_fifo_get(&vid_pool->free_fifo, K_FOREVER);
		if (vbuf == NULL) {
			/*
			 * Woken by k_fifo_cancel_wait() in stop(): teardown is in
			 * progress, so bail out rather than treating this as an
			 * allocation error.
			 */
			return -EPIPE;
		}
	} else {
		vbuf->type = vid_pool->vid_obj->type;
		ret = video_dequeue(vid_pool->vid_obj->vdev, &vbuf, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to dequeue a video buffer");
			return ret;
		}
	}

	/*
	 * The blocking get above (video_dequeue()/k_fifo_get() with K_FOREVER)
	 * can be unblocked by stop(): video_stream_stop() flushes the device and
	 * wakes a pending video_dequeue(). Re-check the flushing flag and claim
	 * the buffer as in-flight atomically under the lock, BEFORE dereferencing
	 * vbuf. If flushing has begun, leave the buffer untouched: it is still
	 * tracked in vbufs[] with in_flight == false, so stop() owns it and frees
	 * it exactly once. This closes the window where stop() could free the
	 * just-dequeued buffer while we are about to read vbuf->buffer.
	 */
	key = k_spin_lock(&vid_pool->lock);
	if (atomic_get(&vid_pool->flushing)) {
		k_spin_unlock(&vid_pool->lock, key);
		return -EPIPE;
	}
	idx = mp_vid_buffer_pool_find(vid_pool, vbuf);
	if (idx >= 0) {
		vid_pool->in_flight[idx] = true;
	}
	k_spin_unlock(&vid_pool->lock, key);

	*buf = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (*buf == NULL) {
		LOG_ERR("Failed to allocate a net_buf wrapper for the video buffer");

		/* Undo the in-flight marker before returning the buffer. */
		key = k_spin_lock(&vid_pool->lock);
		idx = mp_vid_buffer_pool_find(vid_pool, vbuf);
		if (idx >= 0) {
			vid_pool->in_flight[idx] = false;
		}
		k_spin_unlock(&vid_pool->lock, key);

		/* Re-enqueue the video buffer */
		if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_INPUT) {
			k_fifo_put(&vid_pool->free_fifo, vbuf);
		} else {
			(void)video_enqueue(vid_pool->vid_obj->vdev, vbuf);
		}

		return -ENOBUFS;
	}

	/* Set buffer metadata */
	bm = mp_buffer_get_meta(*buf);
	bm->pool = pool;
	bm->driver_buf = vbuf;
	bm->bytes_used = vbuf->bytesused;
	bm->timestamp = vbuf->timestamp;

	return ret;
}

static int mp_vid_buffer_pool_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	struct mp_vid_buffer_pool *vid_pool = (struct mp_vid_buffer_pool *)pool;
	struct video_buffer *vbuf = mp_buffer_get_meta(buf)->driver_buf;
	k_spinlock_key_t key;
	bool flushing;
	int idx;
	int ret = 0;

	if (vbuf == NULL) {
		return -EINVAL;
	}

	/*
	 * Clear the in-flight marker and read the flushing state atomically with
	 * respect to stop(). If the pool is flushing, claim ownership of the slot
	 * (NULL it out) so stop() will not also free this buffer.
	 */
	key = k_spin_lock(&vid_pool->lock);
	idx = mp_vid_buffer_pool_find(vid_pool, vbuf);
	if (idx >= 0) {
		vid_pool->in_flight[idx] = false;
	}
	flushing = (atomic_get(&vid_pool->flushing) != 0);
	if (flushing && idx >= 0) {
		vid_pool->vbufs[idx] = NULL;
	}
	k_spin_unlock(&vid_pool->lock, key);

	/*
	 * Flushing: the pool has stopped, so this is the last reference to the
	 * buffer coming home. Free it here instead of re-enqueueing it into a
	 * stopped device.
	 */
	if (flushing) {
		ret = video_buffer_release(vbuf);
		if (ret != 0) {
			LOG_ERR("Failed to release the video buffer");
		}

		return ret;
	}

	if (vid_pool->vid_obj->type == VIDEO_BUF_TYPE_INPUT) {
		k_fifo_put(&vid_pool->free_fifo, vbuf);
		return 0;
	}

	vbuf->type = vid_pool->vid_obj->type;
	ret = video_enqueue(vid_pool->vid_obj->vdev, vbuf);
	if (ret != 0) {
		LOG_ERR("Failed to re-enqueue the video buffer");
	}

	return ret;
}

void mp_vid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_vid_object *obj)
{
	struct mp_vid_buffer_pool *vid_pool = (struct mp_vid_buffer_pool *)pool;

	k_fifo_init(&vid_pool->free_fifo);
	vid_pool->vid_obj = obj;
	atomic_set(&vid_pool->flushing, 0);

	mp_buffer_pool_init(pool);

	pool->start = mp_vid_buffer_pool_start;
	pool->stop = mp_vid_buffer_pool_stop;
	pool->acquire_buffer = mp_vid_buffer_pool_acquire_buffer;
	pool->release_buffer = mp_vid_buffer_pool_release_buffer;
}
