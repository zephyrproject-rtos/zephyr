/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
 *
 * @brief Nanokernel dynamic-size FIFO queue object.
 *
 * This module provides the nanokernel FIFO object implementation, including
 * the following APIs:
 *
 * nano_fifo_init
 * nano_fiber_fifo_put, nano_task_fifo_put, nano_isr_fifo_put
 * nano_fiber_fifo_get, nano_task_fifo_get, nano_isr_fifo_get
 * nano_fiber_fifo_get_wait, nano_task_fifo_get_wait
 */

/*
 * INTERNAL
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 */

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>

/*
 * INTERNAL
 * Although the existing implementation will support invocation from an ISR
 * context, for future flexibility, this API will be restricted from ISR
 * level invocation.
 */
void nano_fifo_init(struct nano_fifo *fifo)
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

	DEBUG_TRACING_OBJ_INIT(struct nano_fifo *, fifo, _track_list_nano_fifo);
}

FUNC_ALIAS(_fifo_put_non_preemptible, nano_isr_fifo_put, void);
FUNC_ALIAS(_fifo_put_non_preemptible, nano_fiber_fifo_put, void);

/**
 *
 * @brief Internal routine to append data to a fifo
 *
 * @return N/A
 */
static inline void enqueue_data(struct nano_fifo *fifo, void *data)
{
	*(void **)fifo->data_q.tail = data;
	fifo->data_q.tail = data;
	*(int *)data = 0;
}

/**
 *
 * @brief Append an element to a fifo (no context switch)
 *
 * This routine adds an element to the end of a fifo object; it may be called
 * from either either a fiber or an ISR context.   A fiber pending on the fifo
 * object will be made ready, but will NOT be scheduled to execute.
 *
 * If a fiber is waiting on the fifo, the address of the element is returned to
 * the waiting fiber.  Otherwise, the element is linked to the end of the list.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 *
 * INTERNAL
 * This function is capable of supporting invocations from both a fiber and an
 * ISR context.  However, the nano_isr_fifo_put and nano_fiber_fifo_put aliases
 * are created to support any required implementation differences in the future
 * without introducing a source code migration issue.
 */
void _fifo_put_non_preemptible(struct nano_fifo *fifo, void *data)
{
	unsigned int imask;

	imask = irq_lock();

	fifo->stat++;
	if (fifo->stat <= 0) {
		struct tcs *tcs = _nano_wait_q_remove_no_check(&fifo->wait_q);
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int)data);
	} else {
		enqueue_data(fifo, data);
	}

	irq_unlock(imask);
}

void nano_task_fifo_put(struct nano_fifo *fifo, void *data)
{
	unsigned int imask;

	imask = irq_lock();

	fifo->stat++;
	if (fifo->stat <= 0) {
		struct tcs *tcs = _nano_wait_q_remove_no_check(&fifo->wait_q);
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int)data);
		_Swap(imask);
		return;
	} else {
		enqueue_data(fifo, data);
	}

	irq_unlock(imask);
}


void nano_fifo_put(struct nano_fifo *fifo, void *data)
{
	static void (*func[3])(struct nano_fifo *fifo, void *data) = {
		nano_isr_fifo_put, nano_fiber_fifo_put, nano_task_fifo_put
	};
	func[sys_execution_context_type_get()](fifo, data);
}

FUNC_ALIAS(_fifo_get, nano_isr_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_fiber_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_task_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_fifo_get, void *);

/**
 *
 * @brief Internal routine to remove data from a fifo
 *
 * @return The data item removed
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

/**
 * INTERNAL
 * This function is capable of supporting invocations from fiber, task, and ISR
 * execution contexts.  However, the nano_isr_fifo_get, nano_task_fifo_get, and
 * nano_fiber_fifo_get aliases are created to support any required
 * implementation differences in the future without introducing a source code
 * migration issue.
 */
void *_fifo_get(struct nano_fifo *fifo)
{
	void *data = NULL;
	unsigned int imask;

	imask = irq_lock();

	if (fifo->stat > 0) {
		fifo->stat--;
		data = dequeue_data(fifo);
	}
	irq_unlock(imask);
	return data;
}

void *nano_fiber_fifo_get_wait(struct nano_fifo *fifo)
{
	void *data;
	unsigned int imask;

	imask = irq_lock();

	fifo->stat--;
	if (fifo->stat < 0) {
		_nano_wait_q_put(&fifo->wait_q);
		data = (void *)_Swap(imask);
	} else {
		data = dequeue_data(fifo);
		irq_unlock(imask);
	}

	return data;
}

void *nano_task_fifo_get_wait(struct nano_fifo *fifo)
{
	void *data;
	unsigned int imask;

	/* spin until data is put onto the FIFO */

	while (1) {
		imask = irq_lock();

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
	irq_unlock(imask);

	return data;
}

void *nano_fifo_get_wait(struct nano_fifo *fifo)
{
	static void *(*func[3])(struct nano_fifo *fifo) = {
		NULL, nano_fiber_fifo_get_wait, nano_task_fifo_get_wait
	};
	return func[sys_execution_context_type_get()](fifo);
}


#ifdef CONFIG_NANO_TIMEOUTS

void *nano_fiber_fifo_get_wait_timeout(struct nano_fifo *fifo,
		int32_t timeout_in_ticks)
{
	unsigned int key;
	void *data;

	if (unlikely(TICKS_UNLIMITED == timeout_in_ticks)) {
		return nano_fiber_fifo_get_wait(fifo);
	}

	if (unlikely(TICKS_NONE == timeout_in_ticks)) {
		return nano_fiber_fifo_get(fifo);
	}

	key = irq_lock();

	fifo->stat--;
	if (fifo->stat < 0) {
		_nano_timeout_add(_nanokernel.current, &fifo->wait_q, timeout_in_ticks);
		_nano_wait_q_put(&fifo->wait_q);
		data = (void *)_Swap(key);
	} else {
		data = dequeue_data(fifo);
		irq_unlock(key);
	}

	return data;
}

void *nano_task_fifo_get_wait_timeout(struct nano_fifo *fifo,
			int32_t timeout_in_ticks)
{
	int64_t cur_ticks, limit;
	unsigned int key;
	void *data;

	if (unlikely(TICKS_UNLIMITED == timeout_in_ticks)) {
		return nano_task_fifo_get_wait(fifo);
	}

	if (unlikely(TICKS_NONE == timeout_in_ticks)) {
		return nano_task_fifo_get(fifo);
	}

	key = irq_lock();
	cur_ticks = nano_tick_get();
	limit = cur_ticks + timeout_in_ticks;

	while (cur_ticks < limit) {

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(fifo->stat > 0)) {
			fifo->stat--;
			data = dequeue_data(fifo);
			irq_unlock(key);
			return data;
		}

		/* see explanation in nano_stack.c:nano_task_stack_pop_wait() */

		nano_cpu_atomic_idle(key);

		key = irq_lock();
		cur_ticks = nano_tick_get();
	}

	irq_unlock(key);
	return NULL;
}
#endif /* CONFIG_NANO_TIMEOUTS */
