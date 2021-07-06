/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kswap.h>
#include <soc.h>

void z_thread_entry(k_thread_entry_t thread, void *arg1, void *arg2, void *arg3);

extern uint32_t mips_cp0_status_int_mask;

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *arg1, void *arg2, void *arg3)
{
	struct __esf *stack_init;

	/* Initial stack frame for thread */
	stack_init = Z_STACK_PTR_TO_FRAME(struct __esf, stack_ptr);

	thread->callee_saved.sp = (uint32_t)stack_init;

	stack_init->r[3] = (reg_t)entry;
	stack_init->r[4] = (reg_t)arg1;
	stack_init->r[5] = (reg_t)arg2;
	stack_init->r[6] = (reg_t)arg3;
	stack_init->r[28] = (reg_t)stack_init;

	/* FIXME */
	/* EXL is set as status will be written to this value inside an ISR */
	stack_init->status = (reg_t)0x20000003 | mips_cp0_status_int_mask;
	stack_init->epc = (reg_t)z_thread_entry;
}
