/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio buffer pool header file.
 */

#ifndef __MP_ZAUD_BUFFER_POOL_H__
#define __MP_ZAUD_BUFFER_POOL_H__

/**
 * @defgroup mp_zaud_buffer_pools Buffer Pools
 * @ingroup mp_zaud
 * @brief Audio buffer pool helpers.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/zaud/mp_zaud.h>

/**
 * @struct mp_zaud_buffer_pool
 * @brief Audio buffer pool structure
 *
 * This structure manages memory allocation and buffer handling for audio
 * data processing within the plugin.
 */
struct mp_zaud_buffer_pool {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** Pointer to the associated audio device */
	const struct device *zaud_dev;
	/** Memory slab for efficient buffer allocation */
	struct k_mem_slab *mem_slab;
	/** Per-chunk pointers into the mem_slab backing buffer */
	void **blocks;
};

/**
 * @brief Initialize an audio buffer pool
 *
 * This function initializes the audio buffer pool with default
 * values and sets up the function pointers.
 *
 * @param pool Pointer to the buffer pool structure to initialize.
 */
void mp_zaud_buffer_pool_init(struct mp_buffer_pool *pool);

/** @} */

#endif /* __MP_ZAUD_BUFFER_POOL_H__ */
