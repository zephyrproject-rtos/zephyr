/* K_Ch_WO.c */

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

#include <microkernel.h>
#include <minik.h>
#include <kchan.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <ch_buff.h>

/*******************************************************************************
*
* K_ChProcWO - write to the channel
*
* This routine writes to the channel.  If <pPipe> is NULL, then it uses
* <pNewWriter> as the writer.  Otherwise it takes the writer from the channel
* structure.
*
* RETURNS: N/A
*/

void K_ChProcWO(struct pipe_struct *pPipe, struct k_args *pNewWriter)
{
	struct k_args *pWriter;
	struct k_chproc *pWriterArgs;

	int iSize;
	unsigned char *pWrite;
	int id;
	int ret;
	int numIterations = 2;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->Writers;

	__ASSERT_NO_MSG(!((pPipe->Writers != pNewWriter) &&
					  (NULL != pPipe->Writers) && (NULL != pNewWriter)));

	pWriterArgs = &pWriter->Args.ChProc;

	do {
		iSize = min((numIterations == 2) ? pPipe->Buff.iFreeSpaceCont
					: pPipe->Buff.iFreeSpaceAWA,
					pWriterArgs->iSizeTotal - pWriterArgs->iSizeXferred);

		if (iSize == 0) {
			continue;
		}

		struct k_args *Moved_req;

		ret = BuffEnQA(&pPipe->Buff, iSize, &pWrite, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_W2B, pWriter, NULL, pWrite,
			(char *)(pWriterArgs->pData) +
			OCTET_TO_SIZEOFUNIT(pWriterArgs->iSizeXferred),
			ret, (numIterations == 2) ? id : -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pWriterArgs->iNbrPendXfers++;
		pWriterArgs->iSizeXferred += ret;

		if (pWriterArgs->iSizeXferred == pWriterArgs->iSizeTotal) {
			ChReqSetStatus(pWriterArgs, TERM_SATISFIED);
			if (pWriter->Head != NULL) {
				/* only listed requests have a timer */
				DeListWaiter(pWriter);
				myfreetimer(&pWriter->Time.timer);
			}
			return;
		} else {
			ChReqSetStatus(pWriterArgs, XFER_BUSY);
		}

	} while (--numIterations != 0);
}
