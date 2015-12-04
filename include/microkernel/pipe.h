/* microkernel/pipe.h */

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

#ifndef PIPE_H
#define PIPE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Microkernel Pipes
 * @defgroup microkernel_pipe Microkernel Pipes
 * @ingroup microkernel_services
 * @{
 */

#include <sections.h>

/**
 * @cond internal
 */

/**
 * @internal
 * @brief Initialize a pipe struct.
 *
 * @param size Size of pipe buffer.
 * @param buffer Pointer to the buffer.
 * @endinternal
 */
#define __K_PIPE_INITIALIZER(size, buffer) \
	{ \
	  .buffer_size = size, \
	  .Buffer = buffer, \
	}

/**
 * @endcond
 */

/**
 * @brief Pipe write request
 *
 * Attempt to write data from a memory buffer area to the
 * specified pipe with a timeout option.
 *
 * @param id Pipe ID
 * @param buffer Buffer
 * @param bytes_to_write Number of bytes to write
 * @param bytes_written Pointer to number of bytes written
 * @param options Pipe options
 * @param timeout Affects the action taken should the pipe be full. If
 * TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as long
 * as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully wrote data to pipe
 * @retval RC_ALIGNMENT Data is improperly aligned
 * @retval RC_INCOMPLETE Only some of the data was written to the pipe when
 * @a options = _ALL_N
 * @retval RC_TIME Timed out waiting to write to pipe
 * @retval RC_FAIL Failed to immediately write to pipe when
 * @a timeout = TICKS_NONE
 */
extern int task_pipe_put(kpipe_t id, void *buffer, int bytes_to_write,
			int *bytes_written, K_PIPE_OPTION options, int32_t timeout);

/**
 * @brief Pipe read request
 *
 * Attempt to read data into a memory buffer area from the
 * specified pipe with a timeout option.
 *
 * @param id Pipe ID
 * @param buffer Buffer
 * @param bytes_to_read Number of bytes to read
 * @param bytes_read Pointer to number of bytes read
 * @param options Pipe options
 * @param timeout Affects the action taken should the pipe be empty. If
 * TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as long
 * as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully read data from pipe
 * @retval RC_ALIGNMENT Data is improperly aligned
 * @retval RC_INCOMPLETE Only some of the data was read from the pipe when
 * @a options = _ALL_N
 * @retval RC_TIME Timed out waiting to read from pipe
 * @retval RC_FAIL Failed to immediately read from pipe when
 * @a timeout = TICKS_NONE
 */
extern int task_pipe_get(kpipe_t id, void *buffer, int bytes_to_read,
			int *bytes_read, K_PIPE_OPTION options, int32_t timeout);

extern int _task_pipe_block_put(kpipe_t id,
				struct k_block block,
				int size,
				ksem_t sema);

/**
 * @brief Asynchronous pipe write request
 *
 * This routine attempts to write data from a memory pool block to the
 * specified pipe. (Note that partial transfers and timeouts are not
 * supported, unlike the case for synchronous write requests.)
 *
 * @param id pipe ID
 * @param block Block
 * @param size Size of data to be transferred
 * @param sema Semphore ID
 *
 * @return RC_OK, RC_FAIL, or RC_ALIGNMENT
 */
#define task_pipe_block_put(id, block, size, sema) \
			_task_pipe_block_put(id, block, size, sema)


/**
 * @brief Define a private microkernel pipe.
 *
 * @param name Name of the pipe.
 * @param size Size of the pipe buffer, in bytes.
 */
#define DEFINE_PIPE(name, size) \
	char __noinit __pipe_buffer_##name[size]; \
	struct _k_pipe_struct _k_pipe_obj_##name = \
		__K_PIPE_INITIALIZER(size, __pipe_buffer_##name); \
	const kpipe_t name \
		__in_section(_k_pipe_ptr, private, pipe) = \
		(kpipe_t)&_k_pipe_obj_##name;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PIPE_H */
