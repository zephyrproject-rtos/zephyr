/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

void z_thread_entry(k_thread_entry_t thread,
		    void *arg1,
		    void *arg2,
		    void *arg3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	extern void z_openrisc_thread_start(void);

	/* Initial stack frame for thread */
	struct arch_esf *const stack_init = (struct arch_esf *)Z_STACK_PTR_ALIGN(
		Z_STACK_PTR_TO_FRAME(struct arch_esf, stack_ptr));

	/* Setup the initial stack frame */
	stack_init->r3 = (uint32_t)entry;
	stack_init->r4 = (uint32_t)p1;
	stack_init->r5 = (uint32_t)p2;
	stack_init->r6 = (uint32_t)p3;

	stack_init->epcr = (uint32_t)z_thread_entry;

	stack_init->esr = SPR_SR_SM | SPR_SR_IEE | SPR_SR_TEE
#ifdef CONFIG_DCACHE
		| SPR_SR_DCE
#endif
#ifdef CONFIG_ICACHE
		| SPR_SR_ICE
#endif
		;

	thread->callee_saved.r1 = (uint32_t)stack_init;

	/* where to go when returning from z_openrisc_switch() */
	thread->callee_saved.r9 = (uint32_t)z_openrisc_thread_start;

	/* our switch handle is the thread pointer itself */
	thread->switch_handle = thread;
}
