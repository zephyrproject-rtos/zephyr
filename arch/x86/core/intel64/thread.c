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

struct x86_initial_frame {
	/* zeroed return address for ABI */
	uint64_t rip;
};

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	void *switch_entry;
	struct x86_initial_frame *iframe;

#if CONFIG_X86_STACK_PROTECTION
	z_x86_set_stack_guard(stack);
#endif
#ifdef CONFIG_USERSPACE
	switch_entry = z_x86_userspace_prepare_thread(thread);
	thread->arch.cs = X86_KERNEL_CS;
	thread->arch.ss = X86_KERNEL_DS;
#else
	switch_entry = z_thread_entry;
#endif
	iframe = Z_STACK_PTR_TO_FRAME(struct x86_initial_frame, stack_ptr);
	iframe->rip = 0;
	thread->callee_saved.rsp = (long) iframe;
	thread->callee_saved.rip = (long) switch_entry;
	thread->callee_saved.rflags = EFLAGS_INITIAL;

	/* Parameters to entry point, which is populated in
	 * thread->callee_saved.rip
	 */
	thread->arch.rdi = (long) entry;
	thread->arch.rsi = (long) p1;
	thread->arch.rdx = (long) p2;
	thread->arch.rcx = (long) p3;

	x86_sse_init(thread);

	thread->arch.flags = X86_THREAD_FLAG_ALL;
	thread->switch_handle = thread;
}
