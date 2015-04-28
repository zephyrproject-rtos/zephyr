/* timer kernel services */

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


#include <microkernel.h>
#include <toolchain.h>
#include <sections.h>

#include <minik.h>
#include <kticks.h>
#include <drivers/system_timer.h>

/*******************************************************************************
*
* task_node_cycle_get_32 - read the processor's high precision timer
*
* This routine reads the processor's high precision timer.  It reads the
* counter register on the timer device. This counter register increments
* at a relatively high rate (e.g. 20 MHz), and thus is considered a
* "high resolution" timer.  This is in contrast to nano_node_tick_get_32() and
* task_node_tick_get_32() which return the value of the kernel ticks variable.
*
* RETURNS: current high precision clock value
*/

uint32_t task_node_cycle_get_32(void)
{
	return timer_read();
}

/*******************************************************************************
*
* task_node_tick_get - read the current system clock value
*
* This routine returns the current system clock value as measured in ticks.
*
* RETURNS: current system clock value
*/

int64_t task_node_tick_get(void)
{
	return _LowTimeGet();
}

/*******************************************************************************
*
* task_node_tick_get_32 - read the current system clock value
*
* This routine returns the lower 32-bits of the current system clock value
* as measured in ticks.
*
* RETURNS: lower 32-bit of the current system clock value
*/

int32_t task_node_tick_get_32(void)
{
	return (int32_t)_k_sys_clock_tick_count;
}

/*******************************************************************************
*
* enlist_timer - insert a timer into the timer queue
*
* RETURNS: N/A
*/

void enlist_timer(K_TIMER *T)
{
	K_TIMER *P = _k_timer_list_head;
	K_TIMER *Q = NULL;

	while (P && (T->Ti > P->Ti)) {
		T->Ti -= P->Ti;
		Q = P;
		P = P->Forw;
	}
	if (P) {
		P->Ti -= T->Ti;
		P->Back = T;
	} else
		_k_timer_list_tail = T;
	if (Q)
		Q->Forw = T;
	else
		_k_timer_list_head = T;
	T->Forw = P;
	T->Back = Q;
}

/*******************************************************************************
*
* delist_timer - remove a timer from the timer queue
*
* RETURNS: N/A
*/

void delist_timer(K_TIMER *T)
{
	K_TIMER *P = T->Forw;
	K_TIMER *Q = T->Back;

	if (P) {
		P->Ti += T->Ti;
		P->Back = Q;
	} else
		_k_timer_list_tail = Q;
	if (Q)
		Q->Forw = P;
	else
		_k_timer_list_head = P;
	T->Ti = -1;
}

/*******************************************************************************
*
* enlist_timeout - allocate and insert a timer into the timer queue
*
* RETURNS: N/A
*/

void enlist_timeout(struct k_args *P)
{
	K_TIMER *T;

	GETTIMER(T);
	T->Ti = P->Time.ticks;
	T->Tr = 0;
	T->Args = P;
	enlist_timer(T);
	P->Time.timer = T;
}

/*******************************************************************************
*
* force_timeout - remove a non-expired timer from the timer queue
*
* RETURNS: N/A
*/

void force_timeout(struct k_args *A)
{
	K_TIMER *T = A->Time.timer;

	if (T->Ti != -1) {
		delist_timer(T);
		TO_ALIST(&_k_command_stack, A);
	}
}

/*******************************************************************************
*
* delist_timeout - remove a non-expired timer from the timer queue and free it
*
* RETURNS: N/A
*/

void delist_timeout(K_TIMER *T)
{
	if (T->Ti != -1)
		delist_timer(T);
	FREETIMER(T);
}

/*******************************************************************************
*
* _k_timer_alloc - handle timer allocation request
*
* This routine, called by K_swapper(), handles the request for allocating a
* timer.
*
* RETURNS: N/A
*/

void _k_timer_alloc(
	struct k_args *P /* pointer to timer allocation request arguments */
	)
{
	K_TIMER *T;
	struct k_args *A;

	T = _Cget(&_k_timer_free);
	P->Args.c1.timer = T;

	if (T) {
		GETARGS(A);
		T->Args = A;
		T->Ti = -1; /* -1 indicates that timer is disabled */
	}
}

