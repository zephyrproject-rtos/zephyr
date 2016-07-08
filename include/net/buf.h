/** @file
 *  @brief Buffer management.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __NET_BUF_H
#define __NET_BUF_H

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <misc/util.h>
#include <nanokernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Alignment needed for various parts of the buffer definition */
#define __net_buf_align __aligned(sizeof(int))

/** Flag indicating that the buffer has associated fragments. Only used
  * internally by the buffer handling code while the buffer is inside a
  * FIFO, meaning this never needs to be explicitly set or unset by the
  * net_buf API user. As long as the buffer is outside of a FIFO, i.e.
  * in practice always for the user for this API, the buf->frags pointer
  * should be used instead.
  */
#define NET_BUF_FRAGS        BIT(0)

struct net_buf {
	union {
		/** FIFO uses first 4 bytes itself, reserve space */
		int _unused;

		/** Fragments associated with this buffer. */
		struct net_buf *frags;
	};

	/** Size of the user data associated with this buffer. */
	const uint16_t user_data_size;

	/** Reference count. */
	uint8_t ref;

	/** Bit-field of buffer flags. */
	uint8_t flags;

	/** Pointer to the start of data in the buffer. */
	uint8_t *data;

	/** Length of the data behind the data pointer. */
	uint16_t len;

	/** Amount of data that this buffer can store. */
	const uint16_t size;

	/** Where the buffer should go when freed up. */
	struct nano_fifo * const free;

	/** Function to be called when the buffer is freed. */
	void (*const destroy)(struct net_buf *buf);

	/** Start of the data storage. Not to be accessed directly
	 *  (the data pointer should be used instead).
	 */
	uint8_t __buf[0] __net_buf_align;
};

/**
 *  @brief Define a pool of buffers of a certain amount and size.
 *
 *  Defines the necessary memory space (array of structs) for the needed
 *  amount of buffers. After this the net_buf_pool_init() API still
 *  needs to be used (at runtime), after which the buffers can be
 *  accessed using the fifo given as one of the parameters.
 *
 *  If provided with a custom destroy callback this callback is
 *  responsible for eventually returning the buffer back to the free
 *  buffers FIFO through nano_fifo_put(buf->free, buf).
 *
 *  @param _name     Name of buffer pool.
 *  @param _count    Number of buffers in the pool.
 *  @param _size     Maximum data size for each buffer.
 *  @param _fifo     FIFO for the buffers when they are unused.
 *  @param _destroy  Optional destroy callback when buffer is freed.
 *  @param _ud_size  Amount of user data space to reserve.
 */
#define NET_BUF_POOL(_name, _count, _size, _fifo, _destroy, _ud_size)	\
	struct {							\
		struct net_buf buf;					\
		uint8_t data[_size] __net_buf_align;	                \
		uint8_t ud[ROUND_UP(_ud_size, 4)] __net_buf_align;	\
	} _name[_count] = {						\
		[0 ... (_count - 1)] = { .buf = {			\
			.user_data_size = ROUND_UP(_ud_size, 4),	\
			.free = _fifo,					\
			.destroy = _destroy,				\
			.size = _size } },			        \
	}

/**
 *  @brief Initialize an available buffers FIFO based on a pool.
 *
 *  Initializes a buffer pool created using NET_BUF_POOL(). After
 *  calling this API the buffers can ge accessed through the FIFO that
 *  was given to NET_BUF_POOL(), i.e. after this call there should be no
 *  need to access the buffer pool (struct array) directly anymore.
 *
 *  @param pool  Buffer pool to initialize.
 */
#define net_buf_pool_init(pool)						\
	do {								\
		int i;							\
									\
		nano_fifo_init(pool[0].buf.free);			\
									\
		for (i = 0; i < ARRAY_SIZE(pool); i++) {		\
			nano_fifo_put(pool[i].buf.free, &pool[i]);	\
		}							\
	} while (0)

/**
 *  @brief Get a new buffer from a FIFO.
 *
 *  Get buffer from a FIFO. The reserve_head parameter is only relevant
 *  if the FIFO in question is a free buffers pool, i.e. the buffer will
 *  end up being initialized upon return. If called for any other FIFO
 *  the reserve_head parameter will be ignored and should be set to 0.
 *
 *  @param fifo Which FIFO to take the buffer from.
 *  @param reserve_head How much headroom to reserve.
 *
 *  @return New buffer or NULL if out of buffers.
 *
 *  @warning If there are no available buffers and the function is
 *  called from a task or fiber the call will block until a buffer
 *  becomes available in the FIFO. If you want to make sure no blocking
 *  happens use net_buf_get_timeout() instead with TICKS_NONE.
 */
struct net_buf *net_buf_get(struct nano_fifo *fifo, size_t reserve_head);

/**
 *  @brief Get a new buffer from a FIFO.
 *
 *  Get buffer from a FIFO. The reserve_head parameter is only relevant
 *  if the FIFO in question is a free buffers pool, i.e. the buffer will
 *  end up being initialized upon return. If called for any other FIFO
 *  the reserve_head parameter will be ignored and should be set to 0.
 *
 *  @param fifo Which FIFO to take the buffer from.
 *  @param reserve_head How much headroom to reserve.
 *  @param timeout Affects the action taken should the FIFO be empty.
 *         If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then
 *         wait as long as necessary. Otherwise, wait up to the specified
 *         number of ticks before timing out.
 *
 *  @return New buffer or NULL if out of buffers.
 */
struct net_buf *net_buf_get_timeout(struct nano_fifo *fifo,
				    size_t reserve_head, int32_t timeout);

/**
 *  @brief Initialize buffer with the given headroom.
 *
 *  Initializes a buffer with a given headroom. The buffer is not expected to
 *  contain any data when this API is called.
 *
 *  @param buf Buffer to initialize.
 *  @param reserve How much headroom to reserve.
 */
