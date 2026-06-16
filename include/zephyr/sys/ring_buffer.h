/*
 * Copyright (c) 2026 Nicolas Pitre	<npitre@baylibre.com>
 *		 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Simple, header-only ring buffer implementation.
 * @ingroup ring_buffer_apis
 *
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
/* read_idx/write_idx + n must not overflow size_t on 32-bit targets (index < 2N, n <= N) */
#define RING_BUFFER_MAX_SIZE (UINT32_MAX / 4U)
#define RING_BUFFER_SIZE_ASSERT_MSG "Size too large"
#else
typedef uint16_t ring_buf_idx_t;
typedef uint16_t ring_buf_size_t;
/* read_idx/write_idx live in [0, 2N) */
#define RING_BUFFER_MAX_SIZE (UINT16_MAX / 2U)
#define RING_BUFFER_SIZE_ASSERT_MSG \
	"Size too large, please enable CONFIG_RING_BUFFER_LARGE"
#endif
/** @endcond */

/**
 * @brief A structure to represent a ring buffer.
 *
 * The struct stores the buffer capacity @p size (N). No slot is sacrificed:
 * @p read_idx and @p write_idx are free-running indices in [0, 2N), so the
 * full and empty states are disambiguated by their lap rather than by a
 * reserved slot. The user-visible capacity equals @p size.
 */
