/* excstub.S - exception management support for IA-32 architecture */

/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module implements assembly routines to manage exceptions (synchronous
interrupts) on the Intel IA-32 architecture.  More specifically,
exceptions are implemented in this module.  The stubs are invoked when entering
and exiting a C exception handler.
 */

#define _ASMLANGUAGE

#include <nano_private.h>
#include <arch/x86/asm.h>
#include <arch/x86/arch.h> /* For MK_ISR_NAME */
#include <offsets.h>	/* nanokernel structure offset definitions */


#include <asmPrv.h>

	/* exports (internal APIs) */

	GTEXT(_ExcEnt)
	GTEXT(_ExcExit)

	/* externs (internal APIs) */



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
	 * The _IntVecSet() routine creates an interrupt-gate descriptor for
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
	 * Swap eax and return address on the current stack;
	 * this saves eax on the stack without losing knowledge
	 * of how to get back to the exception stub.
	 */

#ifdef CONFIG_LOCK_INSTRUCTION_UNSUPPORTED

	pushl	(%esp)
	movl	%eax, 4(%esp)
	popl	%eax

#else

	xchgl	%eax, (%esp)

#endif /* CONFIG_LOCK_INSTRUCTION_UNSUPPORTED*/

	/*
	 * Push the remaining volatile registers on the existing stack.
	 * Note that eax has already been saved on the execution context stack.
	 */

	pushl	%ecx
	pushl	%edx

#ifdef CONFIG_GDB_INFO

	/*
	 * Push the cooperative registers on the existing stack as they are
	 * required by debug tools.
	 */

	pushl	%edi
	pushl	%esi
	pushl	%ebx
	pushl	%ebp

#endif /* CONFIG_GDB_INFO */

	/*
	 * Save possible faulting address: this has to be done before
	 * interrupts are re-enabled.
	 */
	movl  %cr2, %ecx
	pushl %ecx

	/* ESP is pointing to the ESF at this point */



#if defined(CONFIG_FP_SHARING) ||  defined(CONFIG_GDB_INFO)

	movl	_nanokernel + __tNANO_current_OFFSET, %ecx

	incl	__tTCS_excNestCount_OFFSET(%ecx)	/* inc exception nest count */

#ifdef CONFIG_GDB_INFO

    /*
     * Save the pointer to the stack frame (NANO_ESF *) in
     * the current execution context if this is the outermost exception.
     * The ESF pointer is used by debug tools to locate the volatile
     * registers and the stack of the preempted thread.
     */

	testl	$EXC_ACTIVE, __tTCS_flags_OFFSET (%ecx)
	jne	alreadyInException
	movl	%esp, __tTCS_esfPtr_OFFSET(%ecx)

BRANCH_LABEL(alreadyInException)

#endif /* CONFIG_GDB_INFO */

	/*
	 * Set the EXC_ACTIVE bit in the TCS of the current thread.
	 * This enables _Swap() to preserve the thread's FP registers
	 * (where needed) if the exception handler causes a context switch.
	 * It also indicates to debug tools that an exception is being
	 * handled in the event of a context switch.
	 */

	orl	$EXC_ACTIVE, __tTCS_flags_OFFSET(%ecx)

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

BRANCH_LABEL(allDone)
	pushl	%esp			/* push NANO_ESF * parameter */
	jmp	*%eax			/* "return" back to stub */


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

	/* discard the NANO_ESF * parameter and CR2 */
	addl	$8, %esp

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

BRANCH_LABEL(nestedException)
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

#ifdef CONFIG_GDB_INFO

	/*
	 * Pop the non-volatile registers from the stack.
	 * Note that debug tools may have altered the saved register values while
	 * the task was stopped, and we want to pick up the altered values.
	 */

	popl	%ebp
	popl	%ebx
	popl	%esi
	popl	%edi

#endif /* CONFIG_GDB_INFO */

	/* restore edx and ecx which are always saved on the stack */

	popl	%edx
	popl	%ecx
	popl	%eax

	addl	$4, %esp	/* "pop" error code */

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret

	/* Static exception handler stubs */
#if CONFIG_FP_SHARING
SYS_NANO_CPU_EXC_CONNECT(_FpNotAvailableExcHandler,IV_DEVICE_NOT_AVAILABLE)
#endif /* CONFIG_FP_SHARING */