/*******************************************************************************
*
* task_timer_alloc - allocate a timer and return its object identifier
*
* This routine allocates a timer object and returns its identifier,
* or INVALID_OBJECT if no timer is available.
*
* RETURNS: timer identifier on success, INVALID_OBJECT on error
*/

ktimer_t task_timer_alloc(void)
{
	struct k_args A;
	K_TIMER *timer;

	A.Comm = TALLOC;
	KERNEL_ENTRY(&A);
	timer = A.Args.c1.timer;
	return timer ? _timer_ptr_to_id(timer) : INVALID_OBJECT;
}

/*******************************************************************************
*
* _k_timer_dealloc - handle timer deallocation request
*
* This routine, called by K_swapper(), handles the request for deallocating a
* timer.
*
* RETURNS: N/A
*/

void _k_timer_dealloc(struct k_args *P)
{
	K_TIMER *T = P->Args.c1.timer;
	struct k_args *A = T->Args;

	if (T->Ti != -1)
		delist_timer(T);

	FREETIMER(T);
	FREEARGS(A);
}

/*******************************************************************************
*
* task_timer_free - deallocate a timer
*
* This routine frees the resources associated with the timer.  If a timer was
* started, it has to be stopped using task_timer_stop() before it can be freed.
*
* RETURNS: N/A
*/

