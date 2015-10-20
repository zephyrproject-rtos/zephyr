/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief FIFO kernel services
 *
 * This file contains all the services needed for the implementation of a FIFO
 * for the microkernel.
 *
 *
*/


#include <micro_private.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

/**
 *
 * @brief Finish performing an incomplete FIFO enqueue request
 *
 * @return N/A
 */
void _k_fifo_enque_reply(struct k_args *A)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (A->Time.timer)
		FREETIMER(A->Time.timer);
	if (unlikely(A->Comm == _K_SVC_FIFO_ENQUE_REPLY_TIMEOUT)) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else {
		A->Time.rcode = RC_OK;
	}
#else
	A->Time.rcode = RC_OK;
#endif

	_k_state_bit_reset(A->Ctxt.task, TF_ENQU);
}

/**
 *
 * @brief Finish performing an incomplete FIFO enqueue request with timeout.
 *
 * @param A Pointer to a k_args structure
 *
 * @return N/A
 *
 * @sa _k_fifo_enque_reply
 */
void _k_fifo_enque_reply_timeout(struct k_args *A)
{
	_k_fifo_enque_reply(A);
}

/**
 *
 * @brief Perform a FIFO enqueue request
 *
 * @return N/A
 */
void _k_fifo_enque_request(struct k_args *A)
{
	struct k_args *W;
	struct _k_fifo_struct *Q;
	int Qid, n, w;
	char *p, *q; /* Ski char->uint32_t ??? */

	Qid = A->args.q1.queue;
	Q = (struct _k_fifo_struct *)Qid;
	w = OCTET_TO_SIZEOFUNIT(Q->element_size);
	q = A->args.q1.data;
	n = Q->num_used;
	if (n < Q->Nelms) {
		W = Q->waiters;
		if (W) {
			Q->waiters = W->next;
			p = W->args.q1.data;
			memcpy(p, q, w);

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (W->Time.timer) {
				_k_timeout_cancel(W);
				W->Comm = _K_SVC_FIFO_DEQUE_REPLY;
			} else {
#endif
				W->Time.rcode = RC_OK;
					_k_state_bit_reset(W->Ctxt.task, TF_DEQU);
			}
#ifdef CONFIG_SYS_CLOCK_EXISTS
		}
#endif
		else {
			p = Q->enqueue_point;
			memcpy(p, q, w);
			p = (char *)((int)p + w);
			if (p == Q->end_point)
				Q->enqueue_point = Q->base;
			else
				Q->enqueue_point = p;
			Q->num_used = ++n;
#ifdef CONFIG_OBJECT_MONITOR
			if (Q->high_watermark < n)
				Q->high_watermark = n;
#endif
		}

		A->Time.rcode = RC_OK;
#ifdef CONFIG_OBJECT_MONITOR
		Q->count++;
#endif
	} else {
		if (likely(A->Time.ticks != TICKS_NONE)) {
				A->Ctxt.task = _k_current_task;
				A->priority = _k_current_task->priority;
				_k_state_bit_set(_k_current_task, TF_ENQU);
			INSERT_ELM(Q->waiters, A);
#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (A->Time.ticks == TICKS_UNLIMITED)
				A->Time.timer = NULL;
			else {
				A->Comm = _K_SVC_FIFO_ENQUE_REPLY_TIMEOUT;
				_k_timeout_alloc(A);
			}
#endif
		} else {
			A->Time.rcode = RC_FAIL;
		}
	}
}

/**
 * @brief adds data to the fifo queue
 *
 * @param queue is the FIFO queue to add data to
 * @param ptr is a pointer to the data to add
 * @param time is the maximum number of ticks to wait
 */
int _task_fifo_put(kfifo_t queue, void *data, int32_t time)
{
	struct k_args A;

	A.Comm = _K_SVC_FIFO_ENQUE_REQUEST;
	A.Time.ticks = time;
	A.args.q1.data = (char *)data;
	A.args.q1.queue = queue;

	KERNEL_ENTRY(&A);

	return A.Time.rcode;
}

/**
 *
 * @brief Finish performing an incomplete FIFO dequeue request
 *
 * @return N/A
 */
void _k_fifo_deque_reply(struct k_args *A)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (A->Time.timer)
		FREETIMER(A->Time.timer);
	if (unlikely(A->Comm == _K_SVC_FIFO_DEQUE_REPLY_TIMEOUT)) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else {
		A->Time.rcode = RC_OK;
	}
#else
	A->Time.rcode = RC_OK;
#endif

	_k_state_bit_reset(A->Ctxt.task, TF_DEQU);
}

/**
 *
 * @brief Finish performing an incomplete FIFO dequeue request with timeout.
 *
 * @param A Pointer to a k_args structure.
 *
 * @return N/A
 *
 * @sa _k_fifo_deque_reply
 */
void _k_fifo_deque_reply_timeout(struct k_args *A)
{
	_k_fifo_deque_reply(A);
}

