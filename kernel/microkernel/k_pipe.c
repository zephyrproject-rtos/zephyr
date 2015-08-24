/* pipe kernel services */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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
#include <k_pipe_buffer.h>
#include <k_pipe_util.h>
#include <microkernel/pipe.h>
#include <misc/util.h>

extern kpipe_t _k_pipe_ptr_start[];
extern kpipe_t _k_pipe_ptr_end[];

/**
 *
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
	struct _k_pipe_struct *pPipe;

	for (pipeId = _k_pipe_ptr_start; pipeId < _k_pipe_ptr_end; pipeId++) {
		pPipe = (struct _k_pipe_struct *)(*pipeId);
		BuffInit((unsigned char *)pPipe->Buffer,
				 &(pPipe->buffer_size), &pPipe->desc);
	}
}

/**
 *
 * @brief Pipe read request
 *
 * This routine attempts to read data into a memory buffer area from the
 * specified pipe.
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */

int _task_pipe_get(kpipe_t Id, void *pBuffer,
				   int iNbrBytesToRead, int *piNbrBytesRead,
				   K_PIPE_OPTION Option, int32_t TimeOut)
{
	struct k_args A;

	/* some users do not check the FUNCTION return value,
	   but immediately use iNbrBytesRead; make sure it always
	   has a good value, even when we return failure immediately
	   (see below) */

	*piNbrBytesRead = 0;

	if (unlikely(iNbrBytesToRead % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iNbrBytesToRead)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}
	if (unlikely(_0_TO_N == Option && TICKS_NONE != TimeOut)) {
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_GET_REQUEST;
	A.Time.ticks = TimeOut;

	A.args.pipe_req.req_info.pipe.id = Id;
	A.args.pipe_req.req_type.sync.total_size = iNbrBytesToRead;
	A.args.pipe_req.req_type.sync.data_ptr = pBuffer;

	_k_pipe_option_set(&A.args, Option);
	_k_pipe_request_type_set(&A.args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*piNbrBytesRead = A.args.pipe_ack.iSizeXferred;
	return A.Time.rcode;
}

/**
 *
 * @brief Pipe write request
 *
 * This routine attempts to write data from a memory buffer area to the
 * specified pipe.
 *
 * @return RC_OK, RC_INCOMPLETE, RC_FAIL, RC_TIME, or RC_ALIGNMENT
 */

int _task_pipe_put(kpipe_t Id, void *pBuffer,
				   int iNbrBytesToWrite, int *piNbrBytesWritten,
				   K_PIPE_OPTION Option, int32_t TimeOut)
{
	struct k_args A;

	/* some users do not check the FUNCTION return value,
	   but immediately use iNbrBytesWritten; make sure it always
	   has a good value, even when we return failure immediately
	   (see below) */

	*piNbrBytesWritten = 0;

	if (unlikely(iNbrBytesToWrite % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iNbrBytesToWrite)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}
	if (unlikely(_0_TO_N == Option && TICKS_NONE != TimeOut)) {
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_PUT_REQUEST;
	A.Time.ticks = TimeOut;

	A.args.pipe_req.req_info.pipe.id = Id;
	A.args.pipe_req.req_type.sync.total_size = iNbrBytesToWrite;
	A.args.pipe_req.req_type.sync.data_ptr = pBuffer;

	_k_pipe_option_set(&A.args, Option);
	_k_pipe_request_type_set(&A.args, _SYNCREQ);

	KERNEL_ENTRY(&A);

	*piNbrBytesWritten = A.args.pipe_ack.iSizeXferred;
	return A.Time.rcode;
}

/**
 *
 * @brief Asynchronous pipe write request
 *
 * This routine attempts to write data from a memory pool block to the
 * specified pipe. (Note that partial transfers and timeouts are not
 * supported, unlike the case for synchronous write requests.)
 *
 * @return RC_OK, RC_FAIL, or RC_ALIGNMENT
 */

int _task_pipe_block_put(kpipe_t Id, struct k_block Block,
						 int iReqSize2Xfer, ksem_t Sema)
{
	unsigned int iSize2Xfer;
	struct k_args A;

	iSize2Xfer = min((unsigned)iReqSize2Xfer, (unsigned)(Block.req_size));

	if (unlikely(iSize2Xfer % SIZEOFUNIT_TO_OCTET(1))) {
		return RC_ALIGNMENT;
	}
	if (unlikely(0 == iSize2Xfer)) {
		/* not allowed because enlisted requests with zero size
		   will hang in _k_pipe_process() */
		return RC_FAIL;
	}

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_PIPE_PUT_REQUEST;
	A.Time.ticks = TICKS_UNLIMITED;
		/* same behavior in flow as a blocking call w/o a timeout */

	A.args.pipe_req.req_info.pipe.id = Id;
	A.args.pipe_req.req_type.async.block = Block;
	A.args.pipe_req.req_type.async.total_size = iSize2Xfer;
	A.args.pipe_req.req_type.async.sema = Sema;

	_k_pipe_request_type_set(&A.args, _ASYNCREQ);
	_k_pipe_option_set(&A.args, _ALL_N); /* force ALL_N */

	KERNEL_ENTRY(&A);
	return RC_OK;
}
