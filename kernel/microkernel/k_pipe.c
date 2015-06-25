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

/*******************************************************************************
*
* _k_pipe_init - initialize kernel pipe subsystem
*
* Performs any initialization of statically-defined pipes that wasn't done
* at build time. (Note: most pipe structure fields are set to zero by sysgen.)
*
* RETURNS: N/A
*/

void _k_pipe_init(void)
{
	int i;
	struct pipe_struct *pPipe;

	for (i = 0, pPipe = _k_pipe_list; i < _k_pipe_count; i++, pPipe++) {
		BuffInit((unsigned char *)pPipe->Buffer,
				 &(pPipe->iBufferSize),
				 &(pPipe->Buff));
	}
}

/*******************************************************************************
*
* _task_pipe_get - pipe read request
*
* This routine attempts to read data into a memory buffer area from the
* specified pipe.
*
* RETURNS: RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
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
	A.Comm = CHDEQ_REQ;
	A.Time.ticks = TimeOut;

	struct k_chreq ChReq;
	ChReq.ReqInfo.ChRef.Id = Id;
	ChxxxSetChOpt((K_ARGS_ARGS *)&ChReq, Option);

	ChxxxSetReqType((K_ARGS_ARGS *)&ChReq, _SYNCREQ);
	ChReq.ReqType.Sync.iSizeTotal = iNbrBytesToRead;
	ChReq.ReqType.Sync.pData = pBuffer;

	A.Args.ChReq = ChReq;

	KERNEL_ENTRY(&A);

	*piNbrBytesRead = A.Args.ChAck.iSizeXferred;
	return A.Time.rcode;
}

/*******************************************************************************
*
* _task_pipe_put - pipe write request
*
* This routine attempts to write data from a memory buffer area to the
* specified pipe.
*
* RETURNS: RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
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
	A.Comm = CHENQ_REQ;
	A.Time.ticks = TimeOut;

	struct k_chreq ChReq;
	ChReq.ReqInfo.ChRef.Id = Id;
	ChxxxSetChOpt((K_ARGS_ARGS *)&ChReq, Option);

	ChxxxSetReqType((K_ARGS_ARGS *)&ChReq, _SYNCREQ);
	ChReq.ReqType.Sync.iSizeTotal = iNbrBytesToWrite;
	ChReq.ReqType.Sync.pData = pBuffer;

	A.Args.ChReq = ChReq;

	KERNEL_ENTRY(&A);

	*piNbrBytesWritten = A.Args.ChAck.iSizeXferred;
	return A.Time.rcode;
}

/*******************************************************************************
*
* _task_pipe_put_async - asynchronous pipe write request
*
* This routine attempts to write data from a memory pool block to the
* specified pipe. (Note that partial transfers and timeouts are not
* supported, unlike the case for synchronous write requests.)
*
* RETURNS: RC_OK, RC_FAIL, or RC_ALIGNMENT
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
	A.Comm = CHENQ_REQ;
	A.Time.ticks = TICKS_UNLIMITED;
		/* same behavior in flow as a blocking call w/o a timeout */

	struct k_chreq ChReq;
	ChReq.ReqInfo.ChRef.Id = Id;

	ChxxxSetReqType((K_ARGS_ARGS *)&ChReq, _ASYNCREQ);
	ChxxxSetChOpt((K_ARGS_ARGS *)&ChReq, _ALL_N); /* force ALL_N */
	ChReq.ReqType.Async.block = Block;
	ChReq.ReqType.Async.iSizeTotal = iSize2Xfer;
	ChReq.ReqType.Async.sema = Sema;

	A.Args.ChReq = ChReq;

	KERNEL_ENTRY(&A);
	return RC_OK;
}
