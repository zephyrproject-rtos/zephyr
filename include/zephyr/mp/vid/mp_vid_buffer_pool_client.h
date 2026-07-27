/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Client-side video buffer pool for multi-core pipelines.
 *
 * Provides a lightweight buffer pool on the client (application) core
 * that pairs with a server-side pool over an RPC transport.
 */

#ifndef ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_CLIENT_H_
#define ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_CLIENT_H_

/**
 * @defgroup mp_vid_buffer_pool_clients Buffer Pool Clients
 * @ingroup mp_vid
 * @brief Client-side video buffer pool helpers.
 * @{
 */

#include <zephyr/kernel.h>

#include <zephyr/mp/mp_buffer.h>

/**
 * @brief Client-side video buffer pool structure.
 *
 * Extends @ref mp_buffer_pool with a FIFO for buffer recycling on
 * the client core of a multi-core pipeline.
 */
struct mp_vid_buffer_pool_client {
	/** Base buffer pool structure */
	struct mp_buffer_pool pool;
	/** FIFO queue for available buffers */
	struct k_fifo fifo;
};

/**
 * @brief Initialize a client-side video buffer pool.
 *
 * @param pool Pointer to the @ref mp_buffer_pool to initialize.
 */
void mp_vid_buffer_pool_client_init(struct mp_buffer_pool *pool);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_VID_MP_VID_BUFFER_POOL_CLIENT_H_ */
