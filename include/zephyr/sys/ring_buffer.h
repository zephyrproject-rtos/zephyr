/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_RING_BUFFER
/* TODO: remove everything encapsulated by CONFIG_RING_BUFFER when deprecated ring buffer is
 *	removed
 */
#include <zephyr/sys/ring_buffer_deprecated.h>

#ifndef _RING_BUFFER_SHIM_
#define _RING_BUFFER_SHIM_

struct ring_buffer { struct ring_buf rb; };
#define RING_BUFFER_INIT(buff, size) { .rb = RING_BUF_INIT(buff, size) }
#define RING_BUFFER_DEFINE(buff, size) RING_BUF_DECLARE(buff, size)
static inline void ring_buffer_init(struct ring_buffer *rb, uint8_t *buff, size_t size)
{
	ring_buf_init((struct ring_buf *)rb, size, buff);
}
static inline size_t ring_buffer_write(struct ring_buffer *rb, const uint8_t *data, size_t size)
{
	return ring_buf_put((struct ring_buf *)rb, data, size);
}

static inline size_t ring_buffer_read(struct ring_buffer *rb, uint8_t *data, size_t size)
{
	return ring_buf_get((struct ring_buf *)rb, data, size);
}

static inline size_t ring_buffer_size(const struct ring_buffer *rb)
{
	return ring_buf_size_get((const struct ring_buf *)rb);
}

static inline size_t ring_buffer_capacity(const struct ring_buffer *rb)
{
	return ring_buf_capacity_get((const struct ring_buf *)rb);
}
static inline bool ring_buffer_empty(const struct ring_buffer *rb)
{
	return ring_buf_is_empty((const struct ring_buf *)rb);
}
static inline void ring_buffer_reset(struct ring_buffer *rb)
{
	ring_buf_reset((struct ring_buf *)rb);
}
static inline size_t ring_buffer_space(struct ring_buffer *rb)
{
	return ring_buf_space_get((struct ring_buf *)rb);
}
#endif /* _RING_BUFFER_SHIM_ */

#else

#ifndef ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_
#define ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <stdint.h>
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

#ifdef CONFIG_RING_BUFFER_LARGE
typedef size_t ring_buffer_index_t;
typedef size_t ring_buffer_size_t;
#else
typedef uint16_t ring_buffer_index_t;
typedef uint16_t ring_buffer_size_t;
#endif
#define RING_BUFFER_MAX_SIZE ((size_t)((ring_buffer_index_t)-1))
#define RING_BUFFER_SIZE_ERROR_MSG \
	"ring buffer size exceeds maximum for indexing type"
/** @endcond */

/**
 * @brief A structure to represent a ring buffer
 */
