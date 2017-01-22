/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <kernel_structs.h>
#include <arch/x86/asm.h>
#include <offsets_short.h>
#include <arch/cpu.h>	/* _NANO_ERR_SPURIOUS_INT */
#include <arch/x86/irq_controller.h>

	/* exports (internal APIs) */

	GTEXT(_interrupt_enter)
	GTEXT(_SpuriousIntNoErrCodeHandler)
	GTEXT(_SpuriousIntHandler)
	GTEXT(_irq_sw_handler)

	/* externs */

	GTEXT(_Swap)

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	GTEXT(_sys_power_save_idle_exit)
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
 * _kernel.nested (to support interrupt nesting), switches to the
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
 * void _interrupt_enter(void *isr, void *isr_param);
 */
SECTION_FUNC(TEXT, _interrupt_enter)

	/*
	 * The gen_idt tool creates an interrupt-gate descriptor for
	 * all connections.  The processor will automatically clear the IF
	 * bit in the EFLAGS register upon execution of the handler, hence
	 * this need not issue an 'cli' as the first instruction.
	 *
	 * Clear the direction flag.  It is automatically restored when the
	 * interrupt exits via the IRET instruction.
	 */

	cld

	/*
	 * Note that the processor has pushed both the EFLAGS register
	 * and the logical return address (cs:eip) onto the stack prior
	 * to invoking the handler specified in the IDT. The stack looks
	 * like this:
	 *
	 *  EFLAGS
	 *  CS
	 *  EIP
	 *  isr_param
	 *  isr   <-- stack pointer
	 */


	/*
	 * Swap EAX with isr_param and EDX with isr.
	 * Push ECX onto the stack
	 */
	xchgl	%eax, 4(%esp)
	xchgl	%edx, (%esp)
	pushl	%ecx

	/* Now the stack looks like:
	 *
	 * EFLAGS
	 * CS
	 * EIP
	 * saved EAX
	 * saved EDX
	 * saved ECX
	 *
	 * EAX = isr_param, EDX = isr
	 */

	/* Push EDI as we will use it for scratch space.
	 * Rest of the callee-saved regs get saved by invocation of C
	 * functions (isr handler, _Swap(), etc)
	 */
	pushl	%edi

#ifdef CONFIG_DEBUG_INFO
	/*
	 * Push the cooperative registers on the existing stack as they are
	 * required by debug tools.
	 */

	pushl	%esi
	pushl	%ebx
	pushl	%ebp

	leal	40(%esp), %ecx   /* Calculate ESP before interrupt occurred */
	pushl	%ecx             /* Save calculated ESP */
#endif

#if defined(CONFIG_INT_LATENCY_BENCHMARK) || \
		defined(CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT) || \
		defined(CONFIG_KERNEL_EVENT_LOGGER_SLEEP)

	/* Save these as we are using to keep track of isr and isr_param */
	pushl	%eax
	pushl	%edx

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	/*
	 * Volatile registers are now saved it is safe to start measuring
	 * how long interrupt are disabled.
	 * The interrupt gate created by IRQ_CONNECT disables the
	 * interrupt.
	 */

	call	_int_latency_start
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
	call	_sys_k_event_logger_interrupt
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	call	_sys_k_event_logger_exit_sleep
#endif

	popl	%edx
	popl	%eax
#endif

	/* load %ecx with &_kernel */

	movl	$_kernel, %ecx

	/* switch to the interrupt stack for the non-nested case */

	incl	_kernel_offset_to_nested(%ecx)

	/* use interrupt stack if not nested */
	cmpl	$1, _kernel_offset_to_nested(%ecx)
#ifdef CONFIG_DEBUG_INFO
	jne	nested_save_isf
#else
	jne	alreadyOnIntStack
#endif

	/*
	 * switch to base of the interrupt stack: save esp in edi, then load
	 * irq_stack pointer
	 */

	movl	%esp, %edi
	movl	_kernel_offset_to_irq_stack(%ecx), %esp


	/* save thread's stack pointer onto base of interrupt stack */

	pushl	%edi			/* Save stack pointer */

#ifdef CONFIG_DEBUG_INFO
	/*
	 * The saved stack pointer happens to match the address of the
	 * interrupt stack frame.  To simplify the exit case, push a dummy ISF
	 * for the "old" ISF and save it to the _kernel.isf.
	 */
	pushl	%edi
	movl	%edi, _kernel_offset_to_isf(%ecx)
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	cmpl	$0, _kernel_offset_to_idle(%ecx)
	jne	handle_idle
	/* fast path is !idle, in the pipeline */
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef CONFIG_DEBUG_INFO
	jmp alreadyOnIntStack

nested_save_isf:
	movl	_kernel_offset_to_isf(%ecx), %edi	/* Get old ISF */
	movl	%esp, _kernel_offset_to_isf(%ecx)	/* Save new ISF */
	pushl	%edi                            /* Save old ISF */
