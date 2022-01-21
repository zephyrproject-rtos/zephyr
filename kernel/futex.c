/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>
#include <kswap.h>
#include <syscall_handler.h>
#include <init.h>
#include <ksched.h>
#include <sys/rb.h>
#include <wait_q.h>

K_MEM_SLAB_DEFINE(futex_data_slab, sizeof(struct z_futex_data),
		CONFIG_MAX_THREAD_BYTES * 8, 4);

static bool node_lessthan(struct rbnode *a, struct rbnode *b);

static struct rbtree futex_data_rb_tree = {
	.lessthan_fn = node_lessthan
};

static inline struct z_futex_data *node_to_futex_data(struct rbnode *node)
{
	return CONTAINER_OF(node, struct z_futex_data, node);
}

static inline bool node_lessthan(struct rbnode *a, struct rbnode *b)
{
	struct z_futex_data *afdata = node_to_futex_data(a);
	struct z_futex_data *bfdata = node_to_futex_data(b);

	return afdata->addr < bfdata->addr;
}

static inline void futex_data_init(struct z_futex_data *futex_data)
{
	memset(futex_data, 0, sizeof(struct z_futex_data));
	z_waitq_init(&futex_data->wait_q);
}

static struct z_futex_data *futex_find_data(void *addr)
{
	struct rbnode *n = futex_data_rb_tree.root;

	while (n != NULL) {
		struct z_futex_data *d = node_to_futex_data(n);

		if (addr > d->addr) {
			n = n->children[1];
		} else if (addr < d->addr) {
			n = (struct rbnode *) (((uintptr_t) n->children[0]) & ~1UL);
		} else {
			return d;
		}
	}
	return NULL;
}

static int futex_add_data(struct k_futex *futex, struct z_futex_data **futex_data)
{
	if (k_mem_slab_num_free_get(&futex_data_slab) <= 0) {
		return -ENOMEM;
	}

	int ret = k_mem_slab_alloc(&futex_data_slab, (void **) futex_data, K_NO_WAIT);

	if (ret < 0) {
		return ret;
	}

	futex_data_init(*futex_data);
	(*futex_data)->addr = (void *) futex;
	rb_insert(&futex_data_rb_tree, &(*futex_data)->node);

	return ret;
}

static void futex_remove_data(struct z_futex_data *futex_data)
{
	rb_remove(&futex_data_rb_tree, &futex_data->node);
	k_mem_slab_free(&futex_data_slab, (void **) &futex_data);
}

int z_impl_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	k_spinlock_key_t key;
	unsigned int woken = 0U;
	struct k_thread *thread;
	struct z_futex_data *futex_data;

	futex_data = futex_find_data(futex);
	if (futex_data == NULL) {
		return woken;
	}

	key = k_spin_lock(&futex_data->lock);

	do {
		thread = z_unpend_first_thread(&futex_data->wait_q);
		if (thread != NULL) {
			woken++;
			arch_thread_return_value_set(thread, 0);
			z_ready_thread(thread);
		}
	} while (thread && wake_all);

	z_reschedule(&futex_data->lock, key);

	if (z_waitq_head(&futex_data->wait_q) == NULL) {
		futex_remove_data(futex_data);
	}

	return woken;
}

static inline int z_vrfy_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	if (Z_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wake(futex, wake_all);
}
#include <syscalls/k_futex_wake_mrsh.c>

int z_impl_k_futex_wait(struct k_futex *futex, int expected,
			k_timeout_t timeout)
{
	int ret;
	k_spinlock_key_t key;
	struct z_futex_data *futex_data;

	futex_data = futex_find_data(futex);
	if (futex_data == NULL) {
		ret = futex_add_data(futex, (struct z_futex_data **)&futex_data);
		if (ret < 0) {
			return ret;
		}
	}

	if (atomic_get(&futex->val) != (atomic_val_t)expected) {
		return -EAGAIN;
	}

	key = k_spin_lock(&futex_data->lock);

	ret = z_pend_curr(&futex_data->lock,
			key, &futex_data->wait_q, timeout);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static inline int z_vrfy_k_futex_wait(struct k_futex *futex, int expected,
				      k_timeout_t timeout)
{
	if (Z_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wait(futex, expected, timeout);
}
#include <syscalls/k_futex_wait_mrsh.c>
