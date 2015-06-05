/* nanokernel dynamic-size FIFO queue object */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This module provides the nanokernel FIFO object implementation, including
the following APIs:

   nano_fifo_init
   nano_fiber_fifo_put, nano_task_fifo_put, nano_isr_fifo_put
   nano_fiber_fifo_get, nano_task_fifo_get, nano_isr_fifo_get
   nano_fiber_fifo_get_wait, nano_task_fifo_get_wait


INTERNAL
In some cases the compiler "alias" attribute is used to map two or more
APIs to the same function, since they have identical implementations.
*/

#include <nanok.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>

/*******************************************************************************
*
* nano_fifo_init - initialize a nanokernel multiple-waiter fifo (fifo) object
*
* This function initializes a nanokernel multiple-waiter fifo (fifo) object
* structure.
*
* It may be called from either a fiber or task context.
*
* RETURNS: N/A
*
* INTERNAL
* Although the existing implementation will support invocation from an ISR
* context, for future flexibility, this API will be restricted from ISR
* level invocation.
*/

void nano_fifo_init(
	struct nano_fifo *fifo /* fifo to initialize */
	)
{
	/*
	 * The wait queue and data queue occupy the same space since there cannot
	 * be both queued data and pending fibers in the FIFO. Care must be taken
	 * that, when one of the queues becomes empty, it is reset to a state
	 * that reflects an empty queue to both the data and wait queues.
	 */
	_nano_wait_q_init(&fifo->wait_q);

	/*
	 * If the 'stat' field is a positive value, it indicates how many data
	 * elements reside in the FIFO.  If the 'stat' field is a negative value,
	 * its absolute value indicates how many fibers are pending on the LIFO
	 * object.  Thus a value of '0' indicates that there are no data elements
	 * in the LIFO _and_ there are no pending fibers.
	 */

	fifo->stat = 0;
}

FUNC_ALIAS(_fifo_put_non_preemptible, nano_isr_fifo_put, void);
FUNC_ALIAS(_fifo_put_non_preemptible, nano_fiber_fifo_put, void);

/*******************************************************************************
*
* enqueue_data - internal routine to append data to a fifo
*
* RETURNS: N/A
*/

static inline void enqueue_data(struct nano_fifo *fifo, void *data)
{
	*(void **)fifo->data_q.tail = data;
	fifo->data_q.tail = data;
	*(int *)data = 0;
}

/*******************************************************************************
*
* _fifo_put_non_preemptible - append an element to a fifo (no context switch)
*
* This routine adds an element to the end of a fifo object; it may be called
* from either either a fiber or an ISR context.   A fiber pending on the fifo
* object will be made ready, but will NOT be scheduled to execute.
*
* If a fiber is waiting on the fifo, the address of the element is returned to
* the waiting fiber.  Otherwise, the element is linked to the end of the list.
*
* RETURNS: N/A
*
* INTERNAL
* This function is capable of supporting invocations from both a fiber and an
* ISR context.  However, the nano_isr_fifo_put and nano_fiber_fifo_put aliases
* are created to support any required implementation differences in the future
* without introducing a source code migration issue.
*/

void _fifo_put_non_preemptible(
	struct nano_fifo *fifo,	/* fifo on which to interact */
	void *data				/* data to send */
	)
{
	unsigned int imask;

	imask = irq_lock_inline();

	fifo->stat++;
	if (fifo->stat <= 0) {
		tCCS *ccs = _nano_wait_q_remove_no_check(&fifo->wait_q);
		fiberRtnValueSet(ccs, (unsigned int)data);
	} else {
		enqueue_data(fifo, data);
	}

	irq_unlock_inline(imask);
}

/*******************************************************************************
*
* nano_task_fifo_put - add an element to the end of a fifo
*
* This routine adds an element to the end of a fifo object; it can be called
* from only a task context.  A fiber pending on the fifo object will be made
* ready, and will preempt the running task immediately.
*
* If a fiber is waiting on the fifo, the address of the element is returned to
* the waiting fiber.  Otherwise, the element is linked to the end of the list.
*
* RETURNS: N/A
*/

void nano_task_fifo_put(
	struct nano_fifo *fifo,	/* fifo on which to interact */
	void *data				/* data to send */
	)
{
	unsigned int imask;

	imask = irq_lock_inline();

	fifo->stat++;
	if (fifo->stat <= 0) {
		tCCS *ccs = _nano_wait_q_remove_no_check(&fifo->wait_q);
		fiberRtnValueSet(ccs, (unsigned int)data);
		_Swap(imask);
		return;
	} else {
		enqueue_data(fifo, data);
	}

	irq_unlock_inline(imask);
}

