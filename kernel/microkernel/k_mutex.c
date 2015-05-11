/* mutex kernel services */

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

/*
DESCRIPTION
This module contains routines for handling mutex locking and unlocking.  It
also includes routines that force the release of  mutex objects when a task
is aborted or unloaded.

Mutexes implement a priority inheritance algorithm that boosts the priority
level of the owning task to match the priority level of the highest priority
task waiting on the mutex.

Each mutex that contributes to priority inheritance must be released in the
reverse order in which is was acquired.  Furthermore each subsequent mutex
that contributes to raising the owning task's priority level must be acquired
at a point after the most recent "bumping" of the priority level.

For example, if task A has two mutexes contributing to the raising of its
priority level, the second mutex M2 must be acquired by task A after task
A's priority level was bumped due to owning the first mutex M1.  When
releasing the mutex, task A must release M2 before it releases M1.  Failure
to follow this nested model may result in tasks running at unexpected priority
levels (too high, or too low).

NOMANUAL
*/

/* includes */

#include <microkernel.h>
#include <minik.h>
#include <nanok.h>

/*******************************************************************************
*
* _k_mutex_lock_reply - reply to a mutex lock request (LOCK_TMO, LOCK_RPL)
*
* This routine replies to a mutex lock request.  This will occur if either
* the waiting task times out or acquires the mutex lock.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _k_mutex_lock_reply(
	struct k_args *A /* pointer to mutex lock reply request arguments */
	)
{
#ifndef CONFIG_TICKLESS_KERNEL
	struct mutex_struct *Mutex; /* pointer to internal mutex structure */
	struct k_args *PrioChanger; /* used to change a task's priority level */
	struct k_args *FirstWaiter; /* pointer to first task in wait queue */
	kpriority_t newPriority;    /* priority level to which to drop */
	int MutexId;                /* mutex ID obtained from request args */

	if (A->Time.timer) {
		FREETIMER(A->Time.timer);
	}

	if (A->Comm == LOCK_TMO) {/* Timeout case */

		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;

		MutexId = A->Args.l1.mutex;
		Mutex = _k_mutex_list + OBJ_INDEX(MutexId);

		FirstWaiter = Mutex->Waiters;

		/*
		 * When timing out, there are two cases to consider.
		 * 1. There are no waiting tasks.
		 *    - As there are no waiting tasks, this mutex is no longer
		 *    involved in priority inheritance.  It's current priority
		 *    level should be dropped (if needed) to the original
		 *    priority level.
		 * 2. There is at least one waiting task in a priority ordered
		 *    list.
		 *    - Depending upon the the priority level of the first
		 *    waiting task, the owner task's original priority and
		 *    the ceiling priority, the owner's priority level may
		 *    be dropped but not necessarily to the original priority
		 *    level.
		 */

		newPriority = Mutex->OwnerOriginalPrio;

		if (FirstWaiter != NULL) {
			newPriority = (FirstWaiter->Prio < newPriority)
					      ? FirstWaiter->Prio
					      : newPriority;
			newPriority = (newPriority > CONFIG_PRIORITY_CEILING)
					      ? newPriority
					      : CONFIG_PRIORITY_CEILING;
		}

		if (Mutex->OwnerCurrentPrio != newPriority) {
			GETARGS(PrioChanger);
			PrioChanger->alloc = true;
			PrioChanger->Comm = SPRIO;
			PrioChanger->Prio = newPriority;
			PrioChanger->Args.g1.task = Mutex->Owner;
			PrioChanger->Args.g1.prio = newPriority;
			SENDARGS(PrioChanger);
			Mutex->OwnerCurrentPrio = newPriority;
		}
	} else {/* LOCK_RPL: Reply case */
		A->Time.rcode = RC_OK;
	}
#else
	/* LOCK_RPL: Reply case */
	A->Time.rcode = RC_OK;
#endif

	reset_state_bit(A->Ctxt.proc, TF_LOCK);
}

