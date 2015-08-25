/* pipe processing for data transfer */

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

#include <micro_private.h>
#include <k_pipe_buffer.h>
#include <k_pipe_util.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <misc/util.h>

#define FORCE_XFER_ON_STALL

#define _X_TO_N		(_0_TO_N | _1_TO_N)

/*
 * - artefacts: ???
 * - non-optimal:
	  * from single requester to multiple requesters : basic function is
pipe_read_write()
		pipe_read_write() copies remaining data into buffer; better would be to
possibly copy the remaining data
		to the next requester (if there is one)
	  * ...
 */


/**
 *
 * _k_pipe_movedata_ack -
 *
 * @return N/A
 */

void _k_pipe_movedata_ack(struct k_args *pEOXfer)
{
	struct _pipe_xfer_ack_arg *pipe_xfer_ack = &pEOXfer->args.pipe_xfer_ack;

	switch (pipe_xfer_ack->xfer_type) {
	case XFER_W2B: /* Writer to Buffer */
	{
		struct k_args *writer_ptr = pipe_xfer_ack->writer_ptr;

		if (writer_ptr) { /* Xfer from Writer finished */
			struct _pipe_xfer_req_arg *pipe_write_req =
				&pipe_xfer_ack->writer_ptr->args.pipe_xfer_req;

			--pipe_write_req->num_pending_xfers;
			if (0 == pipe_write_req->num_pending_xfers) {
				if (TERM_XXX & pipe_write_req->status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pipe_xfer_ack->writer_ptr);
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pipe_write_req->status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Xfer to Buffer finished */

			int XferId = pipe_xfer_ack->id;

			BuffEnQA_End(&pipe_xfer_ack->pipe_ptr->desc, XferId,
						 pipe_xfer_ack->size);
		}

		/* invoke continuation mechanism */

		_k_pipe_process(pipe_xfer_ack->pipe_ptr, NULL, NULL);
		FREEARGS(pEOXfer);
		return;
	} /* XFER_W2B */

	case XFER_B2R: {
		struct k_args *reader_ptr = pipe_xfer_ack->reader_ptr;

		if (reader_ptr) { /* Xfer to Reader finished */
			struct _pipe_xfer_req_arg *pipe_read_req =
				&pipe_xfer_ack->reader_ptr->args.pipe_xfer_req;

			--pipe_read_req->num_pending_xfers;
			if (0 == pipe_read_req->num_pending_xfers) {
				if (TERM_XXX & pipe_read_req->status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pipe_xfer_ack->reader_ptr);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pipe_read_req->status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Xfer from Buffer finished */

			int XferId = pipe_xfer_ack->id;

			BuffDeQA_End(&pipe_xfer_ack->pipe_ptr->desc, XferId,
						 pipe_xfer_ack->size);
		}

		/* continuation mechanism */

		_k_pipe_process(pipe_xfer_ack->pipe_ptr, NULL, NULL);
		FREEARGS(pEOXfer);
		return;

	} /* XFER_B2R */

	case XFER_W2R: {
		struct k_args *writer_ptr = pipe_xfer_ack->writer_ptr;

		if (writer_ptr) { /* Transfer from writer finished */
			struct _pipe_xfer_req_arg *pipe_write_req =
				&pipe_xfer_ack->writer_ptr->args.pipe_xfer_req;

			--pipe_write_req->num_pending_xfers;
			if (0 == pipe_write_req->num_pending_xfers) {
				if (TERM_XXX & pipe_write_req->status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pipe_xfer_ack->writer_ptr);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pipe_write_req->status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Transfer to Reader finished */

			struct _pipe_xfer_req_arg *pipe_read_req =
				&pipe_xfer_ack->reader_ptr->args.pipe_xfer_req;

			--pipe_read_req->num_pending_xfers;
			if (0 == pipe_read_req->num_pending_xfers) {
				if (TERM_XXX & pipe_read_req->status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pipe_xfer_ack->reader_ptr);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pipe_read_req->status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		}

		/* invoke continuation mechanism */

		_k_pipe_process(pipe_xfer_ack->pipe_ptr, NULL, NULL);
		FREEARGS(pEOXfer);
		return;
	} /* XFER_W2B */

	default:
		break;
	}
}

/**
 *
 * @brief Determines priority for data move operation
 *
 * Uses priority level of most important participant.
 *
 * Note: It's OK to have one or two participants, but there can't be none!
 *
 * @return N/A
 */

static kpriority_t move_priority_compute(struct k_args *writer_ptr,
										 struct k_args *reader_ptr)
{
	kpriority_t move_priority;

	if (!writer_ptr) {
		move_priority = reader_ptr->priority;
	} else {
		move_priority = writer_ptr->priority;
		if (reader_ptr && (reader_ptr->priority < move_priority)) {
			move_priority = reader_ptr->priority;
		}
	}

	return move_priority;
}

/**
 *
 * setup_movedata -
 *
 * @return N/A
 */

static void setup_movedata(struct k_args *A,
						   struct _k_pipe_struct *pipe_ptr, XFER_TYPE xfer_type,
						   struct k_args *writer_ptr, struct k_args *reader_ptr,
						   void *destination, void *source,
						   uint32_t size, int XferID)
{
	struct k_args *pContSend;
	struct k_args *pContRecv;

	A->Comm = _K_SVC_MOVEDATA_REQ;

	A->Ctxt.task = NULL;
	/* this caused problems when != NULL related to set/reset of state bits */

	A->args.MovedReq.Action = (MovedAction)(MVDACT_SNDACK | MVDACT_RCVACK);
	A->args.MovedReq.source = source;
	A->args.MovedReq.destination = destination;
	A->args.MovedReq.iTotalSize = size;

	/* continuation packet */

	GETARGS(pContSend);
	GETARGS(pContRecv);

	pContSend->next = NULL;
	pContSend->Comm = _K_SVC_PIPE_MOVEDATA_ACK;
	pContSend->args.pipe_xfer_ack.pipe_ptr = pipe_ptr;
	pContSend->args.pipe_xfer_ack.xfer_type = xfer_type;
	pContSend->args.pipe_xfer_ack.id = XferID;
	pContSend->args.pipe_xfer_ack.size = size;

	pContRecv->next = NULL;
	pContRecv->Comm = _K_SVC_PIPE_MOVEDATA_ACK;
	pContRecv->args.pipe_xfer_ack.pipe_ptr = pipe_ptr;
	pContRecv->args.pipe_xfer_ack.xfer_type = xfer_type;
	pContRecv->args.pipe_xfer_ack.id = XferID;
	pContRecv->args.pipe_xfer_ack.size = size;

	A->priority = move_priority_compute(writer_ptr, reader_ptr);
	pContSend->priority = A->priority;
	pContRecv->priority = A->priority;

	switch (xfer_type) {
	case XFER_W2B: /* Writer to Buffer */
	{
		__ASSERT_NO_MSG(NULL == reader_ptr);
		pContSend->args.pipe_xfer_ack.writer_ptr = writer_ptr;
		pContRecv->args.pipe_xfer_ack.writer_ptr = NULL;
		break;
	}
	case XFER_B2R: {
		__ASSERT_NO_MSG(NULL == writer_ptr);
		pContSend->args.pipe_xfer_ack.reader_ptr = NULL;
		pContRecv->args.pipe_xfer_ack.reader_ptr = reader_ptr;
		break;
	}
	case XFER_W2R: {
		__ASSERT_NO_MSG(NULL != writer_ptr && NULL != reader_ptr);
		pContSend->args.pipe_xfer_ack.writer_ptr = writer_ptr;
		pContSend->args.pipe_xfer_ack.reader_ptr = NULL;
		pContRecv->args.pipe_xfer_ack.writer_ptr = NULL;
		pContRecv->args.pipe_xfer_ack.reader_ptr = reader_ptr;
		break;
	}
	default:
		__ASSERT_NO_MSG(1 == 0); /* we should not come here */
	}

	A->args.MovedReq.Extra.Setup.continuation_send = pContSend;
	A->args.MovedReq.Extra.Setup.ContRcv = pContRecv;

	/*
	 * (possible optimisation)
	 * if we could know if it was a send/recv completion, we could use the
	 * SAME cmd packet for continuation on both completion of send and recv !!
	 */
}

static int ReaderInProgressIsBlocked(struct _k_pipe_struct *pipe_ptr,
									 struct k_args *reader_ptr)
{
	int iSizeSpaceInReader;
	int iAvailBufferData;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&reader_ptr->args);
	option = _k_pipe_option_get(&reader_ptr->args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(reader_ptr->args.pipe_xfer_req.xferred_size))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && reader_ptr->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pipe_ptr->desc.num_pending_writes ||
	    0 != pipe_ptr->desc.num_pending_reads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0;
	}

	/* third condition: */

	iSizeSpaceInReader =
		reader_ptr->args.pipe_xfer_req.total_size -
		reader_ptr->args.pipe_xfer_req.xferred_size;
	BuffGetAvailDataTotal(&pipe_ptr->desc, &iAvailBufferData);
	if (iAvailBufferData >= iSizeSpaceInReader) {
		return 0;
	} else {
		return 1;
	}
}

static int WriterInProgressIsBlocked(struct _k_pipe_struct *pipe_ptr,
									 struct k_args *writer_ptr)
{
	int iSizeDataInWriter;
	int iFreeBufferSpace;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&writer_ptr->args);
	option = _k_pipe_option_get(&writer_ptr->args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(writer_ptr->args.pipe_xfer_req.xferred_size))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && writer_ptr->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pipe_ptr->desc.num_pending_writes ||
	    0 != pipe_ptr->desc.num_pending_reads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0; 
	}

	/* third condition: */

	iSizeDataInWriter =
		writer_ptr->args.pipe_xfer_req.total_size -
		writer_ptr->args.pipe_xfer_req.xferred_size;
	BuffGetFreeSpaceTotal(&pipe_ptr->desc, &iFreeBufferSpace);
	if (iFreeBufferSpace >= iSizeDataInWriter) {
		return 0;
	} else {
		return 1;
	}
}

