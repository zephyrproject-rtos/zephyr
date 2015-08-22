/* intstub.S - interrupt management support for IA-32 architecture */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
This module implements assembly routines to manage interrupts on
the Intel IA-32 architecture.  More specifically, the interrupt (asynchronous
exception) stubs are implemented in this module.  The stubs are invoked when
entering and exiting a C interrupt handler.
 */

#define _ASMLANGUAGE

#ifndef CONFIG_NO_ISRS

#include <nano_private.h>
#include <arch/x86/asm.h>
#include <offsets.h>	/* nanokernel structure offset definitions */
#include <arch/cpu.h>	/* _NANO_ERR_SPURIOUS_INT */



	/* exports (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExit)
	GTEXT(_SpuriousIntNoErrCodeHandler)
	GTEXT(_SpuriousIntHandler)

 	/* externs */

	GTEXT(_Swap)

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	GTEXT(_sys_power_save_idle_exit)
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */


#ifdef CONFIG_INT_LATENCY_BENCHMARK
	GTEXT(_int_latency_start)
	GTEXT(_int_latency_stop)
#endif
/**
 *
 * @brief Inform the kernel of an interrupt
 *
 * This function is called from the interrupt stub created by irq_connect()
 * to inform the kernel of an interrupt.  This routine increments
 * _nanokernel.nested (to support interrupt nesting), switches to the
 * base of the interrupt stack, if not already on the interrupt stack, and then
 * saves the volatile integer registers onto the stack.  Finally, control is
 * returned back to the interrupt stub code (which will then invoke the
 * "application" interrupt service routine).
 *
 * Only the volatile integer registers are saved since ISRs are assumed not to
 * utilize floating point (or SSE) instructions.  If an ISR requires the usage
 * of floating point (or SSE) instructions, it must first invoke nanoCpuFpSave()
 * (or nanoCpuSseSave()) at the beginning of the ISR.  A subsequent
 * nanoCpuFpRestore() (or nanoCpuSseRestore()) is needed just prior to returning
 * from the ISR.  Note that the nanoCpuFpSave(), nanoCpuSseSave(),
 * nanoCpuFpRestore(), and nanoCpuSseRestore() APIs have not been
 * implemented yet.
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
 * void _IntEnt (void);
 *
 * NOMANUAL
 */

SECTION_FUNC(TEXT, _IntEnt)

	/*
	 * The _IntVecSet() routine creates an interrupt-gate descriptor for
	 * all connections.  The processor will automatically clear the IF
	 * bit in the EFLAGS register upon execution of the handler, thus
	 * _IntEnt() (and _ExcEnt) need not issue an 'cli' as the first
	 * instruction.
	 *
	 * Clear the direction flag.  It is automatically restored when the
	 * interrupt exits via the IRET instruction.
	 */

	cld



	/*
	 * Note that the processor has pushed both the EFLAGS register
	 * and the logical return address (cs:eip) onto the stack prior
	 * to invoking the handler specified in the IDT
	 */


	/*
	 * swap eax and return address on the current stack;
	 * this saves eax on the stack without losing knowledge
	 * of how to get back to the interrupt stub
	 */

#ifdef CONFIG_LOCK_INSTRUCTION_UNSUPPORTED

	pushl	(%esp)
	movl	%eax, 4(%esp)
	popl	%eax

#else

	xchgl	%eax, (%esp)

#endif /* CONFIG_LOCK_INSTRUCTION_UNSUPPORTED*/

	/*
	 * The remaining volatile registers are pushed onto the current
	 * stack.
	 */

	pushl	%ecx
	pushl	%edx

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	/*
	 * Volatile registers are now saved it is safe to start measuring
	 * how long interrupt are disabled.
	 * The interrupt gate created by irq_connect disables the
	 * interrupt.
	 *
	 * Preserve EAX as it contains the stub return address.
	 */

	pushl	%eax
	call	_int_latency_start
	popl	%eax
