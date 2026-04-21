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
#include <wait_q.h>
#include <scheduler.h>


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


int z_sched_waitq_walk(_wait_q_t *wait_q, _waitq_walk_cb_t walk_func,
		       _waitq_post_walk_cb_t post_func, void *data)
{
	struct k_thread *thread;
	int  status = 0;

	K_SPINLOCK(&_sched_spinlock) {
#ifndef CONFIG_WAITQ_SCALABLE
		struct k_thread *tmp;

		_WAIT_Q_FOR_EACH_SAFE(wait_q, thread, tmp)
#else /* !CONFIG_WAITQ_SCALABLE */
		_WAIT_Q_FOR_EACH(wait_q, thread)
#endif /* !CONFIG_WAITQ_SCALABLE */
		{

			/*
			 * Invoke the callback function on each waiting thread
			 * for as long as there are both waiting threads AND
			 * it returns 0.
			 */

			status = walk_func(thread, data);
			if (status != 0) {
				break;
			}
		}

		/*
		 * Invoke post-walk callback. This is done while
		 * still holding _sched_spinlock to enable atomic
		 * operations (from the scheduler's point of view).
		 */
		if (post_func != NULL) {
			post_func(status, data);
		}
	}

	return status;
}


/*
 * future scheduler.h API implementations
 */
bool z_sched_wake(_wait_q_t *wait_q, int swap_retval, void *swap_data)
{
	struct k_thread *thread;
	bool ret = false;

	K_SPINLOCK(&_sched_spinlock) {
		thread = _priq_wait_best(&wait_q->waitq);

		if (thread != NULL) {
			z_thread_return_value_set_with_data(thread,
							    swap_retval,
							    swap_data);
			unpend_thread_no_timeout(thread);
			z_abort_thread_timeout(thread);
			z_sched_ready_locked(thread);
			ret = true;
		}
	}

	return ret;
}

int z_sched_wait(struct k_spinlock *lock, k_spinlock_key_t key,
		 _wait_q_t *wait_q, k_timeout_t timeout, void **data)
{
	int ret = z_pend_curr(lock, key, wait_q, timeout);

	if (data != NULL) {
		*data = _current->base.swap_data;
	}
	return ret;
}
