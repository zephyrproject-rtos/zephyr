/* KS_ChGet.c */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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

#include <microkernel/k_struct.h>
#include <kchan.h>
#include <minik.h>
#include <toolchain.h>

/*****************************************************************************/
int _task_pipe_get(kpipe_t Id,
		void *pBuffer,
		int iNbrBytesToRead,
		int *piNbrBytesRead,
		K_PIPE_OPTION Option,
		int32_t TimeOut)
{
	struct k_args A;
	*piNbrBytesRead =
		0; /* some users do not check the FUNCTION return value,
		      but immediately use iNbrBytesRead; make sure it always
		      has a good value, even when we return failure immediately
		      (see below) */

	if (unlikely(iNbrBytesToRead % SIZEOFUNIT_TO_OCTET(1)))
		return RC_ALIGNMENT;
	if (unlikely(0 == iNbrBytesToRead))
		return RC_FAIL; /* not allowed because enlisted requests with
				   zero size will hang in K_ChProc() */
	if (unlikely(_0_TO_N == Option && TICKS_NONE != TimeOut))
		return RC_FAIL;

	A.Prio = _k_current_task->Prio;
	A.Comm = CHDEQ_REQ;
	A.Time.ticks = TimeOut;
	{
		struct k_chreq ChReq;
		ChReq.ReqInfo.ChRef.Id = Id;
		ChxxxSetChOpt((K_ARGS_ARGS *)&ChReq, Option);

		ChxxxSetReqType((K_ARGS_ARGS *)&ChReq, _SYNCREQ);
		ChReq.ReqType.Sync.iSizeTotal = iNbrBytesToRead;
		ChReq.ReqType.Sync.pData = pBuffer;

		A.Args.ChReq = ChReq;
	}

	KERNEL_ENTRY(&A);

	*piNbrBytesRead = A.Args.ChAck.iSizeXferred;
	return A.Time.rcode;
}

/*****************************************************************************/
