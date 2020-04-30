/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <init.h>

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

int init_static_pools(const struct device *unused)
{
	ARG_UNUSED(unused);

	Z_STRUCT_SECTION_FOREACH(k_mem_pool, p) {
		k_mem_pool_init(p);
	}

	return 0;
}

SYS_INIT(init_static_pools, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int k_mem_pool_alloc(struct k_mem_pool *p, struct k_mem_block *block,
		     size_t size, k_timeout_t timeout)
{
	int ret;
	uint64_t end = 0;

	__ASSERT(!(arch_is_in_isr() && !K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	end = z_timeout_end_calc(timeout);

	while (true) {
		uint32_t level_num, block_num;

		ret = z_sys_mem_pool_block_alloc(&p->base, size,
						 &level_num, &block_num,
						 &block->data);
		block->id.pool = pool_id(p);
		block->id.level = level_num;
		block->id.block = block_num;

		if (ret == 0 || K_TIMEOUT_EQ(timeout, K_NO_WAIT) ||
		    ret != -ENOMEM) {
			return ret;
		}

		z_pend_curr_unlocked(&p->wait_q, timeout);

		if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			int64_t remaining = end - z_tick_get();

			if (remaining <= 0) {
				break;
			}
			timeout = Z_TIMEOUT_TICKS(remaining);
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
