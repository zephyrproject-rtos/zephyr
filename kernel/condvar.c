/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/toolchain.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/syscall_handler.h>

static struct k_spinlock lock;

int z_impl_k_condvar_init(struct k_condvar *condvar)
{
	z_waitq_init(&condvar->wait_q);
	z_object_init(condvar);

	SYS_PORT_TRACING_OBJ_INIT(k_condvar, condvar, 0);

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_condvar_init(struct k_condvar *condvar)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_init(condvar);
}
#include <syscalls/k_condvar_init_mrsh.c>
#endif

int z_impl_k_condvar_signal(struct k_condvar *condvar)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_condvar, signal, condvar);

	struct k_thread *thread = z_unpend_first_thread(&condvar->wait_q);

	if (thread != NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_condvar, signal, condvar, K_FOREVER);

		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_condvar, signal, condvar, 0);

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_condvar_signal(struct k_condvar *condvar)
{
	Z_OOPS(Z_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_signal(condvar);
}
#include <syscalls/k_condvar_signal_mrsh.c>
#endif

int z_impl_k_condvar_broadcast(struct k_condvar *condvar)
{
	struct k_thread *pending_thread;
	k_spinlock_key_t key;
	int woken = 0;

	key = k_spin_lock(&lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_condvar, broadcast, condvar);

	/* wake up any threads that are waiting to write */
	while ((pending_thread = z_unpend_first_thread(&condvar->wait_q)) !=
	       NULL) {
		woken++;
		arch_thread_return_value_set(pending_thread, 0);
		z_ready_thread(pending_thread);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_condvar, broadcast, condvar, woken);

	z_reschedule(&lock, key);

	return woken;
}
#ifdef CONFIG_USERSPACE
int z_vrfy_k_condvar_broadcast(struct k_condvar *condvar)
{
	Z_OOPS(Z_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	return z_impl_k_condvar_broadcast(condvar);
}
#include <syscalls/k_condvar_broadcast_mrsh.c>
#endif

int z_impl_k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
			  k_timeout_t timeout)
{
	k_spinlock_key_t key;
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_condvar, wait, condvar);

	key = k_spin_lock(&lock);
	k_mutex_unlock(mutex);

	ret = z_pend_curr(&lock, key, &condvar->wait_q, timeout);
	k_mutex_lock(mutex, K_FOREVER);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_condvar, wait, condvar, ret);

	return ret;
}
#ifdef CONFIG_USERSPACE
int z_vrfy_k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
			  k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(condvar, K_OBJ_CONDVAR));
	Z_OOPS(Z_SYSCALL_OBJ(mutex, K_OBJ_MUTEX));
	return z_impl_k_condvar_wait(condvar, mutex, timeout);
}
#include <syscalls/k_condvar_wait_mrsh.c>
#endif
