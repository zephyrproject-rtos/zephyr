/* microkernel.h - public API for VxMicro microkernel */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
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

#ifndef _MICROKERNEL_H
#define _MICROKERNEL_H

#include <nanokernel.h>

#include <cputype.h>
#include <microkernel/k_types.h>
#include <microkernel/k_struct.h>
#include <microkernel/entries.h>
#include <clock_vars.h>

#include <microkernel/task.h>
#include <microkernel/ticks.h>
#include <microkernel/mmap.h>
#include <microkernel/mutex.h>
#include <microkernel/mail.h>
#include <microkernel/fifo.h>
#include <microkernel/sema.h>
#include <microkernel/event.h>
#include <microkernel/pool.h>
#include <microkernel/memcpy.h>
#include <microkernel/chan.h>
#include <microkernel/task_irq.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MON_TSWAP 1
#define MON_STATE 2
#define MON_KSERV 4
#define MON_EVENT 8
#define MON_ALL 15

extern void _task_nop(void);

extern int task_offload_to_fiber(int (*)(), void *);

/* The following two typedefs are used in kernel_main.c */
typedef void (*taskstartfunction)(void);
typedef void (*taskabortfunction)(void);

/* Miscellaneous */
extern void kernel_init(void);
extern void init_node(void);    /* generated function */
extern void init_drivers(void); /* generated function */
extern const knode_t K_ThisNode;
extern const int K_DataSize;
extern int K_DataNall;
extern int K_ArgsNall;
extern int K_TimerNall;
extern int K_StackSize;

extern void *K_DataBlockPtr;
extern struct k_args *K_ArgsBlockPtr;
extern K_TIMER *K_TimerBlockPtr;
extern void *_minik_stckp;

extern const uint32_t MTS_CommBufferSize;
extern int MTS_RxBuffer[];
extern int MTS_TxBuffer[];
extern uint32_t *const MTS_PeekPokeBufferPtr;
extern kmbox_t hostrxmbx;
extern kmbox_t hosttxmbx;
extern kmemory_pool_t hostrxpool;
extern ktask_t ppdrv;

extern int K_NodeCount;
extern int K_PrioCount;
extern int K_TaskCount;
extern int K_QueCount;
extern int K_MapCount;
extern int K_SemCount;
extern int K_MutexCount;
extern int K_MbxCount;
extern int K_PoolCount;
extern int K_PipeCount;
extern const int K_max_eventnr;

extern PFN_CHANNEL_RW pHS_Channel_Put;
extern PFN_CHANNEL_RW pHS_Channel_PutW;
extern PFN_CHANNEL_RWT pHS_Channel_PutWT;
extern PFN_CHANNEL_RW pHS_Channel_Get;
extern PFN_CHANNEL_RW pHS_Channel_GetW;
extern PFN_CHANNEL_RWT pHS_Channel_GetWT;
extern PFN_CHANNEL_RWT pKS_Channel_PutWT;
extern PFN_CHANNEL_RWT pKS_Channel_GetWT;

/* needed by generated kernel_main.c */
extern void	timer_driver	(int priority);
extern void	DefaultHostDriver (void);

/* common event numbers */
#define TICK_EVENT	0

#ifdef __cplusplus
}
#endif

#endif /* _MICROKERNEL_H */
