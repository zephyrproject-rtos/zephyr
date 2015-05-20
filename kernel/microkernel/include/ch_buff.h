/* ch_buff.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
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

#ifndef CH_BUFF_H
#define CH_BUFF_H
/**************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel_struct.h>

/* low-level parameters: */

#define STORE_NBR_MARKERS
/* NOTE: the number of pending write and read Xfers is always stored,
   as it is required for the channels to function properly. It is stored in the
   field ChBuff.iNbrPendingWrites and iNbrPendingReads.

   In the Writer and Reader MarkersList, the number of markers (==nbr. of
   unreleased Xfers)
   is monitored as well. They actually equal iNbrPendingWrites and
   iNbrPendingReads.
   Their existence depends on STORE_NBR_MARKERS. A reason to have them
   additionally is that
   some extra consistency checking is performed in the markers manipulation
   functionality
   itself.
   Drawback: double storage of nbr. of pending write Xfers (but for test
   purposes this is
   acceptable I think)
 */

/* exported functions:
*/
void BuffInit(unsigned char *pBuffer, int *piBuffSize, struct chbuff *pChBuff);
void BuffReset(struct chbuff *pBuff);

void BuffGetFreeSpaceTotal(struct chbuff *pBuff, int *piTotalFreeSpace);
void BuffGetFreeSpace(struct chbuff *pBuff,
		      int *piTotalFreeSpace,
		      int *piFreeSpaceCont,
		      int *piFreeSpaceAWA);
void BuffGetAvailDataTotal(struct chbuff *pBuff, int *piAvailDataTotal);
void BuffGetAvailData(struct chbuff *pBuff,
		      int *piAvailDataTotal,
		      int *piAvailDataCont,
		      int *piAvailDataAWA);

int BuffEnQ(struct chbuff *pBuff, int iSize, unsigned char **ppWrite);
int BuffEnQA(struct chbuff *pBuff,
	     int iSize,
	     unsigned char **ppWrite,
	     int *piTransferID);
void BuffEnQA_End(struct chbuff *pBuff, int iTransferID, int iSize /*optional */);
int BuffDeQ(struct chbuff *pBuff, int iSize, unsigned char **ppRead);
int BuffDeQA(struct chbuff *pBuff,
	     int iSize,
	     unsigned char **ppRead,
	     int *piTransferID);
void BuffDeQA_End(struct chbuff *pBuff, int iTransferID, int iSize /*optional */);

/* internal functions:
*/
void WriteMarkersClear(struct chbuff *pBuff);
void ReadMarkersClear(struct chbuff *pBuff);
int AsyncEnQRegstr(struct chbuff *pBuff, int iSize);
int AsyncDeQRegstr(struct chbuff *pBuff, int iSize);
void AsyncEnQFinished(struct chbuff *pBuff, int iTransferID);
void AsyncDeQFinished(struct chbuff *pBuff, int iTransferID);

int CalcAvailData(struct chbuff *pBuff, int *piDataAvailCont, int *piDataAvailAWA);
int CalcFreeSpace(struct chbuff *pBuff, int *piFreeSpaceCont, int *piFreeSpaceAWA);

void WriteMarkersClear(struct chbuff *pBuff);
void ReadMarkersClear(struct chbuff *pBuff);
int ScanMarkers(struct marker_list *pMarkerList,
		int *piSizeBWA,
		int *pisizeAWA,
		int *piNbrPendingXfers);

int BuffFull(struct chbuff *pBuff);
int BuffEmpty(struct chbuff *pBuff);

void ChannelCheck4Intrusion(struct chbuff *pChBuff, unsigned char *pBegin, int iSize);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*****************************************/
#endif
