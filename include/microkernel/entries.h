/* microkernel/entries.h */

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

#ifndef _ENTRIES_H
#define _ENTRIES_H

#include <microkernel/k_struct.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*kernelfunc)(struct k_args *);

/* Jumptable entrypoints */

extern void K_nop(struct k_args *);
extern void K_offload(struct k_args *);
extern void K_workload(struct k_args *);
extern void K_signals(struct k_args *);
extern void K_signalm(struct k_args *);
extern void K_resets(struct k_args *);
extern void K_resetm(struct k_args *);
extern void K_waitsreq(struct k_args *);
extern void K_waitsrpl(struct k_args *);
extern void K_waitmany(struct k_args *);
extern void K_waitmreq(struct k_args *);
extern void K_waitmrdy(struct k_args *);
extern void K_waitmcan(struct k_args *);
extern void K_waitmacc(struct k_args *);
extern void K_waitmend(struct k_args *);
extern void K_waitmtmo(struct k_args *);
extern void K_inqsema(struct k_args *);
extern void K_lockreq(struct k_args *);
extern void K_lockrpl(struct k_args *);
extern void K_unlock(struct k_args *);
extern void K_enqreq(struct k_args *);
extern void K_enqrpl(struct k_args *);
extern void K_deqreq(struct k_args *);
extern void K_deqrpl(struct k_args *);
extern void K_queue(struct k_args *);
extern void K_sendreq(struct k_args *);
extern void K_sendrpl(struct k_args *);
extern void _k_mbox_send_ack(struct k_args *);
extern void K_senddata(struct k_args *);
extern void K_recvreq(struct k_args *);
extern void K_recvrpl(struct k_args *);
extern void K_recvack(struct k_args *);
extern void K_recvdata(struct k_args *);
extern void K_elapse(struct k_args *);
extern void K_sleep(struct k_args *);
extern void K_wakeup(struct k_args *);
extern void K_taskop(struct k_args *);
extern void K_groupop(struct k_args *);
extern void K_set_prio(struct k_args *);
extern void K_yield(struct k_args *);
extern void K_alloc(struct k_args *);
extern void K_dealloc(struct k_args *);
extern void K_alloc_timer(struct k_args *);
extern void K_dealloc_timer(struct k_args *);
extern void K_start_timer(struct k_args *);
extern void K_stop_timer(struct k_args *);
extern void K_alloctmo(struct k_args *);
extern void K_debug_req(struct k_args *);
extern void K_debug_ack(struct k_args *);
extern void K_event_test(struct k_args *);
extern void K_event_set_handler(struct k_args *);
extern void K_event_signal(struct k_args *);
extern void K_GetBlock(struct k_args *);
extern void K_RelBlock(struct k_args *);
extern void K_GetBlock_Waiters(struct k_args *);
extern void K_gtbltmo(struct k_args *);
extern void K_Defrag(struct k_args *);

extern void K_mvdreq(struct k_args *Req);
extern void K_mvdsndreq(struct k_args *SndReq);
extern void K_mvdrcvreq(struct k_args *RcvReq);
extern void K_rawdata(struct k_args *DataPacket);
extern void K_mvdsndack(struct k_args *SndDAck);
extern void K_mvdrcvack(struct k_args *RcvDAck);

extern void K_ChSendReq(struct k_args *Writer);
extern void K_ChSendTmo(struct k_args *Writer);
extern void K_ChSendRpl(struct k_args *Writer);
extern void K_ChSendAck(struct k_args *Writer);
extern void K_ChRecvReq(struct k_args *Reader);
extern void K_ChRecvTmo(struct k_args *Reader);
extern void K_ChRecvRpl(struct k_args *Reader);
extern void K_ChRecvAck(struct k_args *Reader);
extern void K_ChMovedAck(struct k_args *pEOXfer);
extern void _k_event_test_timeout(struct k_args *A);

#ifdef __cplusplus
}
#endif

#endif /* _ENTRIES_H */
