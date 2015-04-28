/* semaphore kernel services */

/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
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

/* includes */

#include <microkernel.h>
#include <toolchain.h>
#include <sections.h>

#include <minik.h>
#include <ksema.h>
#include <kticks.h>

/*******************************************************************************
*
* signal_semaphore - common code for signaling a semaphore
*
* RETURNS: N/A
*/

static void signal_semaphore(int n, struct sem_struct *S)
{
	struct k_args *A, *X, *Y;

#ifdef CONFIG_OBJECT_MONITOR
	S->Count += n;
#endif

	S->Level += n;
	A = S->Waiters;
	Y = NULL;
	while (A && S->Level) {
		X = A->Forw;

#ifndef LITE
		if (A->Comm == WAITSREQ || A->Comm == WAITSTMO)
#else
		if (A->Comm == WAITSREQ)
#endif
		{
			S->Level--;
			if (Y)
				Y->Forw = X;
			else
				S->Waiters = X;
#ifndef LITE
			if (A->Time.timer) {
				force_timeout(A);
				A->Comm = WAITSRPL;
			} else {
#endif
				A->Time.rcode = RC_OK;
					reset_state_bit(A->Ctxt.proc, TF_SEMA);
#ifndef LITE
			}
#endif
		}
#ifndef LITE
		else if (A->Comm == WAITMREQ) {
			S->Level--;
			A->Comm = WAITMRDY;
			GETARGS(Y);
			*Y = *A;
			SENDARGS(Y);
			Y = A;
		}
#endif
		else
			Y = A;
		A = X;
	}
}

/*******************************************************************************
*
* _k_sem_group_wait - finish handling incomplete waits on semaphores
*
* RETURNS: N/A
*/

void _k_sem_group_wait(struct k_args *R)
{
	struct k_args *A = R->Ctxt.args;

	FREEARGS(R);
	if (--(A->Args.s1.nsem) == 0)
		reset_state_bit(A->Ctxt.proc, TF_LIST);
}

/*******************************************************************************
*
* _k_sem_group_wait_cancel - handle cancellation of a semaphore involved in a
*              semaphore group wait request
*
* This routine only applies to semaphore group wait requests.  It is invoked
* for each semaphore in the semaphore group that "lost" the semaphore group
* wait request.
*
* RETURNS: N/A
*/

void _k_sem_group_wait_cancel(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args)
		{
			if (Y)
				Y->Forw = X->Forw;
			else
				S->Waiters = X->Forw;
			if (X->Comm == WAITMREQ || X->Comm == WAITMRDY) {
				if (X->Comm == WAITMRDY) {
					/* obtain struct k_args of waiting task */

					struct k_args *waitTaskArgs = X->Ctxt.args;

/*
 * Determine if the wait cancellation request is being
 * processed after the state of the 'Waiters' packet state
 * has been updated to WAITMRDY, but before the WAITMRDY
 * packet has been processed. This will occur if a WAITMTMO
 * timer expiry occurs between the update of the packet state
 * and the processing of the WAITMRDY packet.
 */
					if (unlikely(waitTaskArgs->Args.s1
								  .sema ==
							  ENDLIST))
						waitTaskArgs->Args.s1.sema =
							A->Args.s1.sema;
					else
						signal_semaphore(1, S);
				}

				_k_sem_group_wait(X);
			} else
				FREEARGS(X); /* ERROR */
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	A->Forw = X;
	if (Y)
		Y->Forw = A;
	else
		S->Waiters = A;
}

/*******************************************************************************
*
* _k_sem_group_wait_accept - handle acceptance of the ready semaphore request
*
* This routine only applies to semaphore group wait requests.  It handles
* the request for the one semaphore in the group that "wins" the semaphore
* group wait request.
*
* RETURNS: N/A
*/

void _k_sem_group_wait_accept(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args)
		{
			if (Y)
				Y->Forw = X->Forw;
			else
				S->Waiters = X->Forw;
			if (X->Comm == WAITMRDY) {
					_k_sem_group_wait(X);
			} else
				FREEARGS(X); /* ERROR */
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	/* ERROR */
}

/*******************************************************************************
*
* _k_sem_group_wait_timeout - handle semaphore group timeout request
*
* RETURNS: N/A
*/

void _k_sem_group_wait_timeout(struct k_args *A)
{
	ksem_t *L;

	if (A->Time.timer)
		FREETIMER(A->Time.timer);

	L = A->Args.s1.list;
	while (*L != ENDLIST) {
		struct k_args *R;

		GETARGS(R);
		R->Prio = A->Prio;
		R->Comm =
			(K_COMM)((*L == A->Args.s1.sema) ? WAITMACC : WAITMCAN);
		R->Ctxt.args = A;
		R->Args.s1.sema = *L++;
		SENDARGS(R);
	}
}

/*******************************************************************************
*
* _k_sem_group_ready - handle semaphore ready request
*
* This routine only applies to semaphore group wait requests.  It identifies
* the one semaphore in the group that "won" the semaphore group wait request
* before triggering the semaphore group timeout handler.
*
* RETURNS: N/A
*/

