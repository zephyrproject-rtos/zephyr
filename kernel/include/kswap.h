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
extern void _check_stack_sentinel(void);
#else
#define _check_stack_sentinel() /**/
#endif

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either _Swap() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void _smp_reacquire_global_lock(struct k_thread *thread);
void _smp_release_global_lock(struct k_thread *thread);

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* New style context switching.  _arch_switch() is a lower level
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

	_check_stack_sentinel();

#ifdef CONFIG_TRACING
	sys_trace_thread_switched_out();
#endif

	if (is_spinlock) {
		k_spin_release(lock);
	}

	new_thread = _get_next_ready_thread();

	if (new_thread != old_thread) {
		old_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;

		new_thread->base.cpu = _arch_curr_cpu()->id;

		if (!is_spinlock) {
			_smp_release_global_lock(new_thread);
		}
#endif
		_current = new_thread;
		_arch_switch(new_thread->switch_handle,
			     &old_thread->switch_handle);
	}

#ifdef CONFIG_TRACING
	sys_trace_thread_switched_in();
#endif

	if (is_spinlock) {
		_arch_irq_unlock(key);
	} else {
		irq_unlock(key);
	}

	return _current->swap_retval;
}

static inline int _Swap_irqlock(unsigned int key)
{
	return do_swap(key, NULL, 0);
}

static inline int _Swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	return do_swap(key.key, lock, 1);
}

#else /* !CONFIG_USE_SWITCH */

extern int __swap(unsigned int key);

static inline int _Swap_irqlock(unsigned int key)
{
	int ret;
	_check_stack_sentinel();

#ifndef CONFIG_ARM
#ifdef CONFIG_TRACING
	sys_trace_thread_switched_out();
#endif
#endif
	ret = __swap(key);
#ifndef CONFIG_ARM
#ifdef CONFIG_TRACING
	sys_trace_thread_switched_in();
#endif
#endif

	return ret;
}

/* If !USE_SWITCH, then spinlocks are guaranteed degenerate as we
 * can't be in SMP.  The k_spin_release() call is just for validation
 * handling.
 */
static ALWAYS_INLINE int _Swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	k_spin_release(lock);
	return _Swap_irqlock(key.key);
}

#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
