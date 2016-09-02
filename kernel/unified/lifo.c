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
 * @brief dynamic-size LIFO queue object
 */

#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <sched.h>

void k_lifo_init(struct k_lifo *lifo)
{
	lifo->list = (void *)0;
	sys_dlist_init(&lifo->wait_q);

	SYS_TRACING_OBJ_INIT(k_lifo, lifo);
}

void k_lifo_put(struct k_lifo *lifo, void *data)
{
	struct k_thread *first_pending_thread;
	unsigned int key;

	key = irq_lock();

	first_pending_thread = _unpend_first_thread(&lifo->wait_q);

	if (first_pending_thread) {
		_timeout_abort(first_pending_thread);
		_ready_thread(first_pending_thread);

		_set_thread_return_value_with_data(first_pending_thread,
						   0, data);

		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	} else {
		*(void **)data = lifo->list;
		lifo->list = data;
	}

	irq_unlock(key);
}

void *k_lifo_get(struct k_lifo *lifo, int32_t timeout)
{
	unsigned int key;
	void *data;

	key = irq_lock();

	if (likely(lifo->list)) {
		data = lifo->list;
		lifo->list = *(void **)data;
		irq_unlock(key);
		return data;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return NULL;
	}

	_pend_current_thread(&lifo->wait_q, timeout);

	return _Swap(key) ? NULL : _current->swap_data;
}
