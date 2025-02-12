/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on arch/riscv/core/thread.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern uint32_t mips_cp0_status_int_mask;

void z_thread_entry(k_thread_entry_t thread,
			    void *arg1,
			    void *arg2,
			    void *arg3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct arch_esf *stack_init;

	/* Initial stack frame for thread */
	stack_init = (struct arch_esf *)Z_STACK_PTR_ALIGN(
				Z_STACK_PTR_TO_FRAME(struct arch_esf, stack_ptr)
				);

	/* Setup the initial stack frame */
	stack_init->a0 = (unsigned long)entry;
	stack_init->a1 = (unsigned long)p1;
	stack_init->a2 = (unsigned long)p2;
	stack_init->a3 = (unsigned long)p3;

	stack_init->status = CP0_STATUS_DEF_RESTORE
			| mips_cp0_status_int_mask;

	stack_init->epc = (unsigned long)z_thread_entry;

	thread->callee_saved.sp = (unsigned long)stack_init;
}
