/* timer kernel services */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
 * @brief Insert a timer into the timer queue
 * @param T Timer
 * @return N/A
 */
void _k_timer_enlist(struct k_timer *T)
{
	struct k_timer *P = _k_timer_list_head;
	struct k_timer *Q = NULL;

	while (P && (T->duration > P->duration)) {
		T->duration -= P->duration;
		Q = P;
		P = P->next;
	}
	if (P) {
		P->duration -= T->duration;
		P->prev = T;
	} else {
		_k_timer_list_tail = T;
	}
	if (Q) {
		Q->next = T;
	} else {
		_k_timer_list_head = T;
	}
	T->next = P;
	T->prev = Q;
}

/**
 * @brief Remove a timer from the timer queue
 * @param T Timer
 * @return N/A
 */
void _k_timer_delist(struct k_timer *T)
{
	struct k_timer *P = T->next;
	struct k_timer *Q = T->prev;

	if (P) {
		P->duration += T->duration;
		P->prev = Q;
	} else
		_k_timer_list_tail = Q;
	if (Q)
		Q->next = P;
	else
		_k_timer_list_head = P;
	T->duration = -1;
}

/**
 * @brief Allocate timer used for command packet timeout
 *
 * Allocates timer for command packet and inserts it into the timer queue.
 * @param P Arguments
 * @return N/A
 */
void _k_timeout_alloc(struct k_args *P)
{
	struct k_timer *T;

	GETTIMER(T);
	T->duration = P->Time.ticks;
	T->period = 0;
	T->args = P;
	_k_timer_enlist(T);
	P->Time.timer = T;
}

/**
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
 * @brief Free timer used for command packet timeout
 *
 * Cancels timer (if not already expired), then frees it.
 * @param T Timer
 * @return N/A
 */
void _k_timeout_free(struct k_timer *T)
{
	if (T->duration != -1)
		_k_timer_delist(T);
	FREETIMER(T);
}

/**
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
 * @param ticks Number of ticks
 * @return N/A
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
			_k_timer_list_head = T->next;
			_k_timer_list_head->prev = NULL;
		}
		if (T->period) {
			T->duration = T->period;
			_k_timer_enlist(T);
		} else {
			T->duration = -1;
		}
		TO_ALIST(&_k_command_stack, T->args);

		ticks = 0; /* don't decrement duration for subsequent timer(s) */
	}
}

/**
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
	P->args.c1.timer = T;

	GETARGS(A);
	T->args = A;
	T->duration = -1; /* -1 indicates that timer is disabled */
}


ktimer_t task_timer_alloc(void)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_ALLOC;
	KERNEL_ENTRY(&A);

	return (ktimer_t)A.args.c1.timer;
}

/**
 *
 * @brief Handle timer deallocation request
 *
 * This routine, called by _k_server(), handles the request for deallocating a
 * timer.
 * @param P Pointer to timer deallocation request arguments.
 * @return N/A
 */
void _k_timer_dealloc(struct k_args *P)
{
	struct k_timer *T = P->args.c1.timer;
	struct k_args *A = T->args;

	if (T->duration != -1)
		_k_timer_delist(T);

	FREETIMER(T);
	FREEARGS(A);
}


void task_timer_free(ktimer_t timer)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_DEALLOC;
	A.args.c1.timer = (struct k_timer *)timer;
	KERNEL_ENTRY(&A);
}

/**
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
	struct k_timer *T = P->args.c1.timer; /* ptr to the timer to start */

	if (T->duration != -1) { /* Stop the timer if it is active */
		_k_timer_delist(T);
	}

	T->duration = (int32_t)P->args.c1.time1; /* Set the initial delay */
	T->period = P->args.c1.time2;	  /* Set the period */

	/*
	 * Either the initial delay and/or the period is invalid.  Mark
	 * the timer as inactive.
	 */
	if ((T->duration <= 0) || (T->period < 0)) {
		T->duration = -1;
		return;
	}

	/* Track the semaphore to signal for when the timer expires. */
	if (P->args.c1.sema != _USE_CURRENT_SEM) {
		T->args->Comm = _K_SVC_SEM_SIGNAL;
		T->args->args.s1.sema = P->args.c1.sema;
	}
	_k_timer_enlist(T);
}


void task_timer_start(ktimer_t timer, int32_t duration, int32_t period,
					  ksem_t sema)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_START;
	A.args.c1.timer = (struct k_timer *)timer;
	A.args.c1.time1 = (int64_t)duration;
	A.args.c1.time2 = period;
	A.args.c1.sema = sema;
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
	struct k_timer *T = P->args.c1.timer;

	if (T->duration != -1)
		_k_timer_delist(T);
}


void task_timer_stop(ktimer_t timer)
{
	struct k_args A;

	A.Comm = _K_SVC_TIMER_STOP;
	A.args.c1.timer = (struct k_timer *)timer;
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
	struct k_task *X;

	X = P->Ctxt.task;
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
 * @param P   Pointer to timer sleep request arguments.
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
	T->args = P;

	P->Comm = _K_SVC_TASK_WAKEUP;
	P->Ctxt.task = _k_current_task;
	P->Time.timer = T;

	_k_timer_enlist(T);
	_k_state_bit_set(_k_current_task, TF_TIME);
}


void task_sleep(int32_t ticks)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_SLEEP;
	A.Time.ticks = ticks;
	KERNEL_ENTRY(&A);
}
