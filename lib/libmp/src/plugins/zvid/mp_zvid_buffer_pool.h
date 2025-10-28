/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid buffer pool.
 */

#ifndef __MP_ZVID_BUFFER_POOL_H__
#define __MP_ZVID_BUFFER_POOL_H__

#include <zephyr/device.h>

#include <src/core/mp_buffer.h>

#define MP_ZVID_BUFFERPOOL(self) ((struct mp_zvid_buffer_pool *)self)

struct mp_zvid_object;

/**
 * @brief Video Buffer Pool structure
 *
 * This structure represents a specialized buffer pool for video operations.
 * It extends the generic @ref struct mp_buffer_pool with video-specific functionality.
 *
 * The video buffer pool manages video buffers handling buffer allocation,
 * queuing, and dequeuing buffers through the Zephyr video subsystem.
 */
struct mp_zvid_buffer_pool {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** Associated video object */
	struct mp_zvid_object *zvid_obj;
};

/**
 * @brief Initialize a Zephyr video buffer pool
 *
 * @param pool Pointer to the @ref struct mp_buffer_pool structure to initialize
 * @param obj Pointer to the @ref struct mp_zvid_object to associate with this pool
 *
 */
void mp_zvid_buffer_pool_init(struct mp_buffer_pool *pool, struct mp_zvid_object *obj);

#endif /* __MP_ZVID_BUFFER_POOL_H__ */
