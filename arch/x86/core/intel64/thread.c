/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <offsets_short.h>
#include <x86_mmu.h>

extern void x86_sse_init(struct k_thread *thread); /* in locore.S */

/* FIXME: This exists to make space for a "return address" at the top
 * of the stack.  Obviously this is unused at runtime, but is required
 * for alignment: stacks at runtime should be 16-byte aligned, and a
 * CALL will therefore push a return address that leaves the stack
 * misaligned.  Effectively we're wasting 8 bytes here to undo (!) the
 * alignment that the upper level code already tried to do for us.  We
 * should clean this up.
 */
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

#if defined(CONFIG_X86_STACK_PROTECTION) && !defined(CONFIG_THREAD_STACK_MEM_MAPPED)
	/* This unconditionally set the first page of stack as guard page,
	 * which is only needed if the stack is not memory mapped.
	 */
	z_x86_set_stack_guard(stack);
#else
	ARG_UNUSED(stack);
#endif
#ifdef CONFIG_USERSPACE
	switch_entry = z_x86_userspace_prepare_thread(thread);
	thread->arch.cs = X86_KERNEL_CS;
	thread->arch.ss = X86_KERNEL_DS;
#else
	switch_entry = z_thread_entry;
#endif
	iframe = Z_STACK_PTR_TO_FRAME(struct x86_initial_frame, stack_ptr);
	iframe->rip = 0U;
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

int arch_float_disable(struct k_thread *thread)
{
	/* x86-64 always has FP/SSE enabled so cannot be disabled */
	ARG_UNUSED(thread);

	return -ENOTSUP;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	/* x86-64 always has FP/SSE enabled so nothing to do here */
	ARG_UNUSED(thread);
	ARG_UNUSED(options);

	return 0;
}