/*******************************************************************************
*
* _k_mutex_lock_request - process a mutex lock request
*
* This routine processes a mutex lock request (LOCK_REQ).  If the mutex
* is already locked, and the timeout is non-zero then the priority inheritance
* algorithm may be applied to prevent priority inversion scenarios.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _k_mutex_lock_request(struct k_args *A /* pointer to mutex lock
						  request arguments */
				     )
{
	struct mutex_struct *Mutex; /* pointer to internal mutex structure */
	int MutexId;                /* mutex ID obtained from lock request */
	struct k_args *PrioBooster; /* used to change a task's priority level */
	kpriority_t BoostedPrio;    /* new "boosted" priority level */

	MutexId = A->Args.l1.mutex;


	Mutex = _k_mutex_list + OBJ_INDEX(MutexId);
	if (Mutex->Level == 0 || Mutex->Owner == A->Args.l1.task) {
		/* The mutex is either unowned or this is a nested lock. */
#ifdef CONFIG_OBJECT_MONITOR
		Mutex->Count++;
#endif

		Mutex->Owner = A->Args.l1.task;

		/*
		 * Assign the task's priority directly if the requesting
		 * task is on this node.  This may be more recent than
		 * that stored in struct k_args.
		 */
		Mutex->OwnerCurrentPrio = _k_current_task->Prio;

		/*
		 * Save the original priority when first acquiring the lock (but
		 * not on nested locks).  The original priority level only
		 * reflects the priority level of the requesting task at the
		 * time the lock is acquired.  Consequently, if the requesting
		 * task is already involved in priority inheritance, this
		 * original priority reflects its "boosted" priority.
		 */
		if (Mutex->Level == 0) {
			Mutex->OwnerOriginalPrio = Mutex->OwnerCurrentPrio;
		}

		Mutex->Level++;

		A->Time.rcode = RC_OK;

	} else {
		/* The mutex is owned by another task. */
#ifdef CONFIG_OBJECT_MONITOR
		Mutex->Confl++;
#endif

		if (likely(A->Time.ticks != TICKS_NONE)) {
			/* A non-zero timeout was specified. */
				/*
				 * The requesting task is on this node. Ensure
				 * the priority saved in the request is up to
				 * date.
				 */
				A->Ctxt.proc = _k_current_task;
				A->Prio = _k_current_task->Prio;
				set_state_bit(_k_current_task, TF_LOCK);
			/* Note: Mutex->Waiters is a priority sorted list */
			INSERT_ELM(Mutex->Waiters, A);
#ifndef CONFIG_TICKLESS_KERNEL
			if (A->Time.ticks == TICKS_UNLIMITED) {
				/* Request will not time out */
				A->Time.timer = NULL;
			} else {
				/*
				 * Prepare to call _k_mutex_lock_reply() should
				 * the request time out.
				 */
				A->Comm = LOCK_TMO;
				enlist_timeout(A);
			}
#endif
			if (A->Prio < Mutex->OwnerCurrentPrio) {
				/*
				 * The priority level of the owning task is less
				 * than that of the requesting task.  Boost the
				 * priority level of the owning task to match
				 * the priority level of the requesting task.
				 * Note that the boosted priority level is
				 * limited to <K_PrioCeiling>.
				 */
				BoostedPrio = (A->Prio > CONFIG_PRIORITY_CEILING)
						      ? A->Prio
						      : CONFIG_PRIORITY_CEILING;
				if (BoostedPrio < Mutex->OwnerCurrentPrio) {
					/* Boost the priority level */
					GETARGS(PrioBooster);

					PrioBooster->alloc = true;
					PrioBooster->Comm = SPRIO;
					PrioBooster->Prio = BoostedPrio;
					PrioBooster->Args.g1.task = Mutex->Owner;
					PrioBooster->Args.g1.prio = BoostedPrio;
					SENDARGS(PrioBooster);
					Mutex->OwnerCurrentPrio = BoostedPrio;
				}
			}
		} else {
			/*
			 * ERROR.  The mutex is locked by another task and
			 * this is an immediate lock request (timeout = 0).
			 */
			A->Time.rcode = RC_FAIL;
		}
	}
}

