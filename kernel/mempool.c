/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <init.h>
#include <string.h>
#include <sys/__assert.h>
#include <sys/math_extras.h>
#include <stdbool.h>

static struct k_spinlock lock;

static struct k_mem_pool *get_pool(int id)
{
	extern struct k_mem_pool _k_mem_pool_list_start[];
	return &_k_mem_pool_list_start[id];
}

static int pool_id(struct k_mem_pool *pool)
{
	extern struct k_mem_pool _k_mem_pool_list_start[];
	return pool - &_k_mem_pool_list_start[0];
}

static void k_mem_pool_init(struct k_mem_pool *p)
{
	z_waitq_init(&p->wait_q);
	z_sys_mem_pool_base_init(&p->base);
}

int init_static_pools(struct device *unused)
{
	ARG_UNUSED(unused);

	Z_STRUCT_SECTION_FOREACH(k_mem_pool, p) {
		k_mem_pool_init(p);
	}

	return 0;
}

SYS_INIT(init_static_pools, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int k_mem_pool_alloc(struct k_mem_pool *p, struct k_mem_block *block,
		     size_t size, s32_t timeout)
{
	int ret;
	s64_t end = 0;

	__ASSERT(!(z_arch_is_in_isr() && timeout != K_NO_WAIT), "");

	if (timeout > 0) {
		end = k_uptime_get() + timeout;
	}

	while (true) {
		u32_t level_num, block_num;

		ret = z_sys_mem_pool_block_alloc(&p->base, size,
						 &level_num, &block_num,
						 &block->data);
		block->id.pool = pool_id(p);
		block->id.level = level_num;
		block->id.block = block_num;

		if (ret == 0 || timeout == K_NO_WAIT ||
		    ret != -ENOMEM) {
			return ret;
		}

		z_pend_curr_unlocked(&p->wait_q, timeout);

		if (timeout != K_FOREVER) {
			timeout = end - k_uptime_get();
			if (timeout <= 0) {
				break;
			}
		}
	}

	return -EAGAIN;
}

void k_mem_pool_free_id(struct k_mem_block_id *id)
{
	int need_sched = 0;
	struct k_mem_pool *p = get_pool(id->pool);

	z_sys_mem_pool_block_free(&p->base, id->level, id->block);

	/* Wake up anyone blocked on this pool and let them repeat
	 * their allocation attempts
	 *
	 * (Note that this spinlock only exists because z_unpend_all()
	 * is unsynchronized.  Maybe we want to put the lock into the
	 * wait_q instead and make the API safe?)
	 */
	k_spinlock_key_t key = k_spin_lock(&lock);

	need_sched = z_unpend_all(&p->wait_q);

	if (need_sched != 0) {
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}
}

void k_mem_pool_free(struct k_mem_block *block)
{
	k_mem_pool_free_id(&block->id);
}

void *k_mem_pool_malloc(struct k_mem_pool *pool, size_t size)
{
	struct k_mem_block block;

	/*
	 * get a block large enough to hold an initial (hidden) block
	 * descriptor, as well as the space the caller requested
	 */
	if (size_add_overflow(size, WB_UP(sizeof(struct k_mem_block_id)),
			      &size)) {
		return NULL;
	}
	if (k_mem_pool_alloc(pool, &block, size, K_NO_WAIT) != 0) {
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	(void)memcpy(block.data, &block.id, sizeof(struct k_mem_block_id));

	/* return address of the user area part of the block to the caller */
	return (char *)block.data + WB_UP(sizeof(struct k_mem_block_id));
}

void k_free(void *ptr)
{
	if (ptr != NULL) {
		/* point to hidden block descriptor at start of block */
		ptr = (char *)ptr - WB_UP(sizeof(struct k_mem_block_id));

		/* return block to the heap memory pool */
		k_mem_pool_free_id(ptr);
	}
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

/*
 * Heap is defined using HEAP_MEM_POOL_SIZE configuration option.
 *
 * This module defines the heap memory pool and the _HEAP_MEM_POOL symbol
 * that has the address of the associated memory pool struct.
 */

K_MEM_POOL_DEFINE(_heap_mem_pool, CONFIG_HEAP_MEM_POOL_MIN_SIZE,
		  CONFIG_HEAP_MEM_POOL_SIZE, 1, 4);
#define _HEAP_MEM_POOL (&_heap_mem_pool)

void *k_malloc(size_t size)
{
	return k_mem_pool_malloc(_HEAP_MEM_POOL, size);
}

void *k_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t bounds;

	if (size_mul_overflow(nmemb, size, &bounds)) {
		return NULL;
	}

	ret = k_malloc(bounds);
	if (ret != NULL) {
		(void)memset(ret, 0, bounds);
	}
	return ret;
}

void k_thread_system_pool_assign(struct k_thread *thread)
{
	thread->resource_pool = _HEAP_MEM_POOL;
}
#endif

void *z_thread_malloc(size_t size)
{
	void *ret;

	if (_current->resource_pool != NULL) {
		ret = k_mem_pool_malloc(_current->resource_pool, size);
	} else {
		ret = NULL;
	}

	return ret;
}
