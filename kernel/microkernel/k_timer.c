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
#include <drivers/system_timer.h>

extern struct k_timer _k_timer_blocks[];

struct k_timer  *_k_timer_list_head = NULL;
struct k_timer  *_k_timer_list_tail = NULL;

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
* This algorithm also set the upper 16 bits of the object identifier
* to the same value utilized by the microkernel system generator.
*
* RETURNS: timer object identifier
*/

static inline ktimer_t _timer_ptr_to_id(struct k_timer *timer)
{
	return (ktimer_t)(0x00010000u + (uint32_t)(timer - &_k_timer_blocks[0]));
}

/*******************************************************************************
*
* _timer_id_to_ptr - convert timer object identifier to timer pointer
*
* This routine converts a timer object identifier into a timer pointer.
*
* RETURNS: timer pointer
*/

static inline struct k_timer *_timer_id_to_ptr(ktimer_t timer)
{
	return &_k_timer_blocks[OBJ_INDEX(timer)];
}

/*******************************************************************************
*
* enlist_timer - insert a timer into the timer queue
*
* RETURNS: N/A
*/

void enlist_timer(struct k_timer *T)
{
	struct k_timer *P = _k_timer_list_head;
	struct k_timer *Q = NULL;

	while (P && (T->duration > P->duration)) {
		T->duration -= P->duration;
		Q = P;
		P = P->Forw;
	}
	if (P) {
		P->duration -= T->duration;
		P->Back = T;
	} else {
		_k_timer_list_tail = T;
	}
	if (Q) {
		Q->Forw = T;
	} else {
		_k_timer_list_head = T;
	}
	T->Forw = P;
	T->Back = Q;
}

/*******************************************************************************
*
* delist_timer - remove a timer from the timer queue
*
* RETURNS: N/A
*/

void delist_timer(struct k_timer *T)
{
	struct k_timer *P = T->Forw;
	struct k_timer *Q = T->Back;

	if (P) {
		P->duration += T->duration;
		P->Back = Q;
	} else
		_k_timer_list_tail = Q;
	if (Q)
		Q->Forw = P;
	else
		_k_timer_list_head = P;
	T->duration = -1;
}

/*******************************************************************************
*
* enlist_timeout - allocate and insert a timer into the timer queue
*
* RETURNS: N/A
*/

void enlist_timeout(struct k_args *P)
{
	struct k_timer *T;

	GETTIMER(T);
	T->duration = P->Time.ticks;
	T->period = 0;
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
	struct k_timer *T = A->Time.timer;

	if (T->duration != -1) {
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

void delist_timeout(struct k_timer *T)
{
	if (T->duration != -1)
		delist_timer(T);
	FREETIMER(T);
}

/*******************************************************************************
*
* _k_timer_list_update - handle expired timers
*
* Process the sorted list of timers associated with waiting tasks and
* activate each task whose timer has now expired.
*
* With tickless idle, a tick announcement may encompass multiple ticks.
* Due to limitations of the underlying timer driver, the number of elapsed
* ticks may -- under very rare circumstances -- exceed the first timer's
* remaining tick count, although never by more a single tick. This means that
* a task timer may occasionally expire one tick later than it was scheduled to,
* and that a periodic timer may exhibit a slow, ever-increasing degree of drift
* from the main system timer over long intervals.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _k_timer_list_update(int ticks)
{
	struct k_timer *T;

	while (_k_timer_list_head != NULL) {
		_k_timer_list_head->duration -= ticks;

		if (_k_timer_list_head->duration > 0) {
			return;
		}

		T = _k_timer_list_head;
		if (T == _k_timer_list_tail) {
			_k_timer_list_head = _k_timer_list_tail = NULL;
		} else {
			_k_timer_list_head = T->Forw;
			_k_timer_list_head->Back = NULL;
		}
		if (T->period) {
			T->duration = T->period;
			enlist_timer(T);
		} else {
			T->duration = -1;
		}
		TO_ALIST(&_k_command_stack, T->Args);

		ticks = 0; /* don't decrement duration for subsequent timer(s) */
	}
}

/*******************************************************************************
*
* _k_timer_alloc - handle timer allocation request
*
* This routine, called by K_swapper(), handles the request for allocating a
* timer.
*
* @param P   Pointer to timer allocation request arguments.
*
* RETURNS: N/A
*/

