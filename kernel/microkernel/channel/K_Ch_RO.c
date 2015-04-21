/* K_Ch_RO.c */

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
* K_ChProcRO - read from the channel
*
* This routine reads from the channel.  If <pPipe> is NULL, then it uses
* <pNewReader> as the reader.  Otherwise it takes the reader from the channel
* structure.
*
* RETURNS: N/A
*/

void K_ChProcRO(struct pipe_struct *pPipe, struct k_args *pNewReader)
{
	struct k_args *pReader;
	struct k_chproc *pReaderArgs;

	unsigned char *pRead;
	int iSize;
	int id;
	int ret;
	int numIterations = 2;

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->Readers;

	__ASSERT_NO_MSG((pPipe->Readers == pNewReader) ||
	       (NULL == pPipe->Readers) || (NULL == pNewReader));

	pReaderArgs = &pReader->Args.ChProc;

	do {
		iSize = min(
			pPipe->Buff.iAvailDataCont,
			pReaderArgs->iSizeTotal - pReaderArgs->iSizeXferred);

		if (iSize == 0) {
			return;
		}

		struct k_args *Moved_req;

		ret = BuffDeQA(&pPipe->Buff, iSize, &pRead, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(
			Moved_req,
			pPipe,
			XFER_B2R,
			NULL,
			pReader,
			(char *)(pReaderArgs->pData) +
				OCTET_TO_SIZEOFUNIT(pReaderArgs->iSizeXferred),
			pRead,
			ret,
			id);
		K_mvdreq(Moved_req);
		FREEARGS(Moved_req);

		pReaderArgs->iNbrPendXfers++;
		pReaderArgs->iSizeXferred += ret;

		if (pReaderArgs->iSizeXferred == pReaderArgs->iSizeTotal) {
			ChReqSetStatus(pReaderArgs, TERM_SATISFIED);
			if (pReader->Head != NULL) {
				DeListWaiter(pReader);
				myfreetimer(&pReader->Time.timer);
			}
			return;
		} else
			ChReqSetStatus(pReaderArgs, XFER_BUSY);

	} while (--numIterations != 0);
}
