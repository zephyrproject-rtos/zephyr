/* K_ChProc.c */

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

#include <minik.h>
#include <kchan.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>
#include <ch_buff.h>

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


int WriterInProgressIsBlocked(struct pipe_struct *pPipe, struct k_args *pWriter);
int ReaderInProgressIsBlocked(struct pipe_struct *pPipe, struct k_args *pReader);

void K_ChProc(struct pipe_struct *pPipe, struct k_args *pNLWriter,
			  struct k_args *pNLReader) /* this is not a K_ function */
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
		BOOL bALLNWriterNoGo = FALSE;
		BOOL bALLNReaderNoGo = FALSE;

		/* Reader */

		if (NULL != pNLReader) {
			if (pReader != pNLReader) {
				pNextReader = pPipe->Readers;
				if (NULL == pNextReader) {
					if (!(TERM_XXX & ChReqGetStatus(&(pNLReader->Args.ChProc))))
						pNextReader = pNLReader;
				}
			} else {
				/* we already used the extra non-listed Reader */
				if (TERM_XXX & ChReqGetStatus(&(pReader->Args.ChProc))) {
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
					if (!(TERM_XXX & ChReqGetStatus(&(pNLWriter->Args.ChProc))))
						pNextWriter = pNLWriter;
				}
			} else {
				/* we already used the extra non-listed Writer */
				if (TERM_XXX & ChReqGetStatus(&(pWriter->Args.ChProc))) {
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
			if (_ALL_N == ChxxxGetChOpt(&(pWriter->Args)) &&
				!ChReqSizeXferred(&(pWriter->Args.ChProc)) &&
				_TIME_B != ChxxxGetTimeType((K_ARGS_ARGS *)&(pWriter->Args))) {
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
					bALLNWriterNoGo = TRUE;
				}
			}
		}
		if (pReader) {
			if (_ALL_N == ChxxxGetChOpt(&(pReader->Args)) &&
				!ChReqSizeXferred(&(pReader->Args.ChProc)) &&
				_TIME_B != ChxxxGetTimeType((K_ARGS_ARGS *)&(pReader->Args))) {
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
					bALLNReaderNoGo = TRUE;
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
					K_ChProcRO(pPipe, pReader);
					continue;
				} else {
					/* we could break as well,
					   but then nothing else will happen */
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (pReader && (_TIME_NB !=
					ChxxxGetTimeType((K_ARGS_ARGS *)&(pWriter->Args)))) {
					/* force transfer
					   (we make exception for non-blocked writer) */
					K_ChProcWR(pPipe, pWriter, pReader);
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
					K_ChProcWO(pPipe, pWriter);
					continue;
				} else {
					return;
				}
			} else {
#ifdef FORCE_XFER_ON_STALL
				if (pWriter && (_TIME_NB !=
						ChxxxGetTimeType((K_ARGS_ARGS *)&(pReader->Args)))) {
					/* force transfer
					   (we make exception for non-blocked reader) */
					K_ChProcWR(pPipe, pWriter, pReader);
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
					K_ChProcWR(pPipe, pWriter, pReader);
					continue;
				} else {
					K_ChProcRO(pPipe, pReader);
					continue;
				}
			} else {
				if (pWriter) {
					K_ChProcWO(pPipe, pWriter);
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
	  This happens when his request has not been processed  completely yet
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
		__ASSERT_NO_MSG(!(TERM_XXX & ChReqGetStatus(&(pReader->Args.ChProc))) &&
						!(TERM_XXX & ChReqGetStatus(&(pWriter->Args.ChProc))));
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
		__ASSERT_NO_MSG(!(TERM_XXX & ChReqGetStatus(&(pReader->Args.ChProc))));

		/* check if this lonely reader is really blocked, then we will
		   delist him
		   (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (ReaderInProgressIsBlocked(pPipe, pReader)) {
			if (_X_TO_N & ChxxxGetChOpt(&(pReader->Args)) &&
			    ChReqSizeXferred(&(pReader->Args.ChProc))) {
				ChReqSetStatus(&(pReader->Args.ChProc), TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				ChReqSetStatus(&(pReader->Args.ChProc), TERM_FORCED);
			}

			if (pReader->Head) {
				DeListWaiter(pReader);
				myfreetimer(&(pReader->Time.timer));
			}
			if (0 == pReader->Args.ChProc.iNbrPendXfers) {
				pReader->Comm = CHDEQ_RPL;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				K_ChRecvRpl(pReader);
			}
		} else {
			/* temporary stall (must be, processing will continue
			 * later on) */
		}
	} else if (pWriter) {
		__ASSERT_NO_MSG(!(TERM_SATISFIED &
						  ChReqGetStatus(&(pWriter->Args.ChProc))));

		/* check if this lonely Writer is really blocked, then we will
		   delist him (if he was listed uberhaupt) == EMERGENCY BREAK */
		if (WriterInProgressIsBlocked(pPipe, pWriter)) {
			if (_X_TO_N & ChxxxGetChOpt(&(pWriter->Args)) &&
			    ChReqSizeXferred(&(pWriter->Args.ChProc))) {
				ChReqSetStatus(&(pWriter->Args.ChProc), TERM_SATISFIED);
			} else {
				/* in all other cases: forced termination */
				ChReqSetStatus(&(pWriter->Args.ChProc), TERM_FORCED);
			}

			if (pWriter->Head) {
				DeListWaiter(pWriter);
				myfreetimer(&(pWriter->Time.timer));
			}
			if (0 == pWriter->Args.ChProc.iNbrPendXfers) {
				pWriter->Comm = CHENQ_RPL;
				/* if terminated and no pending Xfers anymore,
				   we have to reply */
				K_ChSendRpl(pWriter);
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
		if (ChReqSizeXferred(&(pReader->Args.ChProc))) {
			if (pReader->Head) {
				myfreetimer(&(pReader->Time.timer));
				/* do not delist however */
			}
		}
	}
	if (pWriter) {
		if (ChReqSizeXferred(&(pWriter->Args.ChProc))) {
			if (pWriter->Head) {
				myfreetimer(&(pWriter->Time.timer));
				/* do not delist however */
			}
		}
	}

#endif
}

int WriterInProgressIsBlocked(struct pipe_struct *pPipe,
							  struct k_args *pWriter)
{
	int iSizeDataInWriter;
	int iFreeBufferSpace;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = ChxxxGetTimeType((K_ARGS_ARGS *)&(pWriter->Args));
	option = ChxxxGetChOpt((K_ARGS_ARGS *)&(pWriter->Args));
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

int ReaderInProgressIsBlocked(struct pipe_struct *pPipe,
							  struct k_args *pReader)
{
	int iSizeSpaceInReader;
	int iAvailBufferData;
	TIME_TYPE TimeType;
	K_PIPE_OPTION option;

	/* first condition: request cannot wait any longer: must be -
	 * (non-blocked) or a finite timed wait with a killed timer */

	TimeType = ChxxxGetTimeType((K_ARGS_ARGS *)&(pReader->Args));
	option = ChxxxGetChOpt((K_ARGS_ARGS *)&(pReader->Args));
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