void _k_timer_alloc(struct k_args *P)
{
	struct k_timer *T;
	struct k_args *A;

	GETTIMER(T);
	P->Args.c1.timer = T;

	if (T) {
		GETARGS(A);
		T->Args = A;
		T->duration = -1; /* -1 indicates that timer is disabled */
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
	struct k_timer *timer;

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
	struct k_timer *T = P->Args.c1.timer;
	struct k_args *A = T->Args;

	if (T->duration != -1)
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
* @param timer   Timer to deallocate.
*
* RETURNS: N/A
*/

void task_timer_free(ktimer_t timer)
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
* @param P   Pointer to timer start request arguments.
*
* RETURNS: N/A
*/

void _k_timer_start(struct k_args *P)
{
	struct k_timer *T = P->Args.c1.timer; /* ptr to the timer to start */

	if (T->duration != -1) { /* Stop the timer if it is active */
		delist_timer(T);
	}

	T->duration = (int32_t)P->Args.c1.time1; /* Set the initial delay */
	T->period = P->Args.c1.time2;	  /* Set the period */

	/*
	 * Either the initial delay and/or the period is invalid.  Mark
	 * the timer as inactive.
	 */
	if ((T->duration < 0) || (T->period < 0)) {
		T->duration = -1;
		return;
	}

	if (T->duration == 0) {
		if (T->period != 0) {/* Match the initial delay to the period. */
			T->duration = T->period;
		} else {	    /* duration=0, period=0 is an invalid combination. */
			T->duration = -1; /* Mark the timer as invalid. */
			return;
		}
	}

	/* Track the semaphore to signal for when the timer expires. */
	if (P->Args.c1.sema != ENDLIST) {
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
* When the specified number of ticks, set by <duration>, expires, the semaphore
* is signalled.  The timer repeats the expiration/signal cycle each time
* <period> ticks has elapsed.
*
* Setting <period> to 0 stops the timer at the end of the initial delay.
* Setting <duration> to 0 will cause an initial delay equal to the repetition
* interval.  If both <duration> and <period> are set to 0, or if one or both of
* the values is invalid (negative), then this kernel API acts like a
* task_timer_stop(): if the allocated timer was still running (from a
* previous call), it will be cancelled; if not, nothing will happen.
*
* @param timer      Timer to start.
* @param duration   Initial delay in ticks.
* @param period     Repetition interval in ticks.
* @param sema       Semaphore to signal.
*
* RETURNS: N/A
*/

void task_timer_start(ktimer_t timer, int32_t duration, int32_t period,
					  ksem_t sema)
{
	struct k_args A;

	A.Comm = TSTART;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	A.Args.c1.time1 = (int64_t)duration;
	A.Args.c1.time2 = period;
	A.Args.c1.sema = sema;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_timer_restart - restart a timer
*
* This routine restarts the timer specified by <timer>.
*
* @param timer      Timer to restart.
* @param duration   Initial delay.
* @param period     Repetition interval.
*
* RETURNS: N/A
*/

void task_timer_restart(ktimer_t timer, int32_t duration, int32_t period)
{
	struct k_args A;

	A.Comm = TSTART;
	A.Args.c1.timer = _timer_id_to_ptr(timer);
	A.Args.c1.time1 = (int64_t)duration;
	A.Args.c1.time2 = period;
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
	struct k_timer *T = P->Args.c1.timer;

	if (T->duration != -1)
		delist_timer(T);
}

/*******************************************************************************
*
* task_timer_stop - stop a timer
*
* This routine stops the specified timer. If the timer period has already
* elapsed, the call has no effect.
*
* @param timer   Timer to stop.
*
* RETURNS: N/A
*/

void task_timer_stop(ktimer_t timer)
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
	struct k_timer *T;
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
	struct k_timer *T;

	if ((P->Time.ticks) <= 0) {
		return;
	}

	GETTIMER(T);
	T->duration = P->Time.ticks;
	T->period = 0;
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
* @param ticks   Number of ticks for which to sleep.
*
* RETURNS: N/A
*/

void task_sleep(int32_t ticks)
{
	struct k_args A;

	A.Comm = SLEEP;
	A.Time.ticks = ticks;
	KERNEL_ENTRY(&A);
}
