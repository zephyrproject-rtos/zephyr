/* microkernel/chan.h */

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

#ifndef CHAN_H
#define CHAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* GENERIC channel interface functions (map to host / target channels) */

#define _DEVICE_CHANNEL 0x00008000 /* mask for channels to a ser/par dev */

extern int task_pipe_put(kpipe_t id,
			 void *buffer,
			 int NrOfBytesToWrite,
			 int *NrOfBytesWritten,
			 K_PIPE_OPTION opt);
extern int task_pipe_put_wait(kpipe_t id,
			  void *buffer,
			  int NrOfBytesToWrite,
			  int *NrOfBytesWritten,
			  K_PIPE_OPTION opt);
extern int task_pipe_put_wait_timeout(kpipe_t id,
			   void *buffer,
			   int NrOfBytesToWrite,
			   int *NrOfBytesWritten,
			   K_PIPE_OPTION opt,
			   int32_t TimeOut);
extern int task_pipe_get(kpipe_t id,
			 void *buffer,
			 int NrOfBytesToRead,
			 int *NrOfBytesRead,
			 K_PIPE_OPTION opt);
extern int task_pipe_get_wait(kpipe_t id,
			  void *buffer,
			  int NrOfBytesToRead,
			  int *NrOfBytesRead,
			  K_PIPE_OPTION opt);
extern int task_pipe_get_wait_timeout(kpipe_t id,
			   void *buffer,
			   int NrOfBytesToRead,
			   int *NrOfBytesRead,
			   K_PIPE_OPTION opt,
			   int32_t timeout);

extern int _task_pipe_put_async(kpipe_t id, struct k_block block, int size, ksem_t sema);

#define task_pipe_put_async(Id, block, size, sema) \
	_task_pipe_put_async(Id, block, size, sema)


typedef int (*PFN_CHANNEL_RW)(kpipe_t, void *, int, int *, K_PIPE_OPTION);
typedef int (*PFN_CHANNEL_RWT)(kpipe_t,
			       void *,
			       int,
			       int *,
			       K_PIPE_OPTION,
			       int32_t);

extern PFN_CHANNEL_RWT pKS_Channel_PutWT; /* maps to KS__ChannelPutWT
							== _task_pipe_put */

extern PFN_CHANNEL_RWT pKS_Channel_GetWT; /* maps to KS__ChannelGetWT
							== _task_pipe_get */

/* mapping of KS__ChannelXXX() to _task_pipe_xxx() functions */
#define KS__ChannelPutWT _task_pipe_put

#define KS__ChannelGetWT _task_pipe_get

/* base API functions */

extern void InitPipe(void);

extern int _task_pipe_put(kpipe_t id,
		       void *pBuffer,
		       int iNbrBytesToWrite,
		       int *piNbrBytesWritten,
		       K_PIPE_OPTION Option,
		       int32_t TimeOut);

extern int _task_pipe_get(kpipe_t id,
		       void *pBuffer,
		       int iNbrBytesToRead,
		       int *piNbrBytesRead,
		       K_PIPE_OPTION Option,
		       int32_t TimeOut);

#ifdef __cplusplus
}
#endif

#endif /* CHAN_H */