void task_timer_free(ktimer_t timer /* timer to deallocate */
		     )
{
	struct k_args A;

	A.Comm = TDEALLOC;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* _k_timer_start - handle start timer request
*
* This routine, called by K_swapper(), handles the start timer request from
* both task_timer_start() and task_timer_restart().
*
* RETURNS: N/A
*/

void _k_timer_start(struct k_args *P /* pointer to timer start
						      request arguments */
					 )
{
	K_TIMER *T = P->Args.c1.timer; /* ptr to the timer to start */

	if (T->Ti != -1) /* Stop the timer if it is active */
		delist_timer(T);

	T->Ti = (int32_t)P->Args.c1.time1; /* Set the initial delay */
	T->Tr = P->Args.c1.time2;	  /* Set the period */

	if ((T->Ti < 0) || (T->Tr < 0)) {/* Either the initial delay and/or */
		T->Ti = -1;		 /* the period is invalid.  Mark    */
		return;			 /* the timer as inactive.          */
	}

	if (T->Ti == 0) {
		if (T->Tr != 0) {/* Match the initial delay to the period. */
			T->Ti = T->Tr;
		} else {	    /* Ti=0, Tr=0 is an invalid combination. */
			T->Ti = -1; /* Mark the timer as invalid. */
			return;
		}
	}

	if (P->Args.c1.sema != ENDLIST) { /* Track the semaphore to
					   * signal for when the timer
					   * expires. */
		T->Args->Comm = SIGNALS;
		T->Args->Args.s1.sema = P->Args.c1.sema;
	}
	enlist_timer(T);
}

/*******************************************************************************
*
* task_timer_start - start or restart the specified low resolution timer
*
* This routine starts or restarts the specified low resolution timer.
*
* When the specified number of ticks, set by <Ti>, expires, the semaphore is
* signalled.  The timer repeats the expiration/signal cycle each time <Tr>
* ticks has elapsed.
*
* Setting <Tr> to 0 stops the timer at the end of the initial delay. Setting
* <Ti> to 0 will cause an initial delay equal to the repetition interval.  If
* both <Ti> and <Tr> are set to 0, or if one or both of the values is invalid
* (negative), then this kernel API acts like a task_timer_stop(): if the
* allocated timer was still running (from a previous call), it will be
* cancelled; if not, nothing will happen.
*
* RETURNS: N/A
*/

void task_timer_start(ktimer_t timer, /* timer to start */
		      int32_t Ti,     /* initial delay in ticks */
		      int32_t Tr,     /* repetition interval in ticks */
		      ksem_t sema     /* semaphore to signal */
		      )
{
	struct k_args A;

	A.Comm = TSTART;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	A.Args.c1.time1 = (int64_t)Ti;
	A.Args.c1.time2 = Tr;
	A.Args.c1.sema = sema;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_timer_restart - restart a timer
*
* This routine restarts the timer specified by <timer>.
*
* RETURNS: N/A
*/

void task_timer_restart(ktimer_t timer, /* timer to restart */
			int32_t Ti,     /* initial delay */
			int32_t Tr      /* repetition interval */
			)
{
	struct k_args A;

	A.Comm = TSTART;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	A.Args.c1.time1 = (int64_t)Ti;
	A.Args.c1.time2 = Tr;
	A.Args.c1.sema = ENDLIST;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* _k_timer_stop - handle stop timer request
*
* This routine, called by K_swapper(), handles the request for stopping a
* timer.
*
* RETURNS: N/A
*/

void _k_timer_stop(struct k_args *P)
{
	K_TIMER *T = P->Args.c1.timer;

	if (T->Ti != -1)
		delist_timer(T);
}

/*******************************************************************************
*
* task_timer_stop - stop a timer
*
* This routine stops the specified timer. If the timer period has already
* elapsed, the call has no effect.
*
* RETURNS: N/A
*/

void task_timer_stop(ktimer_t timer /* timer to stop */
		     )
{
	struct k_args A;

	A.Comm = TSTOP;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* _k_task_wakeup - handle internally issued task wakeup request
*
* This routine, called by K_swapper(), handles the request for waking a task
* at the end of its sleep period.
*
* RETURNS: N/A
*/

void _k_task_wakeup(struct k_args *P)
{
	K_TIMER *T;
	struct k_proc *X;

	X = P->Ctxt.proc;
	T = P->Time.timer;

	FREETIMER(T);
	reset_state_bit(X, TF_TIME);
}

/*******************************************************************************
*
* _k_task_sleep - handle task sleep request
*
* This routine, called by K_swapper(), handles the request for putting a task
* to sleep.
*
* RETURNS: N/A
*/

void _k_task_sleep(struct k_args *P)
{
	K_TIMER *T;

	if ((P->Time.ticks) <= 0)
		return;

	GETTIMER(T);
	T->Ti = P->Time.ticks;
	T->Tr = 0;
	T->Args = P;

	P->Comm = WAKEUP;
	P->Ctxt.proc = _k_current_task;
	P->Time.timer = T;

	enlist_timer(T);
	set_state_bit(_k_current_task, TF_TIME);
}

/*******************************************************************************
*
* task_sleep - sleep for a number of ticks
*
* This routine suspends the calling task for the specified number of timer
* ticks.  When the task is awakened, it is rescheduled according to its
* priority.
*
* RETURNS: N/A
*/

void task_sleep(int32_t ticks /* number of ticks for which to sleep */
		  )
{
#ifndef LITE
	struct k_args A;

	A.Comm = SLEEP;
	A.Time.ticks = ticks;
	KERNEL_ENTRY(&A);
#else
	int64_t t = task_node_tick_get();
	int64_t total = 0;

	do {
		task_yield();
		total += task_node_tick_delta(&t);
	} while (total < ticks);
#endif
}

/*******************************************************************************
*
* _k_time_elapse - handle elapsed ticks calculation request
*
* This routine, called by K_swapper(), handles the request for calculating the
* time elapsed since the specified reference time.
*
* RETURNS: N/A
*/

void _k_time_elapse(struct k_args *P)
{
	int64_t now = _LowTimeGet();

	P->Args.c1.time2 = (int32_t)(now - P->Args.c1.time1);
	P->Args.c1.time1 = now;
}

/*******************************************************************************
*
* task_node_tick_delta - return ticks between calls
*
* This function is meant to be used in contained fragments of code. The first
* call to it in a particular code fragment fills in a reference time variable
* which then gets passed and updated every time the function is called. From
* the second call on, the delta between the value passed to it and the current
* tick count is the return value. Since the first call is meant to only fill in
* the reference time, its return value should be discarded.
*
* Since a code fragment that wants to use task_node_tick_delta() passes in its
* own reference time variable, multiple code fragments can make use of this
* function concurrently.
*
* Note that it is not necessary to allocate a timer to use this call.
*
* RETURNS: elapsed time in system ticks
*/

int32_t task_node_tick_delta(int64_t *reftime /* pointer to reference time */
			   )
{
	struct k_args A;

	A.Comm = ELAPSE;
	A.Args.c1.time1 = *reftime;
	KERNEL_ENTRY(&A);
	*reftime = A.Args.c1.time1;
	return A.Args.c1.time2;
}
