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

static struct z_futex_data *k_futex_find_data(struct k_futex *futex)
{
	struct _k_object *obj;

	obj = z_object_find(futex);
	if (obj == NULL || obj->type != K_OBJ_FUTEX) {
		return NULL;
	}

	return (struct z_futex_data *)obj->data;
}

int z_impl_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	k_spinlock_key_t key;
	unsigned int woken = 0;
	struct k_thread *thread;
	struct z_futex_data *futex_data;

	futex_data = k_futex_find_data(futex);
	if (futex_data == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&futex_data->lock);

	do {
		thread = z_unpend_first_thread(&futex_data->wait_q);
		if (thread) {
			z_ready_thread(thread);
			z_arch_thread_return_value_set(thread, 0);
			woken++;
		}
	} while (thread && wake_all);

	z_reschedule(&futex_data->lock, key);

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

int z_impl_k_futex_wait(struct k_futex *futex, int expected, s32_t timeout)
{
	int ret;
	k_spinlock_key_t key;
	struct z_futex_data *futex_data;

	futex_data = k_futex_find_data(futex);
	if (futex_data == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&futex_data->lock);

	if (atomic_get(&futex->val) != (atomic_val_t)expected) {
		k_spin_unlock(&futex_data->lock, key);
		return -EAGAIN;
	}

	ret = z_pend_curr(&futex_data->lock,
			key, &futex_data->wait_q, timeout);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static inline int z_vrfy_k_futex_wait(struct k_futex *futex, int expected,
				      s32_t timeout)
{
	if (Z_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wait(futex, expected, timeout);
}
#include <syscalls/k_futex_wait_mrsh.c>
