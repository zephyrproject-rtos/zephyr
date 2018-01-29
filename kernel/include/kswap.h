/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _KSWAP_H
#define _KSWAP_H

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

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* New style context switching.  _arch_switch() is a lower level
 * primitive that doesn't know about the scheduler or return value.
 * Needed for SMP, where the scheduler requires spinlocking that we
 * don't want to have to do in per-architecture assembly.
 */
static inline unsigned int _Swap(unsigned int key)
{
	struct k_thread *new_thread, *old_thread;
	int ret;

	old_thread = _current;

	_check_stack_sentinel();
	_update_time_slice_before_swap();

	new_thread = _get_next_ready_thread();

	old_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
	old_thread->base.active = 0;
	new_thread->base.active = 1;

	new_thread->base.cpu = _arch_curr_cpu()->id;
#endif

	_current = new_thread;
	_arch_switch(new_thread->switch_handle,
		     &old_thread->switch_handle);

	ret = _current->swap_retval;

	irq_unlock(key);

	return ret;
}

#else /* !CONFIG_USE_SWITCH */

extern unsigned int __swap(unsigned int key);

static inline unsigned int _Swap(unsigned int key)
{
	_check_stack_sentinel();
	_update_time_slice_before_swap();

	return __swap(key);
}
#endif

#endif /* _KSWAP_H */
