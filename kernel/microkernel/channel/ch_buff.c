/* ch_buff.c */

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


/* Implementation remarks:
- when using a floating end pointer: do not use pChBuff->iBuffsize for
(Buff->pEnd - pChBuff->pBegin)
*/

/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/k_types.h>
#include <ch_buff.h>
#include <ch_buffm.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/**********************************************************/

#define CHECK_CHBUFF_POINTER(pData) \
	__ASSERT_NO_MSG(pChBuff->pBegin <= pData && pData < pChBuff->pEnd)

/*******************/
/* General
********************/

void BuffInit(unsigned char *pBuffer, int *piBuffSize, struct chbuff *pChBuff)
{
	pChBuff->pBegin = pBuffer;

	pChBuff->iBuffSize = *piBuffSize;

	/* reset all pointers:
	 */
	{
		pChBuff->pEnd = pChBuff->pBegin +
				OCTET_TO_SIZEOFUNIT(pChBuff->iBuffSize);
		pChBuff->pEndOrig = pChBuff->pEnd;
		BuffReset(pChBuff);
	}
}

void BuffReset(struct chbuff *pChBuff)
{
	/* assumed it is allowed */
	pChBuff->BuffState = BUFF_EMPTY;
	pChBuff->pEnd = pChBuff->pEndOrig;
	pChBuff->pWrite = pChBuff->pBegin;
	pChBuff->pWriteGuard = NULL;
	pChBuff->bWriteWA = FALSE;
	pChBuff->pRead = pChBuff->pBegin;
	pChBuff->pReadGuard = NULL;
	pChBuff->bReadWA = TRUE; /* YES!! */
	/*pChBuff->iFreeSpaceTotal =pChBuff->iBuffSize; */
	pChBuff->iFreeSpaceCont = pChBuff->iBuffSize;
	pChBuff->iFreeSpaceAWA = 0;
	pChBuff->iNbrPendingReads = 0;
	/*pChBuff->iAvailDataTotal =0; */
	pChBuff->iAvailDataCont = 0;
	pChBuff->iAvailDataAWA = 0;
	pChBuff->iNbrPendingWrites = 0;
	WriteMarkersClear(pChBuff);
	ReadMarkersClear(pChBuff);
}

void BuffGetFreeSpaceTotal(struct chbuff *pChBuff, int *piFreeSpaceTotal)
{
	int dummy1, dummy2;
	*piFreeSpaceTotal = CalcFreeSpace(pChBuff, &dummy1, &dummy2);
	__ASSERT_NO_MSG(dummy1 == pChBuff->iFreeSpaceCont);
	__ASSERT_NO_MSG(dummy2 == pChBuff->iFreeSpaceAWA);
}

void BuffGetFreeSpace(struct chbuff *pChBuff, int *piFreeSpaceTotal,
					  int *piFreeSpaceCont, int *piFreeSpaceAWA)
{
	int iFreeSpaceCont;
	int iFreeSpaceAWA;
	int iFreeSpaceTotal;

	iFreeSpaceTotal =
		CalcFreeSpace(pChBuff, &iFreeSpaceCont, &iFreeSpaceAWA);
	__ASSERT_NO_MSG(iFreeSpaceCont == pChBuff->iFreeSpaceCont);
	__ASSERT_NO_MSG(iFreeSpaceAWA == pChBuff->iFreeSpaceAWA);
	*piFreeSpaceTotal = iFreeSpaceTotal;
	*piFreeSpaceCont = pChBuff->iFreeSpaceCont;
	*piFreeSpaceAWA = pChBuff->iFreeSpaceAWA;
}

void BuffGetAvailDataTotal(struct chbuff *pChBuff, int *piAvailDataTotal)
{
	int dummy1, dummy2;
	*piAvailDataTotal = CalcAvailData(pChBuff, &dummy1, &dummy2);
	__ASSERT_NO_MSG(dummy1 == pChBuff->iAvailDataCont);
	__ASSERT_NO_MSG(dummy2 == pChBuff->iAvailDataAWA);
}