/**
 *
 * @brief Read from the pipe
 *
 * This routine reads from the pipe.  If <pipe_ptr> is NULL, then it uses
 * <pNewReader> as the reader.  Otherwise it takes the reader from the pipe
 * structure.
 *
 * @return N/A
 */

static void pipe_read(struct _k_pipe_struct *pipe_ptr, struct k_args *pNewReader)
{
	struct k_args *reader_ptr;
	struct _pipe_xfer_req_arg *pipe_read_req;

	unsigned char *read_ptr;
	int size;
	int id;
	int ret;
	int numIterations = 2;

	reader_ptr = (pNewReader != NULL) ? pNewReader : pipe_ptr->readers;

	__ASSERT_NO_MSG((pipe_ptr->readers == pNewReader) ||
					(NULL == pipe_ptr->readers) || (NULL == pNewReader));

	pipe_read_req = &reader_ptr->args.pipe_xfer_req;

	do {
		size = min(pipe_ptr->desc.available_data_count,
					pipe_read_req->total_size - pipe_read_req->xferred_size);

		if (size == 0) {
			return;
		}

		struct k_args *Moved_req;

		ret = BuffDeQA(&pipe_ptr->desc, size, &read_ptr, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pipe_ptr, XFER_B2R, NULL, reader_ptr,
			(char *)(pipe_read_req->data_ptr) +
			OCTET_TO_SIZEOFUNIT(pipe_read_req->xferred_size),
			read_ptr, ret, id);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_read_req->num_pending_xfers++;
		pipe_read_req->xferred_size += ret;

		if (pipe_read_req->xferred_size == pipe_read_req->total_size) {
			_k_pipe_request_status_set(pipe_read_req, TERM_SATISFIED);
			if (reader_ptr->head != NULL) {
				DeListWaiter(reader_ptr);
				myfreetimer(&reader_ptr->Time.timer);
			}
			return;
		} else {
			_k_pipe_request_status_set(pipe_read_req, XFER_BUSY);
		}

	} while (--numIterations != 0);
}

