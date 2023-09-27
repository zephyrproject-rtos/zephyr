/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/spinlock.h>
#include <kswap.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/init.h>
#include <ksched.h>

static struct z_futex_data *k_futex_find_data(struct k_futex *futex)
{
	struct k_object *obj;

	obj = k_object_find(futex);
	if (obj == NULL || obj->type != K_OBJ_FUTEX) {
		return NULL;
	}

	return obj->data.futex_data;
}

int z_impl_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	k_spinlock_key_t key;
	unsigned int woken = 0U;
	struct k_thread *thread;
	struct z_futex_data *futex_data;

	futex_data = k_futex_find_data(futex);
	if (futex_data == NULL) {
		return -EINVAL;
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

	return woken;
}

static inline int z_vrfy_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
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

	futex_data = k_futex_find_data(futex);
	if (futex_data == NULL) {
		return -EINVAL;
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
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wait(futex, expected, timeout);
}
#include <syscalls/k_futex_wait_mrsh.c>