void _k_sem_group_ready(struct k_args *R)
{
	struct k_args *A = R->Ctxt.args;

	if (A->Args.s1.sema == ENDLIST) {
		A->Args.s1.sema = R->Args.s1.sema;
		A->Comm = WAITMTMO;
		if (A->Time.timer)
			force_timeout(A);
		else
			_k_sem_group_wait_timeout(A);
	}
	FREEARGS(R);
}

/*******************************************************************************
*
* _k_sem_wait_reply - reply to a semaphore wait request
*
* RETURNS: N/A
*/

void _k_sem_wait_reply(struct k_args *A)
{
	if (A->Time.timer)
		FREETIMER(A->Time.timer);
	if (A->Comm == WAITSTMO) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else
		A->Time.rcode = RC_OK;
		reset_state_bit(A->Ctxt.proc, TF_SEMA);
}

/*******************************************************************************
*
* _k_sem_group_wait_request - handle internal wait request on a semaphore involved in a
*              semaphore group wait request
*
* RETURNS: N/A
*/

void _k_sem_group_wait_request(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args)
		{
			if (Y)
				Y->Forw = X->Forw;
			else
				S->Waiters = X->Forw;
			if (X->Comm == WAITMCAN) {
				_k_sem_group_wait(X);
			} else
				FREEARGS(X); /* ERROR */
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	A->Forw = X;
	if (Y)
		Y->Forw = A;
	else
		S->Waiters = A;
	signal_semaphore(0, S);
}

/*******************************************************************************
*
* _k_sem_group_wait_any - handle semaphore group wait request
*
* This routine splits the single semaphore group wait request into several
* internal wait requests--one for each semaphore in the group.
*
* RETURNS: N/A
*/

void _k_sem_group_wait_any(struct k_args *A)
{
	ksem_t *L;

	L = A->Args.s1.list;
	A->Args.s1.sema = ENDLIST;
	A->Args.s1.nsem = 0;

	if (*L == ENDLIST)
		return;

	while (*L != ENDLIST) {
		struct k_args *R;

		GETARGS(R);
		R->Prio = _k_current_task->Prio;
		R->Comm = WAITMREQ;
		R->Ctxt.args = A;
		R->Args.s1.sema = *L++;
		SENDARGS(R);
		(A->Args.s1.nsem)++;
	}

	A->Ctxt.proc = _k_current_task;
	set_state_bit(_k_current_task, TF_LIST);

	if (A->Time.ticks != TICKS_NONE) {
		if (A->Time.ticks == TICKS_UNLIMITED)
			A->Time.timer = NULL;
		else {
			A->Comm = WAITMTMO;
			enlist_timeout(A);
		}
	}
}

/*******************************************************************************
*
* _k_sem_wait_request - handle semaphore test and wait request
*
* RETURNS: N/A
*/

void _k_sem_wait_request(struct k_args *A)
{
	struct sem_struct *S;
	uint32_t Sid;

	Sid = A->Args.s1.sema;
	S = _k_sem_list + OBJ_INDEX(Sid);

	if (S->Level) {
		S->Level--;
		A->Time.rcode = RC_OK;
	} else if (A->Time.ticks != TICKS_NONE) {
		A->Ctxt.proc = _k_current_task;
		A->Prio = _k_current_task->Prio;
		set_state_bit(_k_current_task, TF_SEMA);
		INSERT_ELM(S->Waiters, A);
#ifndef LITE
		if (A->Time.ticks == TICKS_UNLIMITED)
			A->Time.timer = NULL;
		else {
			A->Comm = WAITSTMO;
			enlist_timeout(A);
		}
#endif
		return;
	} else
		A->Time.rcode = RC_FAIL;
}

