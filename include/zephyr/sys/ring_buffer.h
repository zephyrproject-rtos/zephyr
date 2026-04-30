/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup ring_buffer_apis Ring Buffer APIs
 * @ingroup datastructure_apis
 *
 * @brief Simple, header-only ring buffer implementation.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#ifdef CONFIG_RING_BUFFER_LARGE
typedef uint32_t ring_buf_idx_t;
typedef uint32_t ring_buf_size_t;
#define RING_BUFFER_MAX_SIZE	(UINT32_MAX)
#define RING_BUFFER_SIZE_ASSERT_MSG "Size too big"
#else
typedef uint16_t ring_buf_idx_t;
typedef uint16_t ring_buf_size_t;
#define RING_BUFFER_MAX_SIZE     (UINT16_MAX)
#define RING_BUFFER_SIZE_ASSERT_MSG \
	"Size too big, please enable CONFIG_RING_BUFFER_LARGE"
#endif
/** @endcond */

/**
 * @brief A structure to represent a ring buffer.
 */
struct ring_buf {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buffer;
	ring_buf_size_t size;
	ring_buf_idx_t read_ptr;
	ring_buf_idx_t write_ptr;
	bool full;
	/** @endcond */
};

/**
 * @brief Statically initialize a ring buffer.
 *
 * @param _buf  Pointer to the backing byte storage.
 * @param _size Storage size, in bytes.
 */
#define RING_BUF_INIT(_buf, _size)	\
{					\
	.buffer = (_buf),		\
	.size = (_size),		\
}

/**
 * @brief Define and initialize a ring buffer for byte data.
 *
 * The ring buffer can be referenced from other modules with:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size8 Size of ring buffer (in bytes).
 */
