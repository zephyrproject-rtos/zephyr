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

#include <stdio.h>
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

int arch_thread_name_set(struct k_thread *thread, const char *str)
{
#define MAX_HOST_THREAD_NAME 16

	int ret;
	int thread_index;
	posix_thread_status_t *thread_status;
	char th_name[MAX_HOST_THREAD_NAME];

	thread_status = thread->callee_saved.thread_status;
	if (!thread_status) {
		return -EAGAIN;
	}

	thread_index = thread_status->thread_idx;

	if (!str) {
		return -EAGAIN;
	}

	snprintf(th_name, MAX_HOST_THREAD_NAME,
	#if (CONFIG_NATIVE_SIMULATOR_NUMBER_MCUS > 1)
			STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) ":"
	#endif
		"%s", str);

	ret = posix_arch_thread_name_set(thread_index, th_name);
	if (ret) {
		return -EAGAIN;
	}

	return 0;
}

void posix_arch_thread_entry(void *pa_thread_status)
{
	posix_thread_status_t *ptr = pa_thread_status;
	posix_irq_full_unlock();
	z_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	/* Posix always has FPU enabled so cannot be disabled */
	return -ENOTSUP;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(options);

	/* Posix always has FPU enabled so nothing to do here */
	return 0;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

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

	if (arch_current_thread() == thread) {
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
