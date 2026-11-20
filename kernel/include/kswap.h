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
 * do_swap() is inlined in each caller to resolve the lock handling at
 * compile time, as is_spinlock and often lock are constants. The rest
 * is done by z_swap_switch(), called with the scheduler spinlock held.
 * release_global_lock tells z_swap_switch() to release the SMP global
 * irq_lock() of the outgoing thread.
 */
#ifdef CONFIG_SMP
int z_swap_switch(unsigned int key, bool release_global_lock);
#else
/* Uniprocessor builds have no global lock to release. Dropping the
 * argument saves each caller from setting it up.
 */
int z_swap_switch_up(unsigned int key);
#define z_swap_switch(key, release_global_lock) z_swap_switch_up(key)
#endif

static ALWAYS_INLINE int do_swap(unsigned int key,
				 struct k_spinlock *lock,
				 bool is_spinlock)
{
#ifdef CONFIG_SPIN_VALIDATE
	z_assert_can_swap(key, lock);
#endif /* CONFIG_SPIN_VALIDATE */

	/* A sentinel failure aborts the thread, which takes the scheduler
	 * spinlock, so check before taking it.
	 */
	z_check_stack_sentinel();

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

	return z_swap_switch(key, !is_spinlock);
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
