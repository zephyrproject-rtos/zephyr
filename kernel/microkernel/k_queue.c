/* FIFO kernel services */

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

#include <microkernel/k_struct.h>
#include <minik.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

/*******************************************************************************
*
* K_enqrpl - finish performing an incomplete FIFO enqueue request
*
* RETURNS: N/A
*/

void K_enqrpl(struct k_args *A)
{
	if (A->Time.timer)
		FREETIMER(A->Time.timer);
	if (unlikely(A->Comm == ENQ_TMO)) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else
		A->Time.rcode = RC_OK;
		reset_state_bit(A->Ctxt.proc, TF_ENQU);
}

/*******************************************************************************
*
* K_enqreq - perform a FIFO enqueue request
*
* RETURNS: N/A
*/

void K_enqreq(struct k_args *A)
{
	struct k_args *W;
	struct que_struct *Q;
	int Qid, n, w;
	char *p, *q; /* Ski char->uint32_t ??? */

	Qid = A->Args.q1.queue;
	Q = _k_fifo_list + OBJ_INDEX(Qid);
	w = OCTET_TO_SIZEOFUNIT(Q->Esize);
	q = A->Args.q1.data;
	n = Q->Nused;
	if (n < Q->Nelms) {
		W = Q->Waiters;
		if (W) {
			Q->Waiters = W->Forw;
			p = W->Args.q1.data;
			k_memcpy(p, q, w);

#ifndef LITE
			if (W->Time.timer) {
				force_timeout(W);
				W->Comm = DEQ_RPL;
			} else {
#endif
				W->Time.rcode = RC_OK;
					reset_state_bit(W->Ctxt.proc, TF_DEQU);
			}
#ifndef LITE
		}
#endif
		else {
			p = Q->Enqp;
			k_memcpy(p, q, w);
			p = (char *)((int)p + w);
			if (p == Q->Endp)
				Q->Enqp = Q->Base;
			else
				Q->Enqp = p;
			Q->Nused = ++n;
#ifdef CONFIG_OBJECT_MONITOR
			if (Q->Hmark < n)
				Q->Hmark = n;
#endif
		}

		A->Time.rcode = RC_OK;
#ifdef CONFIG_OBJECT_MONITOR
		Q->Count++;
#endif
	} else {
		if (likely(A->Time.ticks != TICKS_NONE)) {
				A->Ctxt.proc = K_Task;
				A->Prio = K_Task->Prio;
				set_state_bit(K_Task, TF_ENQU);
			INSERT_ELM(Q->Waiters, A);
#ifndef LITE
			if (A->Time.ticks == TICKS_UNLIMITED)
				A->Time.timer = NULL;
			else {
				A->Comm = ENQ_TMO;
				enlist_timeout(A);
			}
#endif
		} else {
			A->Time.rcode = RC_FAIL;
		}
	}
}
/*******************************************************************************
*
* _task_fifo_put - FIFO enqueue request
*
* This routine puts an entry at the end of the FIFO queue.
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_fifo_put(kfifo_t queue, /* FIFO queue */
		void *data,   /* ptr to data to add to queue */
		int32_t time  /* maximum number of ticks to wait */
		)
{
	struct k_args A;

	A.Comm = ENQ_REQ;
	A.Time.ticks = time;
	A.Args.q1.data = (char *)data;
	A.Args.q1.queue = queue;

	KERNEL_ENTRY(&A);

	return A.Time.rcode;
}

/*******************************************************************************
*
* K_deqrpl - finish performing an incomplete FIFO dequeue request
*
* RETURNS: N/A
*/

void K_deqrpl(struct k_args *A)
{
	if (A->Time.timer)
		FREETIMER(A->Time.timer);
	if (unlikely(A->Comm == DEQ_TMO)) {
		REMOVE_ELM(A);
		A->Time.rcode = RC_TIME;
	} else {
		A->Time.rcode = RC_OK;
	}

	reset_state_bit(A->Ctxt.proc, TF_DEQU);
}

/*******************************************************************************
*
* K_deqreq - perform FIFO dequeue request
*
* RETURNS: N/A
*/

