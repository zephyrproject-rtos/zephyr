/* KS_ChPtA.c */

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

/* includes */

#include <minik.h>
#include <kchan.h>
#include <toolchain.h>

/*******************************************************************************
*
* _task_pipe_put_async -
*
* RETURNS: RC_ALIGNMENT, RC_FAIL, RC_OK
*/

int _task_pipe_put_async(
	kpipe_t Id,
	struct k_block Block,
	int iReqSize2Xfer,
	ksem_t Sema
)
{
	unsigned int iSize2Xfer;
	struct k_args A;

	iSize2Xfer = min((unsigned)iReqSize2Xfer, (unsigned)(Block.req_size));

	if (unlikely(iSize2Xfer % SIZEOFUNIT_TO_OCTET(1)))
		return RC_ALIGNMENT;
	if (unlikely(0 == iSize2Xfer))
		return RC_FAIL; /* not allowed because enlisted requests with
				   zero size will hang in K_ChProc() */

	A.Prio = _k_current_task->Prio;
	A.Comm = CHENQ_REQ;
	A.Time
		.ticks = TICKS_UNLIMITED; /* same behavior in flow as a blocking
					  call w/o a timeout */
	{
		struct k_chreq ChReq;
		ChReq.ReqInfo.ChRef.Id = Id;
		/* asynchr. Xfer: */
		{
			ChxxxSetReqType((K_ARGS_ARGS *)&ChReq, _ASYNCREQ);
			ChxxxSetChOpt((K_ARGS_ARGS *)&ChReq,
				      _ALL_N); /* force ALL_N */
			ChReq.ReqType.Async.block = Block;
			ChReq.ReqType.Async.iSizeTotal = iSize2Xfer;
			ChReq.ReqType.Async.sema = Sema;
		}
		A.Args.ChReq = ChReq;
	}

	KERNEL_ENTRY(&A);
	return RC_OK;
}
