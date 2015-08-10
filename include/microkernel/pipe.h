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

#include <sections.h>

extern int _task_pipe_put(kpipe_t id,
						  void *pBuffer,
						  int iNbrBytesToWrite,
						  int *piNbrBytesWritten,
						  K_PIPE_OPTION Option,
						  int32_t TimeOut);

#define task_pipe_put(i, b, n, pn, o) \
			_task_pipe_put(i, b, n, pn, o, TICKS_NONE)
#define task_pipe_put_wait(i, b, n, pn, o) \
			_task_pipe_put(i, b, n, pn, o, TICKS_UNLIMITED)
#define task_pipe_put_wait_timeout(i, b, n, pn, o, t) \
			_task_pipe_put(i, b, n, pn, o, t)


extern int _task_pipe_get(kpipe_t id,
						  void *pBuffer,
						  int iNbrBytesToRead,
						  int *piNbrBytesRead,
						  K_PIPE_OPTION Option,
						  int32_t TimeOut);

#define task_pipe_get(i, b, n, pn, o) \
			_task_pipe_get(i, b, n, pn, o, TICKS_NONE)
#define task_pipe_get_wait(i, b, n, pn, o) \
			_task_pipe_get(i, b, n, pn, o, TICKS_UNLIMITED)
#define task_pipe_get_wait_timeout(i, b, n, pn, o, t) \
			_task_pipe_get(i, b, n, pn, o, t)


extern int _task_pipe_block_put(kpipe_t id,
								struct k_block block,
								int size,
								ksem_t sema);

#define task_pipe_block_put(id, block, size, sema) \
			_task_pipe_block_put(id, block, size, sema)

/**
 * @brief Initialize a pipe struct.
 *
 * @param size Size of pipe buffer.
 * @param buffer Pointer to the buffer.
 */
#define __K_PIPE_INITIALIZER(size, buffer) \
	{ \
	  .iBufferSize = size, \
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

#ifdef __cplusplus
}
#endif

#endif /* PIPE_H */
