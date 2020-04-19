/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <offsets_short.h>

extern void x86_sse_init(struct k_thread *); /* in locore.S */

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stack_size, k_thread_entry_t entry,
		     void *parameter1, void *parameter2, void *parameter3,
		     int priority, unsigned int options)
{
	void *switch_entry;

	z_new_thread_init(thread, Z_THREAD_STACK_BUFFER(stack), stack_size);

#if CONFIG_X86_STACK_PROTECTION
	struct z_x86_thread_stack_header *header =
		(struct z_x86_thread_stack_header *)stack;

	/* Set guard area to read-only to catch stack overflows */
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, &header->guard_page,
			    MMU_PAGE_SIZE, MMU_ENTRY_READ, Z_X86_MMU_RW,
			    true);
#endif
#ifdef CONFIG_USERSPACE
	switch_entry = z_x86_userspace_prepare_thread(thread);
	thread->arch.cs = X86_KERNEL_CS;
	thread->arch.ss = X86_KERNEL_DS;
#else
	switch_entry = z_thread_entry;
#endif
	thread->callee_saved.rsp = (long) Z_THREAD_STACK_BUFFER(stack);
	thread->callee_saved.rsp += (stack_size - 8); /* fake RIP for ABI */
	thread->callee_saved.rip = (long) switch_entry;
	thread->callee_saved.rflags = EFLAGS_INITIAL;

	/* Parameters to entry point, which is populated in
	 * thread->callee_saved.rip
	 */
	thread->arch.rdi = (long) entry;
	thread->arch.rsi = (long) parameter1;
	thread->arch.rdx = (long) parameter2;
	thread->arch.rcx = (long) parameter3;

	x86_sse_init(thread);

	thread->arch.flags = X86_THREAD_FLAG_ALL;
	thread->switch_handle = thread;
}
