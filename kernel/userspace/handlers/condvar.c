/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/condvar.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

int z_vrfy_k_condvar_init(struct k_condvar *condvar)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_init(condvar);
}
#include <zephyr/syscalls/k_condvar_init_mrsh.c>

int z_vrfy_k_condvar_signal(struct k_condvar *condvar)
{
	K_OOPS(K_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_signal(condvar);
}
#include <zephyr/syscalls/k_condvar_signal_mrsh.c>

int z_vrfy_k_condvar_broadcast(struct k_condvar *condvar)
{
	K_OOPS(K_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_broadcast(condvar);
}
#include <zephyr/syscalls/k_condvar_broadcast_mrsh.c>

int z_vrfy_k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
			  k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	K_OOPS(K_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_condvar_wait(condvar, mutex, timeout);
}
#include <zephyr/syscalls/k_condvar_wait_mrsh.c>
