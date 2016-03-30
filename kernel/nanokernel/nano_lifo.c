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

/** @file
 *
 * @brief Nanokernel dynamic-size LIFO queue object
 *
 * This module provides the nanokernel LIFO object implementation, including
 * the following APIs:
 *
 *    nano_lifo_init
 *    nano_fiber_lifo_put, nano_task_lifo_put, nano_isr_lifo_put
 *    nano_fiber_lifo_get, nano_task_lifo_get, nano_isr_lifo_get
 *    nano_lifo_get
 */

/** INTERNAL
 *
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 */

#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>

/** INTERNAL
 *
 * Although the existing implementation will support invocation from an ISR
 * context, for future flexibility, this API will be restricted from ISR
 * level invocation.
 */
void nano_lifo_init(struct nano_lifo *lifo)
{
	lifo->list = (void *) 0;
	_nano_wait_q_init(&lifo->wait_q);
	SYS_TRACING_OBJ_INIT(nano_lifo, lifo);
	_TASK_PENDQ_INIT(&lifo->task_q);
}

FUNC_ALIAS(_lifo_put_non_preemptible, nano_isr_lifo_put, void);
FUNC_ALIAS(_lifo_put_non_preemptible, nano_fiber_lifo_put, void);

/** INTERNAL
 *
 * This function is capable of supporting invocations from both a fiber and an
 * ISR context.  However, the nano_isr_lifo_put and nano_fiber_lifo_put aliases
 * are created to support any required implementation differences in the future
 * without introducing a source code migration issue.
 */
void _lifo_put_non_preemptible(struct nano_lifo *lifo, void *data)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();
	tcs = _nano_wait_q_remove(&lifo->wait_q);
	if (tcs) {
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int) data);
	} else {
		*(void **) data = lifo->list;
		lifo->list = data;
		_NANO_UNPEND_TASKS(&lifo->task_q);
	}

	irq_unlock(imask);
}

void nano_task_lifo_put(struct nano_lifo *lifo, void *data)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();
	tcs = _nano_wait_q_remove(&lifo->wait_q);
	if (tcs) {
		_nano_timeout_abort(tcs);
		fiberRtnValueSet(tcs, (unsigned int) data);
		_Swap(imask);
		return;
	}

	*(void **) data = lifo->list;
	lifo->list = data;
	_TASK_NANO_UNPEND_TASKS(&lifo->task_q);

	irq_unlock(imask);
}

void nano_lifo_put(struct nano_lifo *lifo, void *data)
{
	static void (*func[3])(struct nano_lifo *, void *) = {
		nano_isr_lifo_put,
		nano_fiber_lifo_put,
		nano_task_lifo_put
	};

	func[sys_execution_context_type_get()](lifo, data);
}

FUNC_ALIAS(_lifo_get, nano_isr_lifo_get, void *);
FUNC_ALIAS(_lifo_get, nano_fiber_lifo_get, void *);

void *_lifo_get(struct nano_lifo *lifo, int32_t timeout_in_ticks)
{
	void *data = NULL;
	unsigned int imask;

	imask = irq_lock();

	if (likely(lifo->list != NULL)) {
		data = lifo->list;
		lifo->list = *(void **) data;
	} else if (timeout_in_ticks != TICKS_NONE) {
		_NANO_TIMEOUT_ADD(&lifo->wait_q, timeout_in_ticks);
		_nano_wait_q_put(&lifo->wait_q);
		data = (void *) _Swap(imask);
		return data;
	}

	irq_unlock(imask);
	return data;
}

void *nano_task_lifo_get(struct nano_lifo *lifo, int32_t timeout_in_ticks)
{
	int64_t cur_ticks;
	int64_t limit = 0x7fffffffffffffffll;
	unsigned int imask;

	imask = irq_lock();
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

		if (likely(lifo->list != NULL)) {
			void *data = lifo->list;

			lifo->list = *(void **) data;
			irq_unlock(imask);

			return data;
		}

		if (timeout_in_ticks != TICKS_NONE) {
			_NANO_OBJECT_WAIT(&lifo->task_q, &lifo->list,
					timeout_in_ticks, imask);
			cur_ticks = _NANO_TIMEOUT_TICK_GET();
		}
	} while (cur_ticks < limit);

	irq_unlock(imask);

	return NULL;
}

void *nano_lifo_get(struct nano_lifo *lifo, int32_t timeout)
{
	static void *(*func[3])(struct nano_lifo *, int32_t) = {
		nano_isr_lifo_get,
		nano_fiber_lifo_get,
		nano_task_lifo_get
	};

	return func[sys_execution_context_type_get()](lifo, timeout);
}

/*
 * @brief Get first element from lifo and panic if NULL
 *
 * Get the first element from the specified lifo but generate a fatal error
 * if the element is NULL.
 *
 * @param lifo LIFO from which to receive.
 *
 * @return Pointer to first element in the list
 */
void *_nano_fiber_lifo_get_panic(struct nano_lifo *lifo)
{
	void *element;

	element = nano_fiber_lifo_get(lifo, TICKS_NONE);

	if (element == NULL) {
		_NanoFatalErrorHandler(_NANO_ERR_ALLOCATION_FAIL, &_default_esf);
	}

	return element;
}
