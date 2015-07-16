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

/**
 * @file
 *
 * @brief Semaphore kernel services.
 */

#include <microkernel.h>
#include <toolchain.h>
#include <sections.h>

#include <micro_private.h>

/**
 *
 * @brief Common code for signaling a semaphore
 *
 * @return N/A
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

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Comm == WAITSREQ || A->Comm == WAITSTMO)
#else
		if (A->Comm == WAITSREQ)
#endif
		{
			S->Level--;
			if (Y) {
				Y->Forw = X;
			} else {
				S->Waiters = X;
			}
#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (A->Time.timer) {
				_k_timeout_cancel(A);
				A->Comm = WAITSRPL;
			} else {
#endif
				A->Time.rcode = RC_OK;
				_k_state_bit_reset(A->Ctxt.proc, TF_SEMA);
#ifdef CONFIG_SYS_CLOCK_EXISTS
			}
#endif
		} else if (A->Comm == WAITMREQ) {
			S->Level--;
			A->Comm = WAITMRDY;
			GETARGS(Y);
			*Y = *A;
			SENDARGS(Y);
			Y = A;
		} else {
			Y = A;
		}
		A = X;
	}
}

void _k_sem_group_wait(struct k_args *R)
{
	struct k_args *A = R->Ctxt.args;

	FREEARGS(R);
	if (--(A->Args.s1.nsem) == 0) {
		_k_state_bit_reset(A->Ctxt.proc, TF_LIST);
	}
}

void _k_sem_group_wait_cancel(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args) {
			if (Y) {
				Y->Forw = X->Forw;
			} else {
				S->Waiters = X->Forw;
			}
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
					if (unlikely(waitTaskArgs->Args.s1.sema ==
						ENDLIST)) {
						waitTaskArgs->Args.s1.sema = A->Args.s1.sema;
					} else {
						signal_semaphore(1, S);
					}
				}

				_k_sem_group_wait(X);
			} else {
				FREEARGS(X); /* ERROR */
			}
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	A->Forw = X;
	if (Y) {
		Y->Forw = A;
	} else {
		S->Waiters = A;
	}
}

void _k_sem_group_wait_accept(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args) {
			if (Y) {
				Y->Forw = X->Forw;
			} else {
				S->Waiters = X->Forw;
			}
			if (X->Comm == WAITMRDY) {
				_k_sem_group_wait(X);
			} else {
				FREEARGS(X); /* ERROR */
			}
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	/* ERROR */
}

void _k_sem_group_wait_timeout(struct k_args *A)
{
	ksem_t *L;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (A->Time.timer) {
		FREETIMER(A->Time.timer);
	}
#endif

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

void _k_sem_group_ready(struct k_args *R)
{
	struct k_args *A = R->Ctxt.args;

	if (A->Args.s1.sema == ENDLIST) {
		A->Args.s1.sema = R->Args.s1.sema;
		A->Comm = WAITMTMO;
#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.timer) {
			_k_timeout_cancel(A);
		} else
#endif
			_k_sem_group_wait_timeout(A);
	}
	FREEARGS(R);
}

void _k_sem_wait_reply(struct k_args *A)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (A->Time.timer) {
		FREETIMER(A->Time.timer);
	}
	if (A->Comm == WAITSTMO) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else
#endif
		A->Time.rcode = RC_OK;
	_k_state_bit_reset(A->Ctxt.proc, TF_SEMA);
}

void _k_sem_group_wait_request(struct k_args *A)
{
	struct sem_struct *S = _k_sem_list + OBJ_INDEX(A->Args.s1.sema);
	struct k_args *X = S->Waiters;
	struct k_args *Y = NULL;

	while (X && (X->Prio <= A->Prio)) {
		if (X->Ctxt.args == A->Ctxt.args) {
			if (Y) {
				Y->Forw = X->Forw;
			} else {
				S->Waiters = X->Forw;
			}
			if (X->Comm == WAITMCAN) {
				_k_sem_group_wait(X);
			} else {
				FREEARGS(X); /* ERROR */
			}
			FREEARGS(A);
			return;
		} else {
			Y = X;
			X = X->Forw;
		}
	}
	A->Forw = X;
	if (Y) {
		Y->Forw = A;
	} else {
		S->Waiters = A;
	}
	signal_semaphore(0, S);
}

