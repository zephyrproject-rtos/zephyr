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

#include <kernel_structs.h>
#include <arch/x86/asm.h>
#include <arch/x86/arch.h> /* For MK_ISR_NAME */
#include <offsets_short.h>


	/* exports (internal APIs) */

	GTEXT(_exception_enter)

	/* externs (internal APIs) */
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
 * @return N/A
 *
 * C function prototype:
 *
 * void _exception_enter(uint32_t error_code, void *handler)
 *
 */

SECTION_FUNC(TEXT, _exception_enter)

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

	leal	44(%esp), %eax   /* Calculate ESP before interrupt occurred */
	pushl	%eax             /* Save calculated ESP */

	/* ESP is pointing to the ESF at this point */

#if defined(CONFIG_FP_SHARING) ||  defined(CONFIG_GDB_INFO)

	movl	_kernel + _kernel_offset_to_current, %edx

	/* inc exception nest count */
	incl	_thread_offset_to_excNestCount(%edx)

#ifdef CONFIG_GDB_INFO

    /*
     * Save the pointer to the stack frame (NANO_ESF *) in
     * the current execution context if this is the outermost exception.
     * The ESF pointer is used by debug tools to locate the volatile
     * registers and the stack of the preempted thread.
     */

	testb	$_EXC_ACTIVE, _thread_offset_to_thread_state(%edx)
	jne	alreadyInException
	movl	%esp, _thread_offset_to_esf(%edx)

alreadyInException:

#endif /* CONFIG_GDB_INFO */

	/*
	 * Set the _EXC_ACTIVE state bit of the current thread.
	 * This enables _Swap() to preserve the thread's FP registers
	 * (where needed) if the exception handler causes a context switch.
	 * It also indicates to debug tools that an exception is being
	 * handled in the event of a context switch.
	 */

	orb	$_EXC_ACTIVE, _thread_offset_to_thread_state(%edx)

#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

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

	testl	$0x200, __NANO_ESF_eflags_OFFSET(%esp)
	je	allDone
	sti

allDone:
#if CONFIG_X86_IAMCU
	movl	%esp, %eax		/* NANO_ESF * parameter */
#else
	pushl	%esp			/* push NANO_ESF * parameter */
#endif
	call	*%ecx			/* call exception handler */

#ifndef CONFIG_X86_IAMCU
	addl	$0x4, %esp
#endif

#if defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO)

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
	 * Clear the _EXC_ACTIVE bit in the k_thread of the current execution
	 * context if we are not in a nested exception (ie, when we exit the
	 * outermost exception).
	 */

	andb	$~_EXC_ACTIVE, _thread_offset_to_thread_state(%ecx)

nestedException:
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

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
	iret