/*******************************************************************************
*
* nano_fifo_put - add an element to the end of a fifo
*
* This is a convenience wrapper for the context-specific APIs. This is
* helpful whenever the exact scheduling context is not known, but should
* be avoided when the context is known up-front (to avoid unnecessary
* overhead).
*/
void nano_fifo_put(struct nano_fifo *fifo, void *data)
{
	static void (*func[3])(struct nano_fifo *fifo, void *data) = {
		nano_isr_fifo_put, nano_fiber_fifo_put, nano_task_fifo_put
	};
	func[context_type_get()](fifo, data);
}

FUNC_ALIAS(_fifo_get, nano_isr_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_fiber_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_task_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_fifo_get, void *);

/*******************************************************************************
*
* dequeue_data - internal routine to remove data from a fifo
*
* RETURNS: the data item removed
*/

static inline void *dequeue_data(struct nano_fifo *fifo)
{
	void *data = fifo->data_q.head;

	if (fifo->stat == 0) {
		/*
		 * The data_q and wait_q occupy the same space and have the same
		 * format, and there is already an API for resetting the wait_q, so
		 * use it.
		 */
		_nano_wait_q_reset(&fifo->wait_q);
	} else {
		fifo->data_q.head = *(void **)data;
	}

	return data;
}

/*******************************************************************************
*
* _fifo_get - get an element from the head a fifo
*
* Remove the head element from the specified nanokernel multiple-waiter fifo
* linked list fifo; it may be called from a fiber, task, or ISR context.
*
* If no elements are available, NULL is returned.  The first word in the
* element contains invalid data because that memory location was used to store
* a pointer to the next element in the linked list.
*
* RETURNS: Pointer to head element in the list if available, otherwise NULL
*
* INTERNAL
* This function is capable of supporting invocations from fiber, task, and ISR
* contexts.  However, the nano_isr_fifo_get, nano_task_fifo_get, and
* nano_fiber_fifo_get aliases are created to support any required
* implementation differences in the future without introducing a source code
* migration issue.
*/

void *_fifo_get(
	struct nano_fifo *fifo /* fifo on which to interact */
	)
{
	void *data = NULL;
	unsigned int imask;

	imask = irq_lock_inline();

	if (fifo->stat > 0) {
		fifo->stat--;
		data = dequeue_data(fifo);
	}
	irq_unlock_inline(imask);
	return data;
}

/*******************************************************************************
*
* nano_fiber_fifo_get_wait - get the head element of a fifo, wait if emtpy
*
* Remove the head element from the specified system-level multiple-waiter
* fifo; it can only be called from a fiber context.
*
* If no elements are available, the calling fiber will pend until an element
* is put onto the fifo.
*
* The first word in the element contains invalid data because that memory
* location was used to store a pointer to the next element in the linked list.
*
* RETURNS: Pointer to head element in the list
*
* INTERNAL There exists a separate nano_task_fifo_get_wait() implementation
* since a task context cannot pend on a nanokernel object.  Instead tasks will
* poll the fifo object.
*/

void *nano_fiber_fifo_get_wait(
	struct nano_fifo *fifo /* fifo on which to interact */
	)
{
	void *data;
	unsigned int imask;

	imask = irq_lock_inline();

	fifo->stat--;
	if (fifo->stat < 0) {
		_nano_wait_q_put(&fifo->wait_q);
		data = (void *)_Swap(imask);
	} else {
		data = dequeue_data(fifo);
		irq_unlock_inline(imask);
	}

	return data;
}

/*******************************************************************************
*
* nano_task_fifo_get_wait - get the head element of a fifo, poll if empty
*
* Remove the head element from the specified system-level multiple-waiter
* fifo; it can only be called from a task context.
*
* If no elements are available, the calling task will poll until an
* until an element is put onto the fifo.
*
* The first word in the element contains invalid data because that memory
* location was used to store a pointer to the next element in the linked list.
*
* RETURNS: Pointer to head element in the list
*/

void *nano_task_fifo_get_wait(
	struct nano_fifo *fifo /* fifo on which to interact */
	)
{
	void *data;
	unsigned int imask;

	/* spin until data is put onto the FIFO */

	while (1) {
		imask = irq_lock_inline();

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(fifo->stat > 0))
			break;

		/* see explanation in nano_stack.c:nano_task_stack_pop_wait() */

		nano_cpu_atomic_idle(imask);
	}

	fifo->stat--;
	data = dequeue_data(fifo);
	irq_unlock_inline(imask);

	return data;
}

/*******************************************************************************
*
* nano_fifo_get_wait - get the head element of a fifo, poll/pend if empty
*
* This is a convenience wrapper for the context-specific APIs. This is
* helpful whenever the exact scheduling context is not known, but should
* be avoided when the context is known up-front (to avoid unnecessary
* overhead).
*
* It's only valid to call this API from a fiber or a task.
*/
void *nano_fifo_get_wait(struct nano_fifo *fifo)
{
	static void *(*func[3])(struct nano_fifo *fifo) = {
		NULL, nano_fiber_fifo_get_wait, nano_task_fifo_get_wait
	};
	return func[context_type_get()](fifo);
}
