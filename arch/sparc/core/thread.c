/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <offsets_short.h>

/**
 * @brief Create a new kernel execution thread
 *
 * Initialize a new thread and create a stack frame which
 * z_arch_siwtch() and ISR can handle, so that we don't need a special
 * handling for new threads.
 *
 * Stack will have
 *  - a standard stack frame, and
 *  - a context frame
 *
 * arch_switch() will restore the context for a new thread from the
 * context frame, and leave only the standard frame on the stack.  PC
 * is set to z_thread_entry() instead of z_thread_entry_wrapper()
 * unlike other arches because we can directly jump with "rett" to the
 * beginning of a function, z_thread_entry() in this case, without any
 * wrapper.
 *
 * @param thread pointer to thread struct memory, including any space needed
 *		for extra coprocessor context
 * @param stack the pointer to aligned stack memory
 * @param stack_size the stack size in bytes
 * @param entry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stack_size, k_thread_entry_t entry, void *arg1,
		     void *arg2, void *arg3, int prio, unsigned int options)
{
	char *stack_memory = Z_THREAD_STACK_BUFFER(stack);
	u32_t sp;
	struct __esf *context;

	z_new_thread_init(thread, stack_memory, stack_size, prio, options);

	/* Point to top of stack */
	sp = STACK_ROUND_DOWN(stack_memory + stack_size);
	/* Allocate standard frame. */
	sp = sp - __STD_STACK_FRAME_SIZEOF;

	/* Allocate context frame on stack */
	sp = sp - __z_arch_esf_t_SIZEOF;
	context = (struct __esf *)sp;

	/* Create new thread context which starts from z_thread_entry*/
	context->o0 = (u32_t)entry;
	context->o1 = (u32_t)arg1;
	context->o2 = (u32_t)arg2;
	context->o3 = (u32_t)arg3;

	/* Create special registers */
	context->pc = (u32_t)z_thread_entry;
	context->npc = (u32_t)z_thread_entry + 4;
	context->psr = THREAD_INIT_PSR;

	/* Save context stack pointer */
	thread->switch_handle = context;
}
