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
extern int _task_pipe_put(kpipe_t id,
			void *pBuffer,
			int iNbrBytesToWrite,
			int *piNbrBytesWritten,
			K_PIPE_OPTION Option,
			int32_t TimeOut);

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
 * Attempt to write data from a memory buffer area to the specified pipe.
 * Fail immediately if it is not possible.
 *
 * @param i Pipe ID
 * @param b Buffer
 * @param n Number of bytes to write
 * @param pn Pointer to number of bytes written
 * @param o Pipe options
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, or RC_ALIGNMENT
 */
#define task_pipe_put(i, b, n, pn, o) \
			_task_pipe_put(i, b, n, pn, o, TICKS_NONE)

/**
 * @brief Pipe write request with unlimited wait
 *
 * Attempt to write data from a memory buffer area to the
 * specified pipe and wait forever until it succeeds.
 *
 * @param i Pipe ID
 * @param b Buffer
 * @param n Number of bytes to write
 * @param pn Pointer to number of bytes written
 * @param o Pipe options
 *
 * @return RC_OK, RC_INCOMPLETE or RC_ALIGNMENT
 */
#define task_pipe_put_wait(i, b, n, pn, o) \
			_task_pipe_put(i, b, n, pn, o, TICKS_UNLIMITED)

/**
 * @brief Pipe write request with timeout
 *
 * Attemp to write data from a memory buffer area to the
 * specified pipe with a timeout option.
 *
 * @param id Pipe ID
 * @param b Buffer
 * @param n Number of bytes to write
 * @param pn Pointer to number of bytes written
 * @param o Pipe options
 * @param t Timeout
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */
#define task_pipe_put_wait_timeout(id, b, n, pn, o, t) \
			_task_pipe_put(id, b, n, pn, o, t)


extern int _task_pipe_get(kpipe_t id,
			void *pBuffer,
			int iNbrBytesToRead,
			int *piNbrBytesRead,
			K_PIPE_OPTION Option,
			int32_t TimeOut);

/**
 * @brief Pipe read request
 *
 * Attempt to read data into a memory buffer area from the
 * specified pipe and fail immediately if not possible.
 *
 * @param id Pipe ID
 * @param b Buffer
 * @param n Number of bytes to read
 * @param pn Pointer to number of bytes read
 * @param o Pipe options
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, or RC_ALIGNMENT
 */
#define task_pipe_get(id, b, n, pn, o) \
			_task_pipe_get(id, b, n, pn, o, TICKS_NONE)
/**
 * @brief Pipe read request and wait
 *
 * Attempt to read data into a memory buffer area from the
 * specified pipe and wait forever until it succeeds.
 *
 * @param id Pipe ID
 * @param b Buffer
 * @param n Number of bytes to read
 * @param pn Pointer to number of bytes read
 * @param o Pipe options
 *
 * @return RC_OK, RC_INCOMPLETE, or RC_ALIGNMENT
 */
#define task_pipe_get_wait(id, b, n, pn, o) \
			_task_pipe_get(id, b, n, pn, o, TICKS_UNLIMITED)
/**
 * @brief Pipe read request
 *
 * This routine attempts to read data into a memory buffer area from the
 * specified pipe, with a possible timeout option.
 *
 * @param id Pipe ID
 * @param b Buffer
 * @param n Number of bytes to read
 * @param pn Pointer to number of bytes read
 * @param o Pipe options
 * @param t Timeout
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */
#define task_pipe_get_wait_timeout(id, b, n, pn, o, t) \
			_task_pipe_get(id, b, n, pn, o, t)


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
