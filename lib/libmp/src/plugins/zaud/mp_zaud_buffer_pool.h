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

#include <zephyr/device.h>

#include <src/core/mp_buffer.h>

#include "mp_zaud.h"

/** @brief Cast object pointer to mp_zaud_buffer_pool pointer */
#define MP_ZAUD_BUFFER_POOL(self) ((struct mp_zaud_buffer_pool *)self)

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
	/** Pointer to buffer memory */
	void *buffer;
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

#endif /* __MP_ZAUD_BUFFER_POOL_H__ */
