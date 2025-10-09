/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_arch_func.h>
#include <string.h>

#define DSPIC_STATUS_DEFAULT 0

void z_thread_entry(k_thread_entry_t thread, void *arg1, void *arg2, void *arg3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	(void)stack;
	struct arch_esf *init_frame;

	/* Initial stack frame for thread */
	init_frame = (struct arch_esf *)Z_STACK_PTR_ALIGN(
		Z_STACK_PTR_TO_FRAME(struct arch_esf, stack_ptr));
	/* Setting all register to zero*/
	(void)memset(init_frame, 0, sizeof(struct arch_esf));

	/* Setup initial stack frame
	 * During the initial thread entry the w0 serves the function of thread
	 * entry function pointer, but the general usage of w0 as per the C ABI
	 * is to pass back the return data. Here set return value as entry pointer
	 * value
	 */
	arch_thread_return_value_set(thread, (unsigned int)entry);
	init_frame->W0 = (uint32_t)entry;
	init_frame->W1 = (uint32_t)(void *)p1;
	init_frame->W2 = (uint32_t)(void *)p2;
	init_frame->W3 = (uint32_t)(void *)p3;

	/* Initial cpu status register with default value*/
	init_frame->FSR = DSPIC_STATUS_DEFAULT;

	/**
	 * set the pc to the zephyr common thread entry function
	 * Z_thread_entry(entry, p1, p2, p3)
	 */
	init_frame->PC = (uint32_t)z_thread_entry;
	/* FRAME pointer used as LR for initial swap function
	 * In swap function we enter with one SP and exits with another SP
	 * As the swap function is a naked function, it won't use FP but use that
	 * place for LR (for the 1st swap, we need to populate initial LR in FP)
	 */
	init_frame->FRAME = (uint32_t)(void *)init_frame + (sizeof(struct arch_esf));

	/**
	 * Set the stack top to the esf structure. The context switch
	 * code will be using this field to set the stack pointer of
	 * the switched thread
	 */
	thread->callee_saved.stack = (uint32_t)(void *)init_frame + (sizeof(struct arch_esf));
	thread->callee_saved.frame = (uint32_t)thread->callee_saved.stack;
	thread->callee_saved.splim = (uint32_t)(thread->stack_info.start + thread->stack_info.size);
	/*Set the initial key for irq unlock*/
	thread->arch.cpu_level = 1;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	(void)thread;
	return -ENOTSUP;
}
