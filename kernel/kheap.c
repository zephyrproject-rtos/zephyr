/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <init.h>

void k_heap_init(struct k_heap *h, void *mem, size_t bytes)
{
	z_waitq_init(&h->wait_q);
	sys_heap_init(&h->heap, mem, bytes);

	SYS_PORT_TRACING_OBJ_INIT(k_heap, h);
}

static int statics_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	Z_STRUCT_SECTION_FOREACH(k_heap, h) {
		k_heap_init(h, h->heap.init_mem, h->heap.init_bytes);
	}
	return 0;
}

SYS_INIT(statics_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			k_timeout_t timeout)
{
	int64_t now, end = sys_clock_timeout_end_calc(timeout);
	void *ret = NULL;
	k_spinlock_key_t key = k_spin_lock(&h->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap, aligned_alloc, h, timeout);

	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	bool blocked_alloc = false;

	while (ret == NULL) {
		ret = sys_heap_aligned_alloc(&h->heap, align, bytes);

		now = sys_clock_tick_get();
		if (!IS_ENABLED(CONFIG_MULTITHREADING) ||
		    (ret != NULL) || ((end - now) <= 0)) {
			break;
		}

		if (!blocked_alloc) {
			blocked_alloc = true;

			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_heap, aligned_alloc, h, timeout);
		} else {
			/**
			 * @todo	Trace attempt to avoid empty trace segments
			 */
		}

		(void) z_pend_curr(&h->lock, key, &h->wait_q,
				   K_TICKS(end - now));
		key = k_spin_lock(&h->lock);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap, aligned_alloc, h, timeout, ret);

	k_spin_unlock(&h->lock, key);
	return ret;
}

void *k_heap_alloc(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap, alloc, h, timeout);

	void *ret = k_heap_aligned_alloc(h, sizeof(void *), bytes, timeout);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap, alloc, h, timeout, ret);

	return ret;
}

void k_heap_free(struct k_heap *h, void *mem)
{
	k_spinlock_key_t key = k_spin_lock(&h->lock);

	sys_heap_free(&h->heap, mem);

	SYS_PORT_TRACING_OBJ_FUNC(k_heap, free, h);
	if (IS_ENABLED(CONFIG_MULTITHREADING) && z_unpend_all(&h->wait_q) != 0) {
		z_reschedule(&h->lock, key);
	} else {
		k_spin_unlock(&h->lock, key);
	}
}
