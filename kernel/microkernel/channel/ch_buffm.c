/* ch_buffm.c */

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


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************/

#include <stddef.h>
#include <microkernel/k_types.h>
#include <microkernel/k_chstr.h> /* definition of all channel related data \
				    structures */
#include <ch_buffm.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

/*************************************************************************/

int MarkerAddLast(struct marker_list *pMarkerList,
					unsigned char *pointer,
					int iSize,
					BOOL bXferBusy)
{
	int i = MarkerFindFree(pMarkerList->aMarkers);

	if (i == -1)
		return i;
	pMarkerList->aMarkers[i].pointer = pointer;
	pMarkerList->aMarkers[i].size = iSize;
	pMarkerList->aMarkers[i].bXferBusy = bXferBusy;

	if (-1 == pMarkerList->iFirstMarker) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
		pMarkerList->iFirstMarker =
			i; /* we still need to set Prev & Next */
	} else {
		__ASSERT_NO_MSG(-1 != pMarkerList->iLastMarker);
		__ASSERT_NO_MSG(-1 ==
		       pMarkerList->aMarkers[pMarkerList->iLastMarker].Next);
	}

	MarkerLinkToListAfter(
		pMarkerList->aMarkers, pMarkerList->iLastMarker, i);

	__ASSERT_NO_MSG(-1 == pMarkerList->aMarkers[i].Next);
	pMarkerList->iLastMarker = i;

#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers++;
	__ASSERT_NO_MSG(0 < pMarkerList->iNbrMarkers);
#endif

	return i;
}

void MarkerDelete(struct marker_list *pMarkerList,
					/*unsigned char* pointer */ int index)
{
	int i;
	int iPredecessor, iSuccessor;
	/*if ( -1 ==(i =MarkerFind( pMarkerList->aMarkers, pointer)) )
	   return i; */
	i = index;

	__ASSERT_NO_MSG(-1 != i);

	pMarkerList->aMarkers[i].pointer = NULL;
	MarkerUnlinkFromList(
		pMarkerList->aMarkers, i, &iPredecessor, &iSuccessor);
	/* update first/last info:
	 */
	if (i == pMarkerList->iLastMarker /*&& -1 != i */)
		pMarkerList->iLastMarker = iPredecessor;
	if (i == pMarkerList->iFirstMarker /*&& -1 != i */)
		pMarkerList->iFirstMarker = iSuccessor;

#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers--;
	__ASSERT_NO_MSG(0 <= pMarkerList->iNbrMarkers);

	if (0 == pMarkerList->iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iFirstMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
	}
#endif
}

/*************************************************************************/

void MarkersClear(struct marker_list *pMarkerList)
{
	struct marker *pM = pMarkerList->aMarkers;
	int i;

	for (i = 0; i < MAXNBR_MARKERS; i++, pM++) {
		k_memset(pM, 0, sizeof(struct marker));
		/*pM->pointer =NULL; */
		pM->Next = -1;
		pM->Prev = -1;
	}
#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers = 0;
#endif
	pMarkerList->iFirstMarker = -1;
	pMarkerList->iLastMarker = -1;
	pMarkerList->iAWAMarker = -1;
}

/*int MarkerFind( struct marker aMarkers[], unsigned char* pointer)
{
  struct marker *pM =aMarkers;
  int i;
  for ( i=0; i<MAXNBR_MARKERS; i++, pM++)  {
    if ( pointer==pM->pointer ) {
      break;
    }
  }
  return i;
}

int MarkerGetNext( int index)
{
  return -1;
}*/

int MarkerFindFree(struct marker aMarkers[])
{
	struct marker *pM = aMarkers;
	int i;

	for (i = 0; i < MAXNBR_MARKERS; i++, pM++) {
		if (NULL == pM->pointer) {
			break;
		}
	}
	if (MAXNBR_MARKERS == i)
		i = -1;

	return i;
}

void MarkerLinkToListAfter(struct marker aMarkers[],
						 int iMarker,
						 int iNewMarker)
{
	int iNextMarker; /* index of next marker in original list */
	/* let the original list be aware of the new marker: */
	if (-1 != iMarker) {
		iNextMarker = aMarkers[iMarker].Next;
		aMarkers[iMarker].Next = iNewMarker;
		if (-1 != iNextMarker)
			aMarkers[iNextMarker].Prev = iNewMarker;
		else {
			/* there was no next marker */
		}
	} else {
		iNextMarker = -1; /* there wasn't even a marker */
	}
	/* link the new marker with the marker and next marker: */
	aMarkers[iNewMarker].Prev = iMarker;
	aMarkers[iNewMarker].Next = iNextMarker;
}

void MarkerUnlinkFromList(struct marker aMarkers[],
						int iMarker,
						int *piPredecessor,
						int *piSuccessor)
{
	int iNextMarker = aMarkers[iMarker].Next;
	int iPrevMarker = aMarkers[iMarker].Prev;
	/* remove the marker from the list: */
	aMarkers[iMarker].Next = -1;
	aMarkers[iMarker].Prev = -1;
	/* repair the chain: */
	if (-1 != iPrevMarker)
		aMarkers[iPrevMarker].Next = iNextMarker;
	if (-1 != iNextMarker)
		aMarkers[iNextMarker].Prev = iPrevMarker;
	*piPredecessor = iPrevMarker;
	*piSuccessor = iNextMarker;
}

/******************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
