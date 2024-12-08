/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kthread.h>

struct k_spinlock z_thread_monitor_lock;
/*
 * Remove a thread from the kernel's list of active threads.
 */
void z_thread_monitor_exit(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&z_thread_monitor_lock);

	if (thread == _kernel.threads) {
		_kernel.threads = _kernel.threads->next_thread;
	} else {
		struct k_thread *prev_thread;

		prev_thread = _kernel.threads;
		while ((prev_thread != NULL) &&
			(thread != prev_thread->next_thread)) {
			prev_thread = prev_thread->next_thread;
		}
		if (prev_thread != NULL) {
			prev_thread->next_thread = thread->next_thread;
		}
	}

	k_spin_unlock(&z_thread_monitor_lock, key);
}


void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data)
{
	struct k_thread *thread;
	k_spinlock_key_t key;

	__ASSERT(user_cb != NULL, "user_cb can not be NULL");

	/*
	 * Lock is needed to make sure that the _kernel.threads is not being
	 * modified by the user_cb either directly or indirectly.
	 * The indirect ways are through calling k_thread_create and
	 * k_thread_abort from user_cb.
	 */
	key = k_spin_lock(&z_thread_monitor_lock);

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, foreach);

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		user_cb(thread, user_data);
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, foreach);

	k_spin_unlock(&z_thread_monitor_lock, key);
}

void k_thread_foreach_unlocked(k_thread_user_cb_t user_cb, void *user_data)
{
	struct k_thread *thread;
	k_spinlock_key_t key;

	__ASSERT(user_cb != NULL, "user_cb can not be NULL");

	key = k_spin_lock(&z_thread_monitor_lock);

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, foreach_unlocked);

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		k_spin_unlock(&z_thread_monitor_lock, key);
		user_cb(thread, user_data);
		key = k_spin_lock(&z_thread_monitor_lock);
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, foreach_unlocked);

	k_spin_unlock(&z_thread_monitor_lock, key);

}

#ifdef CONFIG_SMP
void k_thread_foreach_filter_by_cpu(unsigned int cpu, k_thread_user_cb_t user_cb,
				  void *user_data)
{
	struct k_thread *thread;
	k_spinlock_key_t key;

	__ASSERT(user_cb != NULL, "user_cb can not be NULL");
	__ASSERT(cpu < CONFIG_MP_MAX_NUM_CPUS, "cpu filter out of bounds");

	/*
	 * Lock is needed to make sure that the _kernel.threads is not being
	 * modified by the user_cb either directly or indirectly.
	 * The indirect ways are through calling k_thread_create and
	 * k_thread_abort from user_cb.
	 */
	key = k_spin_lock(&z_thread_monitor_lock);

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, foreach);

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		if (thread->base.cpu == cpu) {
			user_cb(thread, user_data);
		}
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, foreach);

	k_spin_unlock(&z_thread_monitor_lock, key);
}

void k_thread_foreach_unlocked_filter_by_cpu(unsigned int cpu, k_thread_user_cb_t user_cb,
					     void *user_data)
{
	struct k_thread *thread;
	k_spinlock_key_t key;

	__ASSERT(user_cb != NULL, "user_cb can not be NULL");
	__ASSERT(cpu < CONFIG_MP_MAX_NUM_CPUS, "cpu filter out of bounds");

	key = k_spin_lock(&z_thread_monitor_lock);

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, foreach_unlocked);

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		if (thread->base.cpu == cpu) {
			k_spin_unlock(&z_thread_monitor_lock, key);
			user_cb(thread, user_data);
			key = k_spin_lock(&z_thread_monitor_lock);
		}
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, foreach_unlocked);

	k_spin_unlock(&z_thread_monitor_lock, key);
}
#endif /* CONFIG_SMP */