void BuffGetAvailData(struct chbuff *pChBuff, int *piAvailDataTotal,
					  int *piAvailDataCont, int *piAvailDataAWA)
{
	int iAvailDataCont;
	int iAvailDataAWA;
	int iAvailDataTotal;

	iAvailDataTotal =
		CalcAvailData(pChBuff, &iAvailDataCont, &iAvailDataAWA);
	__ASSERT_NO_MSG(iAvailDataCont == pChBuff->iAvailDataCont);
	__ASSERT_NO_MSG(iAvailDataAWA == pChBuff->iAvailDataAWA);
	*piAvailDataTotal = iAvailDataTotal;
	*piAvailDataCont = pChBuff->iAvailDataCont;
	*piAvailDataAWA = pChBuff->iAvailDataAWA;
}

/*******************/
/* Buffer en-queuing:
********************/

int BuffEnQ(struct chbuff *pChBuff, int iSize, unsigned char **ppWrite)
{
	int iTransferID;
	/*  int ret;*/
	if (0 == BuffEnQA(pChBuff, iSize, ppWrite, &iTransferID)) {
		return 0;
	}

	/* check ret value */
	BuffEnQA_End(pChBuff, iTransferID, iSize /*optional */);
	return iSize;
}

int BuffEnQA(struct chbuff *pChBuff, int iSize, unsigned char **ppWrite,
			 int *piTransferID)
{
	if (iSize > pChBuff->iFreeSpaceCont) {
		return 0;
	}
	*piTransferID = AsyncEnQRegstr(pChBuff, iSize);
	if (-1 == *piTransferID) {
		return 0;
	}

	*ppWrite = pChBuff->pWrite;

	/* adjust write pointer and free space:
	 */
	pChBuff->pWrite += OCTET_TO_SIZEOFUNIT(iSize);
	if (pChBuff->pEnd == pChBuff->pWrite) {
		pChBuff->pWrite = pChBuff->pBegin;
		pChBuff->iFreeSpaceCont = pChBuff->iFreeSpaceAWA;
		pChBuff->iFreeSpaceAWA = 0;
		pChBuff->bWriteWA = TRUE;
		pChBuff->bReadWA = FALSE;
		pChBuff->ReadMarkers.iAWAMarker = -1;
	} else {
		pChBuff->iFreeSpaceCont -= iSize;
	}

	if (pChBuff->pWrite == pChBuff->pRead) {
		pChBuff->BuffState = BUFF_FULL;
	} else {
		pChBuff->BuffState = BUFF_OTHER;
	}

	CHECK_CHBUFF_POINTER(pChBuff->pWrite);

	return iSize;
}

void BuffEnQA_End(struct chbuff *pChBuff, int iTransferID,
				  int iSize /*optional */)
{
	ARG_UNUSED(iSize);

	/* An asynchronous data transfer to the buffer has finished
	 */
	AsyncEnQFinished(pChBuff, iTransferID);
}

/**************************************************/

int AsyncEnQRegstr(struct chbuff *pChBuff, int iSize)
{
	int i;

	ChannelCheck4Intrusion(pChBuff, pChBuff->pWrite, iSize);

	i = MarkerAddLast(&(pChBuff->WriteMarkers), pChBuff->pWrite, iSize, TRUE);
	if (i != -1) {
		/* adjust iNbrPendingWrites: */
		__ASSERT_NO_MSG(0 <= pChBuff->iNbrPendingWrites);
		(pChBuff->iNbrPendingWrites)++;
		/* pReadGuard changes? */
		if (NULL == pChBuff->pReadGuard) {
			pChBuff->pReadGuard = pChBuff->pWrite;
		}
		__ASSERT_NO_MSG(pChBuff->WriteMarkers.aMarkers
						[pChBuff->WriteMarkers.iFirstMarker]
						.pointer == pChBuff->pReadGuard);
		/* iAWAMarker changes? */
		if (-1 == pChBuff->WriteMarkers.iAWAMarker && pChBuff->bWriteWA) {
			pChBuff->WriteMarkers.iAWAMarker = i;
		}
	}
	return i;
}

