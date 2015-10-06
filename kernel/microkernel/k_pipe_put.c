/* command processing for pipe put operation */

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
#include <k_pipe_util.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>


/**
 *
 * @brief Process request command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_request(struct k_args *RequestOrig)
{
	struct k_args *Request;
	struct k_args *RequestProc;

	kpipe_t pipeId = RequestOrig->args.pipe_req.req_info.pipe.id;

	bool bAsync;

	if (_ASYNCREQ == _k_pipe_request_type_get(&RequestOrig->args)) {
		bAsync = true;
	} else {
		bAsync = false;
	}

	if (!bAsync) {
		/* First save the pointer to the task's TCB for rescheduling later */
		RequestOrig->Ctxt.task = _k_current_task;
		_k_state_bit_set(_k_current_task, TF_SEND);
	} else {
		/* No need to put in data about sender, since it's a poster */
		RequestOrig->Ctxt.task = NULL;
	}

	mycopypacket(&Request, RequestOrig);

	/* Now, we need a new packet for processing of the request; the
	   Request package is too small b/c of space lost due to possible
	   embedded local data
	 */

	mycopypacket(&RequestProc, Request);
	RequestProc->args.pipe_xfer_req.req_info.pipe.ptr =
		(struct _k_pipe_struct *)pipeId;

	switch (_k_pipe_request_type_get(&RequestProc->args)) {
	case _SYNCREQ:
		RequestProc->args.pipe_xfer_req.data_ptr =
			Request->args.pipe_req.req_type.sync.data_ptr;
		RequestProc->args.pipe_xfer_req.total_size =
			Request->args.pipe_req.req_type.sync.total_size;
		break;
	case _ASYNCREQ:
		RequestProc->args.pipe_xfer_req.data_ptr =
			Request->args.pipe_req.req_type.async.block.pointer_to_data;
		RequestProc->args.pipe_xfer_req.total_size =
			Request->args.pipe_req.req_type.async.total_size;
		break;
	default:
		break;
	}
	RequestProc->args.pipe_xfer_req.status = XFER_IDLE;
	RequestProc->args.pipe_xfer_req.num_pending_xfers = 0;
	RequestProc->args.pipe_xfer_req.xferred_size = 0;

	RequestProc->next = NULL;
	RequestProc->head = NULL;

	switch (RequestProc->Time.ticks) {
	case TICKS_NONE:
		_k_pipe_time_type_set(&RequestProc->args, _TIME_NB);
		break;
	case TICKS_UNLIMITED:
		_k_pipe_time_type_set(&RequestProc->args, _TIME_B);
		break;
	default:
		_k_pipe_time_type_set(&RequestProc->args, _TIME_BT);
		break;
	}

	/* start processing */

	struct _k_pipe_struct *pipe_ptr;

	pipe_ptr = RequestProc->args.pipe_xfer_req.req_info.pipe.ptr;

	do {
		int iSpace2WriteinReaders;
		int iFreeBufferSpace;
		int iTotalSpace2Write;
		int32_t ticks;

		iSpace2WriteinReaders = CalcFreeReaderSpace(pipe_ptr->readers);
		iFreeBufferSpace =
			pipe_ptr->desc.free_space_count + pipe_ptr->desc.free_space_post_wrap_around;
		iTotalSpace2Write =
			iFreeBufferSpace + iSpace2WriteinReaders;

		if (0 == iTotalSpace2Write)
			break; /* special case b/c even not good enough for 1_TO_N */

		/* (possibly) do some processing */

		ticks = RequestProc->Time.ticks;
		RequestProc->Time.timer = NULL;
		_k_pipe_process(pipe_ptr, RequestProc /* writer */, NULL /* reader */);
		RequestProc->Time.ticks = ticks;

		/* check if request was processed */

		if (TERM_XXX & RequestProc->args.pipe_xfer_req.status) {
			RequestProc->Time.timer = NULL; /* not really required */
			return; /* not listed anymore --> completely processed */
		}

	} while (0);

	/* if we got up to here, we did none or SOME (partial)
	 * processing on the request
	 */

	if (_TIME_NB !=
		_k_pipe_time_type_get(&RequestProc->args)) {
		/* call is blocking */
		INSERT_ELM(pipe_ptr->writers, RequestProc);
		/*
		 * NOTE: It is both faster and simpler to blindly assign the
		 * PIPE_PUT_TIMEOUT microkernel command to the packet even though it
		 * is only useful to the finite timeout case.
		 */
		RequestProc->Comm = _K_SVC_PIPE_PUT_TIMEOUT;
		if (_TIME_B == _k_pipe_time_type_get(&RequestProc->args)) {
			/*
			 * The writer specified TICKS_UNLIMITED; NULL the timer.
			 */
			RequestProc->Time.timer = NULL;
			return;
		} else {
			/* { TIME_BT } */
#ifdef CANCEL_TIMERS
			if (RequestProc->args.pipe_xfer_req.xferred_size != 0) {
				RequestProc->Time.timer = NULL;
			} else
#endif
				/* enlist a new timer into the timeout chain */
				_k_timeout_alloc(RequestProc);

			return;
		}
	} else {
		/* call is non-blocking;
		   Check if we don't have to queue it b/c it could not
		   be processed at once
		 */
		RequestProc->Time.timer = NULL;

		if (XFER_BUSY == RequestProc->args.pipe_xfer_req.status) {
			INSERT_ELM(pipe_ptr->writers, RequestProc);
		} else {
			__ASSERT_NO_MSG(XFER_IDLE ==
				RequestProc->args.pipe_xfer_req.status);
			__ASSERT_NO_MSG(0 == RequestProc->args.pipe_xfer_req.xferred_size);
			RequestProc->Comm = _K_SVC_PIPE_PUT_REPLY;
			_k_pipe_put_reply(RequestProc);
		}
		return;
	}
}

