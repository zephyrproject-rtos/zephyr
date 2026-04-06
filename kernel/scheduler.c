/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/spinlock.h>
#include <zephyr/internal/syscall_handler.h>
#include <run_q.h>


void z_sched_init(void)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		_priq_run_init(&_kernel.cpus[i].ready_q.runq);
	}
#else
	_priq_run_init(&_kernel.ready_q.runq);
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}

void k_sched_lock(void)
{
	K_SPINLOCK(&_sched_spinlock) {
		SYS_PORT_TRACING_FUNC(k_thread, sched_lock);

		__ASSERT(!arch_is_in_isr(), "");
		__ASSERT(_current->base.sched_locked != 1U, "");

		--_current->base.sched_locked;

		compiler_barrier();
	}
}

void k_sched_unlock(void)
{
	SYS_PORT_TRACING_FUNC(k_thread, sched_unlock);

	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	__ASSERT(_current->base.sched_locked != 0U, "");
	__ASSERT(!arch_is_in_isr(), "");
	++_current->base.sched_locked;
	z_sched_lock_reschedule(key);
}

void z_impl_k_reschedule(void)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&_sched_spinlock);

	z_sched_lock_reschedule(key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_reschedule(void)
{
	z_impl_k_reschedule();
}
#include <zephyr/syscalls/k_reschedule_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_tid_t z_impl_k_sched_current_thread_query(void)
{
	return _current;
}

#ifdef CONFIG_USERSPACE
static inline k_tid_t z_vrfy_k_sched_current_thread_query(void)
{
	return z_impl_k_sched_current_thread_query();
}
#include <zephyr/syscalls/k_sched_current_thread_query_mrsh.c>
#endif /* CONFIG_USERSPACE */
