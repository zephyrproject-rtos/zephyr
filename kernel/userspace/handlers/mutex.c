/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/mutex.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_k_mutex_lock(struct k_mutex *mutex,
				      k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_lock(mutex, timeout);
}
#include <zephyr/syscalls/k_mutex_lock_mrsh.c>

static inline int z_vrfy_k_mutex_unlock(struct k_mutex *mutex)
{
	K_OOPS(K_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_mutex_unlock(mutex);
}
#include <zephyr/syscalls/k_mutex_unlock_mrsh.c>
