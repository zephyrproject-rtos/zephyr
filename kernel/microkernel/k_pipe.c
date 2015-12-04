/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Pipe kernel services
 */

#include <micro_private.h>
#include <k_pipe_buffer.h>
#include <k_pipe_util.h>
#include <microkernel/pipe.h>
#include <misc/util.h>

extern kpipe_t _k_pipe_ptr_start[];
extern kpipe_t _k_pipe_ptr_end[];

/**
 * @brief Initialize kernel pipe subsystem
 *
 * Performs any initialization of statically-defined pipes that wasn't done
 * at build time. (Note: most pipe structure fields are set to zero by sysgen.)
 *
 * @return N/A
 */
void _k_pipe_init(void)
{
	kpipe_t *pipeId;
	struct _k_pipe_struct *pipe_ptr;

	for (pipeId = _k_pipe_ptr_start; pipeId < _k_pipe_ptr_end; pipeId++) {
		pipe_ptr = (struct _k_pipe_struct *)(*pipeId);
		BuffInit((unsigned char *)pipe_ptr->Buffer,
				 &(pipe_ptr->buffer_size), &pipe_ptr->desc);
	}
}

int task_pipe_get(kpipe_t id, void *buffer,
		int bytes_to_read, int *bytes_read,
		K_PIPE_OPTION options, int32_t timeout)
{
	struct k_args A;

	/*
	 * some users do not check the FUNCTION return value,
	 * but immediately use bytes_read; make sure it always
	 * has a good value, even when we return failure immediately
	 * (see below)
	 */

	*bytes_read = 0;

	if (unlikely(bytes_to_read % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(bytes_to_read == 0)) {
		/*
		 * not allowed because enlisted requests with zero size
		 * will hang in _k_pipe_process()
		 */
		return RC_FAIL;
	}
	if (unlikely(options == _0_TO_N && timeout != TICKS_NONE)) {
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_GET_REQUEST;
	A.Time.ticks = timeout;

	A.args.pipe_req.req_info.pipe.id = id;
	A.args.pipe_req.req_type.sync.total_size = bytes_to_read;
	A.args.pipe_req.req_type.sync.data_ptr = buffer;

	_k_pipe_option_set(&A.args, options);
	_k_pipe_request_type_set(&A.args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*bytes_read = A.args.pipe_ack.xferred_size;
	return A.Time.rcode;
}

int task_pipe_put(kpipe_t id, void *buffer,
		int bytes_to_write, int *bytes_written,
		K_PIPE_OPTION options, int32_t timeout)
{
	struct k_args A;

	/*
	 * some users do not check the FUNCTION return value,
	 * but immediately use bytes_written; make sure it always
	 * has a good value, even when we return failure immediately
	 * (see below)
	 */

	*bytes_written = 0;

	if (unlikely(bytes_to_write % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(bytes_to_write == 0)) {
		/*
		 * not allowed because enlisted requests with zero size
		 * will hang in _k_pipe_process()
		 */
		return RC_FAIL;
	}
	if (unlikely(options == _0_TO_N && timeout != TICKS_NONE)) {
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_PUT_REQUEST;
	A.Time.ticks = timeout;

	A.args.pipe_req.req_info.pipe.id = id;
	A.args.pipe_req.req_type.sync.total_size = bytes_to_write;
	A.args.pipe_req.req_type.sync.data_ptr = buffer;

	_k_pipe_option_set(&A.args, options);
	_k_pipe_request_type_set(&A.args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*bytes_written = A.args.pipe_ack.xferred_size;
	return A.Time.rcode;
}

/**
 * @brief Asynchronous pipe write request
 *
 * This routine attempts to write data from a memory pool block to the
 * specified pipe. (Note that partial transfers and timeouts are not
 * supported, unlike the case for synchronous write requests.)
 *
 * @return RC_OK, RC_FAIL, or RC_ALIGNMENT
 */
int _task_pipe_block_put(kpipe_t Id, struct k_block Block,
			int iReqSize2Xfer, ksem_t sema)
{
	unsigned int iSize2Xfer;
	struct k_args A;

	iSize2Xfer = min((unsigned)iReqSize2Xfer, (unsigned)(Block.req_size));

	if (unlikely(iSize2Xfer % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(iSize2Xfer == 0)) {
		/*
		 * not allowed because enlisted requests with zero size
		 * will hang in _k_pipe_process()
		 */
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_PUT_REQUEST;
	A.Time.ticks = TICKS_UNLIMITED;
		/* same behavior in flow as a blocking call w/o a timeout */

	A.args.pipe_req.req_info.pipe.id = Id;
	A.args.pipe_req.req_type.async.block = Block;
	A.args.pipe_req.req_type.async.total_size = iSize2Xfer;
	A.args.pipe_req.req_type.async.sema = sema;

	_k_pipe_request_type_set(&A.args, _ASYNCREQ);
	_k_pipe_option_set(&A.args, _ALL_N); /* force ALL_N */

	KERNEL_ENTRY(&A);
	return RC_OK;
}
