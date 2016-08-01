/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define _ASMLANGUAGE

#include <nano_private.h>
#include <arch/x86/asm.h>
#include <arch/x86/arch.h> /* For MK_ISR_NAME */
#include <offsets.h>	/* nanokernel structure offset definitions */



	/* exports (internal APIs) */

	GTEXT(_ExcEnt)
	GTEXT(_ExcEntNoErr)
	GTEXT(_ExcExit)

	/* externs (internal APIs) */

/**
 *
 * @brief Inform the kernel of an exception with no error code
 *
 * This is very similar to _ExcEnt() but the stack is first massaged
 * so that a dummy error code is inserted.
 */
SECTION_FUNC(TEXT, _ExcEntNoErr)
	/* Clear direction flag, auto-restored when the exception exits */
	cld

	/* Stash current value of ECX to free up the register */
	pushl	%ecx

	/* Save the return address of the stub into ECX */
	movl	4(%esp), %ecx

	/*
	 * The spot for the error code contains useless data, but
	 * we don't particularly care since it will be unused.
	 */
	jmp	_ExcEntSetupDone

/**
 *
 * @brief Inform the kernel of an exception
 *
 * This function is called from the exception stub created by nanoCpuExcConnect()
 * to inform the kernel of an exception.  This routine currently does
 * _not_ increment a thread/interrupt specific exception count.  Also,
 * execution of the exception handler occurs on the current stack, i.e.
 * _ExcEnt() does not switch to another stack.  The volatile integer
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
 * void _ExcEnt (void);
 *
 */

SECTION_FUNC(TEXT, _ExcEnt)

	/*
	 * The gen_idt tool creates an interrupt-gate descriptor for
	 * all connections.  The processor will automatically clear the IF
	 * bit in the EFLAGS register upon execution of the handler, thus
	 * _ExcEnt() (and _IntEnt) need not issue an 'cli' as the first
	 * instruction.
	 */


	/*
	 * Note that the processor has pushed both the EFLAGS register
	 * and the linear return address (cs:eip) onto the stack prior
	 * to invoking the handler specified in the IDT.
	 *
	 * Clear the direction flag.  It is automatically restored when the
	 * exception exits.
	 */

	cld


	/*
	 * Swap ecx and return address on the current stack;
	 * this saves ecx on the stack without losing knowledge
	 * of how to get back to the exception stub.
	 */
	xchgl	%ecx, (%esp)

_ExcEntSetupDone:

	/* By the time we get here, the stack should look like this:
	 * ESP -> ECX (excepting task)
	 *	  Exception Error code (or junk)
	 *	  EIP (excepting task)
	 *	  CS (excepting task)
	 *	  EFLAGS (excepting task)
	 *	  ...
	 *
	 * ECX now contains the EIP of the calling exception stub */

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

	movl	_nanokernel + __tNANO_current_OFFSET, %edx

	incl	__tTCS_excNestCount_OFFSET(%edx)	/* inc exception nest count */

#ifdef CONFIG_GDB_INFO

    /*
     * Save the pointer to the stack frame (NANO_ESF *) in
     * the current execution context if this is the outermost exception.
     * The ESF pointer is used by debug tools to locate the volatile
     * registers and the stack of the preempted thread.
     */

	testl	$EXC_ACTIVE, __tTCS_flags_OFFSET (%edx)
	jne	alreadyInException
	movl	%esp, __tTCS_esfPtr_OFFSET(%edx)

alreadyInException:

#endif /* CONFIG_GDB_INFO */

	/*
	 * Set the EXC_ACTIVE bit in the TCS of the current thread.
	 * This enables _Swap() to preserve the thread's FP registers
	 * (where needed) if the exception handler causes a context switch.
	 * It also indicates to debug tools that an exception is being
	 * handled in the event of a context switch.
	 */

	orl	$EXC_ACTIVE, __tTCS_flags_OFFSET(%edx)

#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */



	/*
	 * restore interrupt enable state, then "return" back to exception stub
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
	jmp	*%ecx			/* "return" back to stub */


/**
 *
 * @brief Inform the kernel of an exception exit
 *
 * This function is called from the exception stub created by nanoCpuExcConnect()
 * to inform the kernel that the processing of an exception has
 * completed.  This routine restores the volatile integer registers and
 * then control is returned back to the interrupted thread or ISR.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _ExcExit (void);
 *
 */

SECTION_FUNC(TEXT, _ExcExit)
	/* On entry, interrupts may or may not be enabled. */

#ifndef CONFIG_X86_IAMCU
	popl %ecx      /* discard the NANO_ESF * parameter */
#endif

#if defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO)

	movl	_nanokernel + __tNANO_current_OFFSET, %ecx

	/*
	 * Must lock interrupts to prevent outside interference.
	 * (Using "lock" prefix would be nicer, but this won't work
	 * on platforms that don't respect the CPU's bus lock signal.)
	 */

	cli

	/*
	 * Determine whether exiting from a nested interrupt.
	 */

	decl	__tTCS_excNestCount_OFFSET(%ecx)	/* dec exception nest count */

	cmpl	$0, __tTCS_excNestCount_OFFSET(%ecx)
	jne	nestedException

	/*
	 * Clear the EXC_ACTIVE bit in the tTCS of the current execution context
	 * if we are not in a nested exception (ie, when we exit the outermost
	 * exception).
	 */

	andl	$~EXC_ACTIVE, __tTCS_flags_OFFSET (%ecx)

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