/**
 *
 * @brief Write to the pipe
 *
 * This routine writes to the pipe.  If <pipe_ptr> is NULL, then it uses
 * <pNewWriter> as the writer.  Otherwise it takes the writer from the pipe
 * structure.
 *
 * @return N/A
 */

static void pipe_write(struct _k_pipe_struct *pipe_ptr, struct k_args *pNewWriter)
{
	struct k_args *writer_ptr;
	struct _pipe_xfer_req_arg *pipe_write_req;

	int size;
	unsigned char *write_ptr;
	int id;
	int ret;
	int numIterations = 2;

	writer_ptr = (pNewWriter != NULL) ? pNewWriter : pipe_ptr->writers;

	__ASSERT_NO_MSG(!((pipe_ptr->writers != pNewWriter) &&
					  (NULL != pipe_ptr->writers) && (NULL != pNewWriter)));

	pipe_write_req = &writer_ptr->args.pipe_xfer_req;

	do {
		size = min((numIterations == 2) ? pipe_ptr->desc.free_space_count
					: pipe_ptr->desc.free_space_post_wrap_around,
					pipe_write_req->total_size - pipe_write_req->xferred_size);

		if (size == 0) {
			continue;
		}

		struct k_args *Moved_req;

		ret = BuffEnQA(&pipe_ptr->desc, size, &write_ptr, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pipe_ptr, XFER_W2B, writer_ptr, NULL, write_ptr,
			(char *)(pipe_write_req->data_ptr) +
			OCTET_TO_SIZEOFUNIT(pipe_write_req->xferred_size),
			ret, (numIterations == 2) ? id : -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_write_req->num_pending_xfers++;
		pipe_write_req->xferred_size += ret;

		if (pipe_write_req->xferred_size == pipe_write_req->total_size) {
			_k_pipe_request_status_set(pipe_write_req, TERM_SATISFIED);
			if (writer_ptr->head != NULL) {
				/* only listed requests have a timer */
				DeListWaiter(writer_ptr);
				myfreetimer(&writer_ptr->Time.timer);
			}
			return;
		} else {
			_k_pipe_request_status_set(pipe_write_req, XFER_BUSY);
		}

	} while (--numIterations != 0);
}

/**
 *
 * @brief Update the pipe transfer status
 *
 * @return N/A
 */

static void pipe_xfer_status_update(
	struct k_args *pActor,       /* ptr to struct k_args to be used by actor */
	struct _pipe_xfer_req_arg *pipe_xfer_req, /* ptr to actor's pipe process structure */
	int bytesXferred      /* # of bytes transferred */
	)
{
	pipe_xfer_req->num_pending_xfers++;
	pipe_xfer_req->xferred_size += bytesXferred;

	if (pipe_xfer_req->xferred_size == pipe_xfer_req->total_size) {
		_k_pipe_request_status_set(pipe_xfer_req, TERM_SATISFIED);
		if (pActor->head != NULL) {
			DeListWaiter(pActor);
			myfreetimer(&pActor->Time.timer);
		}
	} else {
		_k_pipe_request_status_set(pipe_xfer_req, XFER_BUSY);
	}
}

/**
 *
 * @brief Read and/or write from/to the pipe
 *
 * @return N/A
 */

static void pipe_read_write(
	struct _k_pipe_struct *pipe_ptr, /* ptr to pipe structure */
	struct k_args *pNewWriter,  /* ptr to new writer struct k_args */
	struct k_args *pNewReader   /* ptr to new reader struct k_args */
	)
{
	struct k_args *reader_ptr;       /* ptr to struct k_args to be used by reader */
	struct k_args *writer_ptr;       /* ptr to struct k_args to be used by writer */
	struct _pipe_xfer_req_arg *pipe_write_req; /* ptr to writer's pipe process structure */
	struct _pipe_xfer_req_arg *pipe_read_req; /* ptr to reader's pipe process structure */

	int iT1;
	int iT2;
	int iT3;

	writer_ptr = (pNewWriter != NULL) ? pNewWriter : pipe_ptr->writers;

	__ASSERT_NO_MSG((pipe_ptr->writers == pNewWriter) ||
					(NULL == pipe_ptr->writers) || (NULL == pNewWriter));

	reader_ptr = (pNewReader != NULL) ? pNewReader : pipe_ptr->readers;

	__ASSERT_NO_MSG((pipe_ptr->readers == pNewReader) ||
					(NULL == pipe_ptr->readers) || (NULL == pNewReader));

	/* Preparation */
	pipe_write_req = &writer_ptr->args.pipe_xfer_req;
	pipe_read_req = &reader_ptr->args.pipe_xfer_req;

	/* Calculate iT1, iT2 and iT3 */
	int iFreeSpaceReader =
		(pipe_read_req->total_size - pipe_read_req->xferred_size);
	int iAvailDataWriter =
		(pipe_write_req->total_size - pipe_write_req->xferred_size);
	int iFreeSpaceBuffer =
		(pipe_ptr->desc.free_space_count + pipe_ptr->desc.free_space_post_wrap_around);
	int iAvailDataBuffer =
		(pipe_ptr->desc.available_data_count + pipe_ptr->desc.available_data_post_wrap_around);

	iT1 = min(iFreeSpaceReader, iAvailDataBuffer);

	iFreeSpaceReader -= iT1;

	if (0 == pipe_ptr->desc.num_pending_writes) {
		/* no incoming data anymore */

		iT2 = min(iFreeSpaceReader, iAvailDataWriter);

		iAvailDataWriter -= iT2;

		iT3 = min(iAvailDataWriter, iFreeSpaceBuffer);
	} else {
		/*
		 * There is still data coming into the buffer from a writer.
		 * Therefore <iT2> must be zero; there is no direct W-to-R
		 * transfer as the buffer is not really 'empty'.
		 */

		iT2 = 0;
		iT3 = 0; /* this is a choice (can be optimised later on) */
	}

	/***************/
	/* ACTION !!!! */
	/***************/

	/* T1 transfer */
	if (iT1 != 0) {
		pipe_read(pipe_ptr, reader_ptr);
	}

	/* T2 transfer */
	if (iT2 != 0) {
		struct k_args *Moved_req;

		__ASSERT_NO_MSG(TERM_SATISFIED != reader_ptr->args.pipe_xfer_req.status);

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pipe_ptr, XFER_W2R, writer_ptr, reader_ptr,
			(char *)(pipe_read_req->data_ptr) +
			OCTET_TO_SIZEOFUNIT(pipe_read_req->xferred_size),
			(char *)(pipe_write_req->data_ptr) +
			OCTET_TO_SIZEOFUNIT(pipe_write_req->xferred_size),
			iT2, -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_xfer_status_update(writer_ptr, pipe_write_req, iT2);

		pipe_xfer_status_update(reader_ptr, pipe_read_req, iT2);
	}

	/* T3 transfer */
	if (iT3 != 0) {
		__ASSERT_NO_MSG(TERM_SATISFIED != writer_ptr->args.pipe_xfer_req.status);
		pipe_write(pipe_ptr, writer_ptr);
	}
}

void _k_pipe_process(struct _k_pipe_struct *pipe_ptr, struct k_args *pNLWriter,
			  struct k_args *pNLReader)
{

	struct k_args *reader_ptr = NULL;
	struct k_args *writer_ptr = NULL;

	__ASSERT_NO_MSG(!(pNLWriter && pNLReader));
	/* both a pNLWriter and pNLReader, is that allowed?
	   Code below has not been designed for that.
	   Anyway, this will not happen in current version. */

	struct k_args *pNextReader;
	struct k_args *pNextWriter;

	do {
		bool bALLNWriterNoGo = false;
		bool bALLNReaderNoGo = false;

		/* Reader */

		if (NULL != pNLReader) {
			if (reader_ptr != pNLReader) {
				pNextReader = pipe_ptr->readers;
				if (NULL == pNextReader) {
					if (!(TERM_XXX & pNLReader->args.pipe_xfer_req.status))
						pNextReader = pNLReader;
				}
			} else {
				/* we already used the extra non-listed Reader */
				if (TERM_XXX & reader_ptr->args.pipe_xfer_req.status) {
					pNextReader = NULL;
				} else {
					pNextReader = reader_ptr; /* == pNLReader */
				}
			}
		} else {
			pNextReader = pipe_ptr->readers;
		}

		/* Writer */

		if (NULL != pNLWriter) {
			if (writer_ptr != pNLWriter) {
				pNextWriter = pipe_ptr->writers;
				if (NULL == pNextWriter) {
					if (!(TERM_XXX & pNLWriter->args.pipe_xfer_req.status))
						pNextWriter = pNLWriter;
				}
			} else {
				/* we already used the extra non-listed Writer */
				if (TERM_XXX & writer_ptr->args.pipe_xfer_req.status) {
					pNextWriter = NULL;
				} else {
					pNextWriter = writer_ptr;
				}
			}
		} else {
			pNextWriter = pipe_ptr->writers;
		}

		/* check if there is uberhaupt something to do */

		if (NULL == pNextReader && NULL == pNextWriter)
			return;
		if (pNextReader == reader_ptr && pNextWriter == writer_ptr)
			break; /* nothing changed, so stop */

		/* go with pNextReader and pNextWriter */

		reader_ptr = pNextReader;
		writer_ptr = pNextWriter;

		if (writer_ptr) {
			if (_ALL_N == _k_pipe_option_get(&writer_ptr->args) &&
				(writer_ptr->args.pipe_xfer_req.xferred_size == 0) &&
				_TIME_B != _k_pipe_time_type_get(&writer_ptr->args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeDataInWriter;
				int iSpace2WriteinReaders;
				int iFreeBufferSpace;
				int iTotalSpace2Write;

				iSpace2WriteinReaders = CalcFreeReaderSpace(pipe_ptr->readers);
				if (pNLReader)
					iSpace2WriteinReaders +=
						(pNLReader->args.pipe_xfer_req.total_size -
						 pNLReader->args.pipe_xfer_req.xferred_size);
				BuffGetFreeSpaceTotal(&pipe_ptr->desc, &iFreeBufferSpace);
				iTotalSpace2Write =
					iFreeBufferSpace + iSpace2WriteinReaders;
				iSizeDataInWriter =
					writer_ptr->args.pipe_xfer_req.total_size -
					writer_ptr->args.pipe_xfer_req.xferred_size;

				if (iSizeDataInWriter > iTotalSpace2Write) {
					bALLNWriterNoGo = true;
				}
			}
		}
		if (reader_ptr) {
			if (_ALL_N == _k_pipe_option_get(&reader_ptr->args) &&
				(reader_ptr->args.pipe_xfer_req.xferred_size == 0) &&
				_TIME_B != _k_pipe_time_type_get(&reader_ptr->args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeFreeSpaceInReader;
				int iData2ReadFromWriters;
				int iAvailBufferData;
				int iTotalData2Read;

				iData2ReadFromWriters = CalcAvailWriterData(pipe_ptr->writers);
				if (pNLWriter)
					iData2ReadFromWriters +=
						(pNLWriter->args.pipe_xfer_req.total_size -
						 pNLWriter->args.pipe_xfer_req.xferred_size);
				BuffGetAvailDataTotal(&pipe_ptr->desc, &iAvailBufferData);
				iTotalData2Read = iAvailBufferData + iData2ReadFromWriters;
				iSizeFreeSpaceInReader =
					reader_ptr->args.pipe_xfer_req.total_size -
					reader_ptr->args.pipe_xfer_req.xferred_size;

				if (iSizeFreeSpaceInReader > iTotalData2Read) {
					bALLNReaderNoGo = true;
				}
			}
		}

		__ASSERT_NO_MSG(!(bALLNWriterNoGo && bALLNReaderNoGo));

		/************/
		/* ACTION:  */
		/************/

		if (bALLNWriterNoGo) {
			/* investigate if we must force a transfer to avoid a stall */
			if (!BuffEmpty(&pipe_ptr->desc)) {
				if (reader_ptr) {
					pipe_read(pipe_ptr, reader_ptr);
					continue;
				} else {
					/* we could break as well,
					   but then nothing else will happen */
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (reader_ptr && (_TIME_NB !=
					_k_pipe_time_type_get(&writer_ptr->args))) {
					/* force transfer
					   (we make exception for non-blocked writer) */
					pipe_read_write(pipe_ptr, writer_ptr, reader_ptr);
					continue;
				} else
#endif
					/* we could break as well,
					   but then nothing else will happen */
					return;
			}
		} else if (bALLNReaderNoGo) {
			/* investigate if we must force a transfer to avoid a stall */
			if (!BuffFull(&pipe_ptr->desc)) {
				if (writer_ptr) {
					pipe_write(pipe_ptr, writer_ptr);
					continue;
				} else {
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (writer_ptr && (_TIME_NB !=
						_k_pipe_time_type_get(&reader_ptr->args))) {
					/* force transfer
					   (we make exception for non-blocked reader) */
					pipe_read_write(pipe_ptr, writer_ptr, reader_ptr);
					continue;
				} else
#endif
					return;
			}
		} else {
			/* no blocked reader and no blocked writer
			   (if there are any of them)
			   == NOMINAL operation
			 */
			if (reader_ptr) {
				if (writer_ptr) {
					pipe_read_write(pipe_ptr, writer_ptr, reader_ptr);
					continue;
				} else {
					pipe_read(pipe_ptr, reader_ptr);
					continue;
				}
			} else {
				if (writer_ptr) {
					pipe_write(pipe_ptr, writer_ptr);
					continue;
				} else {
					/* we should not come here */
					__ASSERT_NO_MSG(1 == 0);
					return;
				}
			}
		}
	} while (1);

	/* We stopped processing because nothing changed anymore (stall)
	   Let's examine the situation a little bit further
	*/

	reader_ptr = pNextReader;
	writer_ptr = pNextWriter;

	/* if we come here, it is b/c reader_ptr and writer_ptr did not change
	anymore.
	- Normally one of them is NULL, which means only a writer, resp. a
	reader remained.
	- The case that none of them is NULL is a special case which 'normally'
	does not occur.
	A remaining reader_ptr and/or writer_ptr are expected to be not-completed.

	  Note that in the case there is only a reader or there is only a
	writer, it can be a ALL_N request.
	  This happens when his request has not been processed completely yet
	(b/c of a copy in and copy out
	  conflict in the buffer e.g.), but is expected to be processed
	completely somewhat later (must be !)
	  */

	/* in the sequel, we will:
	   1. check the hypothesis that an existing reader_ptr/writer_ptr is not
	   completed
	   2. check if we can force the termination of a X_TO_N request when
	   some data transfer took place
	   3. check if we have to cancel a timer when the (first) data has been
	   Xferred
	   4. Check if we have to kick out a queued request because its
	   processing is really blocked (for some reason)
	 */
	if (reader_ptr && writer_ptr) {
		__ASSERT_NO_MSG(!(TERM_XXX & reader_ptr->args.pipe_xfer_req.status) &&
						!(TERM_XXX & writer_ptr->args.pipe_xfer_req.status));
		/* this could be possible when data Xfer operations are jammed
		   (out of data Xfer resources e.g.) */

		/* later on, at least one of them will be completed.
		   Force termination of X_TO_N request?
		   - If one of the requesters is X_TO_N and the other one is
		   ALL_N, we cannot force termination
		   of the X_TO_N request
		   - If they are both X_TO_N, we can do so (but can this
		   situation be?)

		   In this version, we will NOT do so and try to transfer data
		   as much as possible as
		   there are now 2 parties present to exchange data, possibly
		   directly
		   (this is an implementation choice, but I think it is best for
		   overall application performance)
		 */
		;
	} else if (reader_ptr) {
		__ASSERT_NO_MSG(!(TERM_XXX & reader_ptr->args.pipe_xfer_req.status));

		/* check if this lonely reader is really blocked, then we will
		   delist him
		   (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (ReaderInProgressIsBlocked(pipe_ptr, reader_ptr)) {
			if (_X_TO_N & _k_pipe_option_get(&reader_ptr->args) &&
			    (reader_ptr->args.pipe_xfer_req.xferred_size != 0)) {
				_k_pipe_request_status_set(&reader_ptr->args.pipe_xfer_req,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&reader_ptr->args.pipe_xfer_req,
					TERM_FORCED);
			}

			if (reader_ptr->head) {
				DeListWaiter(reader_ptr);
				myfreetimer(&(reader_ptr->Time.timer));
			}
			if (0 == reader_ptr->args.pipe_xfer_req.num_pending_xfers) {
				reader_ptr->Comm = _K_SVC_PIPE_GET_REPLY;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				_k_pipe_get_reply(reader_ptr);
			}
		} else {
			/* temporary stall (must be, processing will continue
			 * later on) */
		}
	} else if (writer_ptr) {
		__ASSERT_NO_MSG(!(TERM_SATISFIED & writer_ptr->args.pipe_xfer_req.status));

		/* check if this lonely Writer is really blocked, then we will
		   delist him (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (WriterInProgressIsBlocked(pipe_ptr, writer_ptr)) {
			if (_X_TO_N & _k_pipe_option_get(&writer_ptr->args) &&
			    (writer_ptr->args.pipe_xfer_req.xferred_size != 0)) {
				_k_pipe_request_status_set(&writer_ptr->args.pipe_xfer_req,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&writer_ptr->args.pipe_xfer_req,
					TERM_FORCED);
			}

			if (writer_ptr->head) {
				DeListWaiter(writer_ptr);
				myfreetimer(&(writer_ptr->Time.timer));
			}
			if (0 == writer_ptr->args.pipe_xfer_req.num_pending_xfers) {
				writer_ptr->Comm = _K_SVC_PIPE_PUT_REPLY;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				_k_pipe_put_reply(writer_ptr);
			}

		} else {
			/* temporary stall (must be, processing will continue
			 * later on) */
		}
	} else {
		__ASSERT_NO_MSG(1 == 0); /* we should not come ... here :-) */
	}

	/* check if we have to cancel a timer for a request */

#ifdef CANCEL_TIMERS

	if (reader_ptr) {
		if (reader_ptr->args.pipe_xfer_req.xferred_size != 0) {
			if (reader_ptr->head) {
				myfreetimer(&(reader_ptr->Time.timer));
				/* do not delist however */
			}
		}
	}
	if (writer_ptr) {
		if (writer_ptr->args.pipe_xfer_req.xferred_size != 0) {
			if (writer_ptr->head) {
				myfreetimer(&(writer_ptr->Time.timer));
				/* do not delist however */
			}
		}
	}

#endif
}