void AsyncEnQFinished(struct chbuff *pChBuff, int iTransferID)
{
	pChBuff->WriteMarkers.aMarkers[iTransferID].bXferBusy = FALSE;

	if (pChBuff->WriteMarkers.iFirstMarker == iTransferID) {
		/*int iSizeCont=0;
		   int isizeAWA=0; */
		int iNewFirstMarker =
			ScanMarkers(&(pChBuff->WriteMarkers),
						&(pChBuff->iAvailDataCont),
						&(pChBuff->iAvailDataAWA),
						&(pChBuff->iNbrPendingWrites));
		/*pChBuff->iAvailDataCont +=iSizeCont;
		   pChBuff->iAvailDataAWA +=iSizeAWA; */
		if (-1 != iNewFirstMarker) {
			pChBuff->pReadGuard =
				pChBuff->WriteMarkers.aMarkers[iNewFirstMarker]
				.pointer;
		} else {
			pChBuff->pReadGuard = NULL;
		}
	}
}

/**********************/
/* Buffer de-queuing: */
/**********************/

int BuffDeQ(struct chbuff *pChBuff, int iSize, unsigned char **ppRead)
{
	int iTransferID;
	/*  int ret;*/
	if (0 == BuffDeQA(pChBuff, iSize, ppRead, &iTransferID)) {
		return 0;
	}
	BuffDeQA_End(pChBuff, iTransferID, iSize /*optional */);
	return iSize;
}

int BuffDeQA(struct chbuff *pChBuff, int iSize, unsigned char **ppRead,
			 int *piTransferID)
{
	/* asynchronous data transfer
	 * read guard pointers must be set
	 */
	if (iSize > pChBuff->iAvailDataCont) {
		/* free space is from read to guard pointer/end pointer */
		return 0;
	}
	*piTransferID = AsyncDeQRegstr(pChBuff, iSize);
	if (-1 == *piTransferID) {
		return 0;
	}

	*ppRead = pChBuff->pRead;

	/* adjust read pointer and avail data:
	 */
	pChBuff->pRead += OCTET_TO_SIZEOFUNIT(iSize);
	if (pChBuff->pEnd == pChBuff->pRead) {
		pChBuff->pRead = pChBuff->pBegin;
		pChBuff->iAvailDataCont = pChBuff->iAvailDataAWA;
		pChBuff->iAvailDataAWA = 0;
		pChBuff->bWriteWA = FALSE;
		pChBuff->bReadWA = TRUE;
		pChBuff->WriteMarkers.iAWAMarker = -1;
	} else {
		pChBuff->iAvailDataCont -= iSize;
	}

	if (pChBuff->pWrite == pChBuff->pRead) {
		pChBuff->BuffState = BUFF_EMPTY;
	} else {
		pChBuff->BuffState = BUFF_OTHER;
	}

	CHECK_CHBUFF_POINTER(pChBuff->pRead);

	return iSize;
}

void BuffDeQA_End(struct chbuff *pChBuff, int iTransferID,
				  int iSize /*optional */)
{
	ARG_UNUSED(iSize);

	/* An asynchronous data transfer from the buffer has finished
	 */
	AsyncDeQFinished(pChBuff, iTransferID);

#if 0
	/* optimisation: reset queue when empty:
	 */
	if  (BUFF_EMPTY == pChBuff->BuffState
	     && NULL == pChBuff->pWriteGuard
	/* no pending read Xfers anymore */
	/* && NULL==pChBuff->pReadGuard no pending write Xfers
	   (here write Xfers could be started after the last DeQA,
	   but then the buffer is not empty anymore) */
	    ) {
	    BuffReset(pChBuff);	/* then we can safely reset the buffer */
	    }
#endif
}

/**********************************************************/

int AsyncDeQRegstr(struct chbuff *pChBuff, int iSize)
{
	int i;

	ChannelCheck4Intrusion(pChBuff, pChBuff->pRead, iSize);

	i = MarkerAddLast(&(pChBuff->ReadMarkers), pChBuff->pRead, iSize, TRUE);
	if (i != -1) {
		/* adjust iNbrPendingReads: */
		__ASSERT_NO_MSG(0 <= pChBuff->iNbrPendingReads);
		(pChBuff->iNbrPendingReads)++;
		/* pWriteGuard changes? */
		if (NULL == pChBuff->pWriteGuard) {
			pChBuff->pWriteGuard = pChBuff->pRead;
		}
		__ASSERT_NO_MSG(pChBuff->ReadMarkers.aMarkers
						[pChBuff->ReadMarkers.iFirstMarker]
						.pointer == pChBuff->pWriteGuard);
		/* iAWAMarker changes? */
		if (-1 == pChBuff->ReadMarkers.iAWAMarker && pChBuff->bReadWA) {
			pChBuff->ReadMarkers.iAWAMarker = i;
		}
	}
	return i;
}

