/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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
 * @brief Event kernel services.
*/

#include <micro_private.h>
#include "microkernel/event.h"
#include <toolchain.h>
#include <sections.h>

extern struct evstr _k_event_list[];

/**
 *
 * @brief Perform set event handler request
 *
 * @return N/A
 */
void _k_event_handler_set(struct k_args *A)
{
	kevent_t event = A->args.e1.event;
	struct evstr *E = _k_event_list + event;

	if (E->func != NULL) {
		if (likely(A->args.e1.func == NULL)) {
			/* uninstall handler */
			E->func = NULL;
			A->Time.rcode = RC_OK;
		} else {
			/* can't overwrite an existing handler */
			A->Time.rcode = RC_FAIL;
		}
	} else {
		/* install handler */
		E->func = A->args.e1.func;
		E->status = 0;
		A->Time.rcode = RC_OK;
	}
}

int task_event_handler_set(kevent_t event, kevent_handler_t handler)
{
	struct k_args A;

	A.Comm = _K_SVC_EVENT_HANDLER_SET;
	A.args.e1.event = event;
	A.args.e1.func = handler;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/**
 *
 * @brief Finish handling a test for event request that timed out
 *
 * @return N/A
 */
void _k_event_test_timeout(struct k_args *A)
{
	kevent_t event = A->args.e1.event;
	struct evstr *E = _k_event_list + event;

	FREETIMER(A->Time.timer);
	A->Time.rcode = RC_TIME;
	E->waiter = NULL;
	_k_state_bit_reset(A->Ctxt.task, TF_EVNT);
}

/**
 *
 * @brief Perform test for event request
 *
 * @return N/A
 */
void _k_event_test(struct k_args *A)
{
	kevent_t event = A->args.e1.event;
	struct evstr *E = _k_event_list + event;

	if (E->status) { /* the next event can be received */
		E->status = 0;
		A->Time.rcode = RC_OK;
	} else {
		if (likely(A->Time.ticks != TICKS_NONE)) {
			/* Caller will wait for the event */
			if (likely(E->waiter == NULL)) {
				A->Ctxt.task = _k_current_task;
				E->waiter = A;
				_k_state_bit_set(_k_current_task, TF_EVNT);
#ifdef CONFIG_SYS_CLOCK_EXISTS
				if (A->Time.ticks == TICKS_UNLIMITED) {
					A->Time.timer = NULL;
				} else {
					A->Comm = _K_SVC_EVENT_TEST_TIMEOUT;
					_k_timeout_alloc(A);
				}
#endif
			} else {
				A->Time.rcode = RC_FAIL; /* already a
								waiter
								present */
			}
		} else {
			/* Caller will not wait for the event */
			A->Time.rcode = RC_FAIL;
		}
	}
}

int _task_event_recv(kevent_t event, int32_t time)
{
	struct k_args A;

	A.Comm = _K_SVC_EVENT_TEST;
	A.args.e1.event = event;
	A.Time.ticks = time;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

/**
 *
 * @brief Signal an event
 *
 * Lowest level event signalling routine, which is invoked directly when the
 * signal is issued by a task and indirectly when the signal is issued by a
 * fiber or ISR. The specified event number must be valid.
 *
 * @return N/A
 */
void _k_do_event_signal(kevent_t event)
{
	struct evstr *E = _k_event_list + event;
	struct k_args *A = E->waiter;
	int ret_val = 1; /* If no handler is available, then ret_val is 1 by default */

	if ((E->func) != NULL) { /* handler available */
		ret_val = (E->func)(event); /* call handler */
	}

	if (ret_val != 0) {
		E->status = 1;
	}
	/* if task waiting, will be rescheduled */
	if (((A) != NULL) && (E->status != 0)) {
#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (A->Time.timer != NULL) {
			_k_timeout_free(A->Time.timer);
			A->Comm = _K_SVC_NOP;
		}
#endif
		A->Time.rcode = RC_OK;

		_k_state_bit_reset(A->Ctxt.task, TF_EVNT);
		E->waiter = NULL;
		E->status = 0;
	}

#ifdef CONFIG_OBJECT_MONITOR
	E->count++;
#endif
}

/**
 *
 * @brief Perform signal an event request
 *
 * @return N/A
 */
void _k_event_signal(struct k_args *A)
{
	kevent_t event = A->args.e1.event;
	_k_do_event_signal(event);
	A->Time.rcode = RC_OK;
}

int task_event_send(kevent_t event)
{
	struct k_args A;

	A.Comm = _K_SVC_EVENT_SIGNAL;
	A.args.e1.event = event;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}

FUNC_ALIAS(isr_event_send, fiber_event_send, void);

void isr_event_send(kevent_t event)
{
	nano_isr_stack_push(&_k_command_stack, (uint32_t)event);
}
