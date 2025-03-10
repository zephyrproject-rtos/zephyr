/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_

#include <zephyr/sys/util.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup ring_buffer_apis Ring Buffer APIs
 * @ingroup datastructure_apis
 *
 * @brief Simple ring buffer implementation.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* The limit is used by algorithm for distinguishing between empty and full
 * state.
 */
#ifdef CONFIG_RING_BUFFER_LARGE
typedef uint32_t ring_buf_idx_t;
#define RING_BUFFER_MAX_SIZE (UINT32_MAX / 2)
#else
typedef uint16_t ring_buf_idx_t;
#define RING_BUFFER_MAX_SIZE (UINT16_MAX / 2)
#endif

#define RING_BUFFER_SIZE_ASSERT_MSG "Size too big"

struct ring_buf_index { ring_buf_idx_t head, tail, base; };

/** @endcond */

/**
 * @brief A structure to represent a ring buffer
 */
struct ring_buf {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buffer;
	struct ring_buf_index put;
	struct ring_buf_index get;
	uint32_t size;
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

uint32_t ring_buf_area_claim(struct ring_buf *buf, struct ring_buf_index *ring,
			     uint8_t **data, uint32_t size);
int ring_buf_area_finish(struct ring_buf *buf, struct ring_buf_index *ring,
			 uint32_t size);

/**
 * @brief Function to force ring_buf internal states to given value
 *
 * Any value other than 0 makes sense only in validation testing context.
 */
static inline void ring_buf_internal_reset(struct ring_buf *buf, ring_buf_idx_t value)
{
	buf->put.head = buf->put.tail = buf->put.base = value;
	buf->get.head = buf->get.tail = buf->get.base = value;
}

/** @endcond */

#define RING_BUF_INIT(buf, size8)	\
{					\
	.buffer = buf,			\
	.size = size8,			\
}

/**
 * @brief Define and initialize a ring buffer for byte data.
 *
 * This macro establishes a ring buffer of an arbitrary size.
 * The basic storage unit is a byte.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size8 Size of ring buffer (in bytes).
 */
#define RING_BUF_DECLARE(name, size8) \
	BUILD_ASSERT(size8 <= RING_BUFFER_MAX_SIZE,\
		RING_BUFFER_SIZE_ASSERT_MSG); \
	static uint8_t __noinit _ring_buffer_data_##name[size8]; \
	struct ring_buf name = RING_BUF_INIT(_ring_buffer_data_##name, size8)

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
#define RING_BUF_ITEM_DECLARE(name, size32) \
	BUILD_ASSERT((size32) <= RING_BUFFER_MAX_SIZE / 4, \
		RING_BUFFER_SIZE_ASSERT_MSG); \
	static uint32_t __noinit _ring_buffer_data_##name[size32]; \
	struct ring_buf name = { \
		.buffer = (uint8_t *) _ring_buffer_data_##name, \
		.size = 4 * (size32) \
	}

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
	RING_BUF_ITEM_DECLARE(name, size32)

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
 * @brief Initialize a ring buffer for byte data.
 *
 * This routine initializes a ring buffer, prior to its first use. It is only
 * used for ring buffers not defined using RING_BUF_DECLARE.
 *
 * @param buf Address of ring buffer.
 * @param size Ring buffer size (in bytes).
 * @param data Ring buffer data area (uint8_t data[size]).
 */
static inline void ring_buf_init(struct ring_buf *buf,
				 uint32_t size,
				 uint8_t *data)
{
	__ASSERT(size <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ASSERT_MSG);

	buf->size = size;
	buf->buffer = data;
	ring_buf_internal_reset(buf, 0);
}

/**
 * @brief Initialize an "item based" ring buffer.
 *
 * This routine initializes a ring buffer, prior to its first use. It is only
 * used for ring buffers not defined using RING_BUF_ITEM_DECLARE.
 *
 * Each data item is an array of 32-bit words (from zero to 1020 bytes in
 * length), coupled with a 16-bit type identifier and an 8-bit integer value.
 *
 * @param buf Address of ring buffer.
 * @param size Ring buffer size (in 32-bit words)
 * @param data Ring buffer data area (uint32_t data[size]).
 */
static inline void ring_buf_item_init(struct ring_buf *buf,
				      uint32_t size,
				      uint32_t *data)
{
	__ASSERT(size <= RING_BUFFER_MAX_SIZE / 4, RING_BUFFER_SIZE_ASSERT_MSG);
	ring_buf_init(buf, 4 * size, (uint8_t *)data);
}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return true if the ring buffer is empty, or false if not.
 */
static inline bool ring_buf_is_empty(const struct ring_buf *buf)
{
	return buf->get.head == buf->put.tail;
}

/**
 * @brief Reset ring buffer state.
 *
 * @param buf Address of ring buffer.
 */
static inline void ring_buf_reset(struct ring_buf *buf)
{
	ring_buf_internal_reset(buf, 0);
}

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer free space (in bytes).
 */
static inline uint32_t ring_buf_space_get(const struct ring_buf *buf)
{
	ring_buf_idx_t allocated = buf->put.head - buf->get.tail;

	return buf->size - allocated;
}

/**
 * @brief Determine free space in an "item based" ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer free space (in 32-bit words).
 */
static inline uint32_t ring_buf_item_space_get(const struct ring_buf *buf)
{
	return ring_buf_space_get(buf) / 4;
}

/**
 * @brief Return ring buffer capacity.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer capacity (in bytes).
 */
static inline uint32_t ring_buf_capacity_get(const struct ring_buf *buf)
{
	return buf->size;
}

/**
 * @brief Determine size of available data in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer data size (in bytes).
 */
static inline uint32_t ring_buf_size_get(const struct ring_buf *buf)
{
	ring_buf_idx_t available = buf->put.tail - buf->get.head;

	return available;
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
 * @param[in]  buf  Address of ring buffer.
 * @param[out] data Pointer to the address. It is set to a location within
 *		    ring buffer.
 * @param[in]  size Requested allocation size (in bytes).
 *
 * @return Size of allocated buffer which can be smaller than requested if
 *	   there is not enough free space or buffer wraps.
 */
static inline uint32_t ring_buf_put_claim(struct ring_buf *buf,
					  uint8_t **data,
					  uint32_t size)
{
	return ring_buf_area_claim(buf, &buf->put, data,
				   MIN(size, ring_buf_space_get(buf)));
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
 * @param  buf  Address of ring buffer.
 * @param  size Number of valid bytes in the allocated buffers.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds free space in the ring buffer.
 */
static inline int ring_buf_put_finish(struct ring_buf *buf, uint32_t size)
{
	return ring_buf_area_finish(buf, &buf->put, size);
}

/**
 * @brief Write (copy) data to a ring buffer.
 *
 * This routine writes data to a ring buffer @a buf.
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
 * @param buf Address of ring buffer.
 * @param data Address of data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written.
 */
uint32_t ring_buf_put(struct ring_buf *buf, const uint8_t *data, uint32_t size);

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
 * @param[in]  buf  Address of ring buffer.
 * @param[out] data Pointer to the address. It is set to a location within
 *		    ring buffer.
 * @param[in]  size Requested size (in bytes).
 *
 * @return Number of valid bytes in the provided buffer which can be smaller
 *	   than requested if there is not enough free space or buffer wraps.
 */
static inline uint32_t ring_buf_get_claim(struct ring_buf *buf,
					  uint8_t **data,
					  uint32_t size)
{
	return ring_buf_area_claim(buf, &buf->get, data,
				   MIN(size, ring_buf_size_get(buf)));
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
 * @param  buf  Address of ring buffer.
 * @param  size Number of bytes that can be freed.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds valid bytes in the ring buffer.
 */
static inline int ring_buf_get_finish(struct ring_buf *buf, uint32_t size)
{
	return ring_buf_area_finish(buf, &buf->get, size);
}

/**
 * @brief Read data from a ring buffer.
 *
 * This routine reads data from a ring buffer @a buf.
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
 * @param buf  Address of ring buffer.
 * @param data Address of the output buffer. Can be NULL to discard data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size);

/**
 * @brief Peek at data from a ring buffer.
 *
 * This routine reads data from a ring buffer @a buf without removal.
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
 * @warning
 * Multiple calls to peek will result in the same data being 'peeked'
 * multiple times. To remove data, use either @ref ring_buf_get or
 * @ref ring_buf_get_claim followed by @ref ring_buf_get_finish with a
 * non-zero `size`.
 *
 * @param buf  Address of ring buffer.
 * @param data Address of the output buffer. Cannot be NULL.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t ring_buf_peek(struct ring_buf *buf, uint8_t *data, uint32_t size);

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
int ring_buf_item_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		      uint32_t *data, uint8_t size32);

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
int ring_buf_item_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
		      uint32_t *data, uint8_t *size32);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
