/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>

/* forward declaration to asm function to adjust setup the arguments
 * to z_thread_entry() since this arch puts the first four arguments
 * in r4-r7 and not on the stack
 */
void z_thread_entry_wrapper(k_thread_entry_t, void *, void *, void *);

struct init_stack_frame {
	/* top of the stack / most recently pushed */

	/* Used by z_thread_entry_wrapper. pulls these off the stack and
	 * into argument registers before calling z_thread_entry()
	 */
	k_thread_entry_t entry_point;
	void *arg1;
	void *arg2;
	void *arg3;

	/* least recently pushed */
};


void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *arg1, void *arg2, void *arg3)
{
	struct init_stack_frame *iframe;

	/* Initial stack frame data, stored at the base of the stack */
	iframe = Z_STACK_PTR_TO_FRAME(struct init_stack_frame, stack_ptr);

	/* Setup the initial stack frame */
	iframe->entry_point = entry;
	iframe->arg1 = arg1;
	iframe->arg2 = arg2;
	iframe->arg3 = arg3;

	thread->callee_saved.sp = (uint32_t)iframe;
	thread->callee_saved.ra = (uint32_t)z_thread_entry_wrapper;
	thread->callee_saved.key = NIOS2_STATUS_PIE_MSK;
	/* Leave the rest of thread->callee_saved junk */
}
