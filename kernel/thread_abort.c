/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Primitive for aborting a thread when an arch-specific one is not
 * needed..
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <string.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <ksched.h>
#include <sys/__assert.h>
#include <syscall_handler.h>

extern void z_thread_single_abort(struct k_thread *thread);

#if !defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void z_impl_k_thread_abort(k_tid_t thread)
{
	__ASSERT((thread->base.user_options & K_ESSENTIAL) == 0U,
		 "essential thread aborted");

	z_thread_single_abort(thread);
	z_thread_monitor_exit(thread);

	/* If we're in an interrupt handler, we reschedule on the way out
	 * anyway, nothing needs to be done here.
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
#endif
