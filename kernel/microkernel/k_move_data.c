/* k_move_data.c */

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
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/**
 *
 * mvdreq_docont -
 *
 * @return N/A
 */

static void mvdreq_docont(struct k_args *Cont)
{
	struct k_args *next;

	while (Cont) {
		next = Cont;
		Cont = Cont->next;
		SENDARGS(next);
	}
}

/**
 *
 * @brief Perform movedata request
 *
 * @return N/A
 */

static void mvdreq_copy(struct moved_req *ReqArgs)
{
	memcpy(ReqArgs->destination, ReqArgs->source,
		   OCTET_TO_SIZEOFUNIT(ReqArgs->iTotalSize));

	if (ReqArgs->Action & MVDACT_SNDACK)
		mvdreq_docont(ReqArgs->Extra.Setup.continuation_send);
	if (ReqArgs->Action & MVDACT_RCVACK)
		mvdreq_docont(ReqArgs->Extra.Setup.ContRcv);
}

/**
 *
 * @brief Process a movedata request
 *
 * @return N/A
 */

void _k_movedata_request(struct k_args *Req)
{
	struct moved_req *ReqArgs;

	ReqArgs = &(Req->args.MovedReq);

	__ASSERT_NO_MSG(0 ==
	       (ReqArgs->iTotalSize %
		SIZEOFUNIT_TO_OCTET(1))); /* must be a multiple of size_t */
	__ASSERT_NO_MSG(!(ReqArgs->Action & MVDACT_INVALID));

	/* If no data is to be transferred, just execute the continuation
	   packet,
	   if any, and get out:
	 */
	if (0 == ReqArgs->iTotalSize) {
		if (ReqArgs->Action & MVDACT_SNDACK)
			mvdreq_docont(
				ReqArgs->Extra.Setup.continuation_send); /* Send ack
								  continuation
								  */
		if (ReqArgs->Action & MVDACT_RCVACK)
			mvdreq_docont(
				ReqArgs->Extra.Setup.ContRcv); /* Recv ack
								  continuation
								  */
		return;
	}

	mvdreq_copy(ReqArgs);
}