#endif


	/* load %ecx with &_nanokernel */

	movl	$_nanokernel, %ecx

	/* switch to the interrupt stack for the non-nested case */

	incl	__tNANO_nested_OFFSET(%ecx)	/* inc interrupt nest count */
	cmpl	$1, __tNANO_nested_OFFSET(%ecx)	/* use int stack if !nested */
	jne	alreadyOnIntStack

	/* switch to base of the interrupt stack */

	movl	%esp, %edx		/* save current thread's stack pointer */
	movl	__tNANO_common_isp_OFFSET(%ecx), %esp	/* load new sp value */


	/* save thread's stack pointer onto base of interrupt stack */

	pushl	%edx			/* Save stack pointer */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	cmpl	$0, __tNANO_idle_OFFSET(%ecx)
	jne	_HandleIdle
	/* fast path is !idle, in the pipeline */
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */


	/* fall through to nested case */

BRANCH_LABEL(alreadyOnIntStack)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	/* preserve eax which contain stub return address */
	pushl	%eax
	call	_int_latency_stop
	popl	%eax
#endif

	sti			/* re-enable interrupts */
	jmp	*%eax		/* "return" back to stub */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
BRANCH_LABEL(_HandleIdle)
	pushl	%eax
	push	__tNANO_idle_OFFSET(%ecx)
	movl	$0, __tNANO_idle_OFFSET(%ecx)

	/*
	 * Beware that a timer driver's _sys_power_save_idle_exit() implementation might
	 * expect that interrupts are disabled when invoked.  This ensures that
	 * the calculation and programming of the device for the next timer
	 * deadline is not interrupted.
	 */

	call	_sys_power_save_idle_exit
	add	$0x4, %esp
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
	sti			/* re-enable interrupts */
	popl	%eax
	jmp	*%eax		/* "return" back to stub */
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */


/**
 *
 * @brief Inform the kernel of an interrupt exit
 *
 * This function is called from the interrupt stub created by irq_connect()
 * to inform the kernel that the processing of an interrupt has
 * completed.  This routine decrements _nanokernel.nested (to support interrupt
 * nesting), restores the volatile integer registers, and then switches
 * back to the interrupted execution context's stack, if this isn't a nested
 * interrupt.
 *
 * Finally, control is returned back to the interrupted fiber or ISR.
 * A context switch _may_ occur if the interrupted context was a task context,
 * in which case one or more other fibers and tasks will execute before
 * this routine resumes and control gets returned to the interrupted task.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _IntExit (void);
 *
 * NOMANUAL
 */

SECTION_FUNC(TEXT, _IntExit)

	cli			/* disable interrupts */
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_start
#endif

	/* determine whether exiting from a nested interrupt */

	movl	$_nanokernel, %ecx
	decl	__tNANO_nested_OFFSET(%ecx)	/* dec interrupt nest count */
	jne	nestedInterrupt                 /* 'iret' if nested case */


	/*
	 * Determine whether the execution of the ISR requires a context
	 * switch.  If the interrupted thread is PREEMPTIBLE (a task) and
	 * _nanokernel.fiber is non-NULL, a _Swap() needs to occur.
	 */

	movl	__tNANO_current_OFFSET (%ecx), %eax
	testl	$PREEMPTIBLE, __tTCS_flags_OFFSET(%eax)
	je	noReschedule
	cmpl	$0, __tNANO_fiber_OFFSET (%ecx)
	je	noReschedule

	/*
	 * Set the INT_ACTIVE bit in the tTCS to allow the upcoming call to
	 * _Swap() to determine whether non-floating registers need to be
	 * preserved using the lazy save/restore algorithm, or to indicate to
	 * debug tools that a preemptive context switch has occurred.
	 *
	 * Setting the NO_METRICS bit tells _Swap() that the per-execution context
         * [totalRunTime] calculation has already been performed and that
	 * there is no need to do it again.
	 */

#if defined(CONFIG_FP_SHARING) ||  defined(CONFIG_GDB_INFO)
	orl	$INT_ACTIVE, __tTCS_flags_OFFSET(%eax)
