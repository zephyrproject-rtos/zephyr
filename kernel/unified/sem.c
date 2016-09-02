/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
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
 * The semaphores are of the 'counting' type, i.e. each 'give' operation will
 * increment the internal count by 1, if no fiber is pending on it. The 'init'
 * call initializes the count to 0. Following multiple 'give' operations, the
 * same number of 'take' operations can be performed without the calling fiber
 * having to pend on the semaphore, or the calling task having to poll.
 */

#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <sched.h>

void k_sem_init(struct k_sem *sem, unsigned int initial_count,
		unsigned int limit)
{
	__ASSERT(limit != 0, "limit cannot be zero");

	sem->count = initial_count;
	sem->limit = limit;
	sys_dlist_init(&sem->wait_q);
	SYS_TRACING_OBJ_INIT(nano_sem, sem);
}

void k_sem_give(struct k_sem *sem)
{
	int key = irq_lock();
	struct tcs *first_pending_thread = _unpend_first_thread(&sem->wait_q);

	if (first_pending_thread) {
		_timeout_abort(first_pending_thread);
		_ready_thread(first_pending_thread);

		_set_thread_return_value(first_pending_thread, 0);

		if (!_is_in_isr() && _must_switch_threads()) {
			_Swap(key);
			return;
		}
	} else {
		if (likely(sem->count != sem->limit)) {
			sem->count++;
		}
	}

	irq_unlock(key);
}

int k_sem_take(struct k_sem *sem, int32_t timeout)
{
	__ASSERT(!_is_in_isr() || timeout == K_NO_WAIT, "");

	unsigned int key = irq_lock();

	if (likely(sem->count > 0)) {
		sem->count--;
		irq_unlock(key);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EBUSY;
	}

	_pend_current_thread(&sem->wait_q, timeout);

	return _Swap(key);
}