void K_deqreq(struct k_args *A)
{
	struct k_args *W;
	struct que_struct *Q;
	int Qid, n, w;
	char *p, *q; /* idem */

	Qid = A->Args.q1.queue;
	Q = _k_fifo_list + OBJ_INDEX(Qid);
	w = OCTET_TO_SIZEOFUNIT(Q->Esize);
	p = A->Args.q1.data;
	n = Q->Nused;
	if (n) {
		q = Q->Deqp;
		k_memcpy(p, q, w);
		q = (char *)((int)q + w);
		if (q == Q->Endp)
			Q->Deqp = Q->Base;
		else
			Q->Deqp = q;

		A->Time.rcode = RC_OK;
		W = Q->Waiters;
		if (W) {
			Q->Waiters = W->Forw;
			p = Q->Enqp;
			q = W->Args.q1.data;
			w = OCTET_TO_SIZEOFUNIT(Q->Esize);
			k_memcpy(p, q, w);
			p = (char *)((int)p + w);
			if (p == Q->Endp)
				Q->Enqp = Q->Base;
			else
				Q->Enqp = p;

#ifndef LITE
			if (W->Time.timer) {
				force_timeout(W);
				W->Comm = ENQ_RPL;
			} else {
#endif
				W->Time.rcode = RC_OK;
				reset_state_bit(W->Ctxt.proc, TF_ENQU);
#ifndef LITE
			}
#endif
#ifdef CONFIG_OBJECT_MONITOR
			Q->Count++;
#endif
		} else
			Q->Nused = --n;
	} else {
		if (likely(A->Time.ticks != TICKS_NONE)) {
			A->Ctxt.proc = K_Task;
			A->Prio = K_Task->Prio;
			set_state_bit(K_Task, TF_DEQU);

			INSERT_ELM(Q->Waiters, A);
#ifndef LITE
			if (A->Time.ticks == TICKS_UNLIMITED)
				A->Time.timer = NULL;
			else {
				A->Comm = DEQ_TMO;
				enlist_timeout(A);
			}
#endif
		} else {
			A->Time.rcode = RC_FAIL;
		}
	}
}

/*******************************************************************************
*
* _task_fifo_get - FIFO dequeue request
*
* This routine tries to read a data element from the FIFO.
*
* If the FIFO is not empty, the oldest entry is removed and copied to the
* address provided by the caller.
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_fifo_get(kfifo_t queue, /* FIFO queue */
		void *data,   /* where to store FIFO entry */
		int32_t time  /* maximum number of ticks to wait */
		)
{
	struct k_args A;

	A.Comm = DEQ_REQ;
	A.Time.ticks = time;
	A.Args.q1.data = (char *)data;
	A.Args.q1.queue = queue;

	KERNEL_ENTRY(&A);

	return A.Time.rcode;
}

/*******************************************************************************
*
* K_queue - perform miscellaneous FIFO request
*
* RETURNS: N/A
*/

void K_queue(struct k_args *A)
{
	struct que_struct *Q;
	int Qid;

	Qid = A->Args.q1.queue;
	Q = _k_fifo_list + OBJ_INDEX(Qid);
	if (A->Args.q1.size) {
		if (Q->Nused) {
			struct k_args *X;

			while ((X = Q->Waiters)) {
				Q->Waiters = X->Forw;
#ifndef LITE
				if (likely(X->Time.timer)) {
					force_timeout(X);
					X->Comm = ENQ_RPL;
				} else {
#endif
					X->Time.rcode = RC_FAIL;
					reset_state_bit(X->Ctxt.proc, TF_ENQU);
#ifndef LITE
				}
#endif
			}
		}
		Q->Nused = 0;
		Q->Enqp = Q->Deqp = Q->Base;
		A->Time.rcode = RC_OK;
	} else
		A->Time.rcode = Q->Nused;
}

/*******************************************************************************
*
* _task_fifo_ioctl - miscellaneous FIFO request
*
* Depending upon the chosen operation, this routine will ...
*   1. <op> = 0 : query the number of FIFO entries
*   2. <op> = 1 : purge the FIFO of its entries
*
* RETURNS: # of FIFO entries on query; RC_OK on purge
*/

int _task_fifo_ioctl(kfifo_t queue, /* FIFO queue */
	     int op	/* 0: status query; 1: purge */
	     )
{
	struct k_args A;

	A.Comm = QUEUE;
	A.Args.q1.queue = queue;
	A.Args.q1.size = op;
	KERNEL_ENTRY(&A);
	return A.Time.rcode;
}
