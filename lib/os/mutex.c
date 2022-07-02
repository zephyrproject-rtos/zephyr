/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/kernel_structs.h>

static struct k_mutex *get_k_mutex(struct sys_mutex *mutex)
{
	struct z_object *obj;

	obj = z_object_find(mutex);
	if (obj == NULL || obj->type != K_OBJ_SYS_MUTEX) {
		return NULL;
	}

	return obj->data.mutex;
}

int z_impl_z_sys_mutex_kernel_lock(struct sys_mutex *mutex, k_timeout_t timeout)
{
	struct k_mutex *kernel_mutex = get_k_mutex(mutex);

	if (kernel_mutex == NULL) {
		return -EINVAL;
	}

	return k_mutex_lock(kernel_mutex, timeout);
}

static inline int z_vrfy_z_sys_mutex_kernel_lock(struct sys_mutex *mutex,
						 k_timeout_t timeout)
{
	return z_impl_z_sys_mutex_kernel_lock(mutex, timeout);
}
#include <syscalls/z_sys_mutex_kernel_lock_mrsh.c>

int z_impl_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	struct k_mutex *kernel_mutex = get_k_mutex(mutex);

	if (kernel_mutex == NULL) {
		return -EINVAL;
	}

	return k_mutex_unlock(kernel_mutex);
}

static inline int z_vrfy_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	return z_impl_z_sys_mutex_kernel_unlock(mutex);
}
#include <syscalls/z_sys_mutex_kernel_unlock_mrsh.c>
