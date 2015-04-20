/* kchan.h */

/*
 * Copyright (c) 1997-2012, 2014 Wind River Systems, Inc.
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

#ifndef KCHAN_H
#define KCHAN_H

/***********************************/
/*	TARGET channels functionality: */
/***********************************/

#include "ch_cfg.h" /* defines the high-level behavior of the channel service \
		       */
typedef uint32_t REQ_TYPE;
#define _ALLREQ ((REQ_TYPE)0x0000FF00)
#define _SYNCREQ ((REQ_TYPE)0x00000100)
#define _SYNCREQL ((REQ_TYPE)0x00000200)
#define _ASYNCREQ ((REQ_TYPE)0x00000400)

/* K functions: */

extern void K_ChSendReq(struct k_args *ReqOrig);
extern void K_ChSendTmo(struct k_args *ReqProc);
extern void K_ChSendRpl(struct k_args *ReqProc);
extern void K_ChSendAck(struct k_args *ReqOrig);
extern void K_ChRecvReq(struct k_args *ReqOrig);
extern void K_ChRecvTmo(struct k_args *ReqProc);
extern void K_ChRecvRpl(struct k_args *ReqProc);
extern void K_ChRecvAck(struct k_args *ReqOrig);
extern void K_ChMovedAck(struct k_args *pEOXfer);

/* these are not real K_ functions: */
extern void K_ChProcWO(struct pipe_struct *pPipe, struct k_args *pWriter);
extern void K_ChProcRO(struct pipe_struct *pPipe, struct k_args *pReader);
extern void K_ChProcWR(struct pipe_struct *pPipe, struct k_args *pWriter, struct k_args *pReader);
extern void K_ChProc(struct pipe_struct *pPipe, struct k_args *pWriter, struct k_args *pReader);

/* internal functions: */

extern void mycopypacket(struct k_args **out, struct k_args *in);

int CalcFreeReaderSpace(struct k_args *pReaderList);
int CalcAvailWriterData(struct k_args *pWriterList);

void DeListWaiter(struct k_args *pReqProc);
void myfreetimer(K_TIMER **ppTimer);

void setup_movedata(struct k_args *A,
		    struct pipe_struct *pPipe,
		    XFER_TYPE XferType,
		    struct k_args *pWriter,
		    struct k_args *pReader,
		    void *destination,
		    void *source,
		    uint32_t size,
		    int XferID);
int CalcChanProcPrio(int iChanDefaultPrio, int iWriterPrio, int iReaderPrio);
void SetChanProcPrio(struct k_args *pMvdReq,
		     struct k_args *pContSend,
		     struct k_args *pContRecv,
		     struct k_args *pWriter,
		     struct k_args *pReader);

K_PIPE_OPTION ChxxxGetChOpt(K_ARGS_ARGS *pChxxx);
void ChxxxSetChOpt(K_ARGS_ARGS *pChxxx, K_PIPE_OPTION option);
REQ_TYPE ChxxxGetReqType(K_ARGS_ARGS *pChxxx);
void ChxxxSetReqType(K_ARGS_ARGS *pChxxx, REQ_TYPE ReqType);

CHREQ_STATUS ChReqGetStatus(struct k_chproc *pChProc);
void ChReqSetStatus(struct k_chproc *pChProc, CHREQ_STATUS Status);
int /*BOOL */ ChReqSizeXferred(struct k_chproc *pChProc);
int /*BOOL */ ChReqSizeLeft(struct k_chproc *pChProc);

TIME_TYPE ChxxxGetTimeType(K_ARGS_ARGS *pChxxx);
void ChxxxSetTimeType(K_ARGS_ARGS *pChxxx, TIME_TYPE TimeType);
/*********************************************************************/
#endif