void AsyncDeQFinished(struct chbuff *pChBuff, int iTransferID)
{
	pChBuff->ReadMarkers.aMarkers[iTransferID].bXferBusy = FALSE;

	if (pChBuff->ReadMarkers.iFirstMarker == iTransferID) {
		/*int iSizeCont=0;
		   int iSizeAWA=0; */
		int iNewFirstMarker = ScanMarkers(&(pChBuff->ReadMarkers),
						  &(pChBuff->iFreeSpaceCont),
						  &(pChBuff->iFreeSpaceAWA),
						  &(pChBuff->iNbrPendingReads));
		/*pChBuff->iFreeSpaceCont +=iSizeCont;
		   pChBuff->iFreeSpaceAWA +=iSizeAWA; */
		if (-1 != iNewFirstMarker) {
			pChBuff->pWriteGuard =
				pChBuff->ReadMarkers.aMarkers[iNewFirstMarker]
				.pointer;
		} else {
			pChBuff->pWriteGuard = NULL;
		}
	}
}

/**********************************************************/

void WriteMarkersClear(struct chbuff *pChBuff)
{
	MarkersClear(&(pChBuff->WriteMarkers));
}

void ReadMarkersClear(struct chbuff *pChBuff)
{
	MarkersClear(&(pChBuff->ReadMarkers));
}

/********************************************************************************/

/* note on setting/clearing markers/guards:

  If there is at least one marker, there is a guard and equals one of the
  markers
  If there are no markers (*), there is no guard
  Consequently, if a marker is add when there were none, the guard will equal
  it.
  If additional markers are add, the guard will not change.
  However, if a marker is deleted:
    if it equals the guard a new guard must be selected (**)
    if not, guard doesn't change

  (*) we need to housekeep how much markers there are or we can inspect the
  guard
  (**) for this, the complete markers table needs to be investigated
*/

/***************************************/

/* This function will see if one or more 'areas' in the buffer can be made
available
(either for writing xor reading).
Note: such a series of areas starts from the beginning.
*/
int ScanMarkers(struct marker_list *pMarkerList, int *piSizeBWA,
				int *piSizeAWA, int *piNbrPendingXfers)
{
	struct marker *pM;
	BOOL bMarkersAreNowAWA;
	int index;

	index = pMarkerList->iFirstMarker;
	/*if ( -1 == index)
	   return;
	 */
	__ASSERT_NO_MSG(-1 != index);

	bMarkersAreNowAWA = FALSE;
	do {
		int index_next;

		__ASSERT_NO_MSG(index == pMarkerList->iFirstMarker);

		if (index == pMarkerList->iAWAMarker) {
			bMarkersAreNowAWA =
				TRUE; /* from now on, everything is AWA */
		}

		pM = &(pMarkerList->aMarkers[index]);

		if (pM->bXferBusy == TRUE) {
			break;
		}

		if (!bMarkersAreNowAWA) {
			*piSizeBWA += pM->size;
		} else {
			*piSizeAWA += pM->size;
		}

		index_next = pM->Next;
		/* pMarkerList->iFirstMarker will be updated */
		MarkerDelete(pMarkerList, index);
		/* adjust *piNbrPendingXfers */
		if (piNbrPendingXfers) {
			__ASSERT_NO_MSG(0 <= *piNbrPendingXfers);
			(*piNbrPendingXfers)--;
		}
		index = index_next;
	} while (-1 != index);

	__ASSERT_NO_MSG(index == pMarkerList->iFirstMarker);

	if (bMarkersAreNowAWA) {
		pMarkerList->iAWAMarker = pMarkerList->iFirstMarker;
	}

#ifdef STORE_NBR_MARKERS
	if (0 == pMarkerList->iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iFirstMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iAWAMarker);
	}
