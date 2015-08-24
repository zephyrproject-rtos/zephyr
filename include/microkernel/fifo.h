/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

/**
 * @file
 *
 * @brief Microkernel FIFO header file.
 *
 */

#ifndef FIFO_H
#define FIFO_H

#include <sections.h>

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

extern int _task_fifo_put(kfifo_t queue, void *data, int32_t time);
extern int _task_fifo_get(kfifo_t queue, void *data, int32_t time);
extern int _task_fifo_ioctl(kfifo_t queue, int op);

/**
 * @brief FIFO enqueue request
 *
 * This routine puts an entry at the end of the FIFO queue.
 *
 * @param q FIFO queue.
 * @param p Pointer to data to add to queue.
 *
 * @return RC_OK  on success, RC_FAIL on failure.
 */
#define task_fifo_put(q, p) _task_fifo_put(q, p, TICKS_NONE)

/**
 * @brief FIFO enqueue request with waiting.
 *
 * This routine tries to put an entry at the end of the FIFO queue.
 *
 * @param q FIFO queue.
 * @param p Pointer to data to add to queue.
 *
 * @return RC_OK  on success, RC_FAIL on failure.
 */
#define task_fifo_put_wait(q, p) _task_fifo_put(q, p, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief FIFO enqueue request with a time out.
 *
 * This routine puts an entry at the end of the FIFO queue with a time out.
 *
 * @param q FIFO queue.
 * @param p Pointer to data to add to queue.
 * @param t Maximum number of ticks to wait.
 *
 * @return RC_OK on success, RC_FAIL on failure, RC_TIME on timeout.
 */
#define task_fifo_put_wait_timeout(q, p, t) _task_fifo_put(q, p, t)
#endif

/**
 * @brief FIFO dequeue request
 *
 * This routine tries to read a data element from the FIFO.
 *
 * If the FIFO is not empty, the oldest entry is removed and copied to the
 * address provided by the caller.
 *
 * @param q FIFO queue.
 * @param p Pointer to storage location of the FIFO entry.
 *
 * @return RC_OK on success, RC_FAIL on failure.
 */
#define task_fifo_get(q, p) _task_fifo_get(q, p, TICKS_NONE)

/**
 * @brief FIFO dequeue request
 *
 * This routine tries to read a data element from the FIFO with wait.
 *
 * If the FIFO is not empty, the oldest entry is removed and copied to the
 * address provided by the caller.
 *
 * @param q FIFO queue.
 * @param p Pointer to storage location of the FIFO entry.
 *
 * @return RC_OK on success, RC_FAIL on failure.
 */
#define task_fifo_get_wait(q, p) _task_fifo_get(q, p, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 *
 * @brief FIFO dequeue request
 *
 * This routine tries to read a data element from the FIFO.
 *
 * If the FIFO is not empty, the oldest entry is removed and copied to the
 * address provided by the caller.
 *
 * @param q FIFO queue.
 * @param p Pointer to storage location of the FIFO entry.
 * @param t Maximum number of ticks to wait.
 *
 * @return RC_OK on success, RC_FAIL on failure, RC_TIME on timeout.
 */
#define task_fifo_get_wait_timeout(q, p, t) _task_fifo_get(q, p, t)
#endif

/**
 * @brief Queries the number of FIFO entries.
 *
 * @param q FIFO queue.
 *
 * @return # of FIFO entries on query.
 */
#define task_fifo_size_get(q) _task_fifo_ioctl(q, 0)

/**
 * @brief Purge the FIFO of all its entries.
 *
 * @return RC_OK on purge.
 */
#define task_fifo_purge(q) _task_fifo_ioctl(q, 1)

/**
 * @brief Initializer for microkernel FIFO
 */
#define __K_FIFO_DEFAULT(depth, width, buffer) \
	{ \
	  .Nelms = depth,\
	  .element_size = width,\
	  .base = buffer,\
	  .end_point = (buffer + (depth * width)),\
	  .enqueue_point = buffer,\
	  .Deqp = buffer,\
	  .waiters = NULL,\
	  .Nused = 0,\
	  .Hmark = 0,\
	  .count = 0,\
	}

/**
 * @brief Define a private microkernel FIFO
 *
 * This declares and initializes a private FIFO. The new FIFO
 * can be passed to the microkernel FIFO functions.
 *
 * @param name Name of the FIFO
 */
#define DEFINE_FIFO(name, depth, width) \
	static char __noinit __##name_buffer[(depth * width)]; \
	struct _k_fifo_struct _k_fifo_obj_##name = \
	       __K_FIFO_DEFAULT(depth, width, __##name_buffer); \
	const kfifo_t name = (kfifo_t)&_k_fifo_obj_##name;

#ifdef __cplusplus
}
#endif

#endif /* FIFO_H */
