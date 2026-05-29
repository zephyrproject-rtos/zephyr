/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
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
#include <zephyr/toolchain.h>

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
#define RING_BUFFER_MAX_SIZE (UINT32_MAX - 1U)
#define RING_BUFFER_SIZE_ASSERT_MSG "Size too large"
#else
typedef uint16_t ring_buf_idx_t;
typedef uint16_t ring_buf_size_t;
#define RING_BUFFER_MAX_SIZE (UINT16_MAX - 1U)
#define RING_BUFFER_SIZE_ASSERT_MSG \
	"Size too large, please enable CONFIG_RING_BUFFER_LARGE"
#endif
/** @endcond */

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

static ALWAYS_INLINE ring_buf_idx_t rb_idx_advance(ring_buf_idx_t v, ring_buf_size_t storage_sz)
{
	return v >= storage_sz ? v - storage_sz : v;
}

static ALWAYS_INLINE ring_buf_idx_t rb_load_acquire(const ring_buf_idx_t *p)
{
	ring_buf_idx_t v;

#if IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	v = __atomic_load_n(p, __ATOMIC_ACQUIRE);
#else
	v = *p;
	compiler_barrier();
#endif
	return v;
}

static ALWAYS_INLINE ring_buf_idx_t rb_load_relaxed(const ring_buf_idx_t *p)
{
#if IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	return __atomic_load_n(p, __ATOMIC_RELAXED);
#else
	return *p;
#endif
}

static ALWAYS_INLINE void rb_store_release(ring_buf_idx_t *p, ring_buf_idx_t v)
{
#if IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	__atomic_store_n(p, v, __ATOMIC_RELEASE);
#else
	compiler_barrier();
	*p = v;
#endif
}

static ALWAYS_INLINE void rb_store_relaxed(ring_buf_idx_t *p, ring_buf_idx_t v)
{
#if IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	__atomic_store_n(p, v, __ATOMIC_RELAXED);
#else
	*p = v;
#endif
}

/** @endcond */

/**
 * @brief Statically initialize a ring buffer.
 *
 * @p size denotes the size of the backing storage in bytes. The user-visible
 * capacity is @p size - 1 because one slot is reserved internally to
 * disambiguate the empty and full states.
 *
 * @param buf Pointer to the backing byte storage.
 * @param sz  Backing storage size, in bytes. The user-visible capacity is
 *            @p sz - 1 because one slot is reserved.
 */
#define RING_BUF_INIT(buf, sz)				\
{							\
	.buffer = buf,					\
	.size = (ring_buf_size_t)((sz) ? (sz) : 1U),	\
}

/**
 * @brief Compute the backing storage size required to hold @p usable_size
 *        user-visible bytes.
 *
 * The implementation reserves one byte internally to disambiguate the
 * empty and full states, so the backing storage must always be one byte
 * larger than the user-visible capacity.
 *
 * @param usable_size User-visible ring buffer capacity (in bytes).
 */
#define RING_BUF_STORAGE_SIZE(usable_size) ((usable_size) + 1U)

/**
 * @brief Define and initialize a ring buffer for byte data.
 *
 * The user-visible capacity equals the requested @p size8 - 1 because one byte is reserved
 * internally to disambiguate the empty and full states. The backing storage is allocated as a
 * static array of uint8_t with size equal to the user-visible capacity plus one byte for the
 * reserved slot. The ring buffer struct is initialized to point to this backing storage and set the
 * size accordingly.
 *
 * The ring buffer can be referenced from other modules with:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size8 backing storage size in bytes.
 */
#define RING_BUF_DECLARE(name, size8)							\
	BUILD_ASSERT(size8 <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);	\
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
	return rb->size - 1U;
}

