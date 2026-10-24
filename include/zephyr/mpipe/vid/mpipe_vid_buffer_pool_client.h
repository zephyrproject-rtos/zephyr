/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Client-side video buffer pool for multi-core pipelines.
 * @ingroup mpipe_vid_buffer_pool_clients
 *
 * Provides a lightweight buffer pool on the client (application) core
 * that pairs with a server-side pool over an RPC transport.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_CLIENT_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_CLIENT_H_

/**
 * @defgroup mpipe_vid_buffer_pool_clients Buffer Pool Clients
 * @ingroup mpipe_vid
 * @brief Client-side video buffer pool helpers.
 * @{
 */

#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe_buffer.h>

/**
 * @brief Client-side video buffer pool structure.
 *
 * Extends @ref mpipe_buffer_pool with a FIFO for buffer recycling on
 * the client core of a multi-core pipeline.
 */
struct mpipe_vid_buffer_pool_client {
	/** Base buffer pool structure */
	struct mpipe_buffer_pool pool;
	/** FIFO queue for available buffers */
	struct k_fifo fifo;
};

/**
 * @brief Initialize a client-side video buffer pool.
 *
 * @param pool Pointer to the @ref mpipe_buffer_pool to initialize.
 */
void mpipe_vid_buffer_pool_client_init(struct mpipe_buffer_pool *pool);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_BUFFER_POOL_CLIENT_H_ */
