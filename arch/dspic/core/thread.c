/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#define RAM_END 0x00007FFC

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

	/* Setup initial stack frame*/
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
	init_frame->FRAME = (uint32_t)(void *)init_frame - (uint32_t)1;

	/**
	 * Set the stack top to the esf structure. The context switch
	 * code will be using this field to set the stack pointer of
	 * the switched thread
	 */
	thread->callee_saved.stack = (uint32_t)(void *)init_frame + (sizeof(struct arch_esf));
	thread->callee_saved.frame = (uint32_t)&init_frame->RCOUNT;
	/*TODO: Need to handle splim properly*/
	thread->callee_saved.splim = RAM_END;
}
