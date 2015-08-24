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

	switch (pipe_xfer_ack->XferType) {
	case XFER_W2B: /* Writer to Buffer */
	{
		struct k_args *pWriter = pipe_xfer_ack->pWriter;

		if (pWriter) { /* Xfer from Writer finished */
			struct _pipe_xfer_req_arg *pipe_write_req =
				&pipe_xfer_ack->pWriter->args.pipe_xfer_req;

			--pipe_write_req->iNbrPendXfers;
			if (0 == pipe_write_req->iNbrPendXfers) {
				if (TERM_XXX & pipe_write_req->status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pipe_xfer_ack->pWriter);
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

			int XferId = pipe_xfer_ack->ID;

			BuffEnQA_End(&pipe_xfer_ack->pPipe->desc, XferId,
						 pipe_xfer_ack->iSize);
		}

		/* invoke continuation mechanism */

		_k_pipe_process(pipe_xfer_ack->pPipe, NULL, NULL);
		FREEARGS(pEOXfer);
		return;
	} /* XFER_W2B */

	case XFER_B2R: {
		struct k_args *pReader = pipe_xfer_ack->pReader;

		if (pReader) { /* Xfer to Reader finished */
			struct _pipe_xfer_req_arg *pipe_read_req =
				&pipe_xfer_ack->pReader->args.pipe_xfer_req;

			--pipe_read_req->iNbrPendXfers;
			if (0 == pipe_read_req->iNbrPendXfers) {
				if (TERM_XXX & pipe_read_req->status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pipe_xfer_ack->pReader);
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

			int XferId = pipe_xfer_ack->ID;

			BuffDeQA_End(&pipe_xfer_ack->pPipe->desc, XferId,
						 pipe_xfer_ack->iSize);
		}

		/* continuation mechanism */

		_k_pipe_process(pipe_xfer_ack->pPipe, NULL, NULL);
		FREEARGS(pEOXfer);
		return;

	} /* XFER_B2R */

	case XFER_W2R: {
		struct k_args *pWriter = pipe_xfer_ack->pWriter;

		if (pWriter) { /* Transfer from writer finished */
			struct _pipe_xfer_req_arg *pipe_write_req =
				&pipe_xfer_ack->pWriter->args.pipe_xfer_req;

			--pipe_write_req->iNbrPendXfers;
			if (0 == pipe_write_req->iNbrPendXfers) {
				if (TERM_XXX & pipe_write_req->status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pipe_xfer_ack->pWriter);
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
				&pipe_xfer_ack->pReader->args.pipe_xfer_req;

			--pipe_read_req->iNbrPendXfers;
			if (0 == pipe_read_req->iNbrPendXfers) {
				if (TERM_XXX & pipe_read_req->status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pipe_xfer_ack->pReader);
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

		_k_pipe_process(pipe_xfer_ack->pPipe, NULL, NULL);
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

static kpriority_t move_priority_compute(struct k_args *pWriter,
										 struct k_args *pReader)
{
	kpriority_t move_priority;

	if (!pWriter) {
		move_priority = pReader->priority;
	} else {
		move_priority = pWriter->priority;
		if (pReader && (pReader->priority < move_priority)) {
			move_priority = pReader->priority;
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
						   struct _k_pipe_struct *pPipe, XFER_TYPE XferType,
						   struct k_args *pWriter, struct k_args *pReader,
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
	pContSend->args.pipe_xfer_ack.pPipe = pPipe;
	pContSend->args.pipe_xfer_ack.XferType = XferType;
	pContSend->args.pipe_xfer_ack.ID = XferID;
	pContSend->args.pipe_xfer_ack.iSize = size;

	pContRecv->next = NULL;
	pContRecv->Comm = _K_SVC_PIPE_MOVEDATA_ACK;
	pContRecv->args.pipe_xfer_ack.pPipe = pPipe;
	pContRecv->args.pipe_xfer_ack.XferType = XferType;
	pContRecv->args.pipe_xfer_ack.ID = XferID;
	pContRecv->args.pipe_xfer_ack.iSize = size;

	A->priority = move_priority_compute(pWriter, pReader);
	pContSend->priority = A->priority;
	pContRecv->priority = A->priority;

	switch (XferType) {
	case XFER_W2B: /* Writer to Buffer */
	{
		__ASSERT_NO_MSG(NULL == pReader);
		pContSend->args.pipe_xfer_ack.pWriter = pWriter;
		pContRecv->args.pipe_xfer_ack.pWriter = NULL;
		break;
	}
	case XFER_B2R: {
		__ASSERT_NO_MSG(NULL == pWriter);
		pContSend->args.pipe_xfer_ack.pReader = NULL;
		pContRecv->args.pipe_xfer_ack.pReader = pReader;
		break;
	}
	case XFER_W2R: {
		__ASSERT_NO_MSG(NULL != pWriter && NULL != pReader);
		pContSend->args.pipe_xfer_ack.pWriter = pWriter;
		pContSend->args.pipe_xfer_ack.pReader = NULL;
		pContRecv->args.pipe_xfer_ack.pWriter = NULL;
		pContRecv->args.pipe_xfer_ack.pReader = pReader;
		break;
	}
	default:
		__ASSERT_NO_MSG(1 == 0); /* we should not come here */
	}

	A->args.MovedReq.Extra.Setup.ContSnd = pContSend;
	A->args.MovedReq.Extra.Setup.ContRcv = pContRecv;

	/*
	 * (possible optimisation)
	 * if we could know if it was a send/recv completion, we could use the
	 * SAME cmd packet for continuation on both completion of send and recv !!
	 */
}

static int ReaderInProgressIsBlocked(struct _k_pipe_struct *pPipe,
									 struct k_args *pReader)
{
	int iSizeSpaceInReader;
	int iAvailBufferData;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&pReader->args);
	option = _k_pipe_option_get(&pReader->args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(pReader->args.pipe_xfer_req.iSizeXferred))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && pReader->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pPipe->desc.iNbrPendingWrites ||
	    0 != pPipe->desc.num_pending_reads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0;
	}

	/* third condition: */

	iSizeSpaceInReader =
		pReader->args.pipe_xfer_req.iSizeTotal -
		pReader->args.pipe_xfer_req.iSizeXferred;
	BuffGetAvailDataTotal(&pPipe->desc, &iAvailBufferData);
	if (iAvailBufferData >= iSizeSpaceInReader) {
		return 0;
	} else {
		return 1;
	}
}

static int WriterInProgressIsBlocked(struct _k_pipe_struct *pPipe,
									 struct k_args *pWriter)
{
	int iSizeDataInWriter;
	int iFreeBufferSpace;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&pWriter->args);
	option = _k_pipe_option_get(&pWriter->args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(pWriter->args.pipe_xfer_req.iSizeXferred))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && pWriter->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pPipe->desc.iNbrPendingWrites ||
	    0 != pPipe->desc.num_pending_reads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0; 
	}

	/* third condition: */

	iSizeDataInWriter =
		pWriter->args.pipe_xfer_req.iSizeTotal -
		pWriter->args.pipe_xfer_req.iSizeXferred;
	BuffGetFreeSpaceTotal(&pPipe->desc, &iFreeBufferSpace);
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
 * This routine reads from the pipe.  If <pPipe> is NULL, then it uses
 * <pNewReader> as the reader.  Otherwise it takes the reader from the pipe
 * structure.
 *
 * @return N/A
 */

static void pipe_read(struct _k_pipe_struct *pPipe, struct k_args *pNewReader)
{
	struct k_args *pReader;
	struct _pipe_xfer_req_arg *pipe_read_req;

	unsigned char *read_ptr;
	int iSize;
	int id;
	int ret;
	int numIterations = 2;

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->readers;

	__ASSERT_NO_MSG((pPipe->readers == pNewReader) ||
					(NULL == pPipe->readers) || (NULL == pNewReader));

	pipe_read_req = &pReader->args.pipe_xfer_req;

	do {
		iSize = min(pPipe->desc.iAvailDataCont,
					pipe_read_req->iSizeTotal - pipe_read_req->iSizeXferred);

		if (iSize == 0) {
			return;
		}

		struct k_args *Moved_req;

		ret = BuffDeQA(&pPipe->desc, iSize, &read_ptr, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_B2R, NULL, pReader,
			(char *)(pipe_read_req->pData) +
			OCTET_TO_SIZEOFUNIT(pipe_read_req->iSizeXferred),
			read_ptr, ret, id);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_read_req->iNbrPendXfers++;
		pipe_read_req->iSizeXferred += ret;

		if (pipe_read_req->iSizeXferred == pipe_read_req->iSizeTotal) {
			_k_pipe_request_status_set(pipe_read_req, TERM_SATISFIED);
			if (pReader->Head != NULL) {
				DeListWaiter(pReader);
				myfreetimer(&pReader->Time.timer);
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
 * This routine writes to the pipe.  If <pPipe> is NULL, then it uses
 * <pNewWriter> as the writer.  Otherwise it takes the writer from the pipe
 * structure.
 *
 * @return N/A
 */

static void pipe_write(struct _k_pipe_struct *pPipe, struct k_args *pNewWriter)
{
	struct k_args *pWriter;
	struct _pipe_xfer_req_arg *pipe_write_req;

	int iSize;
	unsigned char *write_ptr;
	int id;
	int ret;
	int numIterations = 2;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->writers;

	__ASSERT_NO_MSG(!((pPipe->writers != pNewWriter) &&
					  (NULL != pPipe->writers) && (NULL != pNewWriter)));

	pipe_write_req = &pWriter->args.pipe_xfer_req;

	do {
		iSize = min((numIterations == 2) ? pPipe->desc.free_space_count
					: pPipe->desc.free_space_post_wrap_around,
					pipe_write_req->iSizeTotal - pipe_write_req->iSizeXferred);

		if (iSize == 0) {
			continue;
		}

		struct k_args *Moved_req;

		ret = BuffEnQA(&pPipe->desc, iSize, &write_ptr, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_W2B, pWriter, NULL, write_ptr,
			(char *)(pipe_write_req->pData) +
			OCTET_TO_SIZEOFUNIT(pipe_write_req->iSizeXferred),
			ret, (numIterations == 2) ? id : -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_write_req->iNbrPendXfers++;
		pipe_write_req->iSizeXferred += ret;

		if (pipe_write_req->iSizeXferred == pipe_write_req->iSizeTotal) {
			_k_pipe_request_status_set(pipe_write_req, TERM_SATISFIED);
			if (pWriter->Head != NULL) {
				/* only listed requests have a timer */
				DeListWaiter(pWriter);
				myfreetimer(&pWriter->Time.timer);
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
	pipe_xfer_req->iNbrPendXfers++;
	pipe_xfer_req->iSizeXferred += bytesXferred;

	if (pipe_xfer_req->iSizeXferred == pipe_xfer_req->iSizeTotal) {
		_k_pipe_request_status_set(pipe_xfer_req, TERM_SATISFIED);
		if (pActor->Head != NULL) {
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
	struct _k_pipe_struct *pPipe, /* ptr to pipe structure */
	struct k_args *pNewWriter,  /* ptr to new writer struct k_args */
	struct k_args *pNewReader   /* ptr to new reader struct k_args */
	)
{
	struct k_args *pReader;       /* ptr to struct k_args to be used by reader */
	struct k_args *pWriter;       /* ptr to struct k_args to be used by writer */
	struct _pipe_xfer_req_arg *pipe_write_req; /* ptr to writer's pipe process structure */
	struct _pipe_xfer_req_arg *pipe_read_req; /* ptr to reader's pipe process structure */

	int iT1;
	int iT2;
	int iT3;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->writers;

	__ASSERT_NO_MSG((pPipe->writers == pNewWriter) ||
					(NULL == pPipe->writers) || (NULL == pNewWriter));

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->readers;

	__ASSERT_NO_MSG((pPipe->readers == pNewReader) ||
					(NULL == pPipe->readers) || (NULL == pNewReader));

	/* Preparation */
	pipe_write_req = &pWriter->args.pipe_xfer_req;
	pipe_read_req = &pReader->args.pipe_xfer_req;

	/* Calculate iT1, iT2 and iT3 */
	int iFreeSpaceReader =
		(pipe_read_req->iSizeTotal - pipe_read_req->iSizeXferred);
	int iAvailDataWriter =
		(pipe_write_req->iSizeTotal - pipe_write_req->iSizeXferred);
	int iFreeSpaceBuffer =
		(pPipe->desc.free_space_count + pPipe->desc.free_space_post_wrap_around);
	int iAvailDataBuffer =
		(pPipe->desc.iAvailDataCont + pPipe->desc.iAvailDataAWA);

	iT1 = min(iFreeSpaceReader, iAvailDataBuffer);

	iFreeSpaceReader -= iT1;

	if (0 == pPipe->desc.iNbrPendingWrites) {
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
		pipe_read(pPipe, pReader);
	}

	/* T2 transfer */
	if (iT2 != 0) {
		struct k_args *Moved_req;

		__ASSERT_NO_MSG(TERM_SATISFIED != pReader->args.pipe_xfer_req.status);

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_W2R, pWriter, pReader,
			(char *)(pipe_read_req->pData) +
			OCTET_TO_SIZEOFUNIT(pipe_read_req->iSizeXferred),
			(char *)(pipe_write_req->pData) +
			OCTET_TO_SIZEOFUNIT(pipe_write_req->iSizeXferred),
			iT2, -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_xfer_status_update(pWriter, pipe_write_req, iT2);

		pipe_xfer_status_update(pReader, pipe_read_req, iT2);
	}

	/* T3 transfer */
	if (iT3 != 0) {
		__ASSERT_NO_MSG(TERM_SATISFIED != pWriter->args.pipe_xfer_req.status);
		pipe_write(pPipe, pWriter);
	}
}

void _k_pipe_process(struct _k_pipe_struct *pPipe, struct k_args *pNLWriter,
			  struct k_args *pNLReader)
{

	struct k_args *pReader = NULL;
	struct k_args *pWriter = NULL;

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
			if (pReader != pNLReader) {
				pNextReader = pPipe->readers;
				if (NULL == pNextReader) {
					if (!(TERM_XXX & pNLReader->args.pipe_xfer_req.status))
						pNextReader = pNLReader;
				}
			} else {
				/* we already used the extra non-listed Reader */
				if (TERM_XXX & pReader->args.pipe_xfer_req.status) {
					pNextReader = NULL;
				} else {
					pNextReader = pReader; /* == pNLReader */
				}
			}
		} else {
			pNextReader = pPipe->readers;
		}

		/* Writer */

		if (NULL != pNLWriter) {
			if (pWriter != pNLWriter) {
				pNextWriter = pPipe->writers;
				if (NULL == pNextWriter) {
					if (!(TERM_XXX & pNLWriter->args.pipe_xfer_req.status))
						pNextWriter = pNLWriter;
				}
			} else {
				/* we already used the extra non-listed Writer */
				if (TERM_XXX & pWriter->args.pipe_xfer_req.status) {
					pNextWriter = NULL;
				} else {
					pNextWriter = pWriter;
				}
			}
		} else {
			pNextWriter = pPipe->writers;
		}

		/* check if there is uberhaupt something to do */

		if (NULL == pNextReader && NULL == pNextWriter)
			return;
		if (pNextReader == pReader && pNextWriter == pWriter)
			break; /* nothing changed, so stop */

		/* go with pNextReader and pNextWriter */

		pReader = pNextReader;
		pWriter = pNextWriter;

		if (pWriter) {
			if (_ALL_N == _k_pipe_option_get(&pWriter->args) &&
				(pWriter->args.pipe_xfer_req.iSizeXferred == 0) &&
				_TIME_B != _k_pipe_time_type_get(&pWriter->args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeDataInWriter;
				int iSpace2WriteinReaders;
				int iFreeBufferSpace;
				int iTotalSpace2Write;

				iSpace2WriteinReaders = CalcFreeReaderSpace(pPipe->readers);
				if (pNLReader)
					iSpace2WriteinReaders +=
						(pNLReader->args.pipe_xfer_req.iSizeTotal -
						 pNLReader->args.pipe_xfer_req.iSizeXferred);
				BuffGetFreeSpaceTotal(&pPipe->desc, &iFreeBufferSpace);
				iTotalSpace2Write =
					iFreeBufferSpace + iSpace2WriteinReaders;
				iSizeDataInWriter =
					pWriter->args.pipe_xfer_req.iSizeTotal -
					pWriter->args.pipe_xfer_req.iSizeXferred;

				if (iSizeDataInWriter > iTotalSpace2Write) {
					bALLNWriterNoGo = true;
				}
			}
		}
		if (pReader) {
			if (_ALL_N == _k_pipe_option_get(&pReader->args) &&
				(pReader->args.pipe_xfer_req.iSizeXferred == 0) &&
				_TIME_B != _k_pipe_time_type_get(&pReader->args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeFreeSpaceInReader;
				int iData2ReadFromWriters;
				int iAvailBufferData;
				int iTotalData2Read;

				iData2ReadFromWriters = CalcAvailWriterData(pPipe->writers);
				if (pNLWriter)
					iData2ReadFromWriters +=
						(pNLWriter->args.pipe_xfer_req.iSizeTotal -
						 pNLWriter->args.pipe_xfer_req.iSizeXferred);
				BuffGetAvailDataTotal(&pPipe->desc, &iAvailBufferData);
				iTotalData2Read = iAvailBufferData + iData2ReadFromWriters;
				iSizeFreeSpaceInReader =
					pReader->args.pipe_xfer_req.iSizeTotal -
					pReader->args.pipe_xfer_req.iSizeXferred;

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
			if (!BuffEmpty(&pPipe->desc)) {
				if (pReader) {
					pipe_read(pPipe, pReader);
					continue;
				} else {
					/* we could break as well,
					   but then nothing else will happen */
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (pReader && (_TIME_NB !=
					_k_pipe_time_type_get(&pWriter->args))) {
					/* force transfer
					   (we make exception for non-blocked writer) */
					pipe_read_write(pPipe, pWriter, pReader);
					continue;
				} else
#endif
					/* we could break as well,
					   but then nothing else will happen */
					return;
			}
		} else if (bALLNReaderNoGo) {
			/* investigate if we must force a transfer to avoid a stall */
			if (!BuffFull(&pPipe->desc)) {
				if (pWriter) {
					pipe_write(pPipe, pWriter);
					continue;
				} else {
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (pWriter && (_TIME_NB !=
						_k_pipe_time_type_get(&pReader->args))) {
					/* force transfer
					   (we make exception for non-blocked reader) */
					pipe_read_write(pPipe, pWriter, pReader);
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
			if (pReader) {
				if (pWriter) {
					pipe_read_write(pPipe, pWriter, pReader);
					continue;
				} else {
					pipe_read(pPipe, pReader);
					continue;
				}
			} else {
				if (pWriter) {
					pipe_write(pPipe, pWriter);
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

	pReader = pNextReader;
	pWriter = pNextWriter;

	/* if we come here, it is b/c pReader and pWriter did not change
	anymore.
	- Normally one of them is NULL, which means only a writer, resp. a
	reader remained.
	- The case that none of them is NULL is a special case which 'normally'
	does not occur.
	A remaining pReader and/or pWriter are expected to be not-completed.

	  Note that in the case there is only a reader or there is only a
	writer, it can be a ALL_N request.
	  This happens when his request has not been processed completely yet
	(b/c of a copy in and copy out
	  conflict in the buffer e.g.), but is expected to be processed
	completely somewhat later (must be !)
	  */

	/* in the sequel, we will:
	   1. check the hypothesis that an existing pReader/pWriter is not
	   completed
	   2. check if we can force the termination of a X_TO_N request when
	   some data transfer took place
	   3. check if we have to cancel a timer when the (first) data has been
	   Xferred
	   4. Check if we have to kick out a queued request because its
	   processing is really blocked (for some reason)
	 */
	if (pReader && pWriter) {
		__ASSERT_NO_MSG(!(TERM_XXX & pReader->args.pipe_xfer_req.status) &&
						!(TERM_XXX & pWriter->args.pipe_xfer_req.status));
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
	} else if (pReader) {
		__ASSERT_NO_MSG(!(TERM_XXX & pReader->args.pipe_xfer_req.status));

		/* check if this lonely reader is really blocked, then we will
		   delist him
		   (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (ReaderInProgressIsBlocked(pPipe, pReader)) {
			if (_X_TO_N & _k_pipe_option_get(&pReader->args) &&
			    (pReader->args.pipe_xfer_req.iSizeXferred != 0)) {
				_k_pipe_request_status_set(&pReader->args.pipe_xfer_req,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&pReader->args.pipe_xfer_req,
					TERM_FORCED);
			}

			if (pReader->Head) {
				DeListWaiter(pReader);
				myfreetimer(&(pReader->Time.timer));
			}
			if (0 == pReader->args.pipe_xfer_req.iNbrPendXfers) {
				pReader->Comm = _K_SVC_PIPE_GET_REPLY;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				_k_pipe_get_reply(pReader);
			}
		} else {
			/* temporary stall (must be, processing will continue
			 * later on) */
		}
	} else if (pWriter) {
		__ASSERT_NO_MSG(!(TERM_SATISFIED & pWriter->args.pipe_xfer_req.status));

		/* check if this lonely Writer is really blocked, then we will
		   delist him (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (WriterInProgressIsBlocked(pPipe, pWriter)) {
			if (_X_TO_N & _k_pipe_option_get(&pWriter->args) &&
			    (pWriter->args.pipe_xfer_req.iSizeXferred != 0)) {
				_k_pipe_request_status_set(&pWriter->args.pipe_xfer_req,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&pWriter->args.pipe_xfer_req,
					TERM_FORCED);
			}

			if (pWriter->Head) {
				DeListWaiter(pWriter);
				myfreetimer(&(pWriter->Time.timer));
			}
			if (0 == pWriter->args.pipe_xfer_req.iNbrPendXfers) {
				pWriter->Comm = _K_SVC_PIPE_PUT_REPLY;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				_k_pipe_put_reply(pWriter);
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

	if (pReader) {
		if (pReader->args.pipe_xfer_req.iSizeXferred != 0) {
			if (pReader->Head) {
				myfreetimer(&(pReader->Time.timer));
				/* do not delist however */
			}
		}
	}
	if (pWriter) {
		if (pWriter->args.pipe_xfer_req.iSizeXferred != 0) {
			if (pWriter->Head) {
				myfreetimer(&(pWriter->Time.timer));
				/* do not delist however */
			}
		}
	}

#endif
}
