/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KSWAP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSWAP_H_

#include <ksched.h>
#include <spinlock.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_STACK_SENTINEL
extern void z_check_stack_sentinel(void);
#else
#define z_check_stack_sentinel() /**/
#endif

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either z_swap() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void z_smp_reacquire_global_lock(struct k_thread *thread);
void z_smp_release_global_lock(struct k_thread *thread);

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* New style context switching.  z_arch_switch() is a lower level
 * primitive that doesn't know about the scheduler or return value.
 * Needed for SMP, where the scheduler requires spinlocking that we
 * don't want to have to do in per-architecture assembly.
 *
 * Note that is_spinlock is a compile-time construct which will be
 * optimized out when this function is expanded.
 */
static ALWAYS_INLINE unsigned int do_swap(unsigned int key,
					  struct k_spinlock *lock,
					  int is_spinlock)
{
	ARG_UNUSED(lock);
	struct k_thread *new_thread, *old_thread;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_swap(void);
	read_timer_start_of_swap();
#endif

	old_thread = _current;

	z_check_stack_sentinel();

	sys_trace_thread_switched_out();

	if (is_spinlock) {
		k_spin_release(lock);
	}

	new_thread = z_get_next_ready_thread();

	if (new_thread != old_thread) {
#ifdef CONFIG_TIMESLICING
		z_reset_time_slice();
#endif

		old_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;

		new_thread->base.cpu = z_arch_curr_cpu()->id;

		if (!is_spinlock) {
			z_smp_release_global_lock(new_thread);
		}
#endif
		_current = new_thread;
		z_arch_switch(new_thread->switch_handle,
			     &old_thread->switch_handle);
	}

	sys_trace_thread_switched_in();

	if (is_spinlock) {
		z_arch_irq_unlock(key);
	} else {
		irq_unlock(key);
	}

	return _current->swap_retval;
}

static inline int z_swap_irqlock(unsigned int key)
{
	return do_swap(key, NULL, 0);
}

static inline int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	return do_swap(key.key, lock, 1);
}

static inline void z_swap_unlocked(void)
{
	struct k_spinlock lock = {};
	k_spinlock_key_t key = k_spin_lock(&lock);

	(void) z_swap(&lock, key);
}

#else /* !CONFIG_USE_SWITCH */

extern int z_arch_swap(unsigned int key);

static inline int z_swap_irqlock(unsigned int key)
{
	int ret;
	z_check_stack_sentinel();

#ifndef CONFIG_ARM
	sys_trace_thread_switched_out();
#endif
	ret = z_arch_swap(key);
#ifndef CONFIG_ARM
	sys_trace_thread_switched_in();
#endif

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
	(void) z_swap_irqlock(z_arch_irq_lock());
}

#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