void net_buf_reserve(struct net_buf *buf, size_t reserve);

/**
 *  @brief Put a buffer into a FIFO
 *
 *  Put a buffer to the end of a FIFO. If the buffer contains follow-up
 *  fragments this function will take care of inserting them as well
 *  into the FIFO.
 *
 *  @param fifo Which FIFO to put the buffer to.
 *  @param buf Buffer.
 */
void net_buf_put(struct nano_fifo *fifo, struct net_buf *buf);

/**
 *  @brief Decrements the reference count of a buffer.
 *
 *  Decrements the reference count of a buffer and puts it back into the
 *  pool if the count reaches zero.
 *
 *  @param buf A valid pointer on a buffer
 */
void net_buf_unref(struct net_buf *buf);

/**
 *  @brief Increment the reference count of a buffer.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return the buffer newly referenced
 */
struct net_buf *net_buf_ref(struct net_buf *buf);

/**
 *  @brief Duplicate buffer
 *
 *  Duplicate given buffer including any data and headers currently stored.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return Duplicated buffer or NULL if out of buffers.
 */
struct net_buf *net_buf_clone(struct net_buf *buf);

/**
 *  @brief Get a pointer to the user data of a buffer.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return Pointer to the user data of the buffer.
 */
static inline void *net_buf_user_data(struct net_buf *buf)
{
	return (void *)ROUND_UP((buf->__buf + buf->size), sizeof(int));
}

/**
 *  @brief Prepare data to be added at the end of the buffer
 *
 *  Increments the data length of a buffer to account for more data
 *  at the end.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to increment the length with.
 *
 *  @return The original tail of the buffer.
 */
void *net_buf_add(struct net_buf *buf, size_t len);

/**
 *  @brief Add (8-bit) byte at the end of the buffer
 *
 *  Adds a byte at the end of the buffer. Increments the data length of
 *  the  buffer to account for more data at the end.
 *
 *  @param buf Buffer to update.
 *  @param value byte value to be added.
 *
 *  @return Pointer to the value added
 */
uint8_t *net_buf_add_u8(struct net_buf *buf, uint8_t value);

/**
 *  @brief Add 16-bit value at the end of the buffer
 *
 *  Adds 16-bit value in little endian format at the end of buffer.
 *  Increments the data length of a buffer to account for more data
 *  at the end.
 *
 *  @param buf Buffer to update.
 *  @param value 16-bit value to be added.
 */
void net_buf_add_le16(struct net_buf *buf, uint16_t value);

/**
 *  @brief Add 32-bit value at the end of the buffer
 *
 *  Adds 32-bit value in little endian format at the end of buffer.
 *  Increments the data length of a buffer to account for more data
 *  at the end.
 *
 *  @param buf Buffer to update.
 *  @param value 32-bit value to be added.
 */
void net_buf_add_le32(struct net_buf *buf, uint32_t value);

/**
 *  @brief Push data to the beginning of the buffer.
 *
 *  Modifies the data pointer and buffer length to account for more data
 *  in the beginning of the buffer.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to add to the beginning.
 *
 *  @return The new beginning of the buffer data.
 */
void *net_buf_push(struct net_buf *buf, size_t len);

/**
 *  @brief Push 16-bit value to the beginning of the buffer
 *
 *  Adds 16-bit value in little endian format to the beginning of the
 *  buffer.
 *
 *  @param buf Buffer to update.
 *  @param value 16-bit value to be pushed to the buffer.
 */
void net_buf_push_le16(struct net_buf *buf, uint16_t value);

/**
 *  @brief Remove data from the beginning of the buffer.
 *
 *  Removes data from the beginnig of the buffer by modifying the data
 *  pointer and buffer length.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to remove.
 *
 *  @return New beginning of the buffer data.
 */
void *net_buf_pull(struct net_buf *buf, size_t len);

/**
 *  @brief Remove a 8-bit value from the beginning of the buffer
 *
 *  Same idea as with bt_buf_pull(), but a helper for operating on
 *  8-bit values.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return The 8-bit removed value
 */
uint8_t net_buf_pull_u8(struct net_buf *buf);

/**
 *  @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 *  Same idea as with bt_buf_pull(), but a helper for operating on
 *  16-bit little endian data.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return 16-bit value converted from little endian to host endian.
 */
uint16_t net_buf_pull_le16(struct net_buf *buf);

/**
 *  @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 *  Same idea as with bt_buf_pull(), but a helper for operating on
 *  32-bit little endian data.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return 32-bit value converted from little endian to host endian.
 */
uint32_t net_buf_pull_le32(struct net_buf *buf);

/**
 *  @brief Check buffer tailroom.
 *
 *  Check how much free space there is at the end of the buffer.
 *
 *  @param buf A valid pointer on a buffer
 *
 *  @return Number of bytes available at the end of the buffer.
 */
size_t net_buf_tailroom(struct net_buf *buf);

/**
 *  @brief Check buffer headroom.
 *
 *  Check how much free space there is in the beginning of the buffer.
 *
 *  buf A valid pointer on a buffer
 *
 *  @return Number of bytes available in the beginning of the buffer.
 */
size_t net_buf_headroom(struct net_buf *buf);

/**
 *  @brief Get the tail pointer for a buffer.
 *
 *  Get a pointer to the end of the data in a buffer.
 *
 *  @param buf Buffer.
 *
 *  @return Tail pointer for the buffer.
 */
static inline uint8_t *net_buf_tail(struct net_buf *buf)
{
	return buf->data + buf->len;
}

#ifdef __cplusplus
}
#endif

#endif /* __NET_BUF_H */
