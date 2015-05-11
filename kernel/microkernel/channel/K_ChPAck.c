/* K_ChPAck.c */

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

#include <microkernel/k_struct.h>
#include <kchan.h>
#include <minik.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*****************************************************************************/

void K_ChSendAck(struct k_args *Request)
{
	if (_ASYNCREQ == ChxxxGetReqType(&(Request->Args))) {
		struct k_chack *pChAck = (struct k_chack *)&(Request->Args.ChAck);

		{
			/* invoke command to release block: */
			struct k_args A;
			struct k_block *blockptr;
			blockptr = &(pChAck->ReqType.Async.block);
			A.Comm = REL_BLOCK;
			A.Args.p1.poolid = blockptr->poolid;
			A.Args.p1.req_size = blockptr->req_size;
			A.Args.p1.rep_poolptr = blockptr->address_in_pool;
			A.Args.p1.rep_dataptr = blockptr->pointer_to_data;
			_k_mem_pool_block_release(&A); /* will return immediately */
		}

		if ((ksem_t)NULL != pChAck->ReqType.Async.sema) {
			/* invoke command to signal sema: */
			struct k_args A;
			A.Comm = SIGNALS;
			A.Args.s1.sema = pChAck->ReqType.Async.sema;
			_k_sem_signal(&A); /* will return immediately */
		}
	} else {
		/* Reschedule the sender task: */
		struct k_args *LocalReq;

		LocalReq = Request->Ctxt.args;
		LocalReq->Time.rcode = Request->Time.rcode;
		LocalReq->Args.ChAck = Request->Args.ChAck;

		reset_state_bit(LocalReq->Ctxt.proc, TF_SEND | TF_SENDDATA);
	}
	FREEARGS(Request);
}

/*****************************************************************************/
