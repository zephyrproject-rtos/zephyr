/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid buffer pool on the client side.
 */

#ifndef __MP_ZVID_BUFFER_POOL_CLIENT_H__
#define __MP_ZVID_BUFFER_POOL_CLIENT_H__

#include <zephyr/kernel.h>

#include <src/core/mp_buffer.h>

#define MP_ZVID_BUFFERPOOL_CLIENT(self) ((struct mp_zvid_buffer_pool_client *)self)

struct mp_zvid_buffer_pool_client {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** FIFO queue for available buffers */
	struct k_fifo fifo;
};

/**
 * @brief Initialize a Zephyr video buffer pool on the client side
 *
 * @param pool Pointer to the @ref struct mp_buffer_pool structure to initialize
 *
 */
void mp_zvid_buffer_pool_client_init(struct mp_buffer_pool *pool);

#endif /* __MP_ZVID_BUFFER_POOL_CLIENT_H__ */
