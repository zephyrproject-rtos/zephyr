/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R k_thread_abort() routine
 *
 * The ARM Cortex-A and Cortex-R architectures provide their own
 * k_thread_abort() to deal with different CPU modes when a thread aborts.
 */

#include <kernel.h>
#include <kswap.h>

extern void z_thread_single_abort(struct k_thread *thread);

void z_impl_k_thread_abort(k_tid_t thread)
{
	__ASSERT(!(thread->base.user_options & K_ESSENTIAL),
		 "essential thread aborted");

	z_thread_single_abort(thread);
	z_thread_monitor_exit(thread);

	/*
	 * Swap context if and only if the thread is not aborted inside an
	 * interrupt/exception handler; it is not necessary to swap context
	 * inside an interrupt/exception handler because the handler swaps
	 * context when exiting.
	 */
	if (!arch_is_in_isr()) {
		if (thread == _current) {
			/* Direct use of swap: reschedule doesn't have a test
			 * for "is _current dead" and we don't want one for
			 * performance reasons.
			 */
			z_swap_unlocked();
		} else {
			z_reschedule_unlocked();
		}
	}
}
