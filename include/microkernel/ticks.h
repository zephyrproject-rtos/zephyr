/* microkernel/ticks.h - microkernel tick header file */

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

#ifndef TICKS_H
#define TICKS_H

/* includes */

#include <nanokernel.h>
#include <microkernel/k_struct.h>
#include <clock_vars.h>

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

extern int K_ticker(int event);
extern K_TIMER *_k_timer_list_head;
extern K_TIMER *_k_timer_list_tail;
extern struct nano_lifo _k_timer_free;
extern K_TIMER K_TimerBlocks[]; /* array of microkernel timer objects */
extern const knode_t _k_this_node;

extern void scheduler_time_slice_set(int32_t t, kpriority_t p);

/*******************************************************************************
*
* _timer_id_to_ptr - convert timer pointer to timer object identifier
*
* This routine converts a timer pointer into a timer object identifier.
*
* This algorithm relies on the fact that subtracting two pointers that point
* to elements of an array returns the difference between the array subscripts
* of those elements. (That is, "&a[j]-&a[i]" returns "j-i".)
*
*
* RETURNS: timer object identifier
*/

static inline ktimer_t _timer_ptr_to_id(K_TIMER *timer)
{
	return (ktimer_t)((_k_this_node << 16) +
			  (uint32_t)(timer - &K_TimerBlocks[0]));
}

/*******************************************************************************
*
* _timer_id_to_ptr - convert timer object identifier to timer pointer
*
* This routine converts a timer object identifier into a timer pointer.
*
* RETURNS: timer pointer
*/

static inline K_TIMER *_timer_id_to_ptr(ktimer_t timer)
{
	return &K_TimerBlocks[OBJ_INDEX(timer)];
}

extern uint32_t task_node_cycle_get_32(void);
extern int32_t task_node_tick_get_32(void);
extern int64_t task_node_tick_get(void);
extern ktimer_t task_timer_alloc(void);
extern void task_timer_free(ktimer_t timer);
extern void task_timer_start(ktimer_t timer,
			     int32_t duration,
			     int32_t Tr,
			     ksem_t sema);
extern void task_timer_restart(ktimer_t timer, int32_t duration, int32_t Tr);
extern void task_timer_stop(ktimer_t timer);
extern int32_t task_node_tick_delta(int64_t *reftime);

extern void task_sleep(int32_t ticks);
extern int task_node_workload_get(void);
extern void workload_time_slice_set(int32_t t);

extern int kernel_idle(void);

#define isr_node_cycle_get_32() task_node_cycle_get_32()
#define isr_node_tick_get_32() task_node_tick_get_32()
#define isr_node_tick_get() task_node_tick_get()

#ifdef __cplusplus
}
#endif

#endif /* TICKS_H */
