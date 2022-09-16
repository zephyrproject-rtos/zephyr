/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread support primitives
 *
 * This module provides core thread related primitives for the IA-32
 * processor architecture.
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/arch/x86/mmustructs.h>
#include <kswap.h>
#include <x86_mmu.h>

/* forward declaration */

/* Initial thread stack frame, such that everything is laid out as expected
 * for when z_swap() switches to it for the first time.
 */
struct _x86_initial_frame {
	uint32_t swap_retval;
	uint32_t ebp;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	void *thread_entry;
	uint32_t eflags;
	k_thread_entry_t entry;
	void *p1;
	void *p2;
	void *p3;
};

#ifdef CONFIG_X86_USERSPACE
/* Implemented in userspace.S */
extern void z_x86_syscall_entry_stub(void);

/* Syscalls invoked by 'int 0x80'. Installed in the IDT at DPL=3 so that
 * userspace can invoke it.
 */
NANO_CPU_INT_REGISTER(z_x86_syscall_entry_stub, -1, -1, 0x80, 3);
#endif /* CONFIG_X86_USERSPACE */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)

extern int z_float_disable(struct k_thread *thread);

int arch_float_disable(struct k_thread *thread)
{
#if defined(CONFIG_LAZY_FPU_SHARING)
	return z_float_disable(thread);
#else
	return -ENOTSUP;
#endif /* CONFIG_LAZY_FPU_SHARING */
}

extern int z_float_enable(struct k_thread *thread, unsigned int options);

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
#if defined(CONFIG_LAZY_FPU_SHARING)
	return z_float_enable(thread, options);
#else
	return -ENOTSUP;
#endif /* CONFIG_LAZY_FPU_SHARING */
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	void *swap_entry;
	struct _x86_initial_frame *initial_frame;

#if CONFIG_X86_STACK_PROTECTION
	z_x86_set_stack_guard(stack);
#endif

#ifdef CONFIG_USERSPACE
	swap_entry = z_x86_userspace_prepare_thread(thread);
#else
	swap_entry = z_thread_entry;
#endif

	/* Create an initial context on the stack expected by z_swap() */
	initial_frame = Z_STACK_PTR_TO_FRAME(struct _x86_initial_frame,
					     stack_ptr);

	/* z_thread_entry() arguments */
	initial_frame->entry = entry;
	initial_frame->p1 = p1;
	initial_frame->p2 = p2;
	initial_frame->p3 = p3;
	initial_frame->eflags = EFLAGS_INITIAL;
#ifdef _THREAD_WRAPPER_REQUIRED
	initial_frame->edi = (uint32_t)swap_entry;
	initial_frame->thread_entry = z_x86_thread_entry_wrapper;
#else
	initial_frame->thread_entry = swap_entry;
#endif /* _THREAD_WRAPPER_REQUIRED */

	/* Remaining _x86_initial_frame members can be garbage, z_thread_entry()
	 * doesn't care about their state when execution begins
	 */
	thread->callee_saved.esp = (unsigned long)initial_frame;
#if defined(CONFIG_LAZY_FPU_SHARING)
	thread->arch.excNestCount = 0;
#endif /* CONFIG_LAZY_FPU_SHARING */
	thread->arch.flags = 0;

	/*
	 * When "eager FPU sharing" mode is enabled, FPU registers must be
	 * initialised at the time of thread creation because the floating-point
	 * context is always active and no further FPU initialisation is performed
	 * later.
	 */
#if defined(CONFIG_EAGER_FPU_SHARING)
	thread->arch.preempFloatReg.floatRegsUnion.fpRegs.fcw = 0x037f;
	thread->arch.preempFloatReg.floatRegsUnion.fpRegs.ftw = 0xffff;
#if defined(CONFIG_X86_SSE)
	thread->arch.preempFloatReg.floatRegsUnion.fpRegsEx.mxcsr = 0x1f80;
#endif /* CONFIG_X86_SSE */
#endif /* CONFIG_EAGER_FPU_SHARING */
}