/**
 *
 * @brief Perform FIFO dequeue request
 *
 * @return N/A
 */
void _k_fifo_deque_request(struct k_args *A)
{
	struct k_args *W;
	struct _k_fifo_struct *Q;
	int Qid, n, w;
	char *p, *q; /* idem */

	Qid = A->args.q1.queue;
	Q = (struct _k_fifo_struct *)Qid;
	w = OCTET_TO_SIZEOFUNIT(Q->element_size);
	p = A->args.q1.data;
	n = Q->num_used;
	if (n) {
		q = Q->dequeue_point;
		memcpy(p, q, w);
		q = (char *)((int)q + w);
		if (q == Q->end_point)
			Q->dequeue_point = Q->base;
		else
			Q->dequeue_point = q;

		A->Time.rcode = RC_OK;
		W = Q->waiters;
		if (W) {
			Q->waiters = W->next;
			p = Q->enqueue_point;
			q = W->args.q1.data;
			w = OCTET_TO_SIZEOFUNIT(Q->element_size);
			memcpy(p, q, w);
			p = (char *)((int)p + w);
			if (p == Q->end_point)
				Q->enqueue_point = Q->base;
			else
				Q->enqueue_point = p;

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (W->Time.timer) {
				_k_timeout_cancel(W);
				W->Comm = _K_SVC_FIFO_ENQUE_REPLY;
			} else {
#endif
				W->Time.rcode = RC_OK;
				_k_state_bit_reset(W->Ctxt.task, TF_ENQU);
#ifdef CONFIG_SYS_CLOCK_EXISTS
			}
#endif
#ifdef CONFIG_OBJECT_MONITOR
			Q->count++;
#endif
		} else
			Q->num_used = --n;
	} else {
		if (likely(A->Time.ticks != TICKS_NONE)) {
			A->Ctxt.task = _k_current_task;
			A->priority = _k_current_task->priority;
			_k_state_bit_set(_k_current_task, TF_DEQU);

			INSERT_ELM(Q->waiters, A);
#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (A->Time.ticks == TICKS_UNLIMITED)
				A->Time.timer = NULL;
			else {
				A->Comm = _K_SVC_FIFO_DEQUE_REPLY_TIMEOUT;
				_k_timeout_alloc(A);
			}
#endif
		} else {
			A->Time.rcode = RC_FAIL;
		}
	}
}

/**
 *
 * @brief FIFO dequeue request
 *
 * This routine tries to read a data element from the FIFO.
 *
 * If the FIFO is not empty, the oldest entry is removed and copied to the
 * address provided by the caller.
 * @param queue FIFO queue
 * @param data Where to store FIFO entry
 * @param time Maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
int _task_fifo_get(kfifo_t queue, void *data, int32_t time)
{
	struct k_args A;

	A.Comm = _K_SVC_FIFO_DEQUE_REQUEST;
	A.Time.ticks = time;
	A.args.q1.data = (char *)data;
	A.args.q1.queue = queue;

	KERNEL_ENTRY(&A);

	return A.Time.rcode;
}

/**
 *
 * @brief Perform miscellaneous FIFO request
 * @param A Kernel Argument
 *
 * @return N/A
 */
void _k_fifo_ioctl(struct k_args *A)
{
	struct _k_fifo_struct *Q;
	int Qid;

	Qid = A->args.q1.queue;
	Q = (struct _k_fifo_struct *)Qid;
	if (A->args.q1.size) {
		if (Q->num_used) {
			struct k_args *X;

			while ((X = Q->waiters)) {
				Q->waiters = X->next;
#ifdef CONFIG_SYS_CLOCK_EXISTS
				if (likely(X->Time.timer)) {
					_k_timeout_cancel(X);
					X->Comm = _K_SVC_FIFO_ENQUE_REPLY;
				} else {
#endif
					X->Time.rcode = RC_FAIL;
					_k_state_bit_reset(X->Ctxt.task, TF_ENQU);
#ifdef CONFIG_SYS_CLOCK_EXISTS
				}
#endif
			}
		}
		Q->num_used = 0;
		Q->enqueue_point = Q->dequeue_point = Q->base;
		A->Time.rcode = RC_OK;
	} else
		A->Time.rcode = Q->num_used;
}

/**
 *
 * @brief Miscellaneous FIFO request
 *
 * Depending upon the chosen operation, this routine will ...
 *   1. <op> = 0 : query the number of FIFO entries
 *   2. <op> = 1 : purge the FIFO of its entries
 *
 * @param queue FIFO queue
 * @param op 0 for status query and 1 for purge
 * @return # of FIFO entries on query; RC_OK on purge
 */
int _task_fifo_ioctl(kfifo_t queue, int op)
{
	struct k_args A;

	A.Comm = _K_SVC_FIFO_IOCTL;
	A.args.q1.queue = queue;
	A.args.q1.size = op;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}
