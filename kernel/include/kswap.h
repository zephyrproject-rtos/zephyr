/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KSWAP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSWAP_H_

#include <kspinlock.h>
#include <ksched.h>
#include <ksmp.h>
#include <run_q.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <kernel_arch_func.h>
#include <timeslicing.h>
#include <usage.h>

#ifdef CONFIG_STACK_SENTINEL
extern void z_check_stack_sentinel(void);
#else
#define z_check_stack_sentinel() /**/
#endif /* CONFIG_STACK_SENTINEL */

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* New style context switching.  arch_switch() is a lower level
 * primitive that doesn't know about the scheduler or return value.
 * Needed for SMP, where the scheduler requires spinlocking that we
 * don't want to have to do in per-architecture assembly.
 *
 * Note that is_spinlock is a compile-time construct which will be
 * optimized out when this function is expanded.
 */
static ALWAYS_INLINE unsigned int do_swap(unsigned int key,
					  struct k_spinlock *lock,
					  bool is_spinlock)
{
	struct k_thread *new_thread, *old_thread;

#ifdef CONFIG_SPIN_VALIDATE
	z_assert_can_swap(key, lock);
#endif /* CONFIG_SPIN_VALIDATE */

	old_thread = _current;

	z_check_stack_sentinel();

	old_thread->swap_retval = -EAGAIN;

	/* We always take the scheduler spinlock if we don't already
	 * have it.  We "release" other spinlocks here.  But we never
	 * drop the interrupt lock.
	 */
	if (is_spinlock && (lock != NULL) && !z_is_sched_spinlock(lock)) {
		k_spin_release(lock);
	}
	if (IS_ENABLED(CONFIG_SMP) || IS_ENABLED(CONFIG_SPIN_VALIDATE)) {
		/* Taking a nested uniprocessor lock in void context is a noop */
		if (!is_spinlock || !z_is_sched_spinlock(lock)) {
			(void)z_sched_spinlock_lock();
		}
	}

#ifdef CONFIG_SMP
	new_thread = z_swap_next_thread();
#else
	new_thread = _kernel.ready_q.cache;
#endif

	if (new_thread != old_thread) {
		z_sched_usage_switch(new_thread);
		z_smp_swap_prepare(new_thread, !is_spinlock);
		z_thread_mark_switched_out();
		z_sched_switch_spin(new_thread);
		z_current_thread_set(new_thread);

#ifdef CONFIG_TIMESLICING
		z_time_slice_reset(new_thread);
#endif /* CONFIG_TIMESLICING */

		z_sched_spinlock_transfer_owner(new_thread);

		arch_cohere_stacks(old_thread, NULL, new_thread);
		z_smp_switch_requeue(old_thread);

		void *newsh = new_thread->switch_handle;

		z_smp_switch_handle_clear(new_thread);
		z_sched_spinlock_release();
		arch_switch(newsh, &old_thread->switch_handle);
	} else {
		z_sched_spinlock_release();
	}

	if (is_spinlock) {
		arch_irq_unlock(key);
	} else {
		irq_unlock(key);
	}

	return _current->swap_retval;
}

static inline int z_swap_irqlock(unsigned int key)
{
	return do_swap(key, NULL, false);
}

static inline int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	return do_swap(key.key, lock, true);
}

static inline void z_swap_unlocked(void)
{
	(void) do_swap(arch_irq_lock(), NULL, true);
}

#else /* !CONFIG_USE_SWITCH */

static inline int z_swap_irqlock(unsigned int key)
{
	int ret;
	z_check_stack_sentinel();

#ifdef CONFIG_SPIN_VALIDATE
	z_assert_can_swap(key, NULL);
#endif /* CONFIG_SPIN_VALIDATE */

	ret = arch_swap(key);
	return ret;
}

/* If !USE_SWITCH, then spinlocks are guaranteed degenerate as we
 * can't be in SMP.  The k_spin_release() call is just for validation
 * handling.
 */
static ALWAYS_INLINE int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	k_spin_release(lock);
	return z_swap_irqlock(key.key);
}

static inline void z_swap_unlocked(void)
{
	(void) z_swap_irqlock(arch_irq_lock());
}

#endif /* !CONFIG_USE_SWITCH */

/**
 * @brief - Like z_swap() but it is known that the scheduler's spinlock is already held.
 */
static inline int z_swap_locked(k_spinlock_key_t key)
{
	return z_swap(&_sched_spinlock, key);
}

/**
 * Set up a "dummy" thread, used at early initialization to launch the
 * first thread on a CPU.
 *
 * Needs to set enough fields such that the context switching code can
 * use it to properly store state, which will just be discarded.
 *
 * The memory of the dummy thread can be completely uninitialized.
 */
void z_dummy_thread_init(struct k_thread *dummy_thread);

#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
