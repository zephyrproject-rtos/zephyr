/* micro_private.h */

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

#include <stddef.h>
#include <micro_private_types.h>
#include <kernel_main.h>
#include <nano_private.h>

#define KERNEL_ENTRY(A) _k_task_call(A)

#define OBJ_INDEX(objId) ((uint16_t)objId)

extern struct k_tqhd _k_task_priority_list[];

extern struct pool_struct _k_mem_pool_list[];

extern int _k_mem_pool_count;

extern const int _k_num_events;

extern struct k_task *_k_current_task;
extern uint32_t _k_task_priority_bitmap[];

extern struct k_timer *_k_timer_list_head;
extern struct k_timer *_k_timer_list_tail;

extern struct nano_stack _k_command_stack;
extern struct nano_lifo _k_server_command_packet_free;
extern struct nano_lifo _k_timer_free;

extern void _k_timer_enlist(struct k_timer *T);
extern void _k_timer_delist(struct k_timer *T);

extern void _k_timeout_alloc(struct k_args *P);
extern void _k_timeout_free(struct k_timer *T);
extern void _k_timeout_cancel(struct k_args *A);

extern void _k_timer_list_update(int ticks);

extern void _k_do_event_signal(kevent_t event);

extern void _k_state_bit_set(struct k_task *, uint32_t);
extern void _k_state_bit_reset(struct k_task *, uint32_t);
extern void _k_task_call(struct k_args *);

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
#define TF_BLCK 0x00000008    /* Blocked */
#define TF_GDBSTOP 0x00000010 /* Stopped by GDB agent */
#define TF_PRIO 0x00000020    /* Task priority is changing */

#define TF_TIME 0x00000800     /* Sleeping */
#define TF_DRIV 0x00001000     /* Waiting for arch specific driver */
#define TF_RES0 0x00002000     /* Reserved */
#define TF_EVNT 0x00004000     /* Waiting for an event */
#define TF_ENQU 0x00008000     /* Waiting to put data on a FIFO */
#define TF_DEQU 0x00010000     /* Waiting to get data from a FIFO */
#define TF_SEND 0x00020000     /* Waiting to send via mailbox or pipe */
#define TF_RECV 0x00040000     /* Waiting to recv via mailbox or pipe */
#define TF_SEMA 0x00080000     /* Waiting for a semaphore */
#define TF_LIST 0x00100000     /* Waiting for a group of semaphores */
#define TF_LOCK 0x00200000     /* Waiting for a mutex */
#define TF_ALLO 0x00400000     /* Waiting on a memory mapping */
#define TF_GTBL 0x00800000     /* Waiting on a memory pool */
#define TF_RES1 0x01000000     /* Reserved */
#define TF_RES2 0x02000000     /* Reserved */
#define TF_RECVDATA 0x04000000 /* Waiting to receive data */
#define TF_SENDDATA 0x08000000 /* Waiting to send data */

#define TF_ALLW 0x0FFFF800 /* Mask of all wait flags */

#ifdef CONFIG_TASK_DEBUG
extern int _k_debug_halt;
#endif

#ifdef CONFIG_TASK_MONITOR

#define MON_TSWAP 1
#define MON_STATE 2
#define MON_KSERV 4
#define MON_EVENT 8
#define MON_ALL 15

typedef void (*k_task_monitor_hook_t)(ktask_t taskid, uint32_t timestamp);

extern void task_monitor_hook_set(k_task_monitor_hook_t func);

extern void _k_task_monitor(struct k_task *, uint32_t d2);
extern void _k_task_monitor_args(struct k_args *);
extern void _k_task_monitor_read(struct k_args *);

extern const int _k_monitor_mask;

/* task level monitor bits */

#define MO_STBIT0 0x20000000
#define MO_STBIT1 0x30000000
#define MO_EVENT 0x40000000
#define MO_LCOMM 0x50000000
#define MO_RCOMM 0x60000000

#endif

#ifdef CONFIG_WORKLOAD_MONITOR
extern void _k_workload_monitor_calibrate(void);
extern void _k_workload_monitor_update(void);
extern void _k_workload_monitor_idle_start(void);
extern void _k_workload_monitor_idle_end(void);
#else
#define _k_workload_monitor_update()	do { /* nothing */ } while (0)
#endif

#define INSERT_ELM(L, E)                              \
	{                                             \
		struct k_args *X = (L);                      \
		struct k_args *Y = NULL;                     \
		while (X && (X->Prio <= (E)->Prio)) { \
			Y = X;                        \
			X = X->next;                  \
		}                                     \
		if (Y)                                \
			Y->next = (E);                \
		else                                  \
			(L) = (E);                    \
		(E)->next = X;                        \
		(E)->Head = &(L);                     \
	}

#define REMOVE_ELM(E)                                   \
	{                                               \
		struct k_args *X = *((E)->Head);               \
		struct k_args *Y = NULL;                       \
							\
		while (X && (X != (E))) {               \
			Y = X;                          \
			X = X->next;                    \
		}                                       \
		if (X) {                                \
			if (Y)                          \
				Y->next = X->next;      \
			else                            \
				*((E)->Head) = X->next; \
		}                                       \
	}

#define GETARGS(A)                        \
	do {                              \
		(A) = _nano_fiber_lifo_get_panic(&_k_server_command_packet_free); \
	} while (0)
#define GETTIMER(T)                        \
	do {                               \
		(T) = _nano_fiber_lifo_get_panic(&_k_timer_free); \
	} while (0)

#define FREEARGS(A) nano_fiber_lifo_put(&_k_server_command_packet_free, (A))
#define FREETIMER(T) nano_fiber_lifo_put(&_k_timer_free, (T))

#define TO_ALIST(L, A) nano_fiber_stack_push((L), (uint32_t)(A))

#define SENDARGS(A) nano_fiber_stack_push(&_k_command_stack, (uint32_t)(A))

#endif
