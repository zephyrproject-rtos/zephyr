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

#include <micro_private.h>
#include <drivers/system_timer.h>

extern struct k_timer _k_timer_blocks[];

struct k_timer  *_k_timer_list_head = NULL;
struct k_timer  *_k_timer_list_tail = NULL;

/**
 *
 * @brief Insert a timer into the timer queue
 *
 * @return N/A
 */

void _k_timer_enlist(struct k_timer *T)
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

/**
 *
 * @brief Remove a timer from the timer queue
 *
 * @return N/A
 */

void _k_timer_delist(struct k_timer *T)
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

/**
 *
 * @brief Allocate timer used for command packet timeout
 *
 * Allocates timer for command packet and inserts it into the timer queue.
 *
 * @return N/A
 */

void _k_timeout_alloc(struct k_args *P)
{
	struct k_timer *T;

	GETTIMER(T);
	T->duration = P->Time.ticks;
	T->period = 0;
	T->Args = P;
	_k_timer_enlist(T);
	P->Time.timer = T;
}

/**
 *
 * @brief Cancel timer used for command packet timeout
 *
 * Cancels timer (if not already expired), then reschedules the command packet
 * for further processing.
 *
 * The command that is processed following cancellation is typically NOT the
 * command that would have occurred had the timeout expired on its own.
 * 
 * @return N/A
 */

void _k_timeout_cancel(struct k_args *A)
{
	struct k_timer *T = A->Time.timer;

	if (T->duration != -1) {
		_k_timer_delist(T);
		TO_ALIST(&_k_command_stack, A);
	}
}

/**
 *
 * @brief Free timer used for command packet timeout
 *
 * Cancels timer (if not already expired), then frees it.
 *
 * @return N/A
 */

void _k_timeout_free(struct k_timer *T)
{
	if (T->duration != -1)
		_k_timer_delist(T);
	FREETIMER(T);
}

/**
 *
 * @brief Handle expired timers
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
 * @return N/A
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
			_k_timer_enlist(T);
		} else {
			T->duration = -1;
		}
		TO_ALIST(&_k_command_stack, T->Args);

		ticks = 0; /* don't decrement duration for subsequent timer(s) */
	}
}

/**
 *
 * @brief Handle timer allocation request
 *
 * This routine, called by _k_server(), handles the request for allocating a
 * timer.
 *
 * @param P   Pointer to timer allocation request arguments.
 *
 * @return N/A
 */

void _k_timer_alloc(struct k_args *P)
{
	struct k_timer *T;
	struct k_args *A;

	GETTIMER(T);
	P->Args.c1.timer = T;

	GETARGS(A);
	T->Args = A;
	T->duration = -1; /* -1 indicates that timer is disabled */
}

/**
 *
 * @brief Allocate a timer and return its object identifier
 *
 * @return timer identifier
 */

ktimer_t task_timer_alloc(void)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_ALLOC;
	KERNEL_ENTRY(&A);

	return (ktimer_t)A.Args.c1.timer;
}

/**
 *
 * @brief Handle timer deallocation request
 *
 * This routine, called by _k_server(), handles the request for deallocating a
 * timer.
 *
 * @return N/A
 */

void _k_timer_dealloc(struct k_args *P)
{
	struct k_timer *T = P->Args.c1.timer;
	struct k_args *A = T->Args;

	if (T->duration != -1)
		_k_timer_delist(T);

	FREETIMER(T);
	FREEARGS(A);
}

/**
 *
 * @brief Deallocate a timer
 *
 * This routine frees the resources associated with the timer.  If a timer was
 * started, it has to be stopped using task_timer_stop() before it can be freed.
 *
 * @param timer   Timer to deallocate.
 *
 * @return N/A
 */

void task_timer_free(ktimer_t timer)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_DEALLOC;
	A.Args.c1.timer = (struct k_timer *)timer;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Handle start timer request
 *
 * This routine, called by _k_server(), handles the start timer request from
 * both task_timer_start() and task_timer_restart().
 *
 * @param P   Pointer to timer start request arguments.
 *
 * @return N/A
 */

void _k_timer_start(struct k_args *P)
{
	struct k_timer *T = P->Args.c1.timer; /* ptr to the timer to start */

	if (T->duration != -1) { /* Stop the timer if it is active */
		_k_timer_delist(T);
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
		T->Args->Comm = _K_SVC_SEM_SIGNAL;
		T->Args->Args.s1.sema = P->Args.c1.sema;
	}
	_k_timer_enlist(T);
}

/**
 *
 * @brief Start or restart the specified low resolution timer
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
 * @return N/A
 */

void task_timer_start(ktimer_t timer, int32_t duration, int32_t period,
					  ksem_t sema)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_START;
	A.Args.c1.timer = (struct k_timer *)timer;
	A.Args.c1.time1 = (int64_t)duration;
	A.Args.c1.time2 = period;
	A.Args.c1.sema = sema;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Restart a timer
 *
 * This routine restarts the timer specified by <timer>.
 *
 * @param timer      Timer to restart.
 * @param duration   Initial delay.
 * @param period     Repetition interval.
 *
 * @return N/A
 */

void task_timer_restart(ktimer_t timer, int32_t duration, int32_t period)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_START;
	A.Args.c1.timer = (struct k_timer *)timer;
	A.Args.c1.time1 = (int64_t)duration;
	A.Args.c1.time2 = period;
	A.Args.c1.sema = ENDLIST;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Handle stop timer request
 *
 * This routine, called by _k_server(), handles the request for stopping a
 * timer.
 *
 * @return N/A
 */

void _k_timer_stop(struct k_args *P)
{
	struct k_timer *T = P->Args.c1.timer;

	if (T->duration != -1)
		_k_timer_delist(T);
}

/**
 *
 * @brief Stop a timer
 *
 * This routine stops the specified timer. If the timer period has already
 * elapsed, the call has no effect.
 *
 * @param timer   Timer to stop.
 *
 * @return N/A
 */

void task_timer_stop(ktimer_t timer)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_STOP;
	A.Args.c1.timer = (struct k_timer *)timer;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Handle internally issued task wakeup request
 *
 * This routine, called by _k_server(), handles the request for waking a task
 * at the end of its sleep period.
 *
 * @return N/A
 */

void _k_task_wakeup(struct k_args *P)
{
	struct k_timer *T;
	struct k_proc *X;

	X = P->Ctxt.proc;
	T = P->Time.timer;

	FREETIMER(T);
	_k_state_bit_reset(X, TF_TIME);
}

/**
 *
 * @brief Handle task sleep request
 *
 * This routine, called by _k_server(), handles the request for putting a task
 * to sleep.
 *
 * @return N/A
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

	P->Comm = _K_SVC_TASK_WAKEUP;
	P->Ctxt.proc = _k_current_task;
	P->Time.timer = T;

	_k_timer_enlist(T);
	_k_state_bit_set(_k_current_task, TF_TIME);
}

/**
 *
 * @brief Sleep for a number of ticks
 *
 * This routine suspends the calling task for the specified number of timer
 * ticks.  When the task is awakened, it is rescheduled according to its
 * priority.
 *
 * @param ticks   Number of ticks for which to sleep.
 *
 * @return N/A
 */

void task_sleep(int32_t ticks)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_SLEEP;
	A.Time.ticks = ticks;
	KERNEL_ENTRY(&A);
}