#endif

	/* fall through to nested case */

alreadyOnIntStack:
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	pushl	%eax
	pushl	%edx
	call	_int_latency_stop
	popl	%edx
	popl	%eax
#endif

#ifndef CONFIG_X86_IAMCU
	/* EAX has the interrupt handler argument, needs to go on
	 * stack for sys V calling convention
	 */
	push	%eax
#endif
#ifdef CONFIG_NESTED_INTERRUPTS
	sti			/* re-enable interrupts */
#endif
	/* Now call the interrupt handler */
	call	*%edx
#ifndef CONFIG_X86_IAMCU
	/* Discard ISR argument */
	addl	$0x4, %esp
#endif
#ifdef CONFIG_NESTED_INTERRUPTS
	cli			/* disable interrupts again */
#endif

	/* irq_controller.h interface */
	_irq_controller_eoi

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_start
#endif

	/* determine whether exiting from a nested interrupt */
	movl	$_kernel, %ecx
#ifdef CONFIG_DEBUG_INFO
	popl	_kernel_offset_to_isf(%ecx)    /* Restore old ISF */
#endif
	decl	_kernel_offset_to_nested(%ecx)	/* dec interrupt nest count */
	jne	nestedInterrupt                 /* 'iret' if nested case */


#ifdef CONFIG_PREEMPT_ENABLED
	movl	_kernel_offset_to_current(%ecx), %edx

	/*
	 * Non-preemptible thread ? Do not schedule (see explanation of
	 * preempt field in kernel_struct.h).
	 */
	cmpw	$_NON_PREEMPT_THRESHOLD, _thread_offset_to_preempt(%edx)
	jae	noReschedule


	/* reschedule only if the scheduler says that we must do so */
	cmpl	%edx, _kernel_offset_to_ready_q_cache(%ecx)
	je	noReschedule

	/*
	 * Set the _INT_ACTIVE bit in the k_thread to allow the upcoming call to
	 * _Swap() to determine whether non-floating registers need to be
	 * preserved using the lazy save/restore algorithm, or to indicate to
	 * debug tools that a preemptive context switch has occurred.
	 */

#if defined(CONFIG_FP_SHARING) ||  defined(CONFIG_GDB_INFO)
	orb	$_INT_ACTIVE, _thread_offset_to_thread_state(%edx)
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
	 * Clear the _INT_ACTIVE bit in the interrupted thread's state
	 * since it has served its purpose.
	 */

	movl	_kernel + _kernel_offset_to_current, %eax
	andb	$~_INT_ACTIVE, _thread_offset_to_thread_state(%eax)
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

	/* Restore volatile registers and return to the interrupted thread */
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
	popl	%edi
	popl	%ecx
	popl	%edx
	popl	%eax

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret

#endif /* CONFIG_PREEMPT_ENABLED */

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
#endif
	popl	%edi
	popl	%ecx		/* pop volatile registers in reverse order */
	popl	%edx
	popl	%eax
	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret


#ifdef CONFIG_SYS_POWER_MANAGEMENT
handle_idle:
	pushl	%eax
	pushl	%edx
	/* Populate 'ticks' argument to _sys_power_save_idle_exit */
#ifdef CONFIG_X86_IAMCU
	movl	_kernel_offset_to_idle(%ecx), %eax
#else
	/* SYS V calling convention */
	push	_kernel_offset_to_idle(%ecx)
#endif
	/* Zero out _kernel.idle */
	movl	$0, _kernel_offset_to_idle(%ecx)

	/*
	 * Beware that a timer driver's _sys_power_save_idle_exit() implementation might
	 * expect that interrupts are disabled when invoked.  This ensures that
	 * the calculation and programming of the device for the next timer
	 * deadline is not interrupted.
	 */

	call	_sys_power_save_idle_exit
#ifndef CONFIG_X86_IAMCU
	/* SYS V: discard 'ticks' argument passed on the stack */
	add	$0x4, %esp
#endif
	popl	%edx
	popl	%eax
	jmp	alreadyOnIntStack
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

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

#ifndef CONFIG_X86_IAMCU
	pushl	%esp			/* push cur stack pointer: pEsf arg */
#else
	mov	%esp, %edx
#endif

	/* re-enable interrupts */
	sti

	/* push the 'unsigned int reason' parameter */
#ifndef CONFIG_X86_IAMCU
	pushl	$_NANO_ERR_SPURIOUS_INT
#else
	movl	$_NANO_ERR_SPURIOUS_INT, %eax
#endif
	/* call the fatal error handler */
	call	_NanoFatalErrorHandler

	/* handler doesn't  return */

#if CONFIG_IRQ_OFFLOAD
SECTION_FUNC(TEXT, _irq_sw_handler)
	push $0
	push $_irq_do_offload
	jmp _interrupt_enter

#endif
