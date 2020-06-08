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
		     size_t stack_size, k_thread_entry_t thread_func,
		     void *arg1, void *arg2, void *arg3,
		     int priority, unsigned int options)
{

	char *stack_memory = Z_THREAD_STACK_BUFFER(stack);

	posix_thread_status_t *thread_status;

	z_new_thread_init(thread, stack_memory, stack_size);

	/* We store it in the same place where normal archs store the
	 * "initial stack frame"
	 */
	thread_status = (posix_thread_status_t *)
		Z_STACK_PTR_ALIGN(stack_memory + stack_size
				- sizeof(*thread_status));

	/* z_thread_entry() arguments */
	thread_status->entry_point = thread_func;
	thread_status->arg1 = arg1;
	thread_status->arg2 = arg2;
	thread_status->arg3 = arg3;
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
