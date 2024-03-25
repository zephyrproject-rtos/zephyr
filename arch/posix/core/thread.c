/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread support primitives
 *
 * This module provides core thread related primitives for the POSIX
 * architecture
 */

#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>

#include "posix_core.h"
#include <zephyr/arch/posix/posix_soc_if.h>

#ifdef CONFIG_TRACING
#include <zephyr/tracing/tracing_macros.h>
#include <zephyr/tracing/tracing.h>
#endif

/* Note that in this arch we cheat quite a bit: we use as stack a normal
 * pthreads stack and therefore we ignore the stack size
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{

	posix_thread_status_t *thread_status;

	/* We store it in the same place where normal archs store the
	 * "initial stack frame"
	 */
	thread_status = Z_STACK_PTR_TO_FRAME(posix_thread_status_t, stack_ptr);

	/* z_thread_entry() arguments */
	thread_status->entry_point = entry;
	thread_status->arg1 = p1;
	thread_status->arg2 = p2;
	thread_status->arg3 = p3;
#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)
	thread_status->aborted = 0;
#endif

	thread->callee_saved.thread_status = thread_status;

	thread_status->thread_idx = posix_new_thread((void *)thread_status);
}

void posix_arch_thread_entry(void *pa_thread_status)
{
	posix_thread_status_t *ptr = pa_thread_status;
	posix_irq_full_unlock();
	z_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);
}

#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void z_impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;
	int thread_idx;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, abort, thread);

	posix_thread_status_t *tstatus =
					(posix_thread_status_t *)
					thread->callee_saved.thread_status;

	thread_idx = tstatus->thread_idx;

	key = irq_lock();

	if (_current == thread) {
		if (tstatus->aborted == 0) { /* LCOV_EXCL_BR_LINE */
			tstatus->aborted = 1;
		} else {
			posix_print_warning(/* LCOV_EXCL_LINE */
				"POSIX arch: The kernel is trying to abort and swap "
				"out of an already aborted thread %i. This "
				"should NOT have happened\n",
				thread_idx);
		}
		posix_abort_thread(thread_idx);
	}

	z_thread_abort(thread);

	if (tstatus->aborted == 0) {
		PC_DEBUG("%s aborting now [%i] %i\n",
			__func__,
			posix_arch_get_unique_thread_id(thread_idx),
			thread_idx);

		tstatus->aborted = 1;
		posix_abort_thread(thread_idx);
	} else {
		PC_DEBUG("%s ignoring re_abort of [%i] "
			"%i\n",
			__func__,
			posix_arch_get_unique_thread_id(thread_idx),
			thread_idx);
	}

	/* The abort handler might have altered the ready queue. */
	z_reschedule_irqlock(key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, abort, thread);
}
#endif
