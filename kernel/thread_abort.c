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
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_self_abort(void)
{
	/* Self-aborting threads don't clean themselves up, we
	 * have the idle thread for the current CPU do it.
	 */
	int key;
	struct _cpu *cpu;

	/* Lock local IRQs to prevent us from migrating to another CPU
	 * while we set this up
	 */
	key = arch_irq_lock();
	cpu = _current_cpu;
	__ASSERT(cpu->pending_abort == NULL, "already have a thread to abort");
	cpu->pending_abort = _current;

	LOG_DBG("%p self-aborting, handle on idle thread %p",
		_current, cpu->idle_thread);

	k_thread_suspend(_current);
	z_swap_irqlock(key);
	__ASSERT(false, "should never get here");
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

#if !defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void z_impl_k_thread_abort(k_tid_t thread)
{
	if (thread == _current && !arch_is_in_isr()) {
		/* Thread is self-exiting, idle thread on this CPU will do
		 * the cleanup
		 */
		z_self_abort();
	}

	z_thread_single_abort(thread);

	if (!arch_is_in_isr()) {
		/* Don't need to do this if we're in an ISR */
		z_reschedule_unlocked();
	}
}
#endif
