/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Video buffer pool backed by the Zephyr video subsystem.
 *
 * Manages video buffer allocation and queuing through a Zephyr video
 * device, used internally by @ref mp_zvid_object.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_BUFFER_POOL_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_BUFFER_POOL_H_

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <zephyr/mp/core/mp_buffer.h>

struct mp_zvid_object;

/**
 * @brief Video buffer pool structure.
 *
 * Extends @ref mp_buffer_pool with video-specific buffer management,
 * including allocation through the Zephyr video subsystem and a free
 * FIFO for input-side buffer recycling.
 */
struct mp_zvid_buffer_pool {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** Associated video object */
	struct mp_zvid_object *zvid_obj;
	/** Array of video buffer pointers managed by the pool */
	struct video_buffer *vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	/** Number of video buffers currently in this pool */
	uint8_t vbuf_count;
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
 * @param obj  Pointer to the @ref mp_zvid_object to associate with this pool.
 */
void mp_zvid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_zvid_object *obj);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_BUFFER_POOL_H_ */
