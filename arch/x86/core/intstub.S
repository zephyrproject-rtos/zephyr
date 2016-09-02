/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
 * @brief Interrupt management support for IA-32 architecture
 *
 * This module implements assembly routines to manage interrupts on
 * the Intel IA-32 architecture.  More specifically, the interrupt (asynchronous
 * exception) stubs are implemented in this module.  The stubs are invoked when
 * entering and exiting a C interrupt handler.
 */

#define _ASMLANGUAGE

#include <nano_private.h>
#include <arch/x86/asm.h>
#include <offsets.h>	/* nanokernel structure offset definitions */
#include <arch/cpu.h>	/* _NANO_ERR_SPURIOUS_INT */
#include <arch/x86/irq_controller.h>

	/* exports (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExitWithEoi)
	GTEXT(_IntExit)
	GTEXT(_SpuriousIntNoErrCodeHandler)
	GTEXT(_SpuriousIntHandler)
	GTEXT(_irq_sw_handler)

	/* externs */

	GTEXT(_Swap)

#ifdef CONFIG_KERNEL_V2
	GTEXT(__must_switch_threads)
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
#if defined(CONFIG_NANOKERNEL) && defined(CONFIG_TICKLESS_IDLE)
	GTEXT(_power_save_idle_exit)
#else
	GTEXT(_sys_power_save_idle_exit)
#endif
#endif


#ifdef CONFIG_INT_LATENCY_BENCHMARK
	GTEXT(_int_latency_start)
	GTEXT(_int_latency_stop)
#endif
/**
 *
 * @brief Inform the kernel of an interrupt
 *
 * This function is called from the interrupt stub created by IRQ_CONNECT()
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
 */
SECTION_FUNC(TEXT, _IntEnt)

	/*
	 * The gen_idt tool creates an interrupt-gate descriptor for
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
	xchgl	%eax, (%esp)

	/*
	 * The remaining volatile registers are pushed onto the current
	 * stack.
	 */

	pushl	%ecx
	pushl	%edx

#ifdef CONFIG_DEBUG_INFO
	/*
	 * Push the cooperative registers on the existing stack as they are
	 * required by debug tools.
	 */

	pushl	%edi
	pushl	%esi
	pushl	%ebx
	pushl	%ebp

	leal	44(%esp), %ecx   /* Calculate ESP before interrupt occurred */
	pushl	%ecx             /* Save calculated ESP */
#endif

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	/*
	 * Volatile registers are now saved it is safe to start measuring
	 * how long interrupt are disabled.
	 * The interrupt gate created by IRQ_CONNECT disables the
	 * interrupt.
	 *
	 * Preserve EAX as it contains the stub return address.
	 */

	pushl	%eax
	call	_int_latency_start
	popl	%eax
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
	/*
	 * Preserve EAX as it contains the stub return address.
	 */
	pushl	%eax
	call	_sys_k_event_logger_interrupt
	popl	%eax
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	/*
	 * Preserve EAX as it contains the stub return address.
	 */
	pushl	%eax
	call	_sys_k_event_logger_exit_sleep
	popl	%eax
#endif

	/* load %ecx with &_nanokernel */

	movl	$_nanokernel, %ecx

	/* switch to the interrupt stack for the non-nested case */

	incl	__tNANO_nested_OFFSET(%ecx)	/* inc interrupt nest count */
	cmpl	$1, __tNANO_nested_OFFSET(%ecx)	/* use int stack if !nested */
#ifdef CONFIG_DEBUG_INFO
	jne	nested_save_isf
#else
	jne	alreadyOnIntStack
#endif

	/* switch to base of the interrupt stack */

	movl	%esp, %edx		/* save current thread's stack pointer */
	movl	__tNANO_common_isp_OFFSET(%ecx), %esp	/* load new sp value */


	/* save thread's stack pointer onto base of interrupt stack */

	pushl	%edx			/* Save stack pointer */

#ifdef CONFIG_DEBUG_INFO
	/*
	 * The saved stack pointer happens to match the address of the
	 * interrupt stack frame.  To simplify the exit case, push a dummy ISF
	 * for the "old" ISF and save it to the _nanokernel.isf.
	 */
	pushl	%edx
	movl	%edx, __tNANO_isf_OFFSET(%ecx)
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	cmpl	$0, __tNANO_idle_OFFSET(%ecx)
	jne	_HandleIdle
	/* fast path is !idle, in the pipeline */
#endif

#ifdef CONFIG_DEBUG_INFO
	jmp alreadyOnIntStack

nested_save_isf:
	movl	__tNANO_isf_OFFSET(%ecx), %edx	/* Get old ISF */
	movl	%esp, __tNANO_isf_OFFSET(%ecx)	/* Save new ISF */
	pushl	%edx                            /* Save old ISF */
#endif

	/* fall through to nested case */

alreadyOnIntStack:
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	/* preserve eax which contain stub return address */
	pushl	%eax
	call	_int_latency_stop
	popl	%eax
#endif
#ifdef CONFIG_NESTED_INTERRUPTS
	sti			/* re-enable interrupts */
#endif
	jmp	*%eax		/* "return" back to stub */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
_HandleIdle:
	/* Preserve eax which contains stub return address */
#if defined(CONFIG_NANOKERNEL) && defined(CONFIG_TICKLESS_IDLE)
	pushl	%eax
	call _power_save_idle_exit
#else
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
#endif /* CONFIG_NANOKERNEL && CONFIG_TICKLESS_IDLE */
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
	sti			/* re-enable interrupts */
	popl	%eax
	jmp	*%eax		/* "return" back to stub */
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

/**
 * @brief Perform EOI and do interrupt exit
 *
 * This is used by the interrupt stubs, which all leave the stack in
 * a particular state and need to poke the interrupt controller.
 */
