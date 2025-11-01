/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpBuffer.
 */

#ifndef __MP_BUFFER_H__
#define __MP_BUFFER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mp_object.h"
#include "mp_structure.h"

#define MP_BUFFER(obj)      ((MpBuffer *)(obj))
#define MP_BUFFERPOOL(pool) ((MpBufferPool *)pool)

typedef struct _MpBufferPool MpBufferPool;

/**
 * @brief Buffer structure
 *
 * Represents a buffer of data that can be passed between elements
 * in the pipeline. Buffers are managed by buffer pools and use
 * reference counting for memory management.
 */
typedef struct {
	/** Base object that the buffer is based on */
	MpObject object;
	/** The pool this buffer belongs to */
	MpBufferPool *pool;
	/** Pointer to the buffer data */
	void *data;
	/** Index of this buffer in the pool */
	uint8_t index;
	/** Total size of the buffer in bytes */
	size_t size;
	/** Number of bytes of valid data in the buffer */
	uint32_t bytesused;
	/** Timestamp in milliseconds when the last byte of data was received/consumed */
	uint32_t timestamp;
	/** Line offset from the beginning of the frame this buffer represents (in horizontal
	 * lines). This is useful for media devices that produce or consume buffers that
	 * constitute a partial frame.
	 */
	uint16_t line_offset;
} MpBuffer;

typedef struct {
	/** Minimum number of buffers in the pool */
	uint8_t min_buffers;
	/** Maximum number of buffers in the pool */
	uint8_t max_buffers;
	/** Memory alignment requirement in bytes */
	uint16_t align;
	/** Size of each buffer in bytes */
	size_t size;
} MpBufferPoolConfig;

/**
 * @brief Buffer pool structure
 *
 * Manages a pool of buffers for efficient memory allocation and reuse.
 * Provides methods for buffer lifecycle management.
 */
struct _MpBufferPool {
	/** Base object */
	MpObject object;
	/** Array of buffers managed by the pool */
	MpBuffer *buffers;
	/** Pool configuration parameters */
	MpBufferPoolConfig config;

	/* Pool operations */
	/** Configure the pool with given parameters */
	bool (*configure)(MpBufferPool *pool, MpStructure *config);
	/** Start the pool and allocate resources */
	bool (*start)(MpBufferPool *pool);
	/** Stop the pool and free resources */
	bool (*stop)(MpBufferPool *pool);
	/** Acquire a buffer from the pool */
	bool (*acquire_buffer)(MpBufferPool *pool, MpBuffer **buffer);
	/** Release a buffer back to the pool */
	void (*release_buffer)(MpBufferPool *pool, MpBuffer *buffer);
};

/**
 * @brief Initialize a buffer pool
 *
 * Sets up the buffer pool with default function pointers for pool operations.
 *
 * @param pool Pointer to the buffer pool to initialize
 */
void mp_buffer_pool_init(MpBufferPool *pool);

/**
 * @brief Release a buffer object
 *
 * Releases a buffer back to its pool. This is typically called when
 * the buffer reference count reaches zero.
 *
 * @param obj Buffer object to release (must be an @ref MpBuffer)
 */
void mp_buffer_release(MpObject *obj);

/**
 * @brief Increment buffer reference count
 *
 * Increases the reference count of the buffer to prevent it from being
 * released while still in use.
 *
 * @param buffer Buffer to reference
 * @return Pointer to the referenced buffer
 */
static inline MpBuffer *mp_buffer_ref(MpBuffer *buffer)
{
	return (MpBuffer *)mp_object_ref(&buffer->object);
}

/**
 * @brief Decrement buffer reference count
 *
 * Decreases the reference count of the buffer. When the count reaches
 * zero, the buffer will be automatically released back to its pool.
 *
 * @param buffer Buffer to unreference
 */
static inline void mp_buffer_unref(MpBuffer *buffer)
{
	mp_object_unref(&buffer->object);
}

#endif /* __MP_BUFFER_H__ */
