/* K_Ch_Mvd.c */

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

#include "microkernel/k_struct.h"
#include "kchan.h"
#include "kmemcpy.h"
#include "minik.h"
#include "kticks.h"
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*******************************************************************************
*
* setup_movedata -
*
* RETURNS: N/A
*/

void setup_movedata(struct k_args *A,
					  struct pipe_struct *pPipe,
					  XFER_TYPE XferType,
					  struct k_args *pWriter,
					  struct k_args *pReader,
					  void *destination,
					  knode_t srcnode,
					  void *source,
					  uint32_t size,
					  int XferID)
{
	A->Comm = MVD_REQ;

	A->Ctxt.proc = NULL; /* this caused problems when != NULL related to
				set/reset of state bits */

	A->Args.MovedReq.Action = (MovedAction)(MVDACT_SNDACK | MVDACT_RCVACK);
	A->Args.MovedReq.srcnode = srcnode;
	A->Args.MovedReq.source = source;
	A->Args.MovedReq.destination = destination;
	A->Args.MovedReq.iTotalSize = size;

	/* continuation packet:
	 */
	{
		struct k_args *pContSend, *pContRecv;
		GETARGS(pContSend);
		GETARGS(pContRecv);

		pContSend->Forw = NULL;
		pContSend->Comm = CH_MOVED_ACK;
		pContSend->Args.ChMovedAck.pPipe = pPipe;
		pContSend->Args.ChMovedAck.XferType = XferType;
		pContSend->Args.ChMovedAck.ID = XferID;
		pContSend->Args.ChMovedAck.iSize = size;

		pContRecv->Forw = NULL;
		pContRecv->Comm = CH_MOVED_ACK;
		pContRecv->Args.ChMovedAck.pPipe = pPipe;
		pContRecv->Args.ChMovedAck.XferType = XferType;
		pContRecv->Args.ChMovedAck.ID = XferID;
		pContRecv->Args.ChMovedAck.iSize = size;

		SetChanProcPrio(A, pContSend, pContRecv, pWriter, pReader);

		switch (XferType) {
		case XFER_W2B: /* Writer to Buffer */
		{
			__ASSERT_NO_MSG(NULL == pReader);
			pContSend->Args.ChMovedAck.pWriter = pWriter;
			/*pContSend->Args.ChMovedAck.pReader =NULL; */
			pContRecv->Args.ChMovedAck.pWriter = NULL;
			/*pContRecv->Args.ChMovedAck.pReader =NULL; */
			break;
		}
		case XFER_B2R: {
			__ASSERT_NO_MSG(NULL == pWriter);
			/*pContSend->Args.ChMovedAck.pWriter =NULL; */
			pContSend->Args.ChMovedAck.pReader = NULL;
			/*pContRecv->Args.ChMovedAck.pWriter =NULL; */
			pContRecv->Args.ChMovedAck.pReader = pReader;
			break;
		}
		case XFER_W2R: {
			__ASSERT_NO_MSG(NULL != pWriter && NULL != pReader);
			pContSend->Args.ChMovedAck.pWriter = pWriter;
			pContSend->Args.ChMovedAck.pReader = NULL;
			pContRecv->Args.ChMovedAck.pWriter = NULL;
			pContRecv->Args.ChMovedAck.pReader = pReader;
			break;
		}
		default:
			__ASSERT_NO_MSG(1 == 0); /* we should not come here */
		}

		A->Args.MovedReq.Extra.Setup.ContSnd = pContSend;
		A->Args.MovedReq.Extra.Setup.ContRcv = pContRecv;

		/* Remark (possible optimisation)
		   if we could know if it was a send/recv completion, we could
		   use the
		   SAME cmd packet for continuation on both completion of send
		   and recv !!
		 */
	}
}

/*******************************************************************************
*
* K_ChMovedAck -
*
* RETURNS: N/A
*/

