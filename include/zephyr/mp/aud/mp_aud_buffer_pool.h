/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio buffer pool header file.
 */

#ifndef __MP_AUD_BUFFER_POOL_H__
#define __MP_AUD_BUFFER_POOL_H__

/**
 * @defgroup mp_aud_buffer_pools Buffer Pools
 * @ingroup mp_aud
 * @brief Audio buffer pool helpers.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/aud/mp_aud.h>

/**
 * @struct mp_aud_buffer_pool
 * @brief Audio buffer pool structure
 *
 * This structure manages memory allocation and buffer handling for audio
 * data processing within the plugin.
 */
struct mp_aud_buffer_pool {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** Pointer to the associated audio device */
	const struct device *aud_dev;
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
void mp_aud_buffer_pool_init(struct mp_buffer_pool *pool);

/** @} */

#endif /* __MP_AUD_BUFFER_POOL_H__ */