struct ring_buffer {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buffer;
	ring_buffer_size_t size;
	ring_buffer_index_t read_ptr;
	ring_buffer_index_t write_ptr;
	#ifdef CONFIG_RING_BUFFER_CONDITIONAL
	bool full;
	#endif
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

/** @endcond */

#define RING_BUFFER_INIT(buf, sz)		\
{						\
	.buffer = buf,				\
	.size = sz,				\
	.read_ptr = 0,				\
	.write_ptr = 0,				\
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
 * @code extern struct ring_buffer <name>; @endcode
 *
 * @param name  Name of the ring buffer.
 * @param size Size of ring buffer (in bytes).
 */
#define RING_BUFFER_DEFINE(name, size)							\
	static uint8_t __noinit _ring_buffer_data_##name[size];				\
	struct ring_buffer name = RING_BUFFER_INIT(_ring_buffer_data_##name, size);	\
	BUILD_ASSERT(size <= RING_BUFFER_MAX_SIZE, RING_BUFFER_SIZE_ERROR_MSG)

/**
 * @brief Initialize a ring buffer.
 *
 * This routine initializes a ring buffer.
 *
 * @param rb Address of ring buffer.
 * @param data Ring buffer data area (uint8_t data[size]).
 * @param size Ring buffer size (in bytes).
 */
void ring_buffer_init(struct ring_buffer *rb, uint8_t *data, size_t size);

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param rb Address of ring buffer.
 *
 * @return true if the ring buffer is empty, else false.
 */
bool ring_buffer_empty(const struct ring_buffer *rb);

/**
 * @brief Determine if a ring buffer is full.
 *
 * @param rb Address of ring buffer.
 *
 * @return true if the ring buffer is full, else false.
 */
bool ring_buffer_full(const struct ring_buffer *rb);


/**
 * @brief Reset ring buffer state.
 *
 * @param rb Address of ring buffer.
 */
void ring_buffer_reset(struct ring_buffer *rb);

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer free space (in bytes).
 */
size_t ring_buffer_space(const struct ring_buffer *rb);

/**
 * @brief Return ring buffer capacity.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer capacity (in bytes).
 */
size_t ring_buffer_capacity(const struct ring_buffer *rb);

/**
 * @brief Determine size of available data in a ring buffer.
 *
 * @param rb Address of ring buffer.
 *
 * @return Ring buffer data size (in bytes).
 */
size_t ring_buffer_size(const struct ring_buffer *rb);

/**
 * @brief Request buffer for writing data to a ring buffer directly.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is written to allotted area
 * number of bytes written must be confirmed (see @ref ring_buffer_commit).
 *
 * @param rb Address of ring buffer.
 * @param data Pointer to the write address set within the ring buffer.
 *
 * @return Size of the allotted buffer.
 */
size_t ring_buffer_write_ptr(const struct ring_buffer *rb, uint8_t **data);

/**
 * @brief confirm number of bytes written to claimed buffer.
 *
 * The number of bytes must be equal to or lower than the preceding @ref ring_buffer_write_ptr.
 *
 * @param  rb Address of ring buffer.
 * @param  size Number of valid bytes in the allocated buffers.
 */
void ring_buffer_commit(struct ring_buffer *rb, size_t size);

/**
 * @brief Write (copy) data to a ring buffer.
 *
 * This routine writes data to a ring buffer @a rb.
 *
 * @param rb Address of ring buffer.
 * @param data Address of data.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written.
 */
size_t ring_buffer_write(struct ring_buffer *rb, const uint8_t *data, size_t size);

/**
 * @brief Get address of a valid data in a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is processed it must be confirmed
 * using @ref ring_buffer_consume.
 *
 * @param rb Address of ring buffer.
 * @param data Pointer to the address. It is set to a location within ring buffer.
 *
 * @return Number of bytes available in the buffer.
 */
size_t ring_buffer_read_ptr(const struct ring_buffer *rb, uint8_t **data);

/**
 * @brief Indicate number of bytes read from claimed buffer.
 *
 * The number of bytes must be less or equal to the preceding @ref ring_buffer_read_ptr.
 * Surplus bytes will remain available in the buffer.
 *
 * @param  rb Address of ring buffer.
 * @param  size Number of bytes that have been consumed from the buffer.
 *
 */
void ring_buffer_consume(struct ring_buffer *rb, size_t size);

/**
 * @brief Read data from a ring buffer.
 *
 * This routine reads data from a ring buffer @a buf.
 *
 * @param rb Address of ring buffer.
 * @param data Address of the output buffer.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes available in the data buffer.
 */
size_t ring_buffer_read(struct ring_buffer *rb, uint8_t *data, size_t size);

/**
 * @brief Peek at data from a ring buffer.
 *
 * This routine reads data from a ring buffer @a buf without consuming.
 *
 * @param rb Address of ring buffer.
 * @param data Address of the output buffer.
 * @param size Data size (in bytes).
 *
 * @retval Number of bytes written to the output buffer.
 */
size_t ring_buffer_peek(struct ring_buffer *rb, uint8_t *data, size_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RING_BUFFER_H_ */
#endif /* CONFIG_RING_BUFFER */
