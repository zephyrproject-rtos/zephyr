/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel swapper code for POSIX
 *
 * This module implements the z_arch_swap() routine for the POSIX architecture.
 *
 */

#include "kernel.h"
#include <kernel_structs.h>
#include "posix_core.h"
#include "irq.h"
#include "kswap.h"

int z_arch_swap(unsigned int key)
{
	/*
	 * struct k_thread * _kernel.current is the currently runnig thread
	 * struct k_thread * _kernel.ready_q.cache contains the next thread to
	 * run (cannot be NULL)
	 *
	 * Here a "real" arch would save all processor registers, stack pointer
	 * and so forth.  But we do not need to do so because we use posix
	 * threads => those are all nicely kept by the native OS kernel
	 */
	_kernel.current->callee_saved.key = key;
	_kernel.current->callee_saved.retval = -EAGAIN;

	/* retval may be modified with a call to
	 * z_arch_thread_return_value_set()
	 */

	posix_thread_status_t *ready_thread_ptr =
		(posix_thread_status_t *)
		_kernel.ready_q.cache->callee_saved.thread_status;

	posix_thread_status_t *this_thread_ptr  =
		(posix_thread_status_t *)
		_kernel.current->callee_saved.thread_status;


	_kernel.current = _kernel.ready_q.cache;

	/*
	 * Here a "real" arch would load all processor registers for the thread
	 * to run. In this arch case, we just block this thread until allowed
	 * to run later, and signal to whomever is allowed to run to
	 * continue.
	 */
	posix_swap(ready_thread_ptr->thread_idx,
		this_thread_ptr->thread_idx);

	/* When we continue, _kernel->current points back to this thread */

	irq_unlock(_kernel.current->callee_saved.key);

	return _kernel.current->callee_saved.retval;
}



#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
/* This is just a version of z_arch_swap() in which we do not save anything
 * about the current thread.
 *
 * Note that we will never come back to this thread: posix_main_thread_start()
 * does never return.
 */
void z_arch_switch_to_main_thread(struct k_thread *main_thread,
		k_thread_stack_t *main_stack,
		size_t main_stack_size, k_thread_entry_t _main)
{
	posix_thread_status_t *ready_thread_ptr =
			(posix_thread_status_t *)
			_kernel.ready_q.cache->callee_saved.thread_status;

	sys_trace_thread_switched_out();

	_kernel.current = _kernel.ready_q.cache;

	sys_trace_thread_switched_in();

	posix_main_thread_start(ready_thread_ptr->thread_idx);
} /* LCOV_EXCL_LINE */
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
/**
 * If the kernel is in idle mode, take it out
 */
void posix_irq_check_idle_exit(void)
{
	if (_kernel.idle) {
		s32_t idle_val = _kernel.idle;

		_kernel.idle = 0;
		z_sys_power_save_idle_exit(idle_val);
	}
}
#endif
