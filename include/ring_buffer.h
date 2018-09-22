/* ring_buffer.h: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file */

#ifndef ZEPHYR_INCLUDE_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_RING_BUFFER_H_

#include <kernel.h>
#include <misc/util.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE32_OF(x) (sizeof((x))/sizeof(u32_t))

/**
 * @brief A structure to represent a ring buffer
 */
struct ring_buf {
	u32_t head;	 /**< Index in buf for the head element */
	u32_t tail;	 /**< Index in buf for the tail element */
	u32_t dropped_put_count; /**< Running tally of the number of failed
				     * put attempts
				     */
	u32_t size;   /**< Size of buf in 32-bit chunks */
	u32_t *buf;	 /**< Memory region for stored entries */
	u32_t mask;   /**< Modulo mask if size is a power of 2 */
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
#define SYS_RING_BUF_DECLARE_POW2(name, pow) \
	static u32_t _ring_buffer_data_##name[1 << (pow)]; \
	struct ring_buf name = { \
		.size = (1 << (pow)), \
		.mask = (1 << (pow)) - 1, \
		.buf = _ring_buffer_data_##name \
	};

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
#define SYS_RING_BUF_DECLARE_SIZE(name, size32) \
	static u32_t _ring_buffer_data_##name[size32]; \
	struct ring_buf name = { \
		.size = size32, \
		.buf = _ring_buffer_data_##name \
	};

/**
 * @brief Initialize a ring buffer.
 *
 * This routine initializes a ring buffer, prior to its first use. It is only
 * used for ring buffers not defined using SYS_RING_BUF_DECLARE_POW2 or
 * SYS_RING_BUF_DECLARE_SIZE.
 *
 * Setting @a size to a power of 2 establishes a high performance ring buffer
 * that doesn't require the use of modulo arithmetic operations to maintain
 * itself.
 *
 * @param buf Address of ring buffer.
 * @param size Ring buffer size (in 32-bit words).
 * @param data Ring buffer data area (typically u32_t data[size]).
 */
static inline void sys_ring_buf_init(struct ring_buf *buf, u32_t size,
				     u32_t *data)
{
	buf->head = 0;
	buf->tail = 0;
	buf->dropped_put_count = 0;
	buf->size = size;
	buf->buf = data;
	if (is_power_of_two(size)) {
		buf->mask = size - 1;
	} else {
		buf->mask = 0;
	}

}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is empty, or 0 if not.
 */
static inline int sys_ring_buf_is_empty(struct ring_buf *buf)
{
	return (buf->head == buf->tail);
}

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer free space (in 32-bit words).
 */
static inline int sys_ring_buf_space_get(struct ring_buf *buf)
{
	if (sys_ring_buf_is_empty(buf)) {
		return buf->size - 1;
	}

	if (buf->tail < buf->head) {
		return buf->head - buf->tail - 1;
	}

	/* buf->tail > buf->head */
	return (buf->size - buf->tail) + buf->head - 1;
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
int sys_ring_buf_put(struct ring_buf *buf, u16_t type, u8_t value,
		     u32_t *data, u8_t size32);

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
int sys_ring_buf_get(struct ring_buf *buf, u16_t *type, u8_t *value,
		     u32_t *data, u8_t *size32);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RING_BUFFER_H_ */