#define RING_BUF_DECLARE(name, size8)							\
	BUILD_ASSERT((size8) <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);	\
	static uint8_t __noinit _ring_buffer_data_##name[size8];			\
	struct ring_buf name = RING_BUF_INIT(_ring_buffer_data_##name, size8)

/**
 * @brief Return ring buffer capacity.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer capacity (in bytes).
 */
static inline uint32_t ring_buf_capacity_get(const struct ring_buf *rb)
{
	return rb->size;
}

/**
 * @brief Determine if a ring buffer is full.
 *
 * @param rb Address of ring buffer.
 *
 * @return true if the ring buffer is full, or false if not.
 */
static inline bool ring_buf_is_full(const struct ring_buf *rb)
{
	return rb->full;
}

/**
 * @brief Determine size of available data in a ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer data size (in bytes).
 */
static inline uint32_t ring_buf_size_get(const struct ring_buf *rb)
{
	if (unlikely(ring_buf_is_full(rb))) {
		return ring_buf_capacity_get(rb);
	}

	if (rb->read_ptr <= rb->write_ptr) {
		return (uint32_t)(rb->write_ptr - rb->read_ptr);
	}

	return ring_buf_capacity_get(rb) - (rb->read_ptr - rb->write_ptr);
}

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer free space (in bytes).
 */
static inline uint32_t ring_buf_space_get(const struct ring_buf *rb)
{
	return ring_buf_capacity_get(rb) - ring_buf_size_get(rb);
}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param rb Address of ring buffer.
 *
 * @return true if the ring buffer is empty, or false if not.
 */
static inline bool ring_buf_is_empty(const struct ring_buf *rb)
{
	return ring_buf_size_get(rb) == 0;
}

/**
 * @brief Reset ring buffer state.
 *
 * @param rb Address of ring buffer.
 */
static inline void ring_buf_reset(struct ring_buf *rb)
{
	rb->read_ptr = 0;
	rb->write_ptr = 0;
	rb->full = false;
}

/**
 * @brief Initialize a ring buffer for byte data.
 *
 * Used for ring buffers not defined using @ref RING_BUF_DECLARE.
 *
 * @param rb   Address of ring buffer.
 * @param size Ring buffer size (in bytes).
 * @param data Ring buffer data area (uint8_t data[size]).
 */
static inline void ring_buf_init(struct ring_buf *rb, uint32_t size, uint8_t *data)
{
	__ASSERT(size <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);

	rb->size = (ring_buf_size_t)size;
	rb->buffer = data;
	ring_buf_reset(rb);
}

/**
 * @brief Get address of region for writing data to a ring buffer.
 *
 * Memory copying can be reduced since the internal ring buffer storage can be
 * used directly by the user. Once data is written to the allotted area, the
 * number of bytes written must be confirmed via @ref ring_buf_commit.
 *
 * @param[in]  rb   Address of ring buffer.
 * @param[out] data Set to the start of the writable region within the buffer.
 *
 * @return Number of bytes available for writing. May be smaller than the
 *         total free space if the free region wraps around the end of the
 *         buffer.
 */
static inline uint32_t ring_buf_put_ptr(struct ring_buf *rb, uint8_t **data)
{
	*data = &rb->buffer[rb->write_ptr];

	if (unlikely(rb->full)) {
		return 0;
	}

	if (rb->write_ptr >= rb->read_ptr) {
		return (uint32_t)rb->size - rb->write_ptr;
	}

	return rb->read_ptr - rb->write_ptr;
}

/**
 * @brief Indicate number of bytes written to a ring buffer.
 *
 * The size must be less than or equal to the value returned by the most
 * recent @ref ring_buf_put_ptr.
 *
 * @param rb   Address of ring buffer.
 * @param size Number of bytes that have been written.
 */
static inline void ring_buf_commit(struct ring_buf *rb, size_t size)
{
	if (unlikely(size == 0U)) {
		return;
	}

	rb->write_ptr += (ring_buf_idx_t)size;
	if (rb->write_ptr >= rb->size) {
		rb->write_ptr -= rb->size;
	}
	rb->full = (rb->write_ptr == rb->read_ptr);
}

/**
 * @brief Get address of valid data within a ring buffer.
 *
 * Memory copying can be reduced since the internal ring buffer storage can be
 * used directly by the user. Once data is processed it must be released via
 * @ref ring_buf_consume.
 *
 * @param[in]  rb   Address of ring buffer.
 * @param[out] data Set to the start of the readable region within the buffer.
 *
 * @return Number of bytes available for reading. May be smaller than the
 *         total amount of valid data if the data wraps around the end of the
 *         buffer.
 */
static inline uint32_t ring_buf_get_ptr(struct ring_buf *rb, uint8_t **data)
{
	*data = &rb->buffer[rb->read_ptr];

	if (unlikely(rb->full)) {
		return (uint32_t)rb->size - rb->read_ptr;
	}

	if (rb->write_ptr >= rb->read_ptr) {
		return rb->write_ptr - rb->read_ptr;
	}

	return (uint32_t)rb->size - rb->read_ptr;
}

/**
 * @brief Indicate number of bytes consumed from a ring buffer.
 *
 * The size must be less than or equal to the value returned by the most
 * recent @ref ring_buf_get_ptr.
 *
 * @param rb   Address of ring buffer.
 * @param size Number of bytes that have been consumed.
 */
static inline void ring_buf_consume(struct ring_buf *rb, size_t size)
{
	if (unlikely(size == 0U)) {
		return;
	}

	rb->full = false;
	rb->read_ptr += (ring_buf_idx_t)size;
	if (rb->read_ptr >= rb->size) {
		rb->read_ptr -= rb->size;
	}
}

/**
 * @brief Write (copy) data to a ring buffer.
 *
 * @param rb   Address of ring buffer.
 * @param data Source data.
 * @param size Data size (in bytes).
 *
 * @return Number of bytes written.
 */
static inline uint32_t ring_buf_put(struct ring_buf *rb, const uint8_t *data, uint32_t size)
{
	uint8_t *dst;
	uint32_t total = 0;
	uint32_t chunk;

	do {
		chunk = MIN(ring_buf_put_ptr(rb, &dst), size - total);
		if (chunk == 0U) {
			break;
		}
		memcpy(dst, &data[total], chunk);
		ring_buf_commit(rb, chunk);
		total += chunk;
	} while (total < size);

	return total;
}

/**
 * @brief Read data from a ring buffer.
 *
 * @param rb   Address of ring buffer.
 * @param data Destination buffer. May be NULL to discard the data.
 * @param size Maximum number of bytes to read.
 *
 * @return Number of bytes read.
 */
static inline uint32_t ring_buf_get(struct ring_buf *rb, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t total = 0;
	uint32_t chunk;

	/* should really be removed. */
	if (unlikely(data == NULL)) {
		total = MIN(size, ring_buf_size_get(rb));
		ring_buf_consume(rb, total);
		return total;
	}

	do {
		chunk = MIN(ring_buf_get_ptr(rb, &src), size - total);
		if (chunk == 0U) {
			break;
		}
		memcpy(&data[total], src, chunk);
		ring_buf_consume(rb, chunk);
		total += chunk;
	} while (total < size);

	return total;
}

/**
 * @brief Peek at data from a ring buffer without consuming it.
 *
 * Multiple peek operations return the same data; use @ref ring_buf_get or
 * @ref ring_buf_consume to actually advance the read pointer.
 *
 * @param rb   Address of ring buffer.
 * @param data Destination buffer. Must not be NULL.
 * @param size Maximum number of bytes to peek.
 *
 * @return Number of bytes copied into @p data.
 */
static inline uint32_t ring_buf_peek(struct ring_buf *rb, uint8_t *data, uint32_t size)
{
	ring_buf_idx_t saved_read = rb->read_ptr;
	bool saved_full = rb->full;
	uint32_t total;

	total = ring_buf_get(rb, data, size);
	rb->read_ptr = saved_read;
	rb->full = saved_full;

	return total;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
