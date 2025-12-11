/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Weak stub implementation of threading related kernel functions.
 *
 * This file is needed for armlink.
 *
 * When linking with armlink the linker will resolve undefined symbols for all
 * undefined functions even if those functions the reference the undefined
 * symbol is never actually called.
 *
 * This file provides weak stub implementations that are compiled when
 * CONFIG_MULTITHREADING=n to ensure proper linking.
 */

#include <zephyr/kernel.h>

int __weak z_impl_k_mutex_init(struct k_mutex *mutex)
{
	return 0;
}

int __weak z_impl_k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	return 0;
}

int __weak z_impl_k_mutex_unlock(struct k_mutex *mutex)
{
	return 0;
}

void __weak z_impl_k_sem_give(struct k_sem *sem)
{
}

int __weak z_impl_k_sem_init(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit)
{
	return 0;
}

int __weak z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
	return 0;
}

k_tid_t __weak z_impl_k_sched_current_thread_query(void)
{
	return 0;
}

int32_t __weak z_impl_k_usleep(int us)
{
	return 0;
}

void __weak z_thread_abort(struct k_thread *thread)
{
}

void __weak k_sched_lock(void)
{
}

void __weak k_sched_unlock(void)
{
}
