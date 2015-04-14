/* ch_addit.c */

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

#include "microkernel/k_struct.h"
#include "kmemcpy.h"
#include "minik.h"
#include "kticks.h"
#include <string_s.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*****************************************************************************/

void DeListWaiter(struct k_args *pReqProc)
{
	__ASSERT_NO_MSG(NULL != pReqProc->Head);
	REMOVE_ELM(pReqProc);
	pReqProc->Head = NULL;
}

void myfreetimer(K_TIMER * *ppTimer)
{
	if (*ppTimer) {
		delist_timer(*ppTimer);
		FREETIMER(*ppTimer);
		*ppTimer = NULL;
	}
}

void mycopypacket(struct k_args **out,
					struct k_args *in) /* borrowed from mailbox
						       implementation */
/* my version will ALWAYS make a copy = as the name implies it!! */
{
	/*if ( in->Srce == 0) */
	{
		GETARGS(*out);
		k_memcpy_s(*out,
			   sizeof(struct k_args),
			   in,
			   sizeof(struct k_args)); /* And copy the data from 'in' */
		(*out)->Ctxt.args = in;
	}
	/*else
	   {
	   *out = in;
	   } */
}

int CalcFreeReaderSpace(struct k_args *pReaderList)
{
	int iSize = 0;

	if (pReaderList) {
		struct k_args *pReader = pReaderList;
		while (pReader != NULL) {
			iSize += (pReader->Args.ChProc.iSizeTotal -
				  pReader->Args.ChProc.iSizeXferred);
			pReader = pReader->Forw;
		}
	}
	return iSize;
}

int CalcAvailWriterData(struct k_args *pWriterList)
{
	int iSize = 0;

	if (pWriterList) {
		struct k_args *pWriter = pWriterList;
		while (pWriter != NULL) {
			iSize += (pWriter->Args.ChProc.iSizeTotal -
				  pWriter->Args.ChProc.iSizeXferred);
			pWriter = pWriter->Forw;
		}
	}
	return iSize;
}

/********************************************************/

K_PIPE_OPTION ChxxxGetChOpt(K_ARGS_ARGS * pChxxx)
{
	return (K_PIPE_OPTION)(pChxxx->ChProc.ReqInfo.Params & _ALL_OPT);
}

void ChxxxSetChOpt(K_ARGS_ARGS *pChxxx, K_PIPE_OPTION option)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALL_OPT); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(option &
		 _ALL_OPT); /* to make sure we do not screw up other fields */
}

REQ_TYPE ChxxxGetReqType(K_ARGS_ARGS *pChxxx)
{
	return (REQ_TYPE)(pChxxx->ChProc.ReqInfo.Params & _ALLREQ);
}

void ChxxxSetReqType(K_ARGS_ARGS *pChxxx, REQ_TYPE ReqType)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALLREQ); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(ReqType &
		 _ALLREQ); /* to make sure we do not screw up other fields */
}

TIME_TYPE ChxxxGetTimeType(K_ARGS_ARGS *pChxxx)
{
	return (TIME_TYPE)(pChxxx->ChProc.ReqInfo.Params & _ALLTIME);
}

void ChxxxSetTimeType(K_ARGS_ARGS *pChxxx,
					    TIME_TYPE TimeType)
{
	pChxxx->ChProc.ReqInfo.Params &=
		(~_ALLTIME); /* clear destination field */
	pChxxx->ChProc.ReqInfo.Params |=
		(TimeType &
		 _ALLTIME); /* to make sure we do not screw up other fields */
}

/*********************************************************************/

CHREQ_STATUS ChReqGetStatus(struct k_chproc *pChProc)
{
	return pChProc->Status;
}

void ChReqSetStatus(struct k_chproc *pChProc,
					  CHREQ_STATUS Status)
{
#ifdef CONFIG_OBJECT_MONITOR
	/* if transition XFER_IDLE --> XFER_BUSY, TERM_XXX, increment channel
	 * counter: */
	if (XFER_IDLE == pChProc->Status /* current (old) status */
	    &&
	    (XFER_BUSY | TERM_XXX) & Status /* new status */) {
		(pChProc->ReqInfo.ChRef.pPipe->Count)++;
	}
#endif
	pChProc->Status = Status;
}

