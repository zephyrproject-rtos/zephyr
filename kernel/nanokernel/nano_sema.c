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
 * @brief Nanokernel semaphore object.
 *
 * This module provides the nanokernel semaphore object implementation,
 * including the following APIs:
 *
 * nano_sem_init
 * nano_fiber_sem_give, nano_task_sem_give, nano_isr_sem_give
 * nano_fiber_sem_take, nano_task_sem_take, nano_isr_sem_take
 * nano_fiber_sem_take_wait, nano_task_sem_take_wait

 * The semaphores are of the 'counting' type, i.e. each 'give' operation will
 * increment the internal count by 1, if no fiber is pending on it. The 'init'
 * call initializes the count to 0. Following multiple 'give' operations, the
 * same number of 'take' operations can be performed without the calling fiber
 * having to pend on the semaphore, or the calling task having to poll.
 */

/**
 * INTERNAL
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 */

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>

/**
 * INTERNAL
 * Although the existing implementation will support invocation from an ISR
 * context, for future flexibility, this API will be restricted from ISR
 * level invocation.
 */
void nano_sem_init(struct nano_sem *sem)
{
	sem->nsig = 0;
	_nano_wait_q_init(&sem->wait_q);
	DEBUG_TRACING_OBJ_INIT(struct nano_sem *, sem, _track_list_nano_sem);
}

FUNC_ALIAS(_sem_give_non_preemptible, nano_isr_sem_give, void);
FUNC_ALIAS(_sem_give_non_preemptible, nano_fiber_sem_give, void);

#ifdef CONFIG_NANO_TIMEOUTS
	#define set_sem_available(tcs) fiberRtnValueSet(tcs, 1)
#else
	#define set_sem_available(tcs) do { } while ((0))
#endif

/**
 * INTERNAL
 * This function is capable of supporting invocations from both a fiber and an
 * ISR context.  However, the nano_isr_sem_give and nano_fiber_sem_give aliases
 * are created to support any required implementation differences in the future
 * without introducing a source code migration issue.
 */
void _sem_give_non_preemptible(struct nano_sem *sem)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();
	tcs = _nano_wait_q_remove(&sem->wait_q);
	if (!tcs) {
		sem->nsig++;
	} else {
		_nano_timeout_abort(tcs);
		set_sem_available(tcs);
	}

	irq_unlock(imask);
}

void nano_task_sem_give(struct nano_sem *sem)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();
	tcs = _nano_wait_q_remove(&sem->wait_q);
	if (tcs) {
		_nano_timeout_abort(tcs);
		set_sem_available(tcs);
		_Swap(imask);
		return;
	}

	sem->nsig++;

	irq_unlock(imask);
}

void nano_sem_give(struct nano_sem *sem)
{
	static void (*func[3])(struct nano_sem *sem) = {
		nano_isr_sem_give, nano_fiber_sem_give, nano_task_sem_give
	};
	func[sys_execution_context_type_get()](sem);
}

FUNC_ALIAS(_sem_take, nano_isr_sem_take, int);
FUNC_ALIAS(_sem_take, nano_fiber_sem_take, int);
FUNC_ALIAS(_sem_take, nano_task_sem_take, int);

int _sem_take(
	struct nano_sem *sem
	)
{
	unsigned int imask;
	int avail;

	imask = irq_lock();
	avail = (sem->nsig > 0);
	sem->nsig -= avail;
	irq_unlock(imask);

	return avail;
}

/**
 * INTERNAL
 * There exists a separate nano_task_sem_take_wait() implementation since a
 * task cannot pend on a nanokernel object.  Instead, tasks will poll the
 * sempahore object.
 */
void nano_fiber_sem_take_wait(struct nano_sem *sem)
{
	unsigned int imask;

	imask = irq_lock();
	if (sem->nsig == 0) {
		_nano_wait_q_put(&sem->wait_q);
		_Swap(imask);
	} else {
		sem->nsig--;
		irq_unlock(imask);
	}
}

void nano_task_sem_take_wait(struct nano_sem *sem)
{
	unsigned int imask;

	/* spin until the sempahore is signaled */

	while (1) {
		imask = irq_lock();

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(sem->nsig > 0))
			break;

		/* see explanation in nano_stack.c:nano_task_stack_pop_wait() */

		nano_cpu_atomic_idle(imask);
	}

	sem->nsig--;
	irq_unlock(imask);
}

void nano_sem_take_wait(struct nano_sem *sem)
{
	static void (*func[3])(struct nano_sem *sem) = {
		NULL, nano_fiber_sem_take_wait, nano_task_sem_take_wait
	};
	func[sys_execution_context_type_get()](sem);
}

#ifdef CONFIG_NANO_TIMEOUTS

int nano_fiber_sem_take_wait_timeout(struct nano_sem *sem, int32_t timeout_in_ticks)
{
	unsigned int key = irq_lock();

	if (sem->nsig == 0) {
		if (unlikely(TICKS_NONE == timeout_in_ticks)) {
			irq_unlock(key);
			return 0;
		}
		if (likely(timeout_in_ticks != TICKS_UNLIMITED)) {
			_nano_timeout_add(_nanokernel.current, &sem->wait_q,
								timeout_in_ticks);
		}
		_nano_wait_q_put(&sem->wait_q);
		return _Swap(key);
	}

	sem->nsig--;

	irq_unlock(key);

	return 1;
}

int nano_task_sem_take_wait_timeout(struct nano_sem *sem, int32_t timeout_in_ticks)
{
	int64_t cur_ticks, limit;
	unsigned int key;

	if (unlikely(TICKS_UNLIMITED == timeout_in_ticks)) {
		nano_task_sem_take_wait(sem);
		return 1;
	}

	if (unlikely(TICKS_NONE == timeout_in_ticks)) {
		return nano_task_sem_take(sem);
	}

	key = irq_lock();
	cur_ticks = nano_tick_get();
	limit = cur_ticks + timeout_in_ticks;

	while (cur_ticks < limit) {

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(sem->nsig > 0)) {
			sem->nsig--;
			irq_unlock(key);
			return 1;
		}

		/* see explanation in nano_stack.c:nano_task_stack_pop_wait() */

		nano_cpu_atomic_idle(key);

		key = irq_lock();
		cur_ticks = nano_tick_get();
	}

	irq_unlock(key);
	return 0;
}

#endif /* CONFIG_NANO_TIMEOUTS */
