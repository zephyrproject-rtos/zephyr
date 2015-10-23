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
 *    nano_fiber_lifo_get_wait, nano_task_lifo_get_wait
 */

/** INTERNAL
 *
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 */

#include <nano_private.h>
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
	DEBUG_TRACING_OBJ_INIT(struct nano_lifo *, lifo, _track_list_nano_lifo);
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

	irq_unlock(imask);
}

FUNC_ALIAS(_lifo_get, nano_isr_lifo_get, void *);
FUNC_ALIAS(_lifo_get, nano_fiber_lifo_get, void *);
FUNC_ALIAS(_lifo_get, nano_task_lifo_get, void *);

/** INTERNAL
 *
 * This function is capable of supporting invocations from fiber, task, and ISR
 * contexts.  However, the nano_isr_lifo_get, nano_task_lifo_get, and
 * nano_fiber_lifo_get aliases are created to support any required
 * implementation differences in the future without introducing a source code
 * migration issue.
 */
void *_lifo_get(struct nano_lifo *lifo)
{
	void *data;
	unsigned int imask;

	imask = irq_lock();

	data = lifo->list;
	if (data) {
		lifo->list = *(void **) data;
	}

	irq_unlock(imask);

	return data;
}

/** INTERNAL
 *
 * There exists a separate nano_task_lifo_get_wait() implementation since a
 * task cannot pend on a nanokernel object.  Instead, tasks will poll
 * the lifo object.
 */
void *nano_fiber_lifo_get_wait(struct nano_lifo *lifo)
{
	void *data;
	unsigned int imask;

	imask = irq_lock();

	if (!lifo->list) {
		_nano_wait_q_put(&lifo->wait_q);
		data = (void *) _Swap(imask);
	} else {
		data = lifo->list;
		lifo->list = *(void **) data;
		irq_unlock(imask);
	}

	return data;
}

void *nano_task_lifo_get_wait(struct nano_lifo *lifo)
{
	void *data;
	unsigned int imask;

	/* spin until data is put onto the LIFO */

	while (1) {
		imask = irq_lock();

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(lifo->list))
			break;

		/* see explanation in nano_stack.c:nano_task_stack_pop_wait() */

		nano_cpu_atomic_idle(imask);
	}

	data = lifo->list;
	lifo->list = *(void **) data;

	irq_unlock(imask);

	return data;
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

	element = nano_fiber_lifo_get(lifo);

	if (element == NULL) {
		_NanoFatalErrorHandler(_NANO_ERR_ALLOCATION_FAIL, &_default_esf);
	}

	return element;
}

#ifdef CONFIG_NANO_TIMEOUTS

void *nano_fiber_lifo_get_wait_timeout(struct nano_lifo *lifo,
		int32_t timeout_in_ticks)
{
	unsigned int key = irq_lock();
	void *data;

	if (!lifo->list) {
		if (unlikely(TICKS_NONE == timeout_in_ticks)) {
			irq_unlock(key);
			return NULL;
		}
		if (likely(timeout_in_ticks != TICKS_UNLIMITED)) {
			_nano_timeout_add(_nanokernel.current, &lifo->wait_q,
					timeout_in_ticks);
		}
		_nano_wait_q_put(&lifo->wait_q);
		data = (void *)_Swap(key);
	} else {
		data = lifo->list;
		lifo->list = *(void **)data;
		irq_unlock(key);
	}

	return data;
}

void *nano_task_lifo_get_wait_timeout(struct nano_lifo *lifo,
		int32_t timeout_in_ticks)
{
	int64_t cur_ticks, limit;
	unsigned int key;
	void *data;

	if (unlikely(TICKS_UNLIMITED == timeout_in_ticks)) {
		return nano_task_lifo_get_wait(lifo);
	}

	if (unlikely(TICKS_NONE == timeout_in_ticks)) {
		return nano_task_lifo_get(lifo);
	}

	key = irq_lock();
	cur_ticks = nano_tick_get();
	limit = cur_ticks + timeout_in_ticks;

	while (cur_ticks < limit) {

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(lifo->list)) {
			data = lifo->list;
			lifo->list = *(void **)data;
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
