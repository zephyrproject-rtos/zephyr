/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>

extern void x86_sse_init(struct k_thread *); /* in locore.S */

void z_arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		       size_t stack_size, k_thread_entry_t entry,
		       void *parameter1, void *parameter2, void *parameter3,
		       int priority, unsigned int options)
{
	Z_ASSERT_VALID_PRIO(priority, entry);
	z_new_thread_init(thread, Z_THREAD_STACK_BUFFER(stack),
			  stack_size, priority, options);

	thread->callee_saved.rsp = (long) Z_THREAD_STACK_BUFFER(stack);
	thread->callee_saved.rsp += (stack_size - 8); /* fake RIP for ABI */
	thread->callee_saved.rip = (long) z_thread_entry;
	thread->callee_saved.rflags = EFLAGS_INITIAL;

	thread->arch.rdi = (long) entry;
	thread->arch.rsi = (long) parameter1;
	thread->arch.rdx = (long) parameter2;
	thread->arch.rcx = (long) parameter3;

	x86_sse_init(thread);

	thread->arch.flags = X86_THREAD_FLAG_ALL;
	thread->switch_handle = thread;
}
