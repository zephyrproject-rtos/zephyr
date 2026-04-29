/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_buffer.
 */

#ifndef __MP_BUFFER_H__
#define __MP_BUFFER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mp_object.h"
#include "mp_structure.h"

#define MP_BUFFER(obj)       ((struct mp_buffer *)(obj))
#define MP_BUFFER_POOL(pool) ((struct mp_buffer_pool *)pool)

/**
 * @brief Buffer structure
 *
 * Represents a buffer of data that can be passed between elements
 * in the pipeline. Buffers are managed by buffer pools and use
 * reference counting for memory management.
 */
struct mp_buffer {
	/** Base object that the buffer is based on */
	struct mp_object object;
	/** The pool this buffer belongs to */
	struct mp_buffer_pool *pool;
	/** Pointer to the buffer data */
	void *data;
	/** Index of this buffer in the pool */
	uint8_t index;
	/** Number of bytes of valid data in the buffer */
	uint32_t bytes_used;
	/** Timestamp in milliseconds when the last byte of data was received/consumed */
	uint32_t timestamp;
	/** Line offset from the beginning of the frame this buffer represents (in horizontal
	 * lines). This is useful for media devices that produce or consume buffers that
	 * constitute a partial frame.
	 */
	uint16_t line_offset;
};

struct mp_buffer_pool_config {
	/** Minimum number of buffers in the pool */
	uint8_t min_buffers;
	/** Maximum number of buffers in the pool */
	uint8_t max_buffers;
	/** Memory alignment requirement in bytes */
	uint16_t align;
	/** Size of each buffer in bytes */
	size_t size;
};

/**
 * @brief Buffer pool structure
 *
 * Manages a pool of buffers for efficient memory allocation and reuse.
 * Provides methods for buffer lifecycle management.
 */
struct mp_buffer_pool {
	/** Base object */
	struct mp_object object;
	/** Array of buffers managed by the pool */
	struct mp_buffer *buffers;
	/** Pool configuration parameters */
	struct mp_buffer_pool_config config;

	/* Pool operations */
	/** Configure the pool with given parameters */
	bool (*configure)(struct mp_buffer_pool *pool, struct mp_structure *config);
	/** Start the pool and allocate resources */
	bool (*start)(struct mp_buffer_pool *pool);
	/** Stop the pool and free resources */
	bool (*stop)(struct mp_buffer_pool *pool);
	/** Acquire a buffer from the pool */
	bool (*acquire_buffer)(struct mp_buffer_pool *pool, struct mp_buffer **buffer);
	/** Release a buffer back to the pool */
	void (*release_buffer)(struct mp_buffer_pool *pool, struct mp_buffer *buffer);
};

/**
 * @brief Initialize a buffer pool
 *
 * Sets up the buffer pool with default function pointers for pool operations.
 *
 * @param pool Pointer to the buffer pool to initialize
 */
void mp_buffer_pool_init(struct mp_buffer_pool *pool);

/**
 * @brief Release a buffer object
 *
 * Releases a buffer back to its pool. This is typically called when
 * the buffer reference count reaches zero.
 *
 * @param obj Buffer object to release (must be an @ref mp_buffer)
 */
void mp_buffer_release(struct mp_object *obj);

/**
 * @brief Increment buffer reference count
 *
 * Increases the reference count of the buffer to prevent it from being
 * released while still in use.
 *
 * @param buffer Buffer to reference
 * @return Pointer to the referenced buffer
 */
static inline struct mp_buffer *mp_buffer_ref(struct mp_buffer *buffer)
{
	return (struct mp_buffer *)mp_object_ref(&buffer->object);
}

/**
 * @brief Decrement buffer reference count
 *
 * Decreases the reference count of the buffer. When the count reaches
 * zero, the buffer will be automatically released back to its pool.
 *
 * @param buffer Buffer to unreference
 */
static inline void mp_buffer_unref(struct mp_buffer *buffer)
{
	mp_object_unref(&buffer->object);
}

#endif /* __MP_BUFFER_H__ */
