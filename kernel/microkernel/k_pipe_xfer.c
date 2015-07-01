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
K_ProcWR()
		K_ProcWR() copies remaining data into buffer; better would be to
possibly copy the remaining data
		to the next requester (if there is one)
	  * ...
 */


/**
 *
 * _k_pipe_movedata_ack -
 *
 * RETURNS: N/A
 */

void _k_pipe_movedata_ack(struct k_args *pEOXfer)
{
	struct k_chmovedack *pEOXferArgs = &(pEOXfer->Args.ChMovedAck);

	switch (pEOXferArgs->XferType) {
	case XFER_W2B: /* Writer to Buffer */
	{
		struct k_args *pWriter = pEOXferArgs->pWriter;

		if (pWriter) { /* Xfer from Writer finished */
			struct k_chproc *pWriterArg =
				&(pEOXferArgs->pWriter->Args.ChProc);

			--(pWriterArg->iNbrPendXfers);
			if (0 == pWriterArg->iNbrPendXfers) {
				if (TERM_XXX & pWriterArg->Status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pEOXferArgs->pWriter);
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pWriterArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Xfer to Buffer finished */

			int XferId = pEOXferArgs->ID;
			BuffEnQA_End(&(pEOXferArgs->pPipe->Buff), XferId,
						 pEOXferArgs->iSize);
		}

		/* invoke continuation mechanism */

		_k_pipe_process(pEOXferArgs->pPipe, NULL, NULL);
		FREEARGS(pEOXfer);
		return;
	} /* XFER_W2B */

	case XFER_B2R: {
		struct k_args *pReader = pEOXferArgs->pReader;

		if (pReader) { /* Xfer to Reader finished */
			struct k_chproc *pReaderArg =
				&(pEOXferArgs->pReader->Args.ChProc);

			--(pReaderArg->iNbrPendXfers);
			if (0 == pReaderArg->iNbrPendXfers) {
				if (TERM_XXX & pReaderArg->Status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pEOXferArgs->pReader);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pReaderArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Xfer from Buffer finished */

			int XferId = pEOXferArgs->ID;

			BuffDeQA_End(&(pEOXferArgs->pPipe->Buff), XferId,
						 pEOXferArgs->iSize);
		}

		/* continuation mechanism */

		_k_pipe_process(pEOXferArgs->pPipe, NULL, NULL);
		FREEARGS(pEOXfer);
		return;

	} /* XFER_B2R */

	case XFER_W2R: {
		struct k_args *pWriter = pEOXferArgs->pWriter;

		if (pWriter) { /* Transfer from writer finished */
			struct k_chproc *pWriterArg =
				&(pEOXferArgs->pWriter->Args.ChProc);

			--(pWriterArg->iNbrPendXfers);
			if (0 == pWriterArg->iNbrPendXfers) {
				if (TERM_XXX & pWriterArg->Status) {
					/* request is terminated, send reply */
					_k_pipe_put_reply(pEOXferArgs->pWriter);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pWriterArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		} else {
			/* Transfer to Reader finished */

			struct k_chproc *pReaderArg =
				&(pEOXferArgs->pReader->Args.ChProc);

			--(pReaderArg->iNbrPendXfers);
			if (0 == pReaderArg->iNbrPendXfers) {
				if (TERM_XXX & pReaderArg->Status) {
					/* request is terminated, send reply */
					_k_pipe_get_reply(pEOXferArgs->pReader);
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			} else {
				if (TERM_XXX & pReaderArg->Status) {
					/* do nothing */
					/* invoke continuation mechanism (fall through) */
				} else {
					/* invoke continuation mechanism (fall through) */
				}
			}
		}

		/* invoke continuation mechanism */

		_k_pipe_process(pEOXferArgs->pPipe, NULL, NULL);
		FREEARGS(pEOXfer);
		return;
	} /* XFER_W2B */

	default:
		break;
	}
}

/**
 *
 * move_priority_compute - determines priority for data move operation
 *
 * Uses priority level of most important participant.
 *
 * Note: It's OK to have one or two participants, but there can't be none!
 *
 * RETURNS: N/A
 */

static kpriority_t move_priority_compute(struct k_args *pWriter,
										 struct k_args *pReader)
{
	kpriority_t move_priority;

	if (!pWriter) {
		move_priority = pReader->Prio;
	} else {
		move_priority = pWriter->Prio;
		if (pReader && (pReader->Prio < move_priority)) {
			move_priority = pReader->Prio;
		}
	}

	return move_priority;
}

/**
 *
 * setup_movedata -
 *
 * RETURNS: N/A
 */

static void setup_movedata(struct k_args *A,
						   struct pipe_struct *pPipe, XFER_TYPE XferType,
						   struct k_args *pWriter, struct k_args *pReader,
						   void *destination, void *source,
						   uint32_t size, int XferID)
{
	struct k_args *pContSend;
	struct k_args *pContRecv;

	A->Comm = MVD_REQ;

	A->Ctxt.proc = NULL;
	/* this caused problems when != NULL related to set/reset of state bits */

	A->Args.MovedReq.Action = (MovedAction)(MVDACT_SNDACK | MVDACT_RCVACK);
	A->Args.MovedReq.source = source;
	A->Args.MovedReq.destination = destination;
	A->Args.MovedReq.iTotalSize = size;

	/* continuation packet */

	GETARGS(pContSend);
	GETARGS(pContRecv);

	pContSend->Forw = NULL;
	pContSend->Comm = PIPE_MOVEDATA_ACK;
	pContSend->Args.ChMovedAck.pPipe = pPipe;
	pContSend->Args.ChMovedAck.XferType = XferType;
	pContSend->Args.ChMovedAck.ID = XferID;
	pContSend->Args.ChMovedAck.iSize = size;

	pContRecv->Forw = NULL;
	pContRecv->Comm = PIPE_MOVEDATA_ACK;
	pContRecv->Args.ChMovedAck.pPipe = pPipe;
	pContRecv->Args.ChMovedAck.XferType = XferType;
	pContRecv->Args.ChMovedAck.ID = XferID;
	pContRecv->Args.ChMovedAck.iSize = size;

	A->Prio = move_priority_compute(pWriter, pReader);
	pContSend->Prio = A->Prio;
	pContRecv->Prio = A->Prio;

	switch (XferType) {
	case XFER_W2B: /* Writer to Buffer */
	{
		__ASSERT_NO_MSG(NULL == pReader);
		pContSend->Args.ChMovedAck.pWriter = pWriter;
		pContRecv->Args.ChMovedAck.pWriter = NULL;
		break;
	}
	case XFER_B2R: {
		__ASSERT_NO_MSG(NULL == pWriter);
		pContSend->Args.ChMovedAck.pReader = NULL;
		pContRecv->Args.ChMovedAck.pReader = pReader;
		break;
	}
	case XFER_W2R: {
		__ASSERT_NO_MSG(NULL != pWriter && NULL != pReader);
		pContSend->Args.ChMovedAck.pWriter = pWriter;
		pContSend->Args.ChMovedAck.pReader = NULL;
		pContRecv->Args.ChMovedAck.pWriter = NULL;
		pContRecv->Args.ChMovedAck.pReader = pReader;
		break;
	}
	default:
		__ASSERT_NO_MSG(1 == 0); /* we should not come here */
	}

	A->Args.MovedReq.Extra.Setup.ContSnd = pContSend;
	A->Args.MovedReq.Extra.Setup.ContRcv = pContRecv;

	/*
	 * (possible optimisation)
	 * if we could know if it was a send/recv completion, we could use the
	 * SAME cmd packet for continuation on both completion of send and recv !!
	 */
}

static int ReaderInProgressIsBlocked(struct pipe_struct *pPipe,
									 struct k_args *pReader)
{
	int iSizeSpaceInReader;
	int iAvailBufferData;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&pReader->Args);
	option = _k_pipe_option_get(&pReader->Args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(pReader->Args.ChProc.iSizeXferred))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && pReader->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pPipe->Buff.iNbrPendingWrites ||
	    0 != pPipe->Buff.iNbrPendingReads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0;
	}

	/* third condition: */

	iSizeSpaceInReader =
		pReader->Args.ChProc.iSizeTotal - pReader->Args.ChProc.iSizeXferred;
	BuffGetAvailDataTotal(&(pPipe->Buff), &iAvailBufferData);
	if (iAvailBufferData >= iSizeSpaceInReader) {
		return 0;
	} else {
		return 1;
	}
}

static int WriterInProgressIsBlocked(struct pipe_struct *pPipe,
									 struct k_args *pWriter)
{
	int iSizeDataInWriter;
	int iFreeBufferSpace;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = _k_pipe_time_type_get(&pWriter->Args);
	option = _k_pipe_option_get(&pWriter->Args);
	if (((_TIME_B == TimeType) && (_ALL_N == option)) ||
	    ((_TIME_B == TimeType) && (_X_TO_N & option) &&
	     !(pWriter->Args.ChProc.iSizeXferred))
#ifdef CANCEL_TIMERS
	    || ((_TIME_BT == TimeType) && pWriter->Time.timer)
#endif
	    ) {
		/* requester can still wait (for some time or forever),
		   no problem for now */
		return 0;
	}

	/* second condition: buffer activity is null */

	if (0 != pPipe->Buff.iNbrPendingWrites ||
	    0 != pPipe->Buff.iNbrPendingReads) {
		/* buffer activity detected, can't say now that processing is blocked */
		return 0; 
	}

	/* third condition: */

	iSizeDataInWriter =
		pWriter->Args.ChProc.iSizeTotal - pWriter->Args.ChProc.iSizeXferred;
	BuffGetFreeSpaceTotal(&(pPipe->Buff), &iFreeBufferSpace);
	if (iFreeBufferSpace >= iSizeDataInWriter) {
		return 0;
	} else {
		return 1;
	}
}

/**
 *
 * pipe_read - read from the channel
 *
 * This routine reads from the channel.  If <pPipe> is NULL, then it uses
 * <pNewReader> as the reader.  Otherwise it takes the reader from the channel
 * structure.
 *
 * RETURNS: N/A
 */

static void pipe_read(struct pipe_struct *pPipe, struct k_args *pNewReader)
{
	struct k_args *pReader;
	struct k_chproc *pReaderArgs;

	unsigned char *pRead;
	int iSize;
	int id;
	int ret;
	int numIterations = 2;

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->Readers;

	__ASSERT_NO_MSG((pPipe->Readers == pNewReader) ||
					(NULL == pPipe->Readers) || (NULL == pNewReader));

	pReaderArgs = &pReader->Args.ChProc;

	do {
		iSize = min(pPipe->Buff.iAvailDataCont,
					pReaderArgs->iSizeTotal - pReaderArgs->iSizeXferred);

		if (iSize == 0) {
			return;
		}

		struct k_args *Moved_req;

		ret = BuffDeQA(&pPipe->Buff, iSize, &pRead, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_B2R, NULL, pReader,
			(char *)(pReaderArgs->pData) +
			OCTET_TO_SIZEOFUNIT(pReaderArgs->iSizeXferred),
			pRead, ret, id);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pReaderArgs->iNbrPendXfers++;
		pReaderArgs->iSizeXferred += ret;

		if (pReaderArgs->iSizeXferred == pReaderArgs->iSizeTotal) {
			_k_pipe_request_status_set(pReaderArgs, TERM_SATISFIED);
			if (pReader->Head != NULL) {
				DeListWaiter(pReader);
				myfreetimer(&pReader->Time.timer);
			}
			return;
		} else {
			_k_pipe_request_status_set(pReaderArgs, XFER_BUSY);
		}

	} while (--numIterations != 0);
}

/**
 *
 * pipe_write - write to the channel
 *
 * This routine writes to the channel.  If <pPipe> is NULL, then it uses
 * <pNewWriter> as the writer.  Otherwise it takes the writer from the channel
 * structure.
 *
 * RETURNS: N/A
 */

static void pipe_write(struct pipe_struct *pPipe, struct k_args *pNewWriter)
{
	struct k_args *pWriter;
	struct k_chproc *pWriterArgs;

	int iSize;
	unsigned char *pWrite;
	int id;
	int ret;
	int numIterations = 2;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->Writers;

	__ASSERT_NO_MSG(!((pPipe->Writers != pNewWriter) &&
					  (NULL != pPipe->Writers) && (NULL != pNewWriter)));

	pWriterArgs = &pWriter->Args.ChProc;

	do {
		iSize = min((numIterations == 2) ? pPipe->Buff.iFreeSpaceCont
					: pPipe->Buff.iFreeSpaceAWA,
					pWriterArgs->iSizeTotal - pWriterArgs->iSizeXferred);

		if (iSize == 0) {
			continue;
		}

		struct k_args *Moved_req;

		ret = BuffEnQA(&pPipe->Buff, iSize, &pWrite, &id);
		if (0 == ret) {
			return;
		}

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_W2B, pWriter, NULL, pWrite,
			(char *)(pWriterArgs->pData) +
			OCTET_TO_SIZEOFUNIT(pWriterArgs->iSizeXferred),
			ret, (numIterations == 2) ? id : -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pWriterArgs->iNbrPendXfers++;
		pWriterArgs->iSizeXferred += ret;

		if (pWriterArgs->iSizeXferred == pWriterArgs->iSizeTotal) {
			_k_pipe_request_status_set(pWriterArgs, TERM_SATISFIED);
			if (pWriter->Head != NULL) {
				/* only listed requests have a timer */
				DeListWaiter(pWriter);
				myfreetimer(&pWriter->Time.timer);
			}
			return;
		} else {
			_k_pipe_request_status_set(pWriterArgs, XFER_BUSY);
		}

	} while (--numIterations != 0);
}

/**
 *
 * pipe_xfer_status_update - update the channel transfer status
 *
 * RETURNS: N/A
 */

static void pipe_xfer_status_update(
	struct k_args *pActor,       /* ptr to struct k_args to be used by actor */
	struct k_chproc *pActorArgs, /* ptr to actor's channel process structure */
	int bytesXferred      /* # of bytes transferred */
	)
{
	pActorArgs->iNbrPendXfers++;
	pActorArgs->iSizeXferred += bytesXferred;

	if (pActorArgs->iSizeXferred == pActorArgs->iSizeTotal) {
		_k_pipe_request_status_set(pActorArgs, TERM_SATISFIED);
		if (pActor->Head != NULL) {
			DeListWaiter(pActor);
			myfreetimer(&pActor->Time.timer);
		}
	} else {
		_k_pipe_request_status_set(pActorArgs, XFER_BUSY);
	}
}

/**
 *
 * pipe_read_write - read and/or write from/to the channel
 *
 * RETURNS: N/A
 */

static void pipe_read_write(
	struct pipe_struct *pPipe, /* ptr to channel structure */
	struct k_args *pNewWriter,  /* ptr to new writer struct k_args */
	struct k_args *pNewReader   /* ptr to new reader struct k_args */
	)
{
	struct k_args *pReader;       /* ptr to struct k_args to be used by reader */
	struct k_args *pWriter;       /* ptr to struct k_args to be used by writer */
	struct k_chproc *pWriterArgs; /* ptr to writer's channel process structure */
	struct k_chproc *pReaderArgs; /* ptr to reader's channel process structure */

	int iT1;
	int iT2;
	int iT3;

	pWriter = (pNewWriter != NULL) ? pNewWriter : pPipe->Writers;

	__ASSERT_NO_MSG((pPipe->Writers == pNewWriter) ||
					(NULL == pPipe->Writers) || (NULL == pNewWriter));

	pReader = (pNewReader != NULL) ? pNewReader : pPipe->Readers;

	__ASSERT_NO_MSG((pPipe->Readers == pNewReader) ||
					(NULL == pPipe->Readers) || (NULL == pNewReader));

	/* Preparation */
	pWriterArgs = &pWriter->Args.ChProc;
	pReaderArgs = &pReader->Args.ChProc;

	/* Calculate iT1, iT2 and iT3 */
	int iFreeSpaceReader =
		(pReaderArgs->iSizeTotal - pReaderArgs->iSizeXferred);
	int iAvailDataWriter =
		(pWriterArgs->iSizeTotal - pWriterArgs->iSizeXferred);
	int iFreeSpaceBuffer =
		(pPipe->Buff.iFreeSpaceCont + pPipe->Buff.iFreeSpaceAWA);
	int iAvailDataBuffer =
		(pPipe->Buff.iAvailDataCont + pPipe->Buff.iAvailDataAWA);

	iT1 = min(iFreeSpaceReader, iAvailDataBuffer);

	iFreeSpaceReader -= iT1;

	if (0 == pPipe->Buff.iNbrPendingWrites) {
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

		__ASSERT_NO_MSG(TERM_SATISFIED != pReader->Args.ChProc.Status);

		GETARGS(Moved_req);
		setup_movedata(Moved_req, pPipe, XFER_W2R, pWriter, pReader,
			(char *)(pReaderArgs->pData) +
			OCTET_TO_SIZEOFUNIT(pReaderArgs->iSizeXferred),
			(char *)(pWriterArgs->pData) +
			OCTET_TO_SIZEOFUNIT(pWriterArgs->iSizeXferred),
			iT2, -1);
		_k_movedata_request(Moved_req);
		FREEARGS(Moved_req);

		pipe_xfer_status_update(pWriter, pWriterArgs, iT2);

		pipe_xfer_status_update(pReader, pReaderArgs, iT2);
	}

	/* T3 transfer */
	if (iT3 != 0) {
		__ASSERT_NO_MSG(TERM_SATISFIED != pWriter->Args.ChProc.Status);
		pipe_write(pPipe, pWriter);
	}
}

void _k_pipe_process(struct pipe_struct *pPipe, struct k_args *pNLWriter,
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
				pNextReader = pPipe->Readers;
				if (NULL == pNextReader) {
					if (!(TERM_XXX & pNLReader->Args.ChProc.Status))
						pNextReader = pNLReader;
				}
			} else {
				/* we already used the extra non-listed Reader */
				if (TERM_XXX & pReader->Args.ChProc.Status) {
					pNextReader = NULL;
				} else {
					pNextReader = pReader; /* == pNLReader */
				}
			}
		} else {
			pNextReader = pPipe->Readers;
		}

		/* Writer */

		if (NULL != pNLWriter) {
			if (pWriter != pNLWriter) {
				pNextWriter = pPipe->Writers;
				if (NULL == pNextWriter) {
					if (!(TERM_XXX & pNLWriter->Args.ChProc.Status))
						pNextWriter = pNLWriter;
				}
			} else {
				/* we already used the extra non-listed Writer */
				if (TERM_XXX & pWriter->Args.ChProc.Status) {
					pNextWriter = NULL;
				} else {
					pNextWriter = pWriter;
				}
			}
		} else {
			pNextWriter = pPipe->Writers;
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
			if (_ALL_N == _k_pipe_option_get(&pWriter->Args) &&
				(pWriter->Args.ChProc.iSizeXferred == 0) &&
				_TIME_B != _k_pipe_time_type_get(&pWriter->Args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeDataInWriter;
				int iSpace2WriteinReaders;
				int iFreeBufferSpace;
				int iTotalSpace2Write;

				iSpace2WriteinReaders = CalcFreeReaderSpace(pPipe->Readers);
				if (pNLReader)
					iSpace2WriteinReaders +=
						(pNLReader->Args.ChProc.iSizeTotal -
						 pNLReader->Args.ChProc.iSizeXferred);
				BuffGetFreeSpaceTotal(&(pPipe->Buff), &iFreeBufferSpace);
				iTotalSpace2Write =
					iFreeBufferSpace + iSpace2WriteinReaders;
				iSizeDataInWriter =
					pWriter->Args.ChProc.iSizeTotal -
					pWriter->Args.ChProc.iSizeXferred;

				if (iSizeDataInWriter > iTotalSpace2Write) {
					bALLNWriterNoGo = true;
				}
			}
		}
		if (pReader) {
			if (_ALL_N == _k_pipe_option_get(&pReader->Args) &&
				(pReader->Args.ChProc.iSizeXferred == 0) &&
				_TIME_B != _k_pipe_time_type_get(&pReader->Args)) {
				/* investigate if there is a problem for
				 * his request to be satisfied
				 */
				int iSizeFreeSpaceInReader;
				int iData2ReadFromWriters;
				int iAvailBufferData;
				int iTotalData2Read;

				iData2ReadFromWriters = CalcAvailWriterData(pPipe->Writers);
				if (pNLWriter)
					iData2ReadFromWriters +=
						(pNLWriter->Args.ChProc.iSizeTotal -
						 pNLWriter->Args.ChProc.iSizeXferred);
				BuffGetAvailDataTotal( &(pPipe->Buff), &iAvailBufferData);
				iTotalData2Read = iAvailBufferData + iData2ReadFromWriters;
				iSizeFreeSpaceInReader =
					pReader->Args.ChProc.iSizeTotal -
					pReader->Args.ChProc.iSizeXferred;

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
			if (!BuffEmpty(&(pPipe->Buff))) {
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
					_k_pipe_time_type_get(&pWriter->Args))) {
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
			if (!BuffFull(&(pPipe->Buff))) {
				if (pWriter) {
					pipe_write(pPipe, pWriter);
					continue;
				} else {
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (pWriter && (_TIME_NB !=
						_k_pipe_time_type_get(&pReader->Args))) {
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
		__ASSERT_NO_MSG(!(TERM_XXX & pReader->Args.ChProc.Status) &&
						!(TERM_XXX & pWriter->Args.ChProc.Status));
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
		__ASSERT_NO_MSG(!(TERM_XXX & pReader->Args.ChProc.Status));

		/* check if this lonely reader is really blocked, then we will
		   delist him
		   (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (ReaderInProgressIsBlocked(pPipe, pReader)) {
			if (_X_TO_N & _k_pipe_option_get(&pReader->Args) &&
			    (pReader->Args.ChProc.iSizeXferred != 0)) {
				_k_pipe_request_status_set(&pReader->Args.ChProc,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&pReader->Args.ChProc, TERM_FORCED);
			}

			if (pReader->Head) {
				DeListWaiter(pReader);
				myfreetimer(&(pReader->Time.timer));
			}
			if (0 == pReader->Args.ChProc.iNbrPendXfers) {
				pReader->Comm = PIPE_GET_REPLY;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				_k_pipe_get_reply(pReader);
			}
		} else {
			/* temporary stall (must be, processing will continue
			 * later on) */
		}
	} else if (pWriter) {
		__ASSERT_NO_MSG(!(TERM_SATISFIED & pWriter->Args.ChProc.Status));

		/* check if this lonely Writer is really blocked, then we will
		   delist him (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (WriterInProgressIsBlocked(pPipe, pWriter)) {
			if (_X_TO_N & _k_pipe_option_get(&pWriter->Args) &&
			    (pWriter->Args.ChProc.iSizeXferred != 0)) {
				_k_pipe_request_status_set(&pWriter->Args.ChProc,
					TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				_k_pipe_request_status_set(&pWriter->Args.ChProc, TERM_FORCED);
			}

			if (pWriter->Head) {
				DeListWaiter(pWriter);
				myfreetimer(&(pWriter->Time.timer));
			}
			if (0 == pWriter->Args.ChProc.iNbrPendXfers) {
				pWriter->Comm = PIPE_PUT_REPLY;
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
		if (pReader->Args.ChProc.iSizeXferred != 0) {
			if (pReader->Head) {
				myfreetimer(&(pReader->Time.timer));
				/* do not delist however */
			}
		}
	}
	if (pWriter) {
		if (pWriter->Args.ChProc.iSizeXferred != 0) {
			if (pWriter->Head) {
				myfreetimer(&(pWriter->Time.timer));
				/* do not delist however */
			}
		}
	}

#endif
}