/*******************************************************************************
*
* _task_sem_take - test a semaphore
*
* This routine tests a semaphore to see if it has been signaled.  If the signal
* count is greater than zero, it is decremented.
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_sem_take(ksem_t sema, /* semaphore to test */
		 int32_t time /* maximum number of ticks to wait */
		 )
{
	struct k_args A;

	A.Comm = WAITSREQ;
	A.Time.ticks = time;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/*******************************************************************************
*
* _task_sem_group_take - test multiple semaphores
*
* This routine tests a group of semaphores. A semaphore group is an array of
* semaphore names terminated by the predefined constant ENDLIST.
*
* It returns the ID of the first semaphore in the group whose signal count is
* greater than zero, and decrements the signal count.
*
* RETURNS: N/A
*/

ksem_t _task_sem_group_take(ksemg_t group, /* group of semaphores to test */
			 int32_t time   /* maximum number of ticks to wait */
			 )
{
	struct k_args A;

	A.Comm = WAITMANY;
	A.Prio = _k_current_task->Prio;
	A.Time.ticks = time;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
	return A.Args.s1.sema;
}

/*******************************************************************************
*
* _k_sem_signal - handle semaphore signal request
*
* RETURNS: N/A
*/

void _k_sem_signal(struct k_args *A)
{
	uint32_t Sid = A->Args.s1.sema;

	signal_semaphore(1, _k_sem_list + OBJ_INDEX(Sid));
}

/*******************************************************************************
*
* _k_sem_group_signal - handle signal semaphore group request
*
* RETURNS: N/A
*/

void _k_sem_group_signal(struct k_args *A)
{
	ksem_t *L = A->Args.s1.list;

	while ((A->Args.s1.sema = *L++) != ENDLIST)
		_k_sem_signal(A);
}

/*******************************************************************************
*
* task_sem_give - signal a semaphore
*
* This routine signals the specified semaphore.
*
* RETURNS: N/A
*/

void task_sem_give(ksem_t sema /* semaphore to signal */
		   )
{
	struct k_args A;

	A.Comm = SIGNALS;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_sem_group_give - signal a group of semaphores
*
* This routine signals a group of semaphores. A semaphore group is an array of
* semaphore names terminated by the predefined constant ENDLIST.
*
* If the semaphore list of waiting tasks is empty, the signal count is
* incremented, otherwise the highest priority waiting task is released.
*
* Using task_sem_group_give() is faster than using multiple single signals,
* and ensures all signals take place before other tasks run.
*
* RETURNS: N/A
*/

void task_sem_group_give(ksemg_t group /* group of semaphores to signal */
			)
{
	struct k_args A;

	A.Comm = SIGNALM;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* fiber_sem_give - signal a semaphore from a fiber
*
* This routine (to only be called from a fiber) signals a semaphore.  It
* requires a statically allocated command packet (from a command packet set)
* that is implicitly released once the command packet has been processed.
* To signal a semaphore from a task, task_sem_give() should be used instead.
*
* RETURNS: N/A
*/

FUNC_ALIAS(isr_sem_give, fiber_sem_give, void);

/*******************************************************************************
*
* isr_sem_give - signal a semaphore from an ISR
*
* This routine (to only be called from an ISR) signals a semaphore.  It
* requires a statically allocated command packet (from a command packet set)
* that is implicitly released once the command packet has been processed.
* To signal a semaphore from a task, task_sem_give() should be used instead.
*
* RETURNS: N/A
*/

void isr_sem_give(ksem_t sema, /* semaphore to signal */
					  struct cmd_pkt_set *pSet /* ptr to command
							       packet set */
					  )
{
	struct k_args *pCommand; /* ptr to command packet */

	/*
	 * The cmdPkt_t data structure was designed to work seamlessly with the
	 * struct k_args data structure and it is thus safe (and expected) to typecast
	 * the return value of cmdPktGet() to "struct k_args *".
	 */

	pCommand = (struct k_args *)cmdPktGet(pSet);
	pCommand->Comm = SIGNALS;
	pCommand->Args.s1.sema = sema;

	nano_isr_stack_push(&_k_command_stack, (uint32_t)pCommand);
}

/*******************************************************************************
*
* _k_sem_reset - handle semaphore reset request
*
* RETURNS: N/A
*/

void _k_sem_reset(struct k_args *A)
{
	uint32_t Sid = A->Args.s1.sema;

	_k_sem_list[OBJ_INDEX(Sid)].Level = 0;
}

/*******************************************************************************
*
* _k_sem_group_reset - handle semaphore group reset request
*
* RETURNS: N/A
*/

void _k_sem_group_reset(struct k_args *A)
{
	ksem_t *L = A->Args.s1.list;

	while ((A->Args.s1.sema = *L++) != ENDLIST)
		_k_sem_reset(A);
}

/*******************************************************************************
*
* task_sem_reset - reset semaphore count to zero
*
* This routine resets the signal count of the specified semaphore to zero.
*
* RETURNS: N/A
*/

void task_sem_reset(ksem_t sema /* semaphore to reset */
		  )
{
	struct k_args A;

	A.Comm = RESETS;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_sem_group_reset - reset a group of semaphores
*
* This routine resets the signal count for a group of semaphores. A semaphore
* group is an array of semaphore names terminated by the predefined constant
* ENDLIST.
*
* RETURNS: N/A
*/

void task_sem_group_reset(ksemg_t group /* group of semaphores to reset */
		       )
{
	struct k_args A;

	A.Comm = RESETM;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* K_inqsema - handle semaphore inquiry request
*
* RETURNS: N/A
*/

void K_inqsema(struct k_args *A)
{
	struct sem_struct *S;
	uint32_t Sid;

	Sid = A->Args.s1.sema;
	S = _k_sem_list + OBJ_INDEX(Sid);
	A->Time.rcode = S->Level;
}

/*******************************************************************************
*
* task_sem_count_get - read the semaphore signal count
*
* This routine reads the signal count of the specified semaphore.
*
* RETURNS: signal count
*/

int task_sem_count_get(ksem_t sema /* semaphore to query */
		  )
{
	struct k_args A;

	A.Comm = INQSEMA;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}
