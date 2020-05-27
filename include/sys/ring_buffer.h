/* ring_buffer.h: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file */

#ifndef ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_

#include <kernel.h>
#include <sys/util.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE32_OF(x) (sizeof((x))/sizeof(uint32_t))

/**
 * @brief A structure to represent a ring buffer
 */
struct ring_buf {
	uint32_t head;	 /**< Index in buf for the head element */
	uint32_t tail;	 /**< Index in buf for the tail element */
	union ring_buf_misc {
		struct ring_buf_misc_item_mode {
			uint32_t dropped_put_count; /**< Running tally of the
						   * number of failed put
						   * attempts.
						   */
		} item_mode;
		struct ring_buf_misc_byte_mode {
			uint32_t tmp_tail;
			uint32_t tmp_head;
		} byte_mode;
	} misc;
	uint32_t size;   /**< Size of buf in 32-bit chunks */

	union ring_buf_buffer {
		uint32_t *buf32;	 /**< Memory region for stored entries */
		uint8_t *buf8;
	} buf;
	uint32_t mask;   /**< Modulo mask if size is a power of 2 */
};

/**
 * @defgroup ring_buffer_apis Ring Buffer APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Statically define and initialize a high performance ring buffer.
 *
 * This macro establishes a ring buffer whose size must be a power of 2;
 * that is, the ring buffer contains 2^pow 32-bit words, where @a pow is
 * the specified ring buffer size exponent. A high performance ring buffer
 * doesn't require the use of modulo arithmetic operations to maintain itself.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name Name of the ring buffer.
 * @param pow Ring buffer size exponent.
 */
#define RING_BUF_ITEM_DECLARE_POW2(name, pow) \
	static uint32_t _ring_buffer_data_##name[BIT(pow)]; \
	struct ring_buf name = { \
		.size = (BIT(pow)),	  \
		.mask = (BIT(pow)) - 1, \
		.buf = { .buf32 = _ring_buffer_data_##name } \
	}

/** @deprecated Renamed to RING_BUF_ITEM_DECLARE_POW2. */
#define SYS_RING_BUF_DECLARE_POW2(name, pow) \
	__DEPRECATED_MACRO RING_BUF_ITEM_DECLARE_POW2(name, pow)

/**
 * @brief Statically define and initialize a standard ring buffer.
 *
 * This macro establishes a ring buffer of an arbitrary size. A standard
 * ring buffer uses modulo arithmetic operations to maintain itself.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct ring_buf <name>; @endcode
 *
 * @param name Name of the ring buffer.
 * @param size32 Size of ring buffer (in 32-bit words).
 */
#define RING_BUF_ITEM_DECLARE_SIZE(name, size32) \
	static uint32_t _ring_buffer_data_##name[size32]; \
	struct ring_buf name = { \
		.size = size32, \
		.buf = { .buf32 = _ring_buffer_data_##name} \
	}

/** @deprecated Renamed to RING_BUF_ITEM_DECLARE_SIZE. */
#define SYS_RING_BUF_DECLARE_SIZE(name, size32) \
	__DEPRECATED_MACRO RING_BUF_ITEM_DECLARE_SIZE(name, size32)

/**
 * @brief Statically define and initialize a ring buffer for byte data.
 *
 * This macro establishes a ring buffer of an arbitrary size.
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
	static uint8_t _ring_buffer_data_##name[size8]; \
	struct ring_buf name = { \
		.size = size8, \
		.buf = { .buf8 = _ring_buffer_data_##name} \
	}


/**
 * @brief Initialize a ring buffer.
 *
 * This routine initializes a ring buffer, prior to its first use. It is only
 * used for ring buffers not defined using RING_BUF_DECLARE,
 * RING_BUF_ITEM_DECLARE_POW2 or RING_BUF_ITEM_DECLARE_SIZE.
 *
 * Setting @a size to a power of 2 establishes a high performance ring buffer
 * that doesn't require the use of modulo arithmetic operations to maintain
 * itself.
 *
 * @param buf Address of ring buffer.
 * @param size Ring buffer size (in 32-bit words or bytes).
 * @param data Ring buffer data area (uint32_t data[size] or uint8_t data[size] for
 *	       bytes mode).
 */
static inline void ring_buf_init(struct ring_buf *buf, uint32_t size, void *data)
{
	memset(buf, 0, sizeof(struct ring_buf));
	buf->size = size;
	buf->buf.buf32 = (uint32_t *)data;
	if (is_power_of_two(size)) {
		buf->mask = size - 1;
	} else {
		buf->mask = 0U;
	}
}

/** @brief Determine free space based on ring buffer parameters.
 *
 * @note Function for internal use.
 *
 * @param size Ring buffer size.
 * @param head Ring buffer head.
 * @param tail Ring buffer tail.
 *
 *  @return Ring buffer free space (in 32-bit words or bytes).
 */
static inline uint32_t z_ring_buf_custom_space_get(uint32_t size, uint32_t head,
					      uint32_t tail)
{
	if (tail < head) {
		return head - tail - 1;
	}

	/* buf->tail > buf->head */
	return (size - tail) + head - 1;
}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is empty, or 0 if not.
 */
static inline int ring_buf_is_empty(struct ring_buf *buf)
{
	return (buf->head == buf->tail);
}

/**
 * @brief Reset ring buffer state.
 *
 * @param buf Address of ring buffer.
 */
static inline void ring_buf_reset(struct ring_buf *buf)
{
	buf->head = 0;
	buf->tail = 0;
	memset(&buf->misc, 0, sizeof(buf->misc));
}

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer free space (in 32-bit words or bytes).
 */
static inline uint32_t ring_buf_space_get(struct ring_buf *buf)
{
	return z_ring_buf_custom_space_get(buf->size, buf->head, buf->tail);
}

/**
 * @brief Return ring buffer capacity.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer capacity (in 32-bit words or bytes).
 */
static inline uint32_t ring_buf_capacity_get(struct ring_buf *buf)
{
	/* One element is used to distinguish between empty and full state. */
	return buf->size - 1;
}

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
 * @param data Area to store the data item.
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
 * @brief Allocate buffer for writing data to a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is written to allocated area
 * number of bytes written can be confirmed (see @ref ring_buf_put_finish).
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
uint32_t ring_buf_put_claim(struct ring_buf *buf, uint8_t **data, uint32_t size);

/**
 * @brief Indicate number of bytes written to allocated buffers.
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
int ring_buf_put_finish(struct ring_buf *buf, uint32_t size);

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
 * can be used directly by the user. Once data is processed it can be freed
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
uint32_t ring_buf_get_claim(struct ring_buf *buf, uint8_t **data, uint32_t size);

/**
 * @brief Indicate number of bytes read from claimed buffer.
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
int ring_buf_get_finish(struct ring_buf *buf, uint32_t size);

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
 * @param data Address of the output buffer.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
uint32_t ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