void K_ChMovedAck(struct k_args *pEOXfer)
{
	struct k_chmovedack *pEOXferArgs = &(pEOXfer->Args.ChMovedAck);

	switch (pEOXferArgs->XferType) {
	case XFER_W2B: /* Writer to Buffer */
	{
		struct k_args *pWriter = pEOXferArgs->pWriter;

		if (pWriter) { /* Xfer from Writer finished */
			struct k_chproc *pWriterArg =
				&(pEOXferArgs->pWriter->Args.ChProc);
			--(pWriterArg->iNbrPendXfers);
			if (0 == pWriterArg->iNbrPendXfers) {
				if (TERM_XXX & pWriterArg->Status) {
					/* request is terminated, send
					 * reply:
					 */
					K_ChSendRpl(pEOXferArgs->pWriter);
					/* invoke continuation mechanism
					 * (fall through) */
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			} else {
				if (TERM_XXX & pWriterArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism
					 * (fall through) */
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			}
		} else {
			/* Xfer to Buffer finished */

			int XferId = pEOXferArgs->ID;
			BuffEnQA_End(&(pEOXferArgs->pPipe->Buff),
				     XferId,
				     pEOXferArgs->iSize);
		}
		/* invoke continuation mechanism:
		 */
		{
			K_ChProc(pEOXferArgs->pPipe, NULL, NULL);
			FREEARGS(pEOXfer);
			return;
		}
		/*			break;*/
	} /* XFER_W2B */

	case XFER_B2R: {
		struct k_args *pReader = pEOXferArgs->pReader;

		if (pReader) { /* Xfer to Reader finished */
			struct k_chproc *pReaderArg =
				&(pEOXferArgs->pReader->Args.ChProc);
			--(pReaderArg->iNbrPendXfers);
			if (0 == pReaderArg->iNbrPendXfers) {
				if (TERM_XXX & pReaderArg->Status) {
					/* request is terminated, send
					 * reply:
					 */
					K_ChRecvRpl(
						pEOXferArgs->pReader);
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			} else {
				if (TERM_XXX & pReaderArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism
					 * (fall through) */
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			}
		} else
			/* Xfer from Buffer finished */
		{
			int XferId = pEOXferArgs->ID;
			BuffDeQA_End(&(pEOXferArgs->pPipe->Buff),
				     XferId,
				     pEOXferArgs->iSize);
		}

		/* continuation mechanism:
		 */
		{
			K_ChProc(pEOXferArgs->pPipe, NULL, NULL);
			FREEARGS(pEOXfer);
			return;
		}
			/*			break;*/
	} /* XFER_B2R */

	case XFER_W2R: {
		struct k_args *pWriter = pEOXferArgs->pWriter;
		/*			struct k_args *pReader
		 * =pEOXferArgs->pReader;*/

		if (pWriter) { /* Transfer from writer finished */
			struct k_chproc *pWriterArg =
				&(pEOXferArgs->pWriter->Args.ChProc);
			--(pWriterArg->iNbrPendXfers);
			if (0 == pWriterArg->iNbrPendXfers) {
				if (TERM_XXX & pWriterArg->Status) {
					/* request is terminated, send
					 * reply:
					 */
					K_ChSendRpl(pEOXferArgs->pWriter);
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			} else {
				if (TERM_XXX & pWriterArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism
					 * (fall through) */
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			}
		} else
			/* Transfer to Reader finished */
		{
			struct k_chproc *pReaderArg =
				&(pEOXferArgs->pReader->Args.ChProc);
			--(pReaderArg->iNbrPendXfers);
			if (0 == pReaderArg->iNbrPendXfers) {
				if (TERM_XXX & pReaderArg->Status) {
					/* request is terminated, send
					 * reply:
					 */
					K_ChRecvRpl(
						pEOXferArgs->pReader);
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			} else {
				if (TERM_XXX & pReaderArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism
					 * (fall through) */
				} else {
					/* invoke continuation mechanism
					 * (fall through) */
				}
			}
		}

		/* invoke continuation mechanism:
		 */
		{
			K_ChProc(pEOXferArgs->pPipe, NULL, NULL);
			FREEARGS(pEOXfer);
			return;
		}
		/*			break;*/
	} /* XFER_W2B */

	default:
		break;
	}
}

/*******************************************************************************
*
* CalcChanProcPrio -
*
* RETURNS: priority
*/

int CalcChanProcPrio(int iChanDefaultPrio,
					   int iWriterPrio,
					   int iReaderPrio)
{
	int iMaxPrio;

	iMaxPrio = min(iChanDefaultPrio, iWriterPrio);
	iMaxPrio = min(iMaxPrio, iReaderPrio);
	return iMaxPrio;
}

/*******************************************************************************
*
* SetChanProcPrio -
*
* RETURNS: N/A
*/

void SetChanProcPrio(struct k_args *pMvdReq,
					   struct k_args *pContSend,
					   struct k_args *pContRecv,
					   struct k_args *pWriter,
					   struct k_args *pReader)
{
	int iProcPrio;
	int iWriterPrio;
	int iReaderPrio;

	if (pWriter != NULL)
		iWriterPrio = pWriter->Prio;
	else
		iWriterPrio = PRIO_MIN;

	if (pReader != NULL)
		iReaderPrio = pReader->Prio;
	else
		iReaderPrio = PRIO_MIN;

	iProcPrio =
		CalcChanProcPrio(CHANPRIO_DEFAULT, iWriterPrio, iReaderPrio);

	pMvdReq->Prio = iProcPrio;
	pContSend->Prio = iProcPrio;
	pContRecv->Prio = iProcPrio;
}
