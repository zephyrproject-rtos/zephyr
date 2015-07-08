/* pipe kernel services */

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

#include <micro_private.h>
#include <k_pipe_buffer.h>
#include <k_pipe_util.h>
#include <misc/util.h>

/**
 *
 * @brief Initialize kernel pipe subsystem
 *
 * Performs any initialization of statically-defined pipes that wasn't done
 * at build time. (Note: most pipe structure fields are set to zero by sysgen.)
 *
 * @return N/A
 */

void _k_pipe_init(void)
{
	int i;
	struct pipe_struct *pPipe;

	for (i = 0, pPipe = _k_pipe_list; i < _k_pipe_count; i++, pPipe++) {
		BuffInit((unsigned char *)pPipe->Buffer,
				 &(pPipe->iBufferSize), &pPipe->desc);
	}
}

/**
 *
 * @brief Pipe read request
 *
 * This routine attempts to read data into a memory buffer area from the
 * specified pipe.
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */

int _task_pipe_get(kpipe_t Id, void *pBuffer,
				   int iNbrBytesToRead, int *piNbrBytesRead,
				   K_PIPE_OPTION Option, int32_t TimeOut)
{
	struct k_args A;

	/* some users do not check the FUNCTION return value,
	   but immediately use iNbrBytesRead; make sure it always
	   has a good value, even when we return failure immediately
	   (see below) */

	*piNbrBytesRead = 0;

	if (unlikely(iNbrBytesToRead % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iNbrBytesToRead)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}
	if (unlikely(_0_TO_N == Option && TICKS_NONE != TimeOut)) {
		return RC_FAIL;
	}

	A.Prio = _k_current_task->Prio;
	A.Comm = PIPE_GET_REQUEST;
	A.Time.ticks = TimeOut;

	A.Args.pipe_req.ReqInfo.pipe.id = Id;
	A.Args.pipe_req.ReqType.Sync.iSizeTotal = iNbrBytesToRead;
	A.Args.pipe_req.ReqType.Sync.pData = pBuffer;

	_k_pipe_option_set(&A.Args, Option);
	_k_pipe_request_type_set(&A.Args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*piNbrBytesRead = A.Args.pipe_ack.iSizeXferred;
	return A.Time.rcode;
}

/**
 *
 * @brief Pipe write request
 *
 * This routine attempts to write data from a memory buffer area to the
 * specified pipe.
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */

int _task_pipe_put(kpipe_t Id, void *pBuffer,
				   int iNbrBytesToWrite, int *piNbrBytesWritten,
				   K_PIPE_OPTION Option, int32_t TimeOut)
{
	struct k_args A;

	/* some users do not check the FUNCTION return value,
	   but immediately use iNbrBytesWritten; make sure it always
	   has a good value, even when we return failure immediately
	   (see below) */

	*piNbrBytesWritten = 0;

	if (unlikely(iNbrBytesToWrite % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iNbrBytesToWrite)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}
	if (unlikely(_0_TO_N == Option && TICKS_NONE != TimeOut)) {
		return RC_FAIL;
	}

	A.Prio = _k_current_task->Prio;
	A.Comm = PIPE_PUT_REQUEST;
	A.Time.ticks = TimeOut;

	A.Args.pipe_req.ReqInfo.pipe.id = Id;
	A.Args.pipe_req.ReqType.Sync.iSizeTotal = iNbrBytesToWrite;
	A.Args.pipe_req.ReqType.Sync.pData = pBuffer;

	_k_pipe_option_set(&A.Args, Option);
	_k_pipe_request_type_set(&A.Args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*piNbrBytesWritten = A.Args.pipe_ack.iSizeXferred;
	return A.Time.rcode;
}

/**
 *
 * @brief Asynchronous pipe write request
 *
 * This routine attempts to write data from a memory pool block to the
 * specified pipe. (Note that partial transfers and timeouts are not
 * supported, unlike the case for synchronous write requests.)
 *
 * @return RC_OK, RC_FAIL, or RC_ALIGNMENT
 */

int _task_pipe_put_async(kpipe_t Id, struct k_block Block,
						 int iReqSize2Xfer, ksem_t Sema)
{
	unsigned int iSize2Xfer;
	struct k_args A;

	iSize2Xfer = min((unsigned)iReqSize2Xfer, (unsigned)(Block.req_size));

	if (unlikely(iSize2Xfer % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iSize2Xfer)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}

	A.Prio = _k_current_task->Prio;
	A.Comm = PIPE_PUT_REQUEST;
	A.Time.ticks = TICKS_UNLIMITED;
		/* same behavior in flow as a blocking call w/o a timeout */

	A.Args.pipe_req.ReqInfo.pipe.id = Id;
	A.Args.pipe_req.ReqType.Async.block = Block;
	A.Args.pipe_req.ReqType.Async.iSizeTotal = iSize2Xfer;
	A.Args.pipe_req.ReqType.Async.sema = Sema;

	_k_pipe_request_type_set(&A.Args, _ASYNCREQ);
	_k_pipe_option_set(&A.Args, _ALL_N); /* force ALL_N */

	KERNEL_ENTRY(&A);
	return RC_OK;
}
