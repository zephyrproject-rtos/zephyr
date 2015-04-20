/* movedata.c */

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

#include <microkernel/k_struct.h>
#include <minik.h>
#include <kmemcpy.h>
#include <string_s.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/* forward declarations */


static void mvdreq_copy(struct moved_req *ReqArgs);

static void mvdreq_docont(struct k_args *Cont);

/*******************************************************************************
*
* K_mvdreq - process a movedata request
*
* RETURNS: N/A
*/

void K_mvdreq(struct k_args *Req)
{
	struct moved_req *ReqArgs;

	ReqArgs = &(Req->Args.MovedReq);

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
				ReqArgs->Extra.Setup.ContSnd); /* Send ack
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

/*******************************************************************************
*
* mvdreq_copy - perform movedata request
*
* RETURNS: N/A
*/

static void mvdreq_copy(struct moved_req *ReqArgs)
{
	k_memcpy_s(ReqArgs->destination,
		   OCTET_TO_SIZEOFUNIT(ReqArgs->iTotalSize),
		   ReqArgs->source,
		   OCTET_TO_SIZEOFUNIT(ReqArgs->iTotalSize));

	if (ReqArgs->Action & MVDACT_SNDACK)
		mvdreq_docont(ReqArgs->Extra.Setup.ContSnd);
	if (ReqArgs->Action & MVDACT_RCVACK)
		mvdreq_docont(ReqArgs->Extra.Setup.ContRcv);
}


/* Helper functions */

/*******************************************************************************
*
* mvdreq_docont -
*
* RETURNS: N/A
*/

static void mvdreq_docont(struct k_args *Cont)
{
	struct k_args *Next;

	while (Cont) {
		Next = Cont;
		Cont = Cont->Forw;
		SENDARGS(Next);
	}
}
