/* minik.h */

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

#ifndef MINIK_H
#define MINIK_H

#include <microkernel/k_struct.h>
#include <clock_vars.h>

#define KERNEL_ENTRY(A) K_taskcall(A)

extern const knode_t _k_this_node;
extern struct k_proc _k_task_list[];
extern struct k_tqhd _k_task_priority_list[];

extern int _k_task_count;

#ifndef LITE
extern struct map_struct _k_mem_map_list[];
#endif
extern struct mbx_struct _k_mbox_list[];
extern struct mutex_struct _k_mutex_list[];
extern struct sem_struct _k_sem_list[];
extern struct que_struct _k_fifo_list[];
extern struct pool_struct _k_mem_pool_list[];
extern struct pipe_struct _k_pipe_list[];

#ifndef LITE
extern int _k_mem_map_count;
#endif
extern int _k_mem_pool_count;
extern int _k_pipe_count;

extern struct k_proc *_k_current_task;
extern uint32_t _k_task_priority_bitmap[];

#ifndef LITE
extern K_TIMER *_k_timer_list_head;
extern K_TIMER *K_Ttail;
#endif
extern int64_t K_LowTime;

extern struct nano_stack K_Args;
extern struct nano_lifo LinkIn[6];
extern struct nano_lifo LinkOut[6];
extern struct nano_lifo K_DataFree;
extern struct nano_lifo K_ArgsFree;
#ifndef LITE
extern struct nano_lifo K_TimerFree;
#endif

extern void _Cput(struct nano_lifo *, void *);
extern void _Cpsh(struct nano_stack *, uint32_t);
extern void *_Cget(struct nano_lifo *);

extern void set_state_bit(struct k_proc *, uint32_t);
extern void reset_state_bit(struct k_proc *, uint32_t);
extern void K_taskcall(struct k_args *);

#ifndef LITE
extern void delist_timeout(K_TIMER *T);
extern void enlist_timeout(struct k_args *A);
extern void force_timeout(struct k_args *A);
#endif

extern void init_drivers(void);

/*
 * The task status flags may be OR'ed together to form a task's state.  The
 * existence of one or more non-zero bits indicates that the task can not be
 * scheduled for execution because of the conditions associated with those
 * bits.  The task status flags are divided into four (4) groups as follows:
 *
 * Break flags (bits 0..5) are associated with conditions that require an
 * external entity to permit further execution.
 *
 * Spare flags (bits 6..10) are located between the break and wait flags
 * to allow either set to be extended without impacting the other group.
 *
 * Wait flags (bits 11..27) are associated with operations that the task itself
 * initiated, and for which task execution will resume when the requested
 * operation completes.
 *
 * Monitoring bits (bits 28..31) are reserved for use with task level
 * monitoring.
 */

#define TF_STOP 0x00000001    /* Not started */
#define TF_TERM 0x00000002    /* Terminated */
#define TF_SUSP 0x00000004    /* Suspended */
#define TF_BLCK 0x00000008    /* Blocked by host server debugger */
#define TF_GDBSTOP 0x00000010 /* Stopped by GDB agent */
#define TF_PRIO 0x00000020    /* Task priority is changing */

#define TF_TIME 0x00000800     /* Sleeping */
#define TF_DRIV 0x00001000     /* Waiting for arch specific driver */
#define TF_DATA 0x00002000     /* Waiting for movedata to complete */
#define TF_EVNT 0x00004000     /* Waiting for an event */
#define TF_ENQU 0x00008000     /* Waiting to put data on a FIFO */
#define TF_DEQU 0x00010000     /* Waiting to get data from a FIFO */
#define TF_SEND 0x00020000     /* Waiting to send via mbox or channel */
#define TF_RECV 0x00040000     /* Waiting to recv via mbox or channel */
#define TF_SEMA 0x00080000     /* Waiting for a semaphore */
#define TF_LIST 0x00100000     /* Waiting for a group of semaphores */
#define TF_LOCK 0x00200000     /* Waiting for a mutex */
#define TF_ALLO 0x00400000     /* Waiting on a memory mapping */
#define TF_GTBL 0x00800000     /* Waiting on a memory pool */
#define TF_FIFO 0x01000000     /* Waiting on a remote FIFO operation */
#define TF_NETW 0x02000000     /* Obsolete */
#define TF_RECVDATA 0x04000000 /* Waiting to receive data */
#define TF_SENDDATA 0x08000000 /* Waiting to send data */

#define TF_ALLW 0x0FFFF800 /* Mask of all wait flags */

#ifdef CONFIG_TASK_DEBUG

extern int K_DebugHalt;
extern char *K_NodeNames[];
extern char **K_TaskNames[];
#ifndef LITE
extern char **K_MapNames[];
#endif
extern char **K_MbxNames[];
extern char **K_MutexNames[];
extern char **K_SemNames[];
extern char **K_QueNames[];

#define TASK_INFO 1
#define WSP_INFO 2
#define MAP_INFO 3
#define MBX_INFO 4
#define QUE_INFO 5
#define RES_INFO 6
#define SEM_INFO 7
#define TIMER_INFO 8
#define APCK_INFO 9
#define DPCK_INFO 10
#define RESET_COUNTS 11
#define MONIT_INFO 12

#endif

#ifdef CONFIG_TASK_MONITOR

extern void K_monitor_task(struct k_proc *, uint32_t d2);
extern void K_monitor_args(struct k_args *);
extern void K_monitor_read(struct k_args *);

extern const int K_monitor_mask;

/* task level monitor bits */

#define MO_STBIT0 0x20000000
#define MO_STBIT1 0x30000000
#define MO_EVENT 0x40000000
#define MO_LCOMM 0x50000000
#define MO_RCOMM 0x60000000

#endif

#ifdef CONFIG_WORKLOAD_MONITOR

extern void wlMonitorCalibrate(void);

#endif

#define INSERT_ELM(L, E)                              \
	{                                             \
		struct k_args *X = (L);                      \
		struct k_args *Y = NULL;                     \
		while (X && (X->Prio <= (E)->Prio)) { \
			Y = X;                        \
			X = X->Forw;                  \
		}                                     \
		if (Y)                                \
			Y->Forw = (E);                \
		else                                  \
			(L) = (E);                    \
		(E)->Forw = X;                        \
		(E)->Head = &(L);                     \
	}

#define REMOVE_ELM(E)                                   \
	{                                               \
		struct k_args *X = *((E)->Head);               \
		struct k_args *Y = NULL;                       \
							\
		while (X && (X != (E))) {               \
			Y = X;                          \
			X = X->Forw;                    \
		}                                       \
		if (X) {                                \
			if (Y)                          \
				Y->Forw = X->Forw;      \
			else                            \
				*((E)->Head) = X->Forw; \
		}                                       \
	}

#define GETARGS(A)                        \
	do {                              \
		(A) = _Cget(&K_ArgsFree); \
	} while (0)
#define GETDATA(D)                        \
	do {                              \
		(D) = _Cget(&K_DataFree); \
	} while (0)
#ifndef LITE
#define GETTIMER(T)                        \
	do {                               \
		(T) = _Cget(&K_TimerFree); \
	} while (0)
#endif

#define FREEARGS(A) _Cput(&K_ArgsFree, (A))
#define FREEDATA(D) _Cput(&K_DataFree, (D))
#ifndef LITE
#define FREETIMER(T) _Cput(&K_TimerFree, (T))
#endif

#define TO_ALIST(L, A) _Cpsh((L), (uint32_t)(A))

#define SENDARGS(A) _Cpsh(&K_Args, (uint32_t)(A))

#endif
