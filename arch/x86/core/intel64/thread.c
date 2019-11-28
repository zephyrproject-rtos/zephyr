/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>

extern void x86_sse_init(struct k_thread *); /* in locore.S */

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stack_size, k_thread_entry_t entry,
		     void *parameter1, void *parameter2, void *parameter3,
		     int priority, unsigned int options)
{
#if defined(CONFIG_X86_USERSPACE) || defined(CONFIG_X86_STACK_PROTECTION)
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)stack;
#endif

	Z_ASSERT_VALID_PRIO(priority, entry);
	z_new_thread_init(thread, Z_THREAD_STACK_BUFFER(stack),
			  stack_size, priority, options);

#if CONFIG_X86_STACK_PROTECTION
	/* Set guard area to read-only to catch stack overflows */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->guard_page,
			    MMU_PAGE_SIZE, MMU_ENTRY_READ, Z_X86_MMU_RW,
			    true);
#endif

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