/**
 * @brief Capture a snapshot of a ring buffer's current read/write positions.
 *
 * Captures a coherent {read_idx, write_idx} pair from @p rb without
 * modifying it.
 *
 * Intended for advanced callers that want to inspect or reserve a region of
 * the buffer and then decide later how much to actually commit or consume.
 * This generalises the previous "claim / finish" pattern:
 * take a snapshot, compute against the frozen indices and @p rb->buffer / @p rb->size,
 * then advance the real buffer with @ref ring_buf_commit or @ref ring_buf_consume.
 *
 * @param rb Address of ring buffer.
 *
 * @return Snapshot whose fields mirror @p rb at the moment of capture.
 */
static inline struct ring_buf ring_buf_snapshot(const struct ring_buf *rb)
{
	struct ring_buf s;

	s.buffer = rb->buffer;
	s.size = rb->size;
	s.write_idx = rb_load_acquire(&rb->write_idx);
	s.read_idx = rb_load_acquire(&rb->read_idx);
	return s;
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
	struct ring_buf s = ring_buf_snapshot(rb);

	return s.write_idx >= s.read_idx ?
		(ring_buf_size_t)(s.write_idx - s.read_idx) :
		(ring_buf_size_t)(s.size - (s.read_idx - s.write_idx));
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
	struct ring_buf s = ring_buf_snapshot(rb);

	return s.read_idx == s.write_idx;
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
	struct ring_buf s = ring_buf_snapshot(rb);

	return rb_idx_advance(s.write_idx + 1U, s.size) == s.read_idx;
}

/**
 * @brief Reset ring buffer state.
 *
 * @param rb Address of ring buffer.
 */
static inline void ring_buf_reset(struct ring_buf *rb)
{
	rb_store_relaxed(&rb->read_idx, 0);
	rb_store_relaxed(&rb->write_idx, 0);
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
	__ASSERT(size <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);

	rb->size = size ? (ring_buf_size_t)size : 1U;
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
	struct ring_buf s = ring_buf_snapshot(rb);

	/*
	 * One slot is sacrificed to differentiate empty from full.
	 * The sacrificed slot always sits at (read_idx - 1) % size.
	 *
	 * write_idx >= read_idx: return the pre-wrap data buffer[write_idx, size].
	 * If read_idx == 0 the sacrificed slot lives at size - 1, otherwise it's in the wrapped
	 * region we don't return.
	 * write_idx <  read_idx: return [write_idx, read_idx - 1] as the sacrificed slot lives at
	 * read_idx - 1.
	 */
	*data = &rb->buffer[s.write_idx];
	return s.write_idx >= s.read_idx ?
		s.size - s.write_idx - (s.read_idx == 0) :
		s.read_idx - s.write_idx - 1U;
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
	ring_buf_idx_t w = rb_load_relaxed(&rb->write_idx);

	rb_store_release(&rb->write_idx, rb_idx_advance(w + size, rb->size));
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
	struct ring_buf s = ring_buf_snapshot(rb);

	*data = &rb->buffer[s.read_idx];
	return s.write_idx >= s.read_idx ?
		s.write_idx - s.read_idx :
		s.size - s.read_idx;
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
	ring_buf_idx_t r = rb_load_relaxed(&rb->read_idx);

	rb_store_release(&rb->read_idx, rb_idx_advance(r + size, rb->size));
}

/**
 * @brief Write (copy) data to a ring buffer.
 *
 * @param rb   Address of ring buffer.
 * @param data Source data. Must not be NULL.
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
 * @param data Destination buffer. Must not be NULL.
 * @param size Maximum number of bytes to read.
 *
 * @return Number of bytes read.
 */
static inline uint32_t ring_buf_get(struct ring_buf *rb, uint8_t *data, uint32_t size)
{
	uint8_t *src;
	uint32_t chunk;
	uint32_t avail;
	uint32_t total = 0;

	do {
		avail = ring_buf_get_ptr(rb, &src);

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
	struct ring_buf s = ring_buf_snapshot(rb);

	return ring_buf_get(&s, data, size);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
