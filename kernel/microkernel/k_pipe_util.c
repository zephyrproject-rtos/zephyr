/* k_pipe_util.c */

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
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

#define _ALL_OPT	(0x000000FF)


void DeListWaiter(struct k_args *pReqProc)
{
	__ASSERT_NO_MSG(pReqProc->head != NULL);
	REMOVE_ELM(pReqProc);
	pReqProc->head = NULL;
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
	int size = 0;

	if (pReaderList) {
		struct k_args *reader_ptr = pReaderList;

		while (reader_ptr != NULL) {
			size += (reader_ptr->args.pipe_xfer_req.total_size -
				  reader_ptr->args.pipe_xfer_req.xferred_size);
			reader_ptr = reader_ptr->next;
		}
	}
	return size;
}

int CalcAvailWriterData(struct k_args *pWriterList)
{
	int size = 0;

	if (pWriterList) {
		struct k_args *writer_ptr = pWriterList;

		while (writer_ptr != NULL) {
			size += (writer_ptr->args.pipe_xfer_req.total_size -
				  writer_ptr->args.pipe_xfer_req.xferred_size);
			writer_ptr = writer_ptr->next;
		}
	}
	return size;
}

K_PIPE_OPTION _k_pipe_option_get(K_ARGS_ARGS *args)
{
	return (K_PIPE_OPTION)(args->pipe_xfer_req.req_info.params & _ALL_OPT);
}

void _k_pipe_option_set(K_ARGS_ARGS *args, K_PIPE_OPTION option)
{
	/* Ensure that only the pipe option bits are modified */
	args->pipe_xfer_req.req_info.params &= (~_ALL_OPT);
	args->pipe_xfer_req.req_info.params |= (option & _ALL_OPT);
}

REQ_TYPE _k_pipe_request_type_get(K_ARGS_ARGS *args)
{
	return (REQ_TYPE)(args->pipe_xfer_req.req_info.params & _ALLREQ);
}

void _k_pipe_request_type_set(K_ARGS_ARGS *args, REQ_TYPE req_type)
{
	/* Ensure that only the request type bits are modified */
	args->pipe_xfer_req.req_info.params &= (~_ALLREQ);
	args->pipe_xfer_req.req_info.params |= (req_type & _ALLREQ);
}

TIME_TYPE _k_pipe_time_type_get(K_ARGS_ARGS *args)
{
	return (TIME_TYPE)(args->pipe_xfer_req.req_info.params & _ALLTIME);
}

void _k_pipe_time_type_set(K_ARGS_ARGS *args, TIME_TYPE TimeType)
{
	/* Ensure that only the time type bits are modified */
	args->pipe_xfer_req.req_info.params &= (~_ALLTIME);
	args->pipe_xfer_req.req_info.params |= (TimeType & _ALLTIME);
}

void _k_pipe_request_status_set(struct _pipe_xfer_req_arg *pipe_xfer_req,
					PIPE_REQUEST_STATUS status)
{
#ifdef CONFIG_OBJECT_MONITOR
	/*
	 * if transition XFER_IDLE --> XFER_BUSY, TERM_XXX
	 * increment pipe counter
	 */

	if (pipe_xfer_req->status == XFER_IDLE /* current (old) status */
	    && (XFER_BUSY | TERM_XXX) & status /* new status */) {
		(pipe_xfer_req->req_info.pipe.ptr->count)++;
	}
#endif
	pipe_xfer_req->status = status;
}
