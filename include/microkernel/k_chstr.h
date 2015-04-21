/* microkernel/k_chstr.h */

/*
 * Copyright (c) 1997-2012, 2015 Wind River Systems, Inc.
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

#ifndef _K_CHANSTRUCT_H
#define _K_CHANSTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAXNBR_MARKERS 10 /* 1==disable parallel transfers */


struct marker {
	unsigned char *pointer; /* NULL == non valid marker == free */
	int size;
	BOOL bXferBusy;
	int Prev; /* -1 == no predecessor */
	int Next; /* -1 == no successor */
};

struct marker_list {
	int iNbrMarkers;   /* Only used if STORE_NBR_MARKERS is defined */
	int iFirstMarker;
	int iLastMarker;
	int iAWAMarker; /* -1 means no AWAMarkers */
	struct marker aMarkers[MAXNBR_MARKERS];
};

typedef enum {
	BUFF_EMPTY, /* buffer is empty, disregarding the pending data Xfers
		       (reads) still finishing up */
	BUFF_FULL, /* buffer is full, disregarding the pending data Xfers
		      (writes) still finishing up */
	BUFF_OTHER
} BUFF_STATE;

struct chbuff {
	int iBuffSize;
	unsigned char *pBegin;
	unsigned char *pWrite;
	unsigned char *pRead;
	unsigned char *pWriteGuard; /* can be NULL --> invalid */
	unsigned char *pReadGuard;  /* can be NULL --> invalid */
	int iFreeSpaceCont;
	int iFreeSpaceAWA;
	int iNbrPendingReads;
	int iAvailDataCont;
	int iAvailDataAWA; /* AWA == After Wrap Around */
	int iNbrPendingWrites;
	BOOL bWriteWA;
	BOOL bReadWA;
	BUFF_STATE BuffState;
	struct marker_list WriteMarkers;
	struct marker_list ReadMarkers;
	unsigned char *pEnd;
	unsigned char *pEndOrig;
};

#ifdef __cplusplus
}
#endif

#endif /* _K_CHANSTRUCT_H */
