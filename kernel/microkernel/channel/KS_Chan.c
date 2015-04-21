/* KS_Chan.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

/* includes */

#include <microkernel/k_struct.h>
#include <kchan.h>
#include <minik.h>
#include <microkernel/chan.h>
#include <toolchain.h>

/*******************************************************************************
*
* task_pipe_put - put data on a channel
*
* RETURNS: RC_OK or RC_FAIL
*/

int task_pipe_put(kpipe_t id,
		  void *buffer,
		  int NrOfBytesToWrite,
		  int *NrOfBytesWritten,
		  K_PIPE_OPTION opt)
{
	if (unlikely(pKS_Channel_PutWT == NULL))
		return RC_FAIL; /* or an assert */

	return (*pKS_Channel_PutWT)(id, buffer,
					NrOfBytesToWrite,
					NrOfBytesWritten,
					opt, TICKS_NONE);
}

/*******************************************************************************
*
* task_pipe_put_wait - put data on a channel
*
* RETURNS: RC_OK or RC_FAIL
*/

int task_pipe_put_wait(kpipe_t id,
		   void *buffer,
		   int NrOfBytesToWrite,
		   int *NrOfBytesWritten,
		   K_PIPE_OPTION opt)
{
	if (unlikely(pKS_Channel_PutWT == NULL))
		return RC_FAIL; /* or an assert */

	return (*pKS_Channel_PutWT)(id, buffer,
					NrOfBytesToWrite,
					NrOfBytesWritten,
					opt, TICKS_UNLIMITED);
}

/*******************************************************************************
*
* task_pipe_put_wait_timeout - put data on a channel
*
* RETURNS: RC_OK, RC_TIME or RC_FAIL
*/

int task_pipe_put_wait_timeout(kpipe_t id,
		    void *buffer,
		    int NrOfBytesToWrite,
		    int *NrOfBytesWritten,
		    K_PIPE_OPTION opt,
		    int32_t TimeOut)
{
	if (unlikely(pKS_Channel_PutWT == NULL))
		return RC_FAIL; /* or an assert */

	return (*pKS_Channel_PutWT)(id, buffer,
					NrOfBytesToWrite,
					NrOfBytesWritten,
					opt, TimeOut);
}

/*******************************************************************************
*
* task_pipe_get - get data off a channel
*
* RETURNS: RC_OK or RC_FAIL
*/

int task_pipe_get(kpipe_t id,
		  void *buffer,
		  int NrOfBytesToRead,
		  int *NrOfBytesRead,
		  K_PIPE_OPTION opt)
{
	if (unlikely(pKS_Channel_GetWT == NULL))
		return RC_FAIL; /* or an assert */

	return (*pKS_Channel_GetWT)(id, buffer,
					NrOfBytesToRead,
					NrOfBytesRead,
					opt, TICKS_NONE);
}

/*******************************************************************************
*
* task_pipe_get_wait - get data off a channel
*
* RETURNS: RC_OK or RC_FAIL
*/

int task_pipe_get_wait(kpipe_t id,
		   void *buffer,
		   int NrOfBytesToRead,
		   int *NrOfBytesRead,
		   K_PIPE_OPTION opt)
{
	if (unlikely(pKS_Channel_GetWT == NULL))
		return RC_FAIL; /* or an assert */
	return (*pKS_Channel_GetWT)(id, buffer,
					NrOfBytesToRead,
					NrOfBytesRead,
					opt, TICKS_UNLIMITED);
}

/*******************************************************************************
*
* task_pipe_get_wait_timeout - get data off a channel
*
* RETURNS: RC_OK, RC_TIME or RC_FAIL
*/

int task_pipe_get_wait_timeout(kpipe_t id,
		    void *buffer,
		    int NrOfBytesToRead,
		    int *NrOfBytesRead,
		    K_PIPE_OPTION opt,
		    int32_t TimeOut)
{
	if (unlikely(pKS_Channel_GetWT == NULL))
		return RC_FAIL; /* or an assert */
	return (*pKS_Channel_GetWT)(id, buffer,
					NrOfBytesToRead,
					NrOfBytesRead,
					opt, TimeOut);
}
