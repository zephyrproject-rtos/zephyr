/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KSWAP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSWAP_H_

#include <ksched.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_TIMESLICING
extern void _update_time_slice_before_swap(void);
#else
#define _update_time_slice_before_swap() /**/
#endif

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
 */
static inline int _Swap(unsigned int key)
{
	struct k_thread *new_thread, *old_thread;
	int ret = 0;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_swap(void);
	read_timer_start_of_swap();
#endif

	old_thread = _current;

	_check_stack_sentinel();
	_update_time_slice_before_swap();

#ifdef CONFIG_TRACING
	sys_trace_thread_switched_out();
#endif

	new_thread = _get_next_ready_thread();

	if (new_thread != old_thread) {
		old_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;

		new_thread->base.cpu = _arch_curr_cpu()->id;

		_smp_release_global_lock(new_thread);
#endif

		_current = new_thread;
		_arch_switch(new_thread->switch_handle,
			     &old_thread->switch_handle);

		ret = _current->swap_retval;
	}

#ifdef CONFIG_TRACING
	sys_trace_thread_switched_in();
#endif

	irq_unlock(key);

	return ret;
}

#else /* !CONFIG_USE_SWITCH */

extern int __swap(unsigned int key);

static inline int _Swap(unsigned int key)
{
	int ret;
	_check_stack_sentinel();
	_update_time_slice_before_swap();

#ifdef CONFIG_TRACING
	sys_trace_thread_switched_out();
#endif
	ret = __swap(key);
#ifdef CONFIG_TRACING
	sys_trace_thread_switched_in();
#endif

	return ret;
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
