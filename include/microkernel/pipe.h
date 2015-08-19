/* microkernel/pipe.h */

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

#ifndef PIPE_H
#define PIPE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pipes
 * @defgroup microkernel_pipes Microkernel Pipes
 * @ingroup microkernel_services
 * @{
 */

#include <sections.h>

extern int _task_pipe_put(kpipe_t id,
			void *pBuffer,
			int iNbrBytesToWrite,
			int *piNbrBytesWritten,
			K_PIPE_OPTION Option,
			int32_t TimeOut);

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
		__section(_k_pipe_ptr, private, pipe) = \
		(kpipe_t)&_k_pipe_obj_##name;

/*
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* PIPE_H */
