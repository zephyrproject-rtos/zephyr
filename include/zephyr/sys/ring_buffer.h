/*
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
#include <zephyr/sys/barrier.h>
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
#define RING_BUFFER_MAX_SIZE	(UINT32_MAX - 1U)
#define RING_BUFFER_SIZE_ASSERT_MSG "Size too big"
#else
typedef uint16_t ring_buf_idx_t;
typedef uint16_t ring_buf_size_t;
#define RING_BUFFER_MAX_SIZE     (UINT16_MAX - 1U)
#define RING_BUFFER_SIZE_ASSERT_MSG \
	"Size too big, please enable CONFIG_RING_BUFFER_LARGE"
#endif
/** @endcond */

/**
 * @brief Backing-storage size (in bytes) required for a ring buffer of
 *        usable capacity @p capacity.
 *
 * The ring buffer reserves one byte to disambiguate the full and empty
 * states; this macro returns @p capacity + 1.
 *
 * @param capacity User-visible ring buffer capacity (in bytes).
 */
#define RING_BUF_STORAGE_SIZE(capacity) ((capacity) + 1U)

/**
 * @brief A structure to represent a ring buffer.
 *
 * The implementation reserves one slot to disambiguate the empty and full
 * states. The struct stores the backing storage size; the user-visible
 * capacity is implicitly @p size - 1.
 */
struct ring_buf {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buffer;
	ring_buf_size_t size;
	ring_buf_idx_t read_idx;
	ring_buf_idx_t write_idx;
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

static ALWAYS_INLINE ring_buf_idx_t rb_idx_wrap(ring_buf_idx_t v, ring_buf_size_t storage_sz)
{
	return v >= storage_sz ? v - storage_sz : v;
}

/** @endcond */

/**
 * @brief Statically initialize a ring buffer.
 *
 * @p size denotes the size of the backing storage in bytes. The user-visible
 * capacity is @p size - 1 because one slot is reserved internally to
 * disambiguate the empty and full states.
 *
 * @param buf  Pointer to the backing byte storage.
 * @param size Backing storage size, in bytes. The user-visible capacity is
 *             @p size - 1 because one slot is reserved.
 */
#define RING_BUF_INIT(buf, sz)						\
(struct ring_buf) {							\
	.buffer = buf,							\
	.size = (ring_buf_size_t)((sz) ? (sz) : 1U),			\
}

/**
 * @brief Define and initialize a ring buffer for byte data.
 *
 * The user-visible capacity equals the requested @p size8. One additional
 * byte is allocated internally so that the user-visible capacity equals the
 * requested @p size8 (the underlying backing storage is @p size8 + 1 bytes).
 *
 * The ring buffer can be referenced from other modules with:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size8 User-visible ring buffer capacity (in bytes).
 */
#define RING_BUF_DECLARE(name, size8)							\
	BUILD_ASSERT(size8 <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);	\
	static uint8_t __noinit _ring_buffer_data_##name[RING_BUF_STORAGE_SIZE(size8)];	\
	struct ring_buf name =								\
		RING_BUF_INIT(_ring_buffer_data_##name, RING_BUF_STORAGE_SIZE(size8))

/**
 * @brief Return ring buffer capacity.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer capacity (in bytes).
 */
static inline uint32_t ring_buf_capacity_get(const struct ring_buf *rb)
{
	return rb->size - 1U;
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
	ring_buf_idx_t w = rb->write_idx;
	ring_buf_idx_t r = rb->read_idx;

	return w >= r ? w - r : rb->size - (r - w);
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
	return rb->read_idx == rb->write_idx;
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
	return rb_idx_wrap(rb->write_idx + 1U, rb->size) == rb->read_idx;
}

/**
 * @brief Reset ring buffer state.
 *
 * @param rb Address of ring buffer.
 */
static inline void ring_buf_reset(struct ring_buf *rb)
{
	rb->read_idx = 0;
	rb->write_idx = 0;
}

/**
 * @brief Initialize a ring buffer for byte data.
 *
 * Used for ring buffers not defined using @ref RING_BUF_DECLARE. @p size
 * denotes the backing storage size in bytes; the user-visible capacity is
 * @p size - 1 because one slot is reserved to disambiguate the empty and
 * full states.
 *
 * @param rb   Address of ring buffer.
 * @param size Backing storage size (in bytes). A @p size of 0 produces a
 *             degenerate buffer with capacity 0.
 * @param data Ring buffer data area (uint8_t data[size]).
 */
static inline void ring_buf_init(struct ring_buf *rb, uint32_t size, uint8_t *data)
{
	__ASSERT(size <= RING_BUFFER_MAX_SIZE + 1U, RING_BUFFER_SIZE_ASSERT_MSG);

	rb->size = size ? (ring_buf_size_t)size : 1U;
	rb->buffer = data;
	ring_buf_reset(rb);
}

/**
 * @brief Capture a snapshot of a ring buffer's current read/write positions.
 *
 * The source ring buffer is not modified.
 *
 * @param rb Address of ring buffer.
 *
 * @return Snapshot whose fields mirror @p rb at the moment of capture.
 */
static inline struct ring_buf ring_buf_snapshot(const struct ring_buf *rb)
{
	barrier_dmem_fence_full();
	return *rb;
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
	barrier_dmem_fence_full();
	*data = &rb->buffer[rb->write_idx];
	return rb->write_idx >= rb->read_idx ?
		rb->size - rb->write_idx - (rb->read_idx == 0) :
		rb->read_idx - rb->write_idx - 1U;
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
	rb->write_idx = rb_idx_wrap(rb->write_idx + (ring_buf_idx_t)size, rb->size);
	barrier_dmem_fence_full();
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
	barrier_dmem_fence_full();

	*data = &rb->buffer[rb->read_idx];
	return rb->write_idx >= rb->read_idx ? rb->write_idx - rb->read_idx :
		rb->size - rb->read_idx;
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
	rb->read_idx = rb_idx_wrap(rb->read_idx + (ring_buf_idx_t)size, rb->size);
	barrier_dmem_fence_full();
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
		uint32_t avail = ring_buf_put_ptr(rb, &dst);

		chunk = MIN(avail, size - total);
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

	if (unlikely(data == NULL)) {
		total = MIN(size, ring_buf_size_get(rb));
		ring_buf_consume(rb, total);
		return total;
	}

	do {
		uint32_t avail = ring_buf_get_ptr(rb, &src);

		chunk = MIN(avail, size - total);
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
static inline uint32_t ring_buf_peek(const struct ring_buf *rb, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t chunk;
	uint32_t total = 0;
	struct ring_buf snapshot = ring_buf_snapshot(rb);

	do {
		chunk = ring_buf_get_ptr(&snapshot, &src);
		chunk = MIN(chunk, size - total);
		if (chunk == 0U) {
			break;
		}

		memcpy(&data[total], src, chunk);
		ring_buf_consume(&snapshot, chunk);
		total += chunk;
	} while (total < size);

	return total;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
