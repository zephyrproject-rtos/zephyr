/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <ksched.h>
#include <wait_q.h>

static struct k_spinlock lock;

int z_impl_k_condvar_init(struct k_condvar *condvar)
{
	z_waitq_init(&condvar->wait_q);
	z_object_init(condvar);
	return 0;
}

int z_impl_k_condvar_signal(struct k_condvar *condvar)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	struct k_thread *thread = z_unpend_first_thread(&condvar->wait_q);

	if (thread != NULL) {
		arch_thread_return_value_set(thread, 0);
		z_ready_thread(thread);
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}
	return 0;
}

int z_impl_k_condvar_broadcast(struct k_condvar *condvar)
{
	struct k_thread *pending_thread;
	k_spinlock_key_t key;
	int woken = 0;

	key = k_spin_lock(&lock);

	/* wake up any threads that are waiting to write */
	while ((pending_thread = z_unpend_first_thread(&condvar->wait_q)) !=
	       NULL) {
		arch_thread_return_value_set(pending_thread, 0);
		z_ready_thread(pending_thread);
		woken++;
	}

	z_reschedule(&lock, key);

	return woken;
}

int z_impl_k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
			  k_timeout_t timeout)
{
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&lock);
	k_mutex_unlock(mutex);

	ret = z_pend_curr(&lock, key, &condvar->wait_q, timeout);
	k_mutex_lock(mutex, K_FOREVER);

	return ret;
}
