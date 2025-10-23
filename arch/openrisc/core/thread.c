/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static const uint32_t sr_init = SPR_SR_SM | SPR_SR_IEE | SPR_SR_TEE
#ifdef CONFIG_DCACHE
	| SPR_SR_DCE
#endif
#ifdef CONFIG_ICACHE
	| SPR_SR_ICE
#endif
	;

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

	stack_init->esr = sr_init;
	thread->callee_saved.r1 = (uint32_t)stack_init;

	/* where to go when returning from z_openrisc_switch() */
	thread->callee_saved.r9 = (uint32_t)z_openrisc_thread_start;

	/* our switch handle is the thread pointer itself */
	thread->switch_handle = thread;
}

#ifndef CONFIG_MULTITHREADING

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS, CONFIG_ISR_STACK_SIZE);
K_THREAD_STACK_DECLARE(z_main_stack, CONFIG_MAIN_STACK_SIZE);

FUNC_NORETURN void z_openrisc_switch_to_main_no_multithreading(k_thread_entry_t main_entry,
							       void *p1, void *p2, void *p3)
{
	void *main_stack;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	_kernel.cpus[0].id = 0;
	_kernel.cpus[0].irq_stack = (K_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]) +
				     K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));

	main_stack = (K_THREAD_STACK_BUFFER(z_main_stack) +
		      K_THREAD_STACK_SIZEOF(z_main_stack));

	openrisc_write_spr(SPR_SR, sr_init);

	__asm__ volatile (
		"l.ori r1, %0, 0\n"
		"l.jalr %1\n"
		:
		: "r" (main_stack), "r" (main_entry));

	/* infinite loop */
	irq_lock();
	while (true) {
	}

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
#endif /* !CONFIG_MULTITHREADING */
