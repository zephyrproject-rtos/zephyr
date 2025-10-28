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

#define MP_ZVID_BUFFERPOOL(self) ((MpZvidBufferPool *)self)

typedef struct _MpZvidObject MpZvidObject;

/**
 * @brief Video Buffer Pool structure
 *
 * This structure represents a specialized buffer pool for video operations.
 * It extends the generic @ref MpBufferPool with video-specific functionality.
 *
 * The video buffer pool manages video buffers handling buffer allocation,
 * queuing, and dequeuing buffers through the Zephyr video subsystem.
 */
typedef struct {
	/** Base buffer pool structure */
	MpBufferPool pool;
	/** Associated video object */
	MpZvidObject *zvid_obj;
} MpZvidBufferPool;

/**
 * @brief Initialize a Zephyr video buffer pool
 *
 * @param pool Pointer to the @ref MpBufferPool structure to initialize
 * @param obj Pointer to the @ref MpZvidObject to associate with this pool
 *
 */
void mp_zvid_buffer_pool_init(MpBufferPool *pool, MpZvidObject *obj);

#endif /* __MP_ZVID_BUFFER_POOL_H__ */