#endif

	return pMarkerList->iFirstMarker;
}

/**********************************************************/

int CalcAvailData(struct chbuff *pChBuff, int *piAvailDataCont,
				  int *piAvailDataAWA)
{
	unsigned char *pStart = pChBuff->pRead;
	unsigned char *pStop = pChBuff->pWrite;

	if (NULL != pChBuff->pReadGuard) {
		pStop = pChBuff->pReadGuard;
	} else {
		/* else:
		   if BuffState==BUFF_FULL but we have a ReadGuard, we still
		   need to calculate it as a normal [Start,Stop] interval
		 */
		if (BUFF_FULL == pChBuff->BuffState) {
			*piAvailDataCont =
				SIZEOFUNIT_TO_OCTET(pChBuff->pEnd - pStart);
			*piAvailDataAWA =
				SIZEOFUNIT_TO_OCTET(pStop - pChBuff->pBegin);
			return (*piAvailDataCont + *piAvailDataAWA);
			/* this sum equals pEnd-pBegin */
		}
	}
	/* on the other hand, if BuffState is empty, we do not need a special
	   flow;
	   it will be correct as (pStop - pStart) equals 0 */

	if (pStop >= pStart) {
		*piAvailDataCont = SIZEOFUNIT_TO_OCTET(pStop - pStart);
		*piAvailDataAWA = 0;
	} else {
		*piAvailDataCont = SIZEOFUNIT_TO_OCTET(pChBuff->pEnd - pStart);
		*piAvailDataAWA = SIZEOFUNIT_TO_OCTET(pStop - pChBuff->pBegin);
	}
	return (*piAvailDataCont + *piAvailDataAWA);
}

int CalcFreeSpace(struct chbuff *pChBuff, int *piFreeSpaceCont,
				  int *piFreeSpaceAWA)
{
	unsigned char *pStart = pChBuff->pWrite;
	unsigned char *pStop = pChBuff->pRead;

	if (NULL != pChBuff->pWriteGuard) {
		pStop = pChBuff->pWriteGuard;
	} else {
		/* else:
		   if BuffState==BUFF_EMPTY but we have a WriteGuard, we still
		   need to calculate it as a normal [Start,Stop] interval
		 */
		if (BUFF_EMPTY == pChBuff->BuffState) {
			*piFreeSpaceCont =
				SIZEOFUNIT_TO_OCTET(pChBuff->pEnd - pStart);
			*piFreeSpaceAWA =
				SIZEOFUNIT_TO_OCTET(pStop - pChBuff->pBegin);
			return (*piFreeSpaceCont + *piFreeSpaceAWA);
			/* this sum equals pEnd-pBegin */
		}
	}
	/* on the other hand, if BuffState is full, we do not need a special
	   flow;
	   it will be correct as (pStop - pStart) equals 0 */

	if (pStop >= pStart) {
		*piFreeSpaceCont = SIZEOFUNIT_TO_OCTET(pStop - pStart);
		*piFreeSpaceAWA = 0;
	} else {
		*piFreeSpaceCont = SIZEOFUNIT_TO_OCTET(pChBuff->pEnd - pStart);
		*piFreeSpaceAWA = SIZEOFUNIT_TO_OCTET(pStop - pChBuff->pBegin);
	}
	return (*piFreeSpaceCont + *piFreeSpaceAWA);
}

int BuffFull(struct chbuff *pChBuff)
{/* remark: 0==iTotalFreeSpace is an INcorrect condition b/c of async. behavior
    */
	int iAvailDataTotal;

	BuffGetAvailDataTotal(pChBuff, &iAvailDataTotal);
	return (pChBuff->iBuffSize == iAvailDataTotal);
}

int BuffEmpty(struct chbuff *pChBuff)
{/* remark: 0==iAvailDataTotal is an INcorrect condition b/c of async. behavior
    */
	int iTotalFreeSpace;

	BuffGetFreeSpaceTotal(pChBuff, &iTotalFreeSpace);
	return (pChBuff->iBuffSize == iTotalFreeSpace);
}

/*****************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
