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
 * nano_sem_take
 *
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
#include <misc/debug/object_tracing_common.h>
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
	SYS_TRACING_OBJ_INIT(nano_sem, sem);
	_TASK_PENDQ_INIT(&sem->task_q);
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
		_NANO_UNPEND_TASKS(&sem->task_q);
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
	_TASK_NANO_UNPEND_TASKS(&sem->task_q);

	irq_unlock(imask);
}

void nano_sem_give(struct nano_sem *sem)
{
	static void (*func[3])(struct nano_sem *sem) = {
		nano_isr_sem_give,
		nano_fiber_sem_give,
		nano_task_sem_give
	};

	func[sys_execution_context_type_get()](sem);
}

FUNC_ALIAS(_sem_take, nano_isr_sem_take, int);
FUNC_ALIAS(_sem_take, nano_fiber_sem_take, int);

int _sem_take(struct nano_sem *sem, int32_t timeout_in_ticks)
{
	unsigned int key = irq_lock();

	if (likely(sem->nsig > 0)) {
		sem->nsig--;
		irq_unlock(key);
		return 1;
	}

	if (timeout_in_ticks != TICKS_NONE) {
		_NANO_TIMEOUT_ADD(&sem->wait_q, timeout_in_ticks);
		_nano_wait_q_put(&sem->wait_q);
		return _Swap(key);
	}

	irq_unlock(key);
	return 0;
}

/**
 * INTERNAL
 * Since a task cannot pend on a nanokernel object, they poll the
 * sempahore object.
 */
int nano_task_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks)
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

		if (likely(sem->nsig > 0)) {
			sem->nsig--;
			irq_unlock(key);
			return 1;
		}

		if (timeout_in_ticks != TICKS_NONE) {
			_NANO_OBJECT_WAIT(&sem->task_q, &sem->nsig,
					timeout_in_ticks, key);
			cur_ticks = _NANO_TIMEOUT_TICK_GET();

			_NANO_TIMEOUT_UPDATE(timeout_in_ticks,
						limit, cur_ticks);
		}
	} while (cur_ticks < limit);

	irq_unlock(key);
	return 0;
}

int nano_sem_take(struct nano_sem *sem, int32_t timeout)
{
	static int (*func[3])(struct nano_sem *, int32_t) = {
		nano_isr_sem_take,
		nano_fiber_sem_take,
		nano_task_sem_take
	};

	return func[sys_execution_context_type_get()](sem, timeout);
}