/**
 *
 * @brief Perform timeout command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_timeout(struct k_args *ReqProc)
{
	__ASSERT_NO_MSG(NULL != ReqProc->Time.timer);

	myfreetimer(&(ReqProc->Time.timer));
	_k_pipe_request_status_set(&ReqProc->args.pipe_xfer_req, TERM_TMO);

	DeListWaiter(ReqProc);
	if (0 == ReqProc->args.pipe_xfer_req.num_pending_xfers) {
		_k_pipe_put_reply(ReqProc);
	}
}

/**
 *
 * @brief Process reply command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_reply(struct k_args *ReqProc)
{
	__ASSERT_NO_MSG(
		0 == ReqProc->args.pipe_xfer_req.num_pending_xfers /*  no pending Xfers */
	    && NULL == ReqProc->Time.timer /*  no pending timer */
	    && NULL == ReqProc->head); /*  not in list */

	/* orig packet must be sent back, not ReqProc */

	struct k_args *ReqOrig = ReqProc->Ctxt.args;
	PIPE_REQUEST_STATUS status;

	ReqOrig->Comm = _K_SVC_PIPE_PUT_ACK;

	/* determine return value:
	 */
	status = ReqProc->args.pipe_xfer_req.status;
	if (unlikely(TERM_TMO == status)) {
		ReqOrig->Time.rcode = RC_TIME;
	} else if ((TERM_XXX | XFER_IDLE) & status) {
		K_PIPE_OPTION Option = _k_pipe_option_get(&ReqProc->args);

		if (likely(ReqProc->args.pipe_xfer_req.xferred_size ==
				   ReqProc->args.pipe_xfer_req.total_size)) {
			/* All data has been transferred */
			ReqOrig->Time.rcode = RC_OK;
		} else if (ReqProc->args.pipe_xfer_req.xferred_size != 0) {
			/* Some but not all data has been transferred */
			ReqOrig->Time.rcode = (Option == _ALL_N) ? RC_INCOMPLETE : RC_OK;
		} else {
			/* No data has been transferred */
			ReqOrig->Time.rcode = (Option == _0_TO_N) ? RC_OK : RC_FAIL;
		}
	} else {
		/* unknown (invalid) status */
		__ASSERT_NO_MSG(1 == 0); /* should not come here */
	}
	if (_ASYNCREQ != _k_pipe_request_type_get(&ReqOrig->args)) {
		ReqOrig->args.pipe_ack.xferred_size =
			ReqProc->args.pipe_xfer_req.xferred_size;
	}

	SENDARGS(ReqOrig);

	FREEARGS(ReqProc);
}

/**
 *
 * @brief Process acknowledgment command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_ack(struct k_args *Request)
{
	if (_ASYNCREQ == _k_pipe_request_type_get(&Request->args)) {
		struct _pipe_ack_arg *pipe_ack = &Request->args.pipe_ack;
		struct k_args A;
		struct k_block *blockptr;

		/* invoke command to release block */
		blockptr = &pipe_ack->req_type.async.block;
		A.Comm = _K_SVC_MEM_POOL_BLOCK_RELEASE;
		A.args.p1.pool_id = blockptr->pool_id;
		A.args.p1.req_size = blockptr->req_size;
		A.args.p1.rep_poolptr = blockptr->address_in_pool;
		A.args.p1.rep_dataptr = blockptr->pointer_to_data;
		_k_mem_pool_block_release(&A); /* will return immediately */

		if ((ksem_t)NULL != pipe_ack->req_type.async.sema) {
			/* invoke command to signal sema */
			struct k_args A;

			A.Comm = _K_SVC_SEM_SIGNAL;
			A.args.s1.sema = pipe_ack->req_type.async.sema;
			_k_sem_signal(&A); /* will return immediately */
		}
	} else {
		/* Reschedule the sender task */
		struct k_args *LocalReq;

		LocalReq = Request->Ctxt.args;
		LocalReq->Time.rcode = Request->Time.rcode;
		LocalReq->args.pipe_ack = Request->args.pipe_ack;

		_k_state_bit_reset(LocalReq->Ctxt.task, TF_SEND | TF_SENDDATA);
	}

	FREEARGS(Request);
}
