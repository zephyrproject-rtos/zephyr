/* k_move_data.c */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
		   OCTET_TO_SIZEOFUNIT(ReqArgs->total_size));

	if (ReqArgs->action & MVDACT_SNDACK)
		mvdreq_docont(ReqArgs->extra.setup.continuation_send);
	if (ReqArgs->action & MVDACT_RCVACK)
		mvdreq_docont(ReqArgs->extra.setup.continuation_receive);
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

	ReqArgs = &(Req->args.moved_req);

	__ASSERT_NO_MSG(0 ==
	       (ReqArgs->total_size %
		SIZEOFUNIT_TO_OCTET(1))); /* must be a multiple of size_t */
	__ASSERT_NO_MSG(!(ReqArgs->action & MVDACT_INVALID));

	/* If no data is to be transferred, just execute the continuation
	   packet,
	   if any, and get out:
	 */
	if (0 == ReqArgs->total_size) {
		if (ReqArgs->action & MVDACT_SNDACK)
			mvdreq_docont(
				ReqArgs->extra.setup.continuation_send); /* Send ack
								  continuation
								  */
		if (ReqArgs->action & MVDACT_RCVACK)
			mvdreq_docont(
				ReqArgs->extra.setup.continuation_receive); /* Recv ack
								  continuation
								  */
		return;
	}

	mvdreq_copy(ReqArgs);
}
