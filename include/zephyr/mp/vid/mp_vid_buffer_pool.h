/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video buffer pool backed by the Zephyr video subsystem.
 *
 * Manages video buffer allocation and queuing through a Zephyr video
 * device, used internally by @ref mp_vid_object.
 */

#ifndef ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_H_
#define ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_H_

/**
 * @defgroup mp_vid vid
 * @ingroup mp_plugins
 * @brief Video device-backed elements, transforms, and helper APIs.
 */

/**
 * @defgroup mp_vid_buffer_pools Buffer Pools
 * @ingroup mp_vid
 * @brief Video buffer pools backed by the Zephyr video subsystem.
 * @{
 */

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <zephyr/mp/mp_buffer.h>

struct mp_vid_object;

/**
 * @brief Video buffer pool structure.
 *
 * Extends @ref mp_buffer_pool with video-specific buffer management,
 * including allocation through the Zephyr video subsystem and a free
 * FIFO for input-side buffer recycling.
 */
struct mp_vid_buffer_pool {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** Associated video object */
	struct mp_vid_object *vid_obj;
	/** Array of video buffer pointers managed by the pool */
	struct video_buffer *vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	/**
	 * Per-buffer in-flight markers, parallel to @ref vbufs. A buffer is
	 * in-flight between acquire_buffer() (handed to the pipeline) and
	 * release_buffer() (returned to its pool).
	 */
	bool in_flight[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	/** Number of video buffers currently in this pool */
	uint8_t vbuf_count;
	/**
	 * Flushing flag. Set at the start of stop() so that a buffer returned
	 * after the stream has stopped is freed instead of re-enqueued into the
	 * (now stopped) video device, and so a late acquire is refused.
	 */
	atomic_t flushing;
	/**
	 * Protects concurrent access to @ref vbufs and @ref in_flight between
	 * stop() (control thread) and release_buffer()/acquire_buffer() (the
	 * pipeline thread, which may still be running during teardown), so a
	 * buffer is freed exactly once.
	 */
	struct k_spinlock lock;
	/**
	 * FIFO queue of available buffers for acquisition by the upstream element.
	 * Only used by the input pool (VIDEO_BUF_TYPE_INPUT).
	 */
	struct k_fifo free_fifo;
};

/**
 * @brief Initialize a video buffer pool.
 *
 * @param pool Pointer to the @ref mp_buffer_pool to initialize.
 * @param obj  Pointer to the @ref mp_vid_object to associate with this pool.
 */
void mp_vid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_vid_object *obj);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_H_ */
