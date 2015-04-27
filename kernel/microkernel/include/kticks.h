/* kticks.h */

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

#ifndef _KTICKS_H
#define _KTICKS_H

#include "microkernel/k_types.h"
#include "microkernel/k_struct.h"

extern K_TIMER *_k_timer_list_head;
extern K_TIMER *_k_timer_list_tail;
extern int64_t _k_sys_clock_tick_count;
extern unsigned int _k_workload_slice;
extern unsigned int _k_workload_ticks;
extern unsigned int _k_workload_ref_time;
extern unsigned int _k_workload_t0;
extern unsigned int _k_workload_t1;
extern volatile unsigned int WldN0;
extern volatile unsigned int WldN1;
extern volatile unsigned int Wld_i;
extern volatile unsigned int Wld_i0;
extern volatile unsigned int WldTDelta;
extern volatile unsigned int WldT_start;
extern volatile unsigned int WldT_end;

extern void enlist_timer(K_TIMER *T);
extern void delist_timer(K_TIMER *T);
extern void enlist_timeout(struct k_args *P);
extern void delist_timeout(K_TIMER *T);
extern void force_timeout(struct k_args *A);
extern void K_alloc_timer(struct k_args *P);
extern void K_dealloc_timer(struct k_args *P);
extern void K_start_timer(struct k_args *P);
extern void K_stop_timer(struct k_args *P);
extern void K_sleep(struct k_args *P);
extern void K_wakeup(struct k_args *P);
extern void K_elapse(struct k_args *P);
extern void K_workload(struct k_args *P);

extern int64_t _LowTimeGet(void);
extern void _LowTimeInc(int inc);
#endif