SECTION_FUNC(TEXT, _IntExitWithEoi)

	cli			/* disable interrupts */

#ifndef CONFIG_X86_IAMCU
	/* For SYS V, the stub pushes an argument onto the stack to be
	 * consumed by the handler, remove it since the handler is now
	 * finished
	 */
	popl %eax
#endif

	/* irq_controller.h interface */
	_irq_controller_eoi

	jmp _IntExitWithCli

/**
 *
 * @brief Inform the kernel of an interrupt exit
 *
 * This function is called from the interrupt stub created by IRQ_CONNECT()
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
 */
_IntExit:

	cli			/* disable interrupts */

_IntExitWithCli:
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_start
#endif

	/* determine whether exiting from a nested interrupt */

	movl	$_nanokernel, %ecx
#ifdef CONFIG_DEBUG_INFO
	popl	__tNANO_isf_OFFSET(%ecx)    /* Restore old ISF */
#endif
	decl	__tNANO_nested_OFFSET(%ecx)	/* dec interrupt nest count */
	jne	nestedInterrupt                 /* 'iret' if nested case */


	movl	__tNANO_current_OFFSET (%ecx), %edx

#ifdef CONFIG_KERNEL_V2
	/*
	 * Determine whether the execution of the ISR requires a context
	 * switch.  If the thread is preemptible, scheduler is not locked and
	 * a higher priority thread exists, a _Swap() needs to occur.
	 */

	/* do not reschedule coop threads (negative priority) */
	cmpl	$0, __tTCS_prio_OFFSET (%edx)
	jl	noReschedule

	/* do not reschedule if scheduler is locked */
	cmpl	$0, __tTCS_sched_locked_OFFSET (%edx)
	jg	noReschedule


	/* reschedule only if the scheduler says that we must do so */
	call	__must_switch_threads
	cmpl	$0, %eax
	je	noReschedule
#else
	/*
	 * Determine whether the execution of the ISR requires a context
	 * switch.  If the interrupted thread is PREEMPTIBLE (a task) and
	 * _nanokernel.fiber is non-NULL, a _Swap() needs to occur.
	 */

	testl	$PREEMPTIBLE, __tTCS_flags_OFFSET(%edx)
	je	noReschedule
	cmpl	$0, __tNANO_fiber_OFFSET (%ecx)
	je	noReschedule
#endif

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
#ifdef CONFIG_KERNEL_V2
	/* reload _nanokernel.current: __must_switch_threads may clobber it */
	movl	_nanokernel + __tNANO_current_OFFSET, %edx
#endif
	orl	$INT_ACTIVE, __tTCS_flags_OFFSET(%edx)
#endif

	/*
	 * A context reschedule is required: keep the volatile registers of
	 * the interrupted thread on the context's stack.  Utilize
	 * the existing _Swap() primitive to save the remaining
	 * thread's registers (including floating point) and perform
	 * a switch to the new thread.
	 */

	popl	%esp	/* switch back to outgoing thread's stack */

#ifdef CONFIG_DEBUG_INFO
	popl	%ebp        /* Discard saved ESP */
	popl	%ebp
	popl	%ebx
	popl	%esi
	popl	%edi
#endif

	pushfl			/* push KERNEL_LOCK_KEY argument */
#ifdef CONFIG_X86_IAMCU
	/* IAMCU first argument goes into a register, not the stack.
	 */
	popl	%eax
#endif
	call	_Swap

#ifndef CONFIG_X86_IAMCU
	addl 	$4, %esp	/* pop KERNEL_LOCK_KEY argument */
#endif
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

	/* Restore volatile registers and return to the interrupted thread */
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif

	popl	%edx
	popl	%ecx
	popl	%eax

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret


noReschedule:

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

nestedInterrupt:
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif

#ifdef CONFIG_DEBUG_INFO
	popl	%ebp        /* Discard saved ESP */
	popl	%ebp
	popl	%ebx
	popl	%esi
	popl	%edi
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
 * The gen_idt tool creates an interrupt-gate descriptor for all
 * connections.  The processor will automatically clear the IF bit
 * in the EFLAGS register upon execution of the handler,
 * thus _SpuriousIntNoErrCodeHandler()/_SpuriousIntHandler() shall be
 * invoked with interrupts disabled.
 */
SECTION_FUNC(TEXT, _SpuriousIntNoErrCodeHandler)

	pushl	$0			/* push dummy err code onto stk */

	/* fall through to _SpuriousIntHandler */


SECTION_FUNC(TEXT, _SpuriousIntHandler)

	cld				/* Clear direction flag */

	/* Create the ESF */

	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebx
	pushl %ebp

	leal	44(%esp), %ecx   /* Calculate ESP before exception occurred */
	pushl	%ecx             /* Save calculated ESP */

	/*
	 * The task's regular stack is being used, but push the value of ESP
	 * anyway so that _ExcExit can "recover the stack pointer"
	 * without determining whether the exception occurred while CPL=3
	 */
#ifndef CONFIG_X86_IAMCU
	pushl	%esp			/* push cur stack pointer: pEsf arg */
#else
	mov	%esp, %edx
#endif

finishSpuriousInt:

	/* re-enable interrupts */

	sti

	/* push the 'unsigned int reason' parameter */
#ifndef CONFIG_X86_IAMCU
	pushl	$_NANO_ERR_SPURIOUS_INT
#else
	movl	$_NANO_ERR_SPURIOUS_INT, %eax
#endif

callFatalHandler:
	/* call the fatal error handler */

	call	_NanoFatalErrorHandler

	/* handler doesn't  return */

#if CONFIG_IRQ_OFFLOAD
SECTION_FUNC(TEXT, _irq_sw_handler)
	call _IntEnt
	call _irq_do_offload
	jmp _IntExit

#endif
