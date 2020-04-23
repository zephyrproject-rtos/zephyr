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

#include <toolchain.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <wait_q.h>

#include "posix_core.h"
#include <arch/posix/posix_soc_if.h>

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

	posix_new_thread(thread_status);
}

void posix_new_thread_pre_start(void)
{
	posix_irq_full_unlock();
}
