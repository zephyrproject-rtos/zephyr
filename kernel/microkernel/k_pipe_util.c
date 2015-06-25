/* k_pipe_util.c */

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

#include <micro_private.h>
#include <k_pipe_util.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

#define _ALL_OPT	(0x000000FF)


void DeListWaiter(struct k_args *pReqProc)
{
	__ASSERT_NO_MSG(NULL != pReqProc->Head);
	REMOVE_ELM(pReqProc);
	pReqProc->Head = NULL;
}

void myfreetimer(struct k_timer **ppTimer)
{
	if (*ppTimer) {
		_k_timer_delist(*ppTimer);
		FREETIMER(*ppTimer);
		*ppTimer = NULL;
	}
}

/* adapted from mailbox implementation of copypacket() */
void mycopypacket(struct k_args **out, struct k_args *in)
{
	GETARGS(*out);
	memcpy(*out, in, sizeof(struct k_args));
	(*out)->Ctxt.args = in;
}

int CalcFreeReaderSpace(struct k_args *pReaderList)
{
	int iSize = 0;

	if (pReaderList) {
		struct k_args *pReader = pReaderList;
		while (pReader != NULL) {
			iSize += (pReader->Args.ChProc.iSizeTotal -
				  pReader->Args.ChProc.iSizeXferred);
			pReader = pReader->Forw;
		}
	}
	return iSize;
}

int CalcAvailWriterData(struct k_args *pWriterList)
{
	int iSize = 0;

	if (pWriterList) {
		struct k_args *pWriter = pWriterList;
		while (pWriter != NULL) {
			iSize += (pWriter->Args.ChProc.iSizeTotal -
				  pWriter->Args.ChProc.iSizeXferred);
			pWriter = pWriter->Forw;
		}
	}
	return iSize;
}

K_PIPE_OPTION _k_pipe_option_get(K_ARGS_ARGS *pChxxx)
{
	return (K_PIPE_OPTION)(pChxxx->ChProc.ReqInfo.Params & _ALL_OPT);
}

void _k_pipe_option_set(K_ARGS_ARGS *pChxxx, K_PIPE_OPTION option)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALL_OPT); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(option & _ALL_OPT); /* make sure we do not screw up other fields */
}

REQ_TYPE _k_pipe_request_type_get(K_ARGS_ARGS *pChxxx)
{
	return (REQ_TYPE)(pChxxx->ChProc.ReqInfo.Params & _ALLREQ);
}

void _k_pipe_request_type_set(K_ARGS_ARGS *pChxxx, REQ_TYPE ReqType)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALLREQ); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(ReqType & _ALLREQ); /* make sure we do not screw up other fields */
}

TIME_TYPE ChxxxGetTimeType(K_ARGS_ARGS *pChxxx)
{
	return (TIME_TYPE)(pChxxx->ChProc.ReqInfo.Params & _ALLTIME);
}

void ChxxxSetTimeType(K_ARGS_ARGS *pChxxx,
					    TIME_TYPE TimeType)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALLTIME); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(TimeType & _ALLTIME); /* make sure we do not screw up other fields */
}

CHREQ_STATUS ChReqGetStatus(struct k_chproc *pChProc)
{
	return pChProc->Status;
}

void ChReqSetStatus(struct k_chproc *pChProc, CHREQ_STATUS Status)
{
#ifdef CONFIG_OBJECT_MONITOR
	/*
	 * if transition XFER_IDLE --> XFER_BUSY, TERM_XXX
	 * increment channel counter
	 */

	if (XFER_IDLE == pChProc->Status /* current (old) status */
	    && (XFER_BUSY | TERM_XXX) & Status /* new status */) {
		(pChProc->ReqInfo.ChRef.pPipe->Count)++;
	}
#endif
	pChProc->Status = Status;
}

int ChReqSizeXferred(struct k_chproc *pChProc)
{
	return pChProc->iSizeXferred;
}

int ChReqSizeLeft(struct k_chproc *pChProc)
{
	return (pChProc->iSizeTotal - pChProc->iSizeXferred);
}
