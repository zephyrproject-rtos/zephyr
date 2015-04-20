/* K_ChPReq.c */

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

#include "microkernel/k_struct.h"
#include "kchan.h"
#include "kmemcpy.h"
#include "minik.h"
#include "kticks.h"
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*****************************************************************************/

void K_ChSendReq(struct k_args *RequestOrig)
{
	struct k_args *Request;
	struct k_args *RequestProc;

	{
		kpipe_t pipeId = RequestOrig->Args.ChReq.ReqInfo.ChRef.Id;

		BOOL bAsync;

		if (_ASYNCREQ == ChxxxGetReqType(&(RequestOrig->Args)))
			bAsync = TRUE;
		else
			bAsync = FALSE;

		if (!bAsync) {
			RequestOrig->Ctxt.proc =
				K_Task; /* First we save the pointer to
					   the tasks TCB for
					   rescheduling later */
			set_state_bit(K_Task, TF_SEND);
		} else {
			RequestOrig->Ctxt.proc =
				NULL; /* No need to put in data about
					 sender, since it's a poster */
		}

		mycopypacket(&Request, RequestOrig);

		/* {if we end up here, we arrived at destination node and the
		   packet
		   Request is not local} */

		/* Now, we need a new packet for processing of the request; the
		   Request package is too small b/c of space lost due to
		   possible
		   embedded local data:
		 */
		{
			mycopypacket(&RequestProc, Request);
			RequestProc->Args.ChProc.ReqInfo.ChRef.pPipe =
				&(K_PipeList[OBJ_INDEX(pipeId)]);
			switch (ChxxxGetReqType(&(RequestProc->Args))) {
			case _SYNCREQ:
				RequestProc->Args.ChProc.pData =
					Request->Args.ChReq.ReqType.Sync
					.pData;
				RequestProc->Args.ChProc.iSizeTotal =
					Request->Args.ChReq.ReqType.Sync
					.iSizeTotal;
				break;
			case _ASYNCREQ:
				RequestProc->Args.ChProc.DataNode =
					OBJ_NODE(Request->Args.ChReq
						 .ReqType.Async
						 .block.poolid);
				RequestProc->Args.ChProc.pData =
					Request->Args.ChReq.ReqType
					.Async.block
					.pointer_to_data;
				RequestProc->Args.ChProc.iSizeTotal =
					Request->Args.ChReq.ReqType
					.Async.iSizeTotal;
				break;
			default:
				break;
			}
			RequestProc->Args.ChProc.Status = XFER_IDLE;
			RequestProc->Args.ChProc.iNbrPendXfers = 0;
			RequestProc->Args.ChProc.iSizeXferred = 0;

			RequestProc->Forw = NULL;
			RequestProc->Head = NULL;

			switch (RequestProc->Time.ticks) {
			case TICKS_NONE:
				ChxxxSetTimeType(
					(K_ARGS_ARGS *)&(RequestProc->Args),
					_TIME_NB);
				break;
			case TICKS_UNLIMITED:
				ChxxxSetTimeType(
					(K_ARGS_ARGS *)&(RequestProc->Args),
					_TIME_B);
				break;
			default:
				ChxxxSetTimeType(
					(K_ARGS_ARGS *)&(RequestProc->Args),
					_TIME_BT);
				break;
			}
		}
	}

	/* start processing:
	 */
	{
		struct pipe_struct *pPipe;
		struct k_chproc *pReqProcArgs;

		pReqProcArgs = &(RequestProc->Args.ChProc);
		pPipe = pReqProcArgs->ReqInfo.ChRef.pPipe;

		do {
			int iSpace2WriteinReaders, iFreeBufferSpace;
			int iTotalSpace2Write;
			int32_t ticks;

			iSpace2WriteinReaders =
				CalcFreeReaderSpace(pPipe->Readers);
			iFreeBufferSpace = pPipe->Buff.iFreeSpaceCont +
					   pPipe->Buff.iFreeSpaceAWA;
			iTotalSpace2Write =
				iFreeBufferSpace + iSpace2WriteinReaders;

			if (0 == iTotalSpace2Write)
				break; /* special case b/c even not good enough
					  for 1_TO_N */

			/* (possibly) do some processing:
			 */
			ticks = RequestProc->Time.ticks;
			RequestProc->Time.timer = NULL;
			K_ChProc(pPipe,
				 RequestProc /*writer */
				 ,
				 NULL /*reader */);
			RequestProc->Time.ticks = ticks;

			/* check if request was processed:
			 */
			if (TERM_XXX &
			    ChReqGetStatus(&(RequestProc->Args.ChProc))) {
				RequestProc->Time.timer =
					NULL; /* not really required */
				return; /* not listed anymore --> completely
					   processed */
			}

		} while (0);

		/* if we got up to here, we did none or SOME (partial)
		 * processing on the request.
		 */

		if (_TIME_NB !=
		    ChxxxGetTimeType((K_ARGS_ARGS *)&(RequestProc->Args))) {
			/*{ call is blocking } */
			INSERT_ELM(pPipe->Writers, RequestProc);
			/*
			 * NOTE: It is both faster and simpler to blindly assign the
			 * CHENQ_TMO microkernel command to the packet even though it
			 * is only useful to the finite timeout case.
			 */
			RequestProc->Comm = CHENQ_TMO;
			if (_TIME_B ==
			    ChxxxGetTimeType(
				    (K_ARGS_ARGS *)&(RequestProc->Args))) {
				/*
				 * The writer specified TICKS_UNLIMITED.
				 * NULL the timer.
				 */
				RequestProc->Time.timer = NULL;
				return;
			} else {
/* { TIME_BT } */
#ifdef CANCEL_TIMERS
				if (ChReqSizeXferred(
					    &(RequestProc->Args.ChProc))) {
					RequestProc->Time.timer = NULL;
				} else
#endif
					enlist_timeout(
						RequestProc); /* Else enlist a
								 new timer into
								 the timeout
								 chain */

				return;
			}
		} else {
			/*{ call is non-blocking }
			   Check if we don't have to queue it b/c it could not
			   be processed at once:
			 */
			RequestProc->Time.timer = NULL;

			if (XFER_BUSY ==
			    ChReqGetStatus(&(RequestProc->Args.ChProc))) {
				INSERT_ELM(pPipe->Writers, RequestProc);
			} else {
				__ASSERT_NO_MSG(XFER_IDLE ==
				       ChReqGetStatus(
					       &(RequestProc->Args.ChProc)));
				__ASSERT_NO_MSG(0 ==
				       ChReqSizeXferred(
					       &(RequestProc->Args.ChProc)));
				RequestProc->Comm = CHENQ_RPL;
				K_ChSendRpl(RequestProc);
			}
			return;
		}
	}
}

/********************************************************************************/