int ChReqSizeXferred(struct k_chproc *pChProc)
{
	return pChProc->iSizeXferred;
}

int ChReqSizeLeft(struct k_chproc *pChProc)
{
	return (pChProc->iSizeTotal - pChProc->iSizeXferred);
}

/********************************************************/

BOOL AreasCheck4Intrusion(unsigned char *pBegin1,
			  int iSize1,
			  unsigned char *pBegin2,
			  int iSize2);

void ChannelCheck4Intrusion(struct chbuff *pChBuff,
						  unsigned char *pBegin,
						  int iSize)
{
	/* check possible collision with all existing data areas,
	   both for read and write areas: */

	int index;
	struct marker_list *pMarkerList;

/* write markers:
 */

/* first a small consistency check:
 */
#ifdef STORE_NBR_MARKERS
	if (0 == pChBuff->WriteMarkers.iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pChBuff->WriteMarkers.iFirstMarker);
		__ASSERT_NO_MSG(-1 == pChBuff->WriteMarkers.iLastMarker);
		__ASSERT_NO_MSG(-1 == pChBuff->WriteMarkers.iAWAMarker);
	}
#endif

	pMarkerList = &(pChBuff->WriteMarkers);
	index = pMarkerList->iFirstMarker;

	while (-1 != index) {
		struct marker *pM;

		pM = &(pMarkerList->aMarkers[index]);

		if (0 != AreasCheck4Intrusion(
				 pBegin, iSize, pM->pointer, pM->size)) {
			__ASSERT_NO_MSG(1 == 0);
		}
		index = pM->Next;
	}

/* read markers:
 */
/* first a small consistency check:
 */
#ifdef STORE_NBR_MARKERS
	if (0 == pChBuff->ReadMarkers.iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pChBuff->ReadMarkers.iFirstMarker);
		__ASSERT_NO_MSG(-1 == pChBuff->ReadMarkers.iLastMarker);
		__ASSERT_NO_MSG(-1 == pChBuff->ReadMarkers.iAWAMarker);
	}
#endif

	pMarkerList = &(pChBuff->ReadMarkers);
	index = pMarkerList->iFirstMarker;

	while (-1 != index) {
		struct marker *pM;

		pM = &(pMarkerList->aMarkers[index]);

		if (0 != AreasCheck4Intrusion(
				 pBegin, iSize, pM->pointer, pM->size)) {
			__ASSERT_NO_MSG(1 == 0);
		}
		index = pM->Next;
	}
}

BOOL AreasCheck4Intrusion(unsigned char *pBegin1,
						int iSize1,
						unsigned char *pBegin2,
						int iSize2)
{
	unsigned char *pEnd1, *pEnd2;

	pEnd1 = pBegin1 + OCTET_TO_SIZEOFUNIT(iSize1);
	pEnd2 = pBegin2 + OCTET_TO_SIZEOFUNIT(iSize2);

	/* 2 tests are required to determine the status of the 2 areas, in terms
	   of
	   their position wrt each other */

	if (pBegin2 >=
	    pBegin1) {/* check intrusion of pBegin2 in [pBegin1, pEnd1( */
		if (pBegin2 < pEnd1)
			return TRUE; /* intrusion!! */
		else
			return FALSE; /* pBegin2 lies outside and to the right
					 of the first area, intrusion is
					 impossible */
	} else
		/* pBegin2 lies to the left of (pBegin1, pEnd1) */
	{
		if (
			    pEnd2 > pBegin1) /* check end pointer: is pEnd2 in
						(pBegin1, pEnd1( ?? */
			return TRUE; /* intrusion!! */
		else
			return FALSE; /* pEnd2 lies outside and to the left of
					 the first area, intrusion is impossible
					 */
	}
}

/******************************************************************************/
