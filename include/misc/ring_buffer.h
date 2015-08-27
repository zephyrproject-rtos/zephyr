/* ring_buffer.h: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/** @file */

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <nanokernel.h>
#include <misc/util.h>
#include <errno.h>

#define SIZE32_OF(x) (sizeof((x))/sizeof(uint32_t))

/**
 * @brief A structure to represent a ring buffer
 */
struct ring_buf {
	uint32_t head;	 /**< Index in buf for the head element */
	uint32_t tail;	 /**< Index in buf for the tail element */
	uint32_t dropped_put_count; /**< Running tally of the number of failed
				     * put attempts */
	uint32_t size;   /**< Size of buf in 32-bit chunks */
	uint32_t *buf;	 /**< Memory region for stored entries */
        uint32_t mask;   /**< Modulo mask if size is a power of 2 */
};

/**
 * @brief Declare a power-of-two sized ring buffer
 *
 * Use of this macro is preferred over SYS_RING_BUF_DECLARE_SIZE() as it
 * will not need to use expensive modulo operations.
 *
 * @param name File-scoped name of the ring buffer to declare
 * @param pow Create a buffer of 2^pow 32-bit elements */
#define SYS_RING_BUF_DECLARE_POW2(name, pow) \
	static uint32_t _ring_buffer_data_##name[1 << (pow)]; \
	struct ring_buf name = { \
		.size = (1 << (pow)), \
		.mask = (1 << (pow)) - 1, \
		.buf = _ring_buffer_data_##name \
	};

/**
 * @brief Declare an arbitrary sized ring buffer
 *
 * A ring buffer declared in this way has more flexibility on buffer size
 * but will use more expensive modulo operations to maintain itself.
 *
 * @param name File-scoped name of the ring buffer to declare
 * @param size32 Size of buffer in 32-bit elements */
#define SYS_RING_BUF_DECLARE_SIZE(name, size32) \
	static uint32_t _ring_buffer_data_##name[size32]; \
	struct ring_buf name = { \
		.size = size32, \
		.buf = _ring_buffer_data_##name \
	};

/**
 * @brief Initialize a ring buffer, in cases where DECLARE_RING_BUF_STATIC
 * isn't used.
 *
 * For optimal performance, use size values that are a power of 2 as they
 * don't require expensive modulo operations when maintaining the buffer.
 *
 * @param buf Ring buffer to initialize
 * @param size Size of the provided buffer in 32-bit chunks
 * @param data Data area for the ring buffer, typically
 *	  uint32_t data[size]
 */
static inline void sys_ring_buf_init(struct ring_buf *buf, uint32_t size,
				     uint32_t *data)
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
 * @brief Determine if a ring buffer is empty
 *
 * @return nonzero if the buffer is empty */
static inline int sys_ring_buf_is_empty(struct ring_buf *buf)
{
	return (buf->head == buf->tail);
}

/**
 * @brief Obtain available space in a ring buffer
 *
 * @param buf Ring buffer to examine
 * @return Available space in the buffer in 32-bit chunks
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
 * @brief Place an entry into the ring buffer
 *
 * Concurrency control is not implemented, however no synchronization is needed
 * between put() and get() operations as they independently work on the
 * tail and head values, respectively.
 * Any use-cases involving multiple producers will need to synchronize use
 * of this function, by either disabling preemption or using a mutex.
 *
 * @param buf Ring buffer to insert data to
 * @param type Application-specific type identifier
 * @param value Integral data to include, application specific
 * @param data Pointer to a buffer containing data to enqueue
 * @param size32 Size of data buffer, in 32-bit chunks (not bytes)
 * @return 0 on success, -ENOSPC if there isn't sufficient space
 */
int sys_ring_buf_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		     uint32_t *data, uint8_t size32);

/**
 * @brief Fetch data from the ring buffer
 *
 * @param buf Ring buffer to extract data from
 * @param type Return storage of the retrieved event type
 * @param value Return storage of the data value
 * @param data Buffer to copy data into
 * @param size32 Indicates the size of the data buffer. On return,
 *	updated with the actual amount of 32-bit chunks written to the buffer
 * @return 0 on success, -EAGAIN if the ring buffer is empty, -EMSGSIZE
 *	if the supplied buffer is too small (size32 will be updated with
 *	the actual size needed)
 */
int sys_ring_buf_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
		     uint32_t *data, uint8_t *size32);

#endif /* __RING_BUFFER_H__ */
