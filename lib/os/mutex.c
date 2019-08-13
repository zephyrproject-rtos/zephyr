/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/mutex.h>
#include <syscall_handler.h>
#include <kernel_structs.h>

static struct k_mutex *get_k_mutex(struct sys_mutex *mutex)
{
	struct _k_object *obj;

	obj = z_object_find(mutex);
	if (obj == NULL || obj->type != K_OBJ_SYS_MUTEX) {
		return NULL;
	}

	return (struct k_mutex *)obj->data;
}

static bool check_sys_mutex_addr(u32_t addr)
{
	/* sys_mutex memory is never touched, just used to lookup the
	 * underlying k_mutex, but we don't want threads using mutexes
	 * that are outside their memory domain
	 */
	return Z_SYSCALL_MEMORY_WRITE(addr, sizeof(struct sys_mutex));
}

int z_impl_z_sys_mutex_kernel_lock(struct sys_mutex *mutex, s32_t timeout)
{
	struct k_mutex *kernel_mutex = get_k_mutex(mutex);

	if (kernel_mutex == NULL) {
		return -EINVAL;
	}

	return k_mutex_lock(kernel_mutex, timeout);
}

static inline int z_vrfy_z_sys_mutex_kernel_lock(struct sys_mutex *mutex,
						 s32_t timeout)
{
	if (check_sys_mutex_addr((u32_t) mutex)) {
		return -EACCES;
	}

	return z_impl_z_sys_mutex_kernel_lock(mutex, timeout);
}
#include <syscalls/z_sys_mutex_kernel_lock_mrsh.c>

int z_impl_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	struct k_mutex *kernel_mutex = get_k_mutex(mutex);

	if (kernel_mutex == NULL || kernel_mutex->lock_count == 0) {
		return -EINVAL;
	}

	if (kernel_mutex->owner != _current) {
		return -EPERM;
	}

	k_mutex_unlock(kernel_mutex);
	return 0;
}

static inline int z_vrfy_z_sys_mutex_kernel_unlock(struct sys_mutex *mutex)
{
	if (check_sys_mutex_addr((u32_t) mutex)) {
		return -EACCES;
	}

	return z_impl_z_sys_mutex_kernel_unlock(mutex);
}
#include <syscalls/z_sys_mutex_kernel_unlock_mrsh.c>
