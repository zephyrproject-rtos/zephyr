/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception management support for IA-32 architecture
 *
 * This module implements assembly routines to manage exceptions (synchronous
 * interrupts) on the Intel IA-32 architecture.  More specifically,
 * exceptions are implemented in this module.  The stubs are invoked when entering
 * and exiting a C exception handler.
 */

#include <zephyr/arch/x86/ia32/asm.h>
#include <zephyr/arch/x86/ia32/arch.h> /* For MK_ISR_NAME */
#include <offsets_short.h>


	/* exports (internal APIs) */

	GTEXT(_exception_enter)
	GTEXT(_kernel_oops_handler)

	/* externs (internal APIs) */
	GTEXT(z_x86_do_kernel_oops)

/**
 *
 * @brief Inform the kernel of an exception
 *
 * This function is called from the exception stub created by nanoCpuExcConnect()
 * to inform the kernel of an exception.  This routine currently does
 * _not_ increment a thread/interrupt specific exception count.  Also,
 * execution of the exception handler occurs on the current stack, i.e.
 * this does not switch to another stack.  The volatile integer
 * registers are saved on the stack, and control is returned back to the
 * exception stub.
 *
 * WARNINGS
 *
 * Host-based tools and the target-based GDB agent depend on the stack frame
 * created by this routine to determine the locations of volatile registers.
 * These tools must be updated to reflect any changes to the stack frame.
 *
 * C function prototype:
 *
 * void _exception_enter(uint32_t error_code, void *handler)
 *
 */

SECTION_FUNC(PINNED_TEXT, _exception_enter)

	/*
	 * The gen_idt tool creates an interrupt-gate descriptor for
	 * all connections.  The processor will automatically clear the IF
	 * bit in the EFLAGS register upon execution of the handler, thus
	 * this does need not issue an 'cli' as the first instruction.
	 *
	 * Note that the processor has pushed both the EFLAGS register
	 * and the linear return address (cs:eip) onto the stack prior
	 * to invoking the handler specified in the IDT.
	 *
	 * Clear the direction flag.  It is automatically restored when the
	 * exception exits.
	 */

	cld

#ifdef CONFIG_X86_KPTI
	call z_x86_trampoline_to_kernel
#endif
	/*
	 * Swap ecx and handler function on the current stack;
	 */
	xchgl	%ecx, (%esp)

	/* By the time we get here, the stack should look like this:
	 * ESP -> ECX (excepting task)
	 *	  Exception Error code (or junk)
	 *	  EIP (excepting task)
	 *	  CS (excepting task)
	 *	  EFLAGS (excepting task)
	 *	  ...
	 *
	 * ECX now contains the address of the handler function */

	/*
	 * Push the remaining volatile registers on the existing stack.
	 */

	pushl	%eax
	pushl	%edx

	/*
	 * Push the cooperative registers on the existing stack as they are
	 * required by debug tools.
	 */

	pushl	%edi
	pushl	%esi
	pushl	%ebx
	pushl	%ebp

#ifdef CONFIG_USERSPACE
	/* Test if interrupted context was in ring 3 */
	testb	$3, 36(%esp)
	jz 1f
	/* It was. The original stack pointer is on the stack 44 bytes
	 * from the current top
	 */
	pushl	44(%esp)
	jmp 2f
1:
#endif
	leal	44(%esp), %eax   /* Calculate ESP before interrupt occurred */
	pushl	%eax             /* Save calculated ESP */
#ifdef CONFIG_USERSPACE
2:
#endif

#ifdef CONFIG_GDBSTUB
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushl %ss
#endif
	/* ESP is pointing to the ESF at this point */

#if defined(CONFIG_LAZY_FPU_SHARING)

	movl	_kernel + _kernel_offset_to_current, %edx

	/* inc exception nest count */
	incl	_thread_offset_to_excNestCount(%edx)

	/*
	 * Set X86_THREAD_FLAG_EXC in the current thread. This enables
	 * z_swap() to preserve the thread's FP registers (where needed)
	 * if the exception handler causes a context switch. It also
	 * indicates to debug tools that an exception is being handled
	 * in the event of a context switch.
	 */

	orb	$X86_THREAD_FLAG_EXC, _thread_offset_to_flags(%edx)

#endif /* CONFIG_LAZY_FPU_SHARING */

	/*
	 * restore interrupt enable state, then call the handler
	 *
	 * interrupts are enabled only if they were allowed at the time
	 * the exception was triggered -- this protects kernel level code
	 * that mustn't be interrupted
	 *
	 * Test IF bit of saved EFLAGS and re-enable interrupts if IF=1.
	 */

	/* ESP is still pointing to the ESF at this point */

	testl	$0x200, __struct_arch_esf_eflags_OFFSET(%esp)
	je	allDone
	sti

allDone:
	pushl	%esp			/* push struct_arch_esf * parameter */
	call	*%ecx			/* call exception handler */
	addl	$0x4, %esp

#if defined(CONFIG_LAZY_FPU_SHARING)

	movl	_kernel + _kernel_offset_to_current, %ecx

	/*
	 * Must lock interrupts to prevent outside interference.
	 * (Using "lock" prefix would be nicer, but this won't work
	 * on platforms that don't respect the CPU's bus lock signal.)
	 */

	cli

	/*
	 * Determine whether exiting from a nested interrupt.
	 */

	decl	_thread_offset_to_excNestCount(%ecx)

	cmpl	$0, _thread_offset_to_excNestCount(%ecx)
	jne	nestedException

	/*
	 * Clear X86_THREAD_FLAG_EXC in the k_thread of the current execution
	 * context if we are not in a nested exception (ie, when we exit the
	 * outermost exception).
	 */

	andb	$~X86_THREAD_FLAG_EXC, _thread_offset_to_flags(%ecx)

nestedException:
#endif /* CONFIG_LAZY_FPU_SHARING */

#ifdef CONFIG_GDBSTUB
	popl %ss
	popl %gs
	popl %fs
	popl %es
	popl %ds
#endif
	/*
	 * Pop the non-volatile registers from the stack.
	 * Note that debug tools may have altered the saved register values while
	 * the task was stopped, and we want to pick up the altered values.
	 */

	popl	%ebp		/* Discard saved ESP */
	popl	%ebp
	popl	%ebx
	popl	%esi
	popl	%edi

	/* restore edx and ecx which are always saved on the stack */

	popl	%edx
	popl	%eax
	popl	%ecx

	addl	$4, %esp	/* "pop" error code */

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	KPTI_IRET

SECTION_FUNC(PINNED_TEXT, _kernel_oops_handler)
	push $0 /* dummy error code */
	push $z_x86_do_kernel_oops
	jmp _exception_enter
