/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * @brief dynamic-size LIFO queue object
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <ksched.h>
#include <init.h>

extern struct k_lifo _k_lifo_list_start[];
extern struct k_lifo _k_lifo_list_end[];

struct k_lifo *_trace_list_k_lifo;

#ifdef CONFIG_OBJECT_TRACING

/*
 * Complete initialization of statically defined lifos.
 */
static int init_lifo_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_lifo *lifo;

	for (lifo = _k_lifo_list_start; lifo < _k_lifo_list_end; lifo++) {
		SYS_TRACING_OBJ_INIT(k_lifo, lifo);
	}
	return 0;
}

SYS_INIT(init_lifo_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

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
		_abort_thread_timeout(first_pending_thread);
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

	return _Swap(key) ? NULL : _current->base.swap_data;
}