#endif

	/*
	 * A context reschedule is required: keep the volatile registers of
	 * the interrupted thread on the context's stack.  Utilize
	 * the existing _Swap() primitive to save the remaining
	 * thread's registers (including floating point) and perform
	 * a switch to the new thread.
	 */

	popl	%esp		/* switch back to kernel stack */

	pushfl			/* push KERNEL_LOCK_KEY argument */
 	call	_Swap

	/*
	 * The interrupted thread has now been scheduled,
	 * as the result of a _later_ invocation of _Swap().
	 *
	 * Now need to restore the interrupted thread's environment before
	 * returning control to it at the point where it was interrupted ...
	 */


#if ( defined(CONFIG_FP_SHARING) ||  \
      defined(CONFIG_GDB_INFO) )
	/*
	 * _Swap() has restored the floating point registers, if needed.
	 * Clear the INT_ACTIVE bit of the interrupted thread's TCS
	 * since it has served its purpose.
	 */

	movl	_nanokernel + __tNANO_current_OFFSET, %eax
	andl	$~INT_ACTIVE, __tTCS_flags_OFFSET (%eax)
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */


	addl 	$4, %esp	/* pop KERNEL_LOCK_KEY argument */




	/* Restore volatile registers and return to the interrupted thread */
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif

	popl	%edx
	popl	%ecx
	popl	%eax

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret


BRANCH_LABEL(noReschedule)

	/*
	 * A thread reschedule is not required; switch back to the
	 * interrupted thread's stack and restore volatile registers
	 */

	popl	%esp		/* pop thread stack pointer */


	/* fall through to 'nestedInterrupt' */


	/*
	 * For the nested interrupt case, the interrupt stack must still be
	 * utilized, and more importantly, a rescheduling decision must
	 * not be performed.
	 */

BRANCH_LABEL(nestedInterrupt)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
	popl	%edx		/* pop volatile registers in reverse order */
	popl	%ecx
	popl	%eax
	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret


/**
 *
 * _SpuriousIntHandler -
 * @brief Spurious interrupt handler stubs
 *
 * Interrupt-gate descriptors are statically created for all slots in the IDT
 * that point to _SpuriousIntHandler() or _SpuriousIntNoErrCodeHandler().  The
 * former stub is connected to exception vectors where the processor pushes an
 * error code onto the stack (or kernel stack) in addition to the EFLAGS/CS/EIP
 * records.
 *
 * A spurious interrupt is considered a fatal condition, thus this routine
 * merely sets up the 'reason' and 'pEsf' parameters to the routine
 *  _SysFatalHwErrorHandler().  In other words, there is no provision to return
 * to the interrupted execution context and thus the volatile registers are not
 * saved.
 *
 * @return Never returns
 *
 * C function prototype:
 *
 * void _SpuriousIntHandler (void);
 *
 * INTERNAL
 * The _IntVecSet() routine creates an interrupt-gate descriptor for all
 * connections.  The processor will automatically clear the IF bit
 * in the EFLAGS register upon execution of the handler,
 * thus _SpuriousIntNoErrCodeHandler()/_SpuriousIntHandler() shall be
 * invoked with interrupts disabled.
 *
 * NOMANUAL
 */

SECTION_FUNC(TEXT, _SpuriousIntNoErrCodeHandler)

	pushl	$0			/* push dummy err code onto stk */

	/* fall through to _SpuriousIntHandler */


SECTION_FUNC(TEXT, _SpuriousIntHandler)

	cld				/* Clear direction flag */


	/*
	 * The task's regular stack is being used, but push the value of ESP
	 * anyway so that _ExcExit can "recover the stack pointer"
	 * without determining whether the exception occurred while CPL=3
	 */

	pushl	%esp			/* push cur stack pointer: pEsf arg */

BRANCH_LABEL(finishSpuriousInt)

	/* re-enable interrupts */

	sti

	/* push the 'unsigned int reason' parameter */

	pushl	$_NANO_ERR_SPURIOUS_INT

BRANCH_LABEL(callFatalHandler)

	/* call the fatal error handler */

	call	_NanoFatalErrorHandler

	/* handler shouldn't return, but call it again if it does */

	jmp	callFatalHandler

#endif /* CONFIG_NO_ISRS */