struct ring_buf {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buffer;
	ring_buf_size_t size;
	ring_buf_idx_t read_idx;
	ring_buf_idx_t write_idx;
#ifdef CONFIG_RING_BUFFER
	ring_buf_idx_t put_claimed;
	ring_buf_idx_t get_claimed;
#endif /* CONFIG_RING_BUFFER */
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

/*
 * Advance a free-running index into [0, 2N) and convert it to the index type.
 * Takes the buffer capacity N and forms the 2N wrap limit internally, so the
 * [0, 2N) convention stays in one place. Callers guarantee value < 4N (index
 * < 2N plus an increment <= N), so a single conditional subtraction suffices.
 */
static ALWAYS_INLINE ring_buf_idx_t rb_idx_advance(size_t value, ring_buf_size_t size)
{
	size_t lim = 2U * size;

	return (ring_buf_idx_t)(value >= lim ? value - lim : value);
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
 * @p size denotes the buffer capacity in bytes, which equals the
 * user-visible capacity; no slot is reserved internally.
 *
 * @param buf Pointer to the backing byte storage.
 * @param sz  Buffer capacity, in bytes. Equals the user-visible capacity.
 */
#define RING_BUF_INIT(buf, sz)			\
{						\
	.buffer = (buf),			\
	.size = (ring_buf_size_t)(sz),		\
}

/**
 * @brief Define and initialize a ring buffer for byte data.
 *
 * The user-visible capacity equals the requested @p size8.
 * The backing storage is allocated as a static array of uint8_t
 * with size equal to @p size8. The ring buffer struct is initialized to point
 * to this backing storage and set the size accordingly.
 *
 * The ring buffer can be referenced from other modules with:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size8 buffer capacity in bytes.
 */
#define RING_BUF_DECLARE(name, size8)							\
	BUILD_ASSERT((size8) <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);	\
	static uint8_t __noinit _ring_buffer_data_##name[(size8)];			\
	struct ring_buf name = RING_BUF_INIT(_ring_buffer_data_##name, (size8))

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
 * @brief Determine size of available data in a ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer data size (in bytes).
 */
static inline uint32_t ring_buf_size_get(const struct ring_buf *rb)
{
	ring_buf_idx_t write_idx = rb_load_acquire(&rb->write_idx);
	ring_buf_idx_t read_idx = rb_load_acquire(&rb->read_idx);
	ring_buf_size_t occ = write_idx - read_idx;

	if (write_idx < read_idx) {
		occ += 2U * rb->size;
	}
	return occ;
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
	ring_buf_idx_t write_idx = rb_load_acquire(&rb->write_idx);
	ring_buf_idx_t read_idx = rb_load_acquire(&rb->read_idx);

	return write_idx == read_idx;
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
	return ring_buf_size_get(rb) == ring_buf_capacity_get(rb);
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
#ifdef CONFIG_RING_BUFFER
	rb->put_claimed = 0;
	rb->get_claimed = 0;
#endif /* CONFIG_RING_BUFFER */
}

/**
 * @brief Initialize a ring buffer for byte data.
 *
 * Used for ring buffers not defined using @ref RING_BUF_DECLARE. @p size
 * denotes the buffer capacity in bytes, which equals the user-visible
 * capacity; no slot is reserved internally.
 *
 * @param rb   Address of ring buffer.
 * @param size Buffer capacity (in bytes). A @p size of 0 produces a
 *             degenerate buffer with capacity 0.
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
 * @param[in]  offset Bytes past the current write index that the caller has
 *                    already tentatively reserved (0 for a fresh reservation).
 *                    Must not exceed the free space.
 *
 * @return Number of bytes available for writing. May be smaller than the
 *         total free space if the free region wraps around the end of the
 *         buffer.
 */
static inline uint32_t ring_buf_put_ptr(struct ring_buf *rb, uint8_t **data, size_t offset)
{
	ring_buf_idx_t write_idx = rb_load_relaxed(&rb->write_idx);
	ring_buf_idx_t read_idx = rb_load_acquire(&rb->read_idx);
	ring_buf_idx_t off, avail;

	__ASSERT_NO_MSG(offset <= ring_buf_space_get(rb));
	if (offset > 0) {
		write_idx = rb_idx_advance((size_t)write_idx + offset, rb->size);
	}

	if (write_idx >= rb->size) {
		off = write_idx - rb->size;
		avail = (read_idx >= rb->size ? rb->size : read_idx) - off;
	} else {
		off = write_idx;
		avail = (read_idx >= rb->size ? read_idx - rb->size : rb->size) - off;
	}
	*data = &rb->buffer[off];
	return avail;
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
	size_t write_idx = rb_load_relaxed(&rb->write_idx);

	__ASSERT_NO_MSG(size <= ring_buf_space_get(rb));
	rb_store_release(&rb->write_idx, rb_idx_advance(write_idx + size, rb->size));
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
 * @param[in]  offset Bytes past the current read index that the caller has
 *                    already tentatively consumed (0 for a fresh peek).
 *                    Must not exceed the amount of valid data.
 *
 * @return Number of bytes available for reading. May be smaller than the
 *         total amount of valid data if the data wraps around the end of the
 *         buffer.
 */
static inline uint32_t ring_buf_get_ptr(struct ring_buf *rb, uint8_t **data, size_t offset)
{
	ring_buf_idx_t read_idx = rb_load_relaxed(&rb->read_idx);
	ring_buf_idx_t write_idx = rb_load_acquire(&rb->write_idx);
	ring_buf_idx_t off, avail;

	__ASSERT_NO_MSG(offset <= ring_buf_size_get(rb));
	if (offset > 0) {
		read_idx = rb_idx_advance((size_t)read_idx + offset, rb->size);
	}

	if (read_idx >= rb->size) {
		off = read_idx - rb->size;
		avail = (write_idx >= rb->size ? write_idx : 2U * rb->size) - read_idx;
	} else {
		off = read_idx;
		avail = (write_idx >= rb->size ? rb->size : write_idx) - read_idx;
	}
	*data = &rb->buffer[off];
	return avail;
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
	size_t read_idx = rb_load_relaxed(&rb->read_idx);

	__ASSERT_NO_MSG(size <= ring_buf_size_get(rb));
	rb_store_release(&rb->read_idx, rb_idx_advance(read_idx + size, rb->size));
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
	uint32_t avail;

	do {
		avail = ring_buf_put_ptr(rb, &dst, total);
		chunk = MIN(avail, size - total);
		if (chunk == 0U) {
			break;
		}
		memcpy(dst, &data[total], chunk);
		total += chunk;
	} while (total < size);
	ring_buf_commit(rb, total);

	return total;
}

/**
 * @brief Read data from a ring buffer.
 *
 * @param rb   Address of ring buffer.
 * @param data Destination buffer.
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
		avail = ring_buf_get_ptr(rb, &src, total);
		chunk = MIN(avail, size - total);
		if (chunk == 0U) {
			break;
		}
		if (!IS_ENABLED(CONFIG_RING_BUFFER) || data != NULL) {
			memcpy(&data[total], src, chunk);
		}
		total += chunk;
	} while (total < size);
	ring_buf_consume(rb, total);

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
	uint32_t avail;
	uint32_t total = 0;

	do {
		avail = ring_buf_get_ptr((struct ring_buf *)rb, &src, total);
		chunk = MIN(avail, size - total);
		if (chunk == 0U) {
			break;
		}
		memcpy(&data[total], src, chunk);
		total += chunk;
	} while (total < size);

	return total;
}

#ifdef CONFIG_RING_BUFFER

/**
 * @brief Define and initialize an "item based" ring buffer.
 *
 * This macro establishes an "item based" ring buffer. Each data item is
 * an array of 32-bit words (from zero to 1020 bytes in length), coupled
 * with a 16-bit type identifier and an 8-bit integer value.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name Name of the ring buffer.
 * @param size32 Size of ring buffer (in 32-bit words).
 */
#define RING_BUF_ITEM_DECLARE(name, size32)						\
	BUILD_ASSERT((size32) <= RING_BUFFER_MAX_SIZE / 4, RING_BUFFER_SIZE_ASSERT_MSG);	\
	static uint32_t __noinit _ring_buffer_data_##name[(size32)];			\
	struct ring_buf name = RING_BUF_INIT((uint8_t *)_ring_buffer_data_##name, 4 * (size32))

/**
 * @brief Define and initialize an "item based" ring buffer.
 *
 * This exists for backward compatibility reasons. @ref RING_BUF_ITEM_DECLARE
 * should be used instead.
 *
 * @param name Name of the ring buffer.
 * @param size32 Size of ring buffer (in 32-bit words).
 */
#define RING_BUF_ITEM_DECLARE_SIZE(name, size32) \
	RING_BUF_ITEM_DECLARE(name, (size32))

/**
 * @brief Define and initialize a power-of-2 sized "item based" ring buffer.
 *
 * This macro establishes an "item based" ring buffer by specifying its
 * size using a power of 2. This exists mainly for backward compatibility
 * reasons. @ref RING_BUF_ITEM_DECLARE should be used instead.
 *
 * @param name Name of the ring buffer.
 * @param pow Ring buffer size exponent.
 */
#define RING_BUF_ITEM_DECLARE_POW2(name, pow) \
	RING_BUF_ITEM_DECLARE(name, BIT(pow))

/**
 * @brief Compute the ring buffer size in 32-bit needed to store an element
 *
 * The argument can be a type or an expression.
 * Note: rounds up if the size is not a multiple of 32 bits.
 *
 * @param expr Expression or type to compute the size of
 */
#define RING_BUF_ITEM_SIZEOF(expr) DIV_ROUND_UP(sizeof(expr), sizeof(uint32_t))

/**
 * @brief Initialize an "item based" ring buffer.
 *
 * This routine initializes a ring buffer, prior to its first use. It is only
 * used for ring buffers not defined using RING_BUF_ITEM_DECLARE.
 *
 * Each data item is an array of 32-bit words (from zero to 1020 bytes in
 * length), coupled with a 16-bit type identifier and an 8-bit integer value.
 *
 * @param rb Address of ring buffer.
 * @param size Ring buffer size (in 32-bit words)
 * @param data Ring buffer data area (uint32_t data[size]).
 */
__deprecated /* use #include <zephyr/sys/ringq.h> instead */
static inline void ring_buf_item_init(struct ring_buf *rb, uint32_t size, uint32_t *data)
{
	__ASSERT(size <= RING_BUFFER_MAX_SIZE / 4, RING_BUFFER_SIZE_ASSERT_MSG);
	ring_buf_init(rb, 4 * size, (uint8_t *)data);
}

/**
 * @brief Determine free space in an "item based" ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer free space (in 32-bit words).
 */
__deprecated /* use #include <zephyr/sys/ringq.h> instead */
static inline uint32_t ring_buf_item_space_get(const struct ring_buf *rb)
{
	return ring_buf_space_get(rb) / 4;
}

/**
 * @brief Allocate buffer for writing data to a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is written to allocated area
 * number of bytes written must be confirmed (see @ref ring_buf_put_finish).
 *
 * @warning
 * Use cases involving multiple writers to the ring buffer must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the ring buffer.
 *
 * @warning
 * Ring buffer instance should not mix byte access and item access
 * (calls prefixed with ring_buf_item_).
 *
 * @param[in]  rb	Address of ring buffer.
 * @param[out] data	Pointer to the address. It is set to a location within
 *			ring buffer.
 * @param[in]  size	Requested allocation size (in bytes).
 *
 * @return Size of allocated buffer which can be smaller than requested if
 *	   there is not enough free space or buffer wraps.
 */
__deprecated /* use ring_buf_put_ptr(...) & ring_buf_commit(...) instead */
static inline uint32_t ring_buf_put_claim(struct ring_buf *rb, uint8_t **data, uint32_t size)
{
	uint32_t claimed = MIN(ring_buf_put_ptr(rb, data, rb->put_claimed), size);

	rb->put_claimed += claimed;
	return claimed;
}

/**
 * @brief Indicate number of bytes written to allocated buffers.
 *
 * The number of bytes must be equal to or lower than the sum corresponding
 * to all preceding @ref ring_buf_put_claim invocations (or even 0). Surplus
 * bytes will be returned to the available free buffer space.
 *
 * @warning
 * Use cases involving multiple writers to the ring buffer must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the ring buffer.
 *
 * @warning
 * Ring buffer instance should not mix byte access and item access
 * (calls prefixed with ring_buf_item_).
 *
 * @param  rb	Address of ring buffer.
 * @param  size Number of valid bytes in the allocated buffers.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds free space in the ring buffer.
 */
__deprecated /* use ring_buf_put_ptr(...) & ring_buf_commit(...) instead */
static inline int ring_buf_put_finish(struct ring_buf *rb, uint32_t size)
{
	if (rb->put_claimed < size) {
		return -EINVAL;
	}
	ring_buf_commit(rb, size);
	rb->put_claimed = 0;
	return 0;
}


/**
 * @brief Get address of a valid data in a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is processed it must be freed
 * using @ref ring_buf_get_finish.
 *
 * @warning
 * Use cases involving multiple reads of the ring buffer must prevent
 * concurrent read operations, either by preventing all readers from
 * being preempted or by using a mutex to govern reads to the ring buffer.
 *
 * @warning
 * Ring buffer instance should not mix byte access and item access
 * (calls prefixed with ring_buf_item_).
 *
 * @param[in]  rb	Address of ring buffer.
 * @param[out] data	Pointer to the address. It is set to a location within
 *			ring buffer.
 * @param[in]  size	Requested size (in bytes).
 *
 * @return Number of valid bytes in the provided buffer which can be smaller
 *	   than requested if there is not enough free space or buffer wraps.
 */
__deprecated /* use ring_buf_get_ptr(...) & ring_buf_consume(...) instead */
static inline uint32_t ring_buf_get_claim(struct ring_buf *rb, uint8_t **data, uint32_t size)
{
	uint32_t claimed = MIN(ring_buf_get_ptr(rb, data, rb->get_claimed), size);

	rb->get_claimed += claimed;
	return claimed;
}

/**
 * @brief Indicate number of bytes read from claimed buffer.
 *
 * The number of bytes must be equal or lower than the sum corresponding to
 * all preceding @ref ring_buf_get_claim invocations (or even 0). Surplus
 * bytes will remain available in the buffer.
 *
 * @warning
 * Use cases involving multiple reads of the ring buffer must prevent
 * concurrent read operations, either by preventing all readers from
 * being preempted or by using a mutex to govern reads to the ring buffer.
 *
 * @warning
 * Ring buffer instance should not mix byte access and  item mode
 * (calls prefixed with ring_buf_item_).
 *
 * @param  rb	Address of ring buffer.
 * @param  size Number of bytes that can be freed.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds valid bytes in the ring buffer.
 */
__deprecated /* use ring_buf_get_ptr(...) & ring_buf_consume(...) instead */
static inline int ring_buf_get_finish(struct ring_buf *rb, uint32_t size)
{
	if (rb->get_claimed < size) {
		return -EINVAL;
	}

	ring_buf_consume(rb, size);
	rb->get_claimed = 0;
	return 0;
}

/** @cond INTERNAL_HIDDEN */
struct ring_element {
	uint32_t type   : 16;
	uint32_t length : 8;
	uint32_t value  : 8;
};

/**
* Only used internally by ring_buf_item_put to declutter the code.
* ring_buf_item_put already knows the data fits in the ring_buffer before
* calling this function, hence no space check is performed here.
*/
static inline void z_rb_write_no_commit(struct ring_buf *rb, const uint8_t *data,
					uint32_t size, uint32_t offset)
{
	uint32_t avail;
	uint32_t chunk;
	uint8_t *dst;
	uint32_t off = 0;

	while (off < size) {
		avail = ring_buf_put_ptr(rb, &dst, offset + off);
		chunk = MIN(avail, size - off);

		memcpy(dst, &data[off], chunk);
		off += chunk;
	}
}

/** @endcond */

/**
 * @brief Write a data item to a ring buffer.
 *
 * This routine writes a data item to ring buffer @a buf. The data item
 * is an array of 32-bit words (from zero to 1020 bytes in length),
 * coupled with a 16-bit type identifier and an 8-bit integer value.
 *
 * @warning
 * Use cases involving multiple writers to the ring buffer must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param type Data item's type identifier (application specific).
 * @param value Data item's integer value (application specific).
 * @param data Address of data item.
 * @param size32 Data item size (number of 32-bit words).
 *
 * @retval 0 Data item was written.
 * @retval -EMSGSIZE Ring buffer has insufficient free space.
 */
__deprecated /* use #include <zephyr/sys/ringq.h> instead */
static inline int ring_buf_item_put(struct ring_buf *buf, uint16_t type, uint8_t value,
					uint32_t *data, uint8_t size32)
{
	struct ring_element header;
	uint32_t size = size32 * 4;

	if (size + sizeof(header) > ring_buf_space_get(buf)) {
		return -EMSGSIZE;
	}

	header.type = type;
	header.length = size32;
	header.value = value;
	z_rb_write_no_commit(buf, (const uint8_t *)&header, sizeof(header), 0);
	z_rb_write_no_commit(buf, (const uint8_t *)data, size, sizeof(header));
	ring_buf_commit(buf, sizeof(header) + size);
	return 0;
}

/**
 * @brief Read a data item from a ring buffer.
 *
 * This routine reads a data item from ring buffer @a buf. The data item
 * is an array of 32-bit words (up to 1020 bytes in length),
 * coupled with a 16-bit type identifier and an 8-bit integer value.
 *
 * @warning
 * Use cases involving multiple reads of the ring buffer must prevent
 * concurrent read operations, either by preventing all readers from
 * being preempted or by using a mutex to govern reads to the ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param type Area to store the data item's type identifier.
 * @param value Area to store the data item's integer value.
 * @param data Area to store the data item. Can be NULL to discard data.
 * @param size32 Size of the data item storage area (number of 32-bit chunks).
 *
 * @retval 0 Data item was fetched; @a size32 now contains the number of
 *         32-bit words read into data area @a data.
 * @retval -EAGAIN Ring buffer is empty.
 * @retval -EMSGSIZE Data area @a data is too small; @a size32 now contains
 *         the number of 32-bit words needed.
 */
__deprecated /* use #include <zephyr/sys/ringq.h> instead */
static inline int ring_buf_item_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
					uint32_t *data, uint8_t *size32)
{
	struct ring_element header;

	if (ring_buf_is_empty(buf)) {
		return -EAGAIN;
	}

	ring_buf_peek(buf, (uint8_t *)&header, sizeof(header));
	if (data != NULL && (header.length > *size32)) {
		*size32 = header.length;
		return -EMSGSIZE;
	}

	*size32 = header.length;
	*type = header.type;
	*value = header.value;

	ring_buf_consume(buf, sizeof(header));
	ring_buf_get(buf, (uint8_t *)data, header.length * 4);
	return 0;
}

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Force ring_buf internal indices to a given value.
 *
 * Any value other than 0 makes sense only in a validation testing context,
 * where it is used to push the free-running indices close to their wrap
 * boundary. The value is reduced into the valid [0, 2N) index range so the
 * internal invariants keep holding.
 */
static inline void ring_buf_internal_reset(struct ring_buf *rb, ring_buf_idx_t value)
{
	ring_buf_size_t lim = 2U * rb->size;

	value = (lim != 0U) ? (ring_buf_idx_t)(value % lim) : 0U;
	rb_store_relaxed(&rb->read_idx, value);
	rb_store_relaxed(&rb->write_idx, value);
	rb->put_claimed = 0;
	rb->get_claimed = 0;
}

/** @endcond */

#else /* CONFIG_RING_BUFFER */

/** @cond INTERNAL_HIDDEN */
#define Z_RING_BUF_CLAIM_REMOVED(name)						\
	({									\
		BUILD_ASSERT(0, #name "() is deprecated and not available "	\
			"when CONFIG_RING_BUFFER=n. Enable "			\
			"CONFIG_RING_BUFFER to keep it during the "		\
			"deprecation period, or migrate as described in the "	\
			"Zephyr 4.5 migration guide.");				\
		0;								\
	})

#define Z_RING_BUF_DECLARE_REMOVED(name)					\
	BUILD_ASSERT(0, #name " is deprecated and not available when "		\
		"CONFIG_RING_BUFFER=n. Enable CONFIG_RING_BUFFER "		\
		"to keep it during the deprecation period, or migrate as "	\
		"described in the Zephyr 4.5 migration guide.")

#define ring_buf_put_claim(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_put_claim)
#define ring_buf_put_finish(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_put_finish)
#define ring_buf_get_claim(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_get_claim)
#define ring_buf_get_finish(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_get_finish)
#define ring_buf_item_init(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_init)
#define ring_buf_item_put(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_put)
#define ring_buf_item_get(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_get)
#define ring_buf_item_space_get(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_space_get)
#define ring_buf_internal_reset(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_internal_reset)

#define RING_BUF_ITEM_DECLARE(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE)
#define RING_BUF_ITEM_DECLARE_SIZE(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE_SIZE)
#define RING_BUF_ITEM_DECLARE_POW2(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE_POW2)
/** @endcond */
#endif /* CONFIG_RING_BUFFER */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
