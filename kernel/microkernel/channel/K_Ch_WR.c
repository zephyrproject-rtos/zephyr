/* K_Ch_WR.c */

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

/* includes */

#include <microkernel.h>
#include <kchan.h>
#include <kmemcpy.h>
#include <minik.h>
#include <kticks.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*******************************************************************************
*
* _UpdateChannelXferStatus - update the channel transfer status
*
* RETURNS: N/A
*/

static void _UpdateChannelXferStatus(
	struct k_args *pActor,       /* ptr to struct k_args to be used by actor */
	struct k_chproc *pActorArgs, /* ptr to actor's channel process structure */
	int bytesXferred      /* # of bytes transferred */
	)
{
	pActorArgs->iNbrPendXfers++;
	pActorArgs->iSizeXferred += bytesXferred;

	if (pActorArgs->iSizeXferred == pActorArgs->iSizeTotal) {
		ChReqSetStatus(pActorArgs, TERM_SATISFIED);
		if (pActor->Head != NULL) {
			DeListWaiter(pActor);
			myfreetimer(&pActor->Time.timer);
		}
	} else
		ChReqSetStatus(pActorArgs, XFER_BUSY);
}

/*******************************************************************************
*
* K_ChProcWR - read and/or write from/to the channel
*
* RETURNS: N/A
*/

void K_ChProcWR(
	struct pipe_struct *pPipe, /* ptr to channel structure */
	struct k_args *pNewWriter,  /* ptr to new writer struct k_args */
	struct k_args *pNewReader   /* ptr to new reader struct k_args */
	)
{
	struct k_args *pReader;       /* ptr to struct k_args to be used by reader */
	struct k_args *pWriter;       /* ptr to struct k_args to be used by writer */
	struct k_chproc *pWriterArgs; /* ptr to writer's channel process structure */
	struct k_chproc *pReaderArgs; /* ptr to reader's channel process structure */

	int iT1;
	int iT2;
	int iT3;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->Writers;

	__ASSERT_NO_MSG((pPipe->Writers == pNewWriter) ||
	       (NULL == pPipe->Writers) || (NULL == pNewWriter));

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->Readers;

	__ASSERT_NO_MSG((pPipe->Readers == pNewReader) ||
	       (NULL == pPipe->Readers) || (NULL == pNewReader));

	/* Preparation */
	pWriterArgs = &pWriter->Args.ChProc;
	pReaderArgs = &pReader->Args.ChProc;

	/* Calculate iT1, iT2 and iT3 */
	int iFreeSpaceReader =
		(pReaderArgs->iSizeTotal -
		 pReaderArgs->iSizeXferred);
	int iAvailDataWriter =
		(pWriterArgs->iSizeTotal - pWriterArgs->iSizeXferred);
	int iFreeSpaceBuffer =
		(pPipe->Buff.iFreeSpaceCont + pPipe->Buff.iFreeSpaceAWA);
	int iAvailDataBuffer =
		(pPipe->Buff.iAvailDataCont + pPipe->Buff.iAvailDataAWA);

	iT1 = min(iFreeSpaceReader, iAvailDataBuffer);

	iFreeSpaceReader -= iT1;

	if (0 == pPipe->Buff.iNbrPendingWrites) {/* { no incoming data
						       anymore } */

		iT2 = min(iFreeSpaceReader, iAvailDataWriter);

		iAvailDataWriter -= iT2;

		iT3 = min(iAvailDataWriter, iFreeSpaceBuffer);
	} else {
		/*
		 * There is still data coming into the buffer from a writer.
		 * Therefore <iT2> must be zero; there is no direct W-to-R
		 * transfer as
		 * the buffer is not really 'empty'.
		 */

		iT2 = 0;
		iT3 = 0; /* this is a choice (can be optimised later on) */
	}

	/***************/
	/* ACTION !!!! */
	/***************/

	/* T1 transfer: */
	if (iT1 != 0) {
		K_ChProcRO(pPipe, pReader);
	}

	/* T2 transfer: */
	if (iT2 != 0) {
		struct k_args *Moved_req;

		__ASSERT_NO_MSG(TERM_SATISFIED != ChReqGetStatus(&pReader->Args.ChProc));

		GETARGS(Moved_req);
		setup_movedata(
			Moved_req,
			pPipe,
			XFER_W2R,
			pWriter,
			pReader,
			(char *)(pReaderArgs->pData) +
				OCTET_TO_SIZEOFUNIT(pReaderArgs->iSizeXferred),
			(char *)(pWriterArgs->pData) +
				OCTET_TO_SIZEOFUNIT(pWriterArgs->iSizeXferred),
			iT2,
			-1);
		K_mvdreq(Moved_req);
		FREEARGS(Moved_req);

		_UpdateChannelXferStatus(pWriter, pWriterArgs, iT2);

		_UpdateChannelXferStatus(pReader, pReaderArgs, iT2);
	}

	/* T3 transfer: */
	if (iT3 != 0) {
		__ASSERT_NO_MSG(TERM_SATISFIED != ChReqGetStatus(&pWriter->Args.ChProc));
		K_ChProcWO(pPipe, pWriter);
	}
}
