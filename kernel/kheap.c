/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/iterable_sections.h>
/* private kernel APIs */
#include <ksched.h>
#include <wait_q.h>

void k_heap_init(struct k_heap *h, void *mem, size_t bytes)
{
	z_waitq_init(&h->wait_q);
	sys_heap_init(&h->heap, mem, bytes);

	SYS_PORT_TRACING_OBJ_INIT(k_heap, h);
}

static int statics_init(void)
{
	STRUCT_SECTION_FOREACH(k_heap, h) {
#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
		/* Some heaps may not present at boot, so we need to wait for
		 * paging mechanism to be initialized before we can initialize
		 * each heap.
		 */
		extern bool z_sys_post_kernel;
		bool do_clear = z_sys_post_kernel;

		/* During pre-kernel init, z_sys_post_kernel == false,
		 * initialize if within pinned region. Otherwise skip.
		 * In post-kernel init, z_sys_post_kernel == true, skip those in
		 * pinned region as they have already been initialized and
		 * possibly already in use. Otherwise initialize.
		 */
		if (lnkr_is_pinned((uint8_t *)h) &&
		    lnkr_is_pinned((uint8_t *)&h->wait_q) &&
		    lnkr_is_region_pinned((uint8_t *)h->heap.init_mem,
					  h->heap.init_bytes)) {
			do_clear = !do_clear;
		}

		if (do_clear)
#endif /* CONFIG_DEMAND_PAGING && !CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT */
		{
			k_heap_init(h, h->heap.init_mem, h->heap.init_bytes);
		}
	}
	return 0;
}

SYS_INIT_NAMED(statics_init_pre, statics_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
/* Need to wait for paging mechanism to be initialized before
 * heaps that are not in pinned sections can be initialized.
 */
SYS_INIT_NAMED(statics_init_post, statics_init, POST_KERNEL, 0);
#endif /* CONFIG_DEMAND_PAGING && !CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT */

void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	void *ret = NULL;

	k_spinlock_key_t key = k_spin_lock(&h->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap, aligned_alloc, h, timeout);

	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	bool blocked_alloc = false;

	while (ret == NULL) {
		ret = sys_heap_aligned_alloc(&h->heap, align, bytes);

		if (!IS_ENABLED(CONFIG_MULTITHREADING) ||
		    (ret != NULL) || K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
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

		timeout = sys_timepoint_timeout(end);
		(void) z_pend_curr(&h->lock, key, &h->wait_q, timeout);
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
