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
 * nano_fifo_get
 */

/*
 * INTERNAL
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 */

#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>

struct fifo_node {
	void *next;
};

/**
 * @brief Internal routine to append data to a fifo
 *
 * @return N/A
 */
static inline void data_q_init(struct _nano_queue *q)
{
	q->head = NULL;
	q->tail = &q->head;
}

/**
 * @brief Internal routine to test if queue is empty
 *
 * @return N/A
 */
static inline int is_q_empty(struct _nano_queue *q)
{
	return q->head == NULL;
}

/*
 * INTERNAL
 * Although the existing implementation will support invocation from an ISR
 * context, for future flexibility, this API will be restricted from ISR
 * level invocation.
 */
void nano_fifo_init(struct nano_fifo *fifo)
{
	_nano_wait_q_init(&fifo->wait_q);
	data_q_init(&fifo->data_q);

	_TASK_PENDQ_INIT(&fifo->task_q);

	SYS_TRACING_OBJ_INIT(nano_fifo, fifo);
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
	struct fifo_node *node = data;
	struct fifo_node *tail = fifo->data_q.tail;

	tail->next = node;
	fifo->data_q.tail = node;
	node->next = NULL;
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
	struct tcs *tcs;
	unsigned int key;

	key = irq_lock();

	tcs = _nano_wait_q_remove(&fifo->wait_q);
	if (tcs) {
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int)data);
	} else {
		enqueue_data(fifo, data);
		_NANO_UNPEND_TASKS(&fifo->task_q);
	}

	irq_unlock(key);
}

void nano_task_fifo_put(struct nano_fifo *fifo, void *data)
{
	struct tcs *tcs;
	unsigned int key;

	key = irq_lock();
	tcs = _nano_wait_q_remove(&fifo->wait_q);
	if (tcs) {
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int)data);
		_Swap(key);
		return;
	}

	enqueue_data(fifo, data);
	_TASK_NANO_UNPEND_TASKS(&fifo->task_q);

	irq_unlock(key);
}


void nano_fifo_put(struct nano_fifo *fifo, void *data)
{
	static void (*func[3])(struct nano_fifo *fifo, void *data) = {
		nano_isr_fifo_put,
		nano_fiber_fifo_put,
		nano_task_fifo_put
	};

	func[sys_execution_context_type_get()](fifo, data);
}

/**
 *
 * @brief Internal routine to remove data from a fifo
 *
 * @return The data item removed
 */
static inline void *dequeue_data(struct nano_fifo *fifo)
{
	struct fifo_node *head = fifo->data_q.head;

	fifo->data_q.head = head->next;
	if (fifo->data_q.tail == head) {
		fifo->data_q.tail = &fifo->data_q.head;
	}

	return head;
}

FUNC_ALIAS(_fifo_get, nano_isr_fifo_get, void *);
FUNC_ALIAS(_fifo_get, nano_fiber_fifo_get, void *);

void *_fifo_get(struct nano_fifo *fifo, int32_t timeout_in_ticks)
{
	unsigned int key;
	void *data = NULL;

	key = irq_lock();

	if (likely(!is_q_empty(&fifo->data_q))) {
		data = dequeue_data(fifo);
	} else if (timeout_in_ticks != TICKS_NONE) {
		_NANO_TIMEOUT_ADD(&fifo->wait_q, timeout_in_ticks);
		_nano_wait_q_put(&fifo->wait_q);
		data = (void *)_Swap(key);
		return data;
	}

	irq_unlock(key);
	return data;
}

void *nano_task_fifo_get(struct nano_fifo *fifo, int32_t timeout_in_ticks)
{
	int64_t cur_ticks;
	int64_t limit = 0x7fffffffffffffffll;
	unsigned int key;

	key = irq_lock();
	cur_ticks = _NANO_TIMEOUT_TICK_GET();
	if (timeout_in_ticks != TICKS_UNLIMITED) {
		limit = cur_ticks + timeout_in_ticks;
	}

	do {
		/*
		 * Predict that the branch will be taken to break out of the
		 * loop.  There is little cost to a misprediction since that
		 * leads to idle.
		 */

		if (likely(!is_q_empty(&fifo->data_q))) {
			void *data = dequeue_data(fifo);

			irq_unlock(key);
			return data;
		}

		if (timeout_in_ticks != TICKS_NONE) {
			_NANO_OBJECT_WAIT(&fifo->task_q, &fifo->data_q.head,
					timeout_in_ticks, key);
			cur_ticks = _NANO_TIMEOUT_TICK_GET();
			_NANO_TIMEOUT_UPDATE(timeout_in_ticks,
						limit, cur_ticks);
		}
	} while (cur_ticks < limit);

	irq_unlock(key);
	return NULL;
}

void *nano_fifo_get(struct nano_fifo *fifo, int32_t timeout)
{
	static void *(*func[3])(struct nano_fifo *, int32_t) = {
		nano_isr_fifo_get,
		nano_fiber_fifo_get,
		nano_task_fifo_get
	};

	return func[sys_execution_context_type_get()](fifo, timeout);
}
