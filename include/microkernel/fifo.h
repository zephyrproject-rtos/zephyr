/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

/**
 * @file
 *
 * @brief Microkernel FIFO header file.
 *
 */

/**
 * @brief Microkernel FIFOs
 * @defgroup microkernel_fifo Microkernel FIFOs
 * @ingroup microkernel_services
 * @{
 */

#ifndef FIFO_H
#define FIFO_H

#include <sections.h>

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond internal
 */
extern int _task_fifo_get(kfifo_t queue, void *data, int32_t time);
extern int _task_fifo_ioctl(kfifo_t queue, int op);

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
	  .dequeue_point = buffer,\
	  .waiters = NULL,\
	  .num_used = 0,\
	  .high_watermark = 0,\
	  .count = 0,\
	}

/**
 * @endcond
 */

/**
 * @brief FIFO enqueue request
 *
 * This routine adds an item to the FIFO queue. If the FIFO is currently full
 * then the routine either waits until space becomes available, or until the
 * specified time limit is reached.
 *
 * @param queue FIFO queue.
 * @param data Pointer to data to add to queue.
 * @param timeout Affects the action taken should the FIFO be full. If
 * TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as long
 * as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully added item to FIFO
 * @retval RC_TIME Timed out while waiting to add item to FIFO
 * @retval RC_FAIL Failed to immediately add item to FIFO when
 * @a timeout = TICKS_NONE
 */
extern int task_fifo_put(kfifo_t queue, void *data, int32_t timeout);

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
 * @brief Define a private microkernel FIFO
 *
 * This declares and initializes a private FIFO. The new FIFO
 * can be passed to the microkernel FIFO functions.
 *
 * @param name Name of the FIFO
 * @param depth Depth of the FIFO
 * @param width Width of the FIFO
 */
#define DEFINE_FIFO(name, depth, width) \
	static char __noinit __##name_buffer[(depth * width)]; \
	struct _k_fifo_struct _k_fifo_obj_##name = \
	       __K_FIFO_DEFAULT(depth, width, __##name_buffer); \
	const kfifo_t name = (kfifo_t)&_k_fifo_obj_##name;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FIFO_H */
