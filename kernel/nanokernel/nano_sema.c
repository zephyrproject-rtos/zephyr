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
	} else {
		sem->nsig++;
	}

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