/*******************************************************************************
*
* _task_mutex_lock - mutex lock kernel service
*
* This routine is the entry to the mutex lock kernel service.
*
* RETURNS: RC_OK on success, RC_FAIL on error, RC_TIME on timeout
*/

int _task_mutex_lock(
	kmutex_t mutex,   /* mutex to lock */
	int32_t time /* max # of ticks to wait for mutex */
	)
{
	struct k_args A; /* argument packet */

	A.Comm = LOCK_REQ;
	A.Time.ticks = time;
	A.Args.l1.mutex = mutex;
	A.Args.l1.task = _k_current_task->Ident;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/*******************************************************************************
*
* _k_mutex_unlock - process a mutex unlock request
*
* This routine processes a mutex unlock request (UNLOCK).  If the mutex
* was involved in priority inheritance, then it will change the priority level
* of the current owner to the priority level it had when it acquired the
* mutex.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _k_mutex_unlock(struct k_args *A /* pointer to mutex unlock
						 request arguments */
				    )
{
	struct mutex_struct *Mutex; /* pointer internal mutex structure */
	int MutexId;                /* mutex ID obtained from unlock request */
	struct k_args *PrioDowner;  /* used to change a task's priority level */

	MutexId = A->Args.l1.mutex;
	Mutex = _k_mutex_list + OBJ_INDEX(MutexId);
	if (Mutex->Owner == A->Args.l1.task && --(Mutex->Level) == 0) {
		/*
		 * The requesting task owns the mutex and all locks
		 * have been released.
		 */

		struct k_args *X;

#ifdef CONFIG_OBJECT_MONITOR
		Mutex->Count++;
#endif

		if (Mutex->OwnerCurrentPrio != Mutex->OwnerOriginalPrio) {
			/*
			 * This mutex is involved in priority inheritance.
			 * Send a request to revert the priority level of
			 * the owning task back to its priority level when
			 * it first acquired the mutex.
			 */
			GETARGS(PrioDowner);

			PrioDowner->alloc = true;
			PrioDowner->Comm = SPRIO;
			PrioDowner->Prio = Mutex->OwnerOriginalPrio;
			PrioDowner->Args.g1.task = Mutex->Owner;
			PrioDowner->Args.g1.prio = Mutex->OwnerOriginalPrio;
			SENDARGS(PrioDowner);
		}

		X = Mutex->Waiters;
		if (X != NULL) {
			/*
			 * At least one task was waiting for the mutex.
			 * Assign the new owner of the task to be the
			 * first in the queue.
			 */

			Mutex->Waiters = X->Forw;
			Mutex->Owner = X->Args.l1.task;
			Mutex->Level = 1;
			Mutex->OwnerCurrentPrio = X->Prio;
			Mutex->OwnerOriginalPrio = X->Prio;

#ifndef CONFIG_TICKLESS_KERNEL
			if (X->Time.timer) {
				/*
				 * Trigger a call to _k_mutex_lock_reply()--it will
				 * send a reply with a return code of RC_OK.
				 */
				force_timeout(X);
				X->Comm = LOCK_RPL;
			} else {
#endif
				/*
				 * There is no timer to update.
				 * Set the return code.
				 */
				X->Time.rcode = RC_OK;
					reset_state_bit(X->Ctxt.proc, TF_LOCK);
#ifndef CONFIG_TICKLESS_KERNEL
			}
#endif
		} else {
			/* No task is waiting in the queue. */
			Mutex->Owner = ANYTASK;
			Mutex->Level = 0;
		}
	}
}

/*******************************************************************************
*
* _task_mutex_unlock - mutex unlock kernel service
*
* This routine is the entry to the mutex unlock kernel service.
*
* RETURNS: N/A
*/

void _task_mutex_unlock(kmutex_t mutex /* mutex to unlock */
					 )
{
	struct k_args A; /* argument packet */

	A.Comm = UNLOCK;
	A.Args.l1.mutex = mutex;
	A.Args.l1.task = _k_current_task->Ident;
	KERNEL_ENTRY(&A);
}


