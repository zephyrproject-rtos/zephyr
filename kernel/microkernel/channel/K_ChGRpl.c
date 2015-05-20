/* K_ChGRpl.c */

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

#include <minik.h>
#include <kchan.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>


void K_ChRecvRpl(struct k_args *ReqProc)
{
	__ASSERT_NO_MSG(
		(0 == ReqProc->Args.ChProc.iNbrPendXfers) /*  no pending Xfers */
	    && (NULL == ReqProc->Time.timer) /*  no pending timer */
	    && (NULL == ReqProc->Head)); /*  not in list */

	/* orig packet must be sent back, not ReqProc */

	struct k_args *ReqOrig = ReqProc->Ctxt.args;
	CHREQ_STATUS ChReqStatus;
	ReqOrig->Comm = CHDEQ_ACK;

	/* determine return value */

	ChReqStatus = ChReqGetStatus(&(ReqProc->Args.ChProc));
	if (TERM_TMO == ChReqStatus) {
		ReqOrig->Time.rcode = RC_TIME;
	} else if ((TERM_XXX | XFER_IDLE) & ChReqStatus) {
		K_PIPE_OPTION Option = ChxxxGetChOpt(&(ReqProc->Args));

		if (likely(0 == ChReqSizeLeft(&(ReqProc->Args.ChProc)))) {
			/* All data has been transferred */
			ReqOrig->Time.rcode = RC_OK;
		} else if (ChReqSizeXferred(&(ReqProc->Args.ChProc))) {
			/* Some but not all data has been transferred */
			ReqOrig->Time.rcode = (Option == _ALL_N) ?
								  RC_INCOMPLETE : RC_OK;
		} else {
			/* No data has been transferred */
			ReqOrig->Time.rcode = (Option == _0_TO_N) ? RC_OK : RC_FAIL;
		}
	} else {
		/* unknown (invalid) status */
		__ASSERT_NO_MSG(1 == 0); /* should not come here */
	}

	ReqOrig->Args.ChAck.iSizeXferred = ReqProc->Args.ChProc.iSizeXferred;
	SENDARGS(ReqOrig);

	FREEARGS(ReqProc);
}
