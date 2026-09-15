/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Buffer and buffer pool APIs.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_BUFFER_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_BUFFER_H_

/**
 * @defgroup mp_buffer Buffers
 * @ingroup mp_core
 * @brief Buffer metadata and buffer-pool APIs.
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net_buf.h>

/**
 * @brief Buffer pool config structure
 */
struct mp_buffer_pool_config {
	/** Size of each buffer in bytes */
	uint32_t size;
	/** Alignment of each buffer in bytes */
	uint16_t align;
	/** Minimum number of buffers for the pool to work */
	uint8_t min_buffers;
	/** Maximum number of buffers in the pool */
	uint8_t max_buffers;
};

struct mp_structure;

/**
 * @brief Buffer pool structure
 */
struct mp_buffer_pool {
	/** Configuration parameters of the buffer pool */
	struct mp_buffer_pool_config config;
	/** net_buf pool associated with the buffer pool */
	struct net_buf_pool *nb_pool;

	/** Configure the pool with the given caps structure */
	int (*configure)(struct mp_buffer_pool *pool, struct mp_structure *config);
	/** Start the pool and allocate resources */
	int (*start)(struct mp_buffer_pool *pool);
	/** Stop the pool and free resources */
	int (*stop)(struct mp_buffer_pool *pool);
	/** Acquire a buffer from the pool */
	int (*acquire_buffer)(struct mp_buffer_pool *pool, struct net_buf **buf);
	/**
	 * Release a buffer back to the pool, automatically called when
	 * buffer refcount reaches 0.
	 */
	int (*release_buffer)(struct mp_buffer_pool *pool, struct net_buf *buf);

	/** Flag indicating if the pool has been started */
	bool started;
};

/**
 * @brief Common buffer metadata stored in net_buf user_data.
 */
struct mp_buffer_meta {
	/** Buffer pool this buffer belongs to. */
	struct mp_buffer_pool *pool;
	/** Valid payload in bytes. */
	uint32_t bytes_used;
	/** Timestamp in milliseconds. */
	uint32_t timestamp;
	/** Pointer to the real driver-own buffer. */
	void *driver_buf;
	/** Opaque pointer for plugin-specific usage. */
	void *priv;
};

/** Get buffer metadata */
static inline struct mp_buffer_meta *mp_buffer_get_meta(struct net_buf *buf)
{
	return (struct mp_buffer_meta *)net_buf_user_data(buf);
}

/**
 * @brief Destroy callback for net_buf.
 *
 * Automatically called when buffer reference count reaches zero.
 *
 * @param buf Pointer to the net_buf to destroy.
 */
void mp_buffer_destroy(struct net_buf *buf);

/**
 * @brief Helper to configure a buffer pool
 *
 * @param pool Pointer to the buffer pool to configure
 * @param config Caps structure to configure the buffer pool
 *
 * @return 0 on success, negative errno on failure
 */
int mp_buffer_pool_configure(struct mp_buffer_pool *pool, struct mp_structure *config);

/**
 * @brief Helper to start a buffer pool
 *
 * @param pool Pointer to the buffer pool to start
 *
 * @return 0 on success, negative errno on failure
 */
int mp_buffer_pool_start(struct mp_buffer_pool *pool);

/**
 * @brief Helper to stop a buffer pool
 *
 * @param pool Pointer to the buffer pool to stop
 *
 * @return 0 on success, negative errno on failure
 */
int mp_buffer_pool_stop(struct mp_buffer_pool *pool);

/**
 * @brief Initialize a buffer pool
 *
 * @param pool Pointer to the buffer pool to initialize
 */
void mp_buffer_pool_init(struct mp_buffer_pool *pool);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_BUFFER_H_ */