void _k_sem_group_wait_any(struct k_args *A)
{
	ksem_t *L;

	L = A->Args.s1.list;
	A->Args.s1.sema = ENDLIST;
	A->Args.s1.nsem = 0;

	if (*L == ENDLIST) {
		return;
	}

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
	_k_state_bit_set(_k_current_task, TF_LIST);

#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (A->Time.ticks != TICKS_NONE) {
		if (A->Time.ticks == TICKS_UNLIMITED) {
			A->Time.timer = NULL;
		} else {
			A->Comm = WAITMTMO;
			_k_timeout_alloc(A);
		}
	}
#endif
}

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
		_k_state_bit_set(_k_current_task, TF_SEMA);
		INSERT_ELM(S->Waiters, A);
#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.ticks == TICKS_UNLIMITED) {
			A->Time.timer = NULL;
		} else {
			A->Comm = WAITSTMO;
			_k_timeout_alloc(A);
		}
#endif
		return;
	} else {
		A->Time.rcode = RC_FAIL;
	}
}

int _task_sem_take(ksem_t sema, int32_t time)
{
	struct k_args A;

	A.Comm = WAITSREQ;
	A.Time.ticks = time;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

ksem_t _task_sem_group_take(ksemg_t group, int32_t time)
{
	struct k_args A;

	A.Comm = WAITMANY;
	A.Prio = _k_current_task->Prio;
	A.Time.ticks = time;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
	return A.Args.s1.sema;
}

void _k_sem_signal(struct k_args *A)
{
	uint32_t Sid = A->Args.s1.sema;

	signal_semaphore(1, _k_sem_list + OBJ_INDEX(Sid));
}

void _k_sem_group_signal(struct k_args *A)
{
	ksem_t *L = A->Args.s1.list;

	while ((A->Args.s1.sema = *L++) != ENDLIST) {
		_k_sem_signal(A);
	}
}

void task_sem_give(ksem_t sema)
{
	struct k_args A;

	A.Comm = SIGNALS;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
}

void task_sem_group_give(ksemg_t group)
{
	struct k_args A;

	A.Comm = SIGNALM;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
}

FUNC_ALIAS(isr_sem_give, fiber_sem_give, void);

void isr_sem_give(ksem_t sema, struct cmd_pkt_set *pSet)
{
	struct k_args *pCommand; /* ptr to command packet */

	/*
	 * The cmdPkt_t data structure was designed to work seamlessly with the
	 * struct k_args data structure and it is thus safe (and expected) to typecast
	 * the return value of _cmd_pkt_get() to "struct k_args *".
	 */

	pCommand = (struct k_args *)_cmd_pkt_get(pSet);
	pCommand->Comm = SIGNALS;
	pCommand->Args.s1.sema = sema;

	nano_isr_stack_push(&_k_command_stack, (uint32_t)pCommand);
}

void _k_sem_reset(struct k_args *A)
{
	uint32_t Sid = A->Args.s1.sema;

	_k_sem_list[OBJ_INDEX(Sid)].Level = 0;
}

void _k_sem_group_reset(struct k_args *A)
{
	ksem_t *L = A->Args.s1.list;

	while ((A->Args.s1.sema = *L++) != ENDLIST) {
		_k_sem_reset(A);
	}
}

void task_sem_reset(ksem_t sema)
{
	struct k_args A;

	A.Comm = RESETS;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
}

void task_sem_group_reset(ksemg_t group)
{
	struct k_args A;

	A.Comm = RESETM;
	A.Args.s1.list = group;
	KERNEL_ENTRY(&A);
}

void _k_sem_inquiry(struct k_args *A)
{
	struct sem_struct *S;
	uint32_t Sid;

	Sid = A->Args.s1.sema;
	S = _k_sem_list + OBJ_INDEX(Sid);
	A->Time.rcode = S->Level;
}

int task_sem_count_get(ksem_t sema)
{
	struct k_args A;

	A.Comm = INQSEMA;
	A.Args.s1.sema = sema;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}
