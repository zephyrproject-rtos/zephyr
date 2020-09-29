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

void *k_heap_alloc(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
	int64_t now, end = z_timeout_end_calc(timeout);
	void *ret = NULL;
	k_spinlock_key_t key = k_spin_lock(&h->lock);

	__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");

	while (ret == NULL) {
		ret = sys_heap_alloc(&h->heap, bytes);

		now = z_tick_get();
		if ((ret != NULL) || ((end - now) <= 0)) {
			break;
		}

		(void) z_pend_curr(&h->lock, key, &h->wait_q,
				   K_TICKS(end - now));
		key = k_spin_lock(&h->lock);
	}

	k_spin_unlock(&h->lock, key);
	return ret;
}

void k_heap_free(struct k_heap *h, void *mem)
{
	k_spinlock_key_t key = k_spin_lock(&h->lock);

	sys_heap_free(&h->heap, mem);

	if (z_unpend_all(&h->wait_q) != 0) {
		z_reschedule(&h->lock, key);
	} else {
		k_spin_unlock(&h->lock, key);
	}
}

#ifdef CONFIG_MEM_POOL_HEAP_BACKEND
/* Compatibility layer for legacy k_mem_pool code on top of a k_heap
 * backend.
 */

int k_mem_pool_alloc(struct k_mem_pool *p, struct k_mem_block *block,
		     size_t size, k_timeout_t timeout)
{
	block->id.heap = p->heap;
	block->data = k_heap_alloc(p->heap, size, timeout);

	/* The legacy API returns -EAGAIN on timeout expiration, but
	 * -ENOMEM if the timeout was K_NO_WAIT. Don't ask.
	 */
	if (size != 0 && block->data == NULL) {
		return K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? -ENOMEM : -EAGAIN;
	} else {
		return 0;
	}
}

void k_mem_pool_free_id(struct k_mem_block_id *id)
{
	k_heap_free(id->heap, id->data);
}

#endif /* CONFIG_MEM_POOL_HEAP_BACKEND */
