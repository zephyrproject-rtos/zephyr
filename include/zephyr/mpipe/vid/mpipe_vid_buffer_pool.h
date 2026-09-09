/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video buffer pool backed by the Zephyr video subsystem.
 * @ingroup mpipe_vid_buffer_pools
 *
 * Manages video buffer allocation and queuing through a Zephyr video
 * device, used internally by @ref mpipe_vid_object.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_H_

/**
 * @defgroup mpipe_vid Video
 * @ingroup mpipe_plugins
 * @brief Elements backed by Zephyr video devices, and the software fallbacks.
 *
 * The video plugin sits on Zephyr's video API. It covers capture from a camera,
 * memory-to-memory transforms performed by hardware, and a software converter
 * for the pixel-format changes no hardware on the board can do.
 *
 * Video is where zero-copy matters most, so the pools here hand out the
 * driver's own buffers rather than copies of them, and the shared video object
 * translates between what a driver reports and what a capability says. The two
 * spellings do not match exactly - the video API states a single supported size
 * as a degenerate range, mpipe as a fixed value - and that translation lives in
 * one place so the round trip stays exact.
 */

/**
 * @defgroup mpipe_vid_buffer_pools Buffer Pools
 * @ingroup mpipe_vid
 * @brief Video buffer pools backed by the Zephyr video subsystem.
 * @{
 */

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe_buffer.h>

struct mpipe_vid_object;

/**
 * @brief Video buffer pool structure.
 *
 * Extends @ref mpipe_buffer_pool with video-specific buffer management,
 * including allocation through the Zephyr video subsystem and a free
 * FIFO for input-side buffer recycling.
 */
struct mpipe_vid_buffer_pool {
	/** Base buffer pool structure */
	struct mpipe_buffer_pool pool;
	/** Associated video object */
	struct mpipe_vid_object *vid_obj;
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
 * @param pool Pointer to the @ref mpipe_buffer_pool to initialize.
 * @param obj  Pointer to the @ref mpipe_vid_object to associate with this pool.
 */
void mpipe_vid_buffer_pool_init(struct mpipe_buffer_pool *pool, struct mpipe_vid_object *obj);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_H_ */
