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
#include <arch/x86/ia32/asm.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <drivers/interrupt_controller/sysapic.h>

	/* exports (internal APIs) */

	GTEXT(_interrupt_enter)
	GTEXT(z_SpuriousIntNoErrCodeHandler)
	GTEXT(z_SpuriousIntHandler)
	GTEXT(_irq_sw_handler)
	GTEXT(z_dynamic_stubs_begin)

	/* externs */

	GTEXT(z_arch_swap)

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	GTEXT(z_sys_power_save_idle_exit)
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
 * utilize floating point (or SSE) instructions.
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
	 * Note that the processor has pushed both the EFLAGS register
	 * and the logical return address (cs:eip) onto the stack prior
	 * to invoking the handler specified in the IDT. The stack looks
	 * like this:
	 *
	 *  24 SS (only on privilege level change)
	 *  20 ESP (only on privilege level change)
	 *  16 EFLAGS
	 *  12 CS
	 *  8  EIP
	 *  4  isr_param
	 *  0  isr   <-- stack pointer
	 */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	pushl %eax
	pushl %edx
	rdtsc
	mov %eax, z_arch_timing_irq_start
	mov %edx, z_arch_timing_irq_start+4
	pop %edx
	pop %eax
#endif
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

#ifdef CONFIG_X86_KPTI
	call z_x86_trampoline_to_kernel
#endif
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
	 * functions (isr handler, z_arch_swap(), etc)
	 */
	pushl	%edi


#if defined(CONFIG_TRACING)

	/* Save these as we are using to keep track of isr and isr_param */
	pushl	%eax
	pushl	%edx

	call	sys_trace_isr_enter

	popl	%edx
	popl	%eax
#endif

	/* load %ecx with &_kernel */

	movl	$_kernel, %ecx

	/* switch to the interrupt stack for the non-nested case */

	incl	_kernel_offset_to_nested(%ecx)

	/* use interrupt stack if not nested */
	cmpl	$1, _kernel_offset_to_nested(%ecx)
	jne	alreadyOnIntStack

	/*
	 * switch to base of the interrupt stack: save esp in edi, then load
	 * irq_stack pointer
	 */

	movl	%esp, %edi
	movl	_kernel_offset_to_irq_stack(%ecx), %esp


	/* save thread's stack pointer onto base of interrupt stack */

	pushl	%edi			/* Save stack pointer */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	cmpl	$0, _kernel_offset_to_idle(%ecx)
	jne	handle_idle
	/* fast path is !idle, in the pipeline */
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

	/* fall through to nested case */

alreadyOnIntStack:

	push	%eax	/* interrupt handler argument */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	/* Save the eax and edx registers before reading the time stamp
	* once done pop the values
	*/
	pushl %eax
	pushl %edx
	rdtsc
	mov %eax,z_arch_timing_irq_end
	mov %edx,z_arch_timing_irq_end+4
	pop %edx
	pop %eax
#endif

#ifdef CONFIG_NESTED_INTERRUPTS
	sti			/* re-enable interrupts */
#endif
	/* Now call the interrupt handler */
	INDIRECT_CALL(%edx)
	/* Discard ISR argument */
	addl	$0x4, %esp
#ifdef CONFIG_NESTED_INTERRUPTS
	cli			/* disable interrupts again */
#endif

	xorl	%eax, %eax
#if defined(CONFIG_X2APIC)
	xorl	%edx, %edx
	movl	$(X86_X2APIC_BASE_MSR + (LOAPIC_EOI >> 4)), %ecx
	wrmsr
#else /* xAPIC */
	movl	%eax, (CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI)
#endif

	/* determine whether exiting from a nested interrupt */
	movl	$_kernel, %ecx
	decl	_kernel_offset_to_nested(%ecx)	/* dec interrupt nest count */
	jne	nestedInterrupt                 /* 'iret' if nested case */


#ifdef CONFIG_PREEMPT_ENABLED
	movl	_kernel_offset_to_current(%ecx), %edx

	/* reschedule only if the scheduler says that we must do so */
	cmpl	%edx, _kernel_offset_to_ready_q_cache(%ecx)
	je	noReschedule

	/*
	 * Set X86_THREAD_FLAG_INT bit in k_thread to allow the upcoming call
	 * to z_arch_swap() to determine whether non-floating registers need to be
	 * preserved using the lazy save/restore algorithm, or to indicate to
	 * debug tools that a preemptive context switch has occurred.
	 */

#if defined(CONFIG_LAZY_FP_SHARING)
	orb	$X86_THREAD_FLAG_INT, _thread_offset_to_flags(%edx)
#endif

	/*
	 * A context reschedule is required: keep the volatile registers of
	 * the interrupted thread on the context's stack.  Utilize
	 * the existing z_arch_swap() primitive to save the remaining
	 * thread's registers (including floating point) and perform
	 * a switch to the new thread.
	 */

	popl	%esp	/* switch back to outgoing thread's stack */

#ifdef CONFIG_STACK_SENTINEL
	call	z_check_stack_sentinel
#endif
	pushfl			/* push KERNEL_LOCK_KEY argument */
	call	z_arch_swap
	addl 	$4, %esp	/* pop KERNEL_LOCK_KEY argument */

	/*
	 * The interrupted thread has now been scheduled,
	 * as the result of a _later_ invocation of z_arch_swap().
	 *
	 * Now need to restore the interrupted thread's environment before
	 * returning control to it at the point where it was interrupted ...
	 */

#if defined(CONFIG_LAZY_FP_SHARING)
	/*
	 * z_arch_swap() has restored the floating point registers, if needed.
	 * Clear X86_THREAD_FLAG_INT in the interrupted thread's state
	 * since it has served its purpose.
	 */

	movl	_kernel + _kernel_offset_to_current, %eax
	andb	$~X86_THREAD_FLAG_INT, _thread_offset_to_flags(%eax)
#endif /* CONFIG_LAZY_FP_SHARING */

	/* Restore volatile registers and return to the interrupted thread */
	popl	%edi
	popl	%ecx
	popl	%edx
	popl	%eax

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	KPTI_IRET

#endif /* CONFIG_PREEMPT_ENABLED */

noReschedule:

	/*
	 * A thread reschedule is not required; switch back to the
	 * interrupted thread's stack and restore volatile registers
	 */

	popl	%esp		/* pop thread stack pointer */

#ifdef CONFIG_STACK_SENTINEL
	call	z_check_stack_sentinel
#endif

	/* fall through to 'nestedInterrupt' */


	/*
	 * For the nested interrupt case, the interrupt stack must still be
	 * utilized, and more importantly, a rescheduling decision must
	 * not be performed.
	 */

nestedInterrupt:
	popl	%edi
	popl	%ecx		/* pop volatile registers in reverse order */
	popl	%edx
	popl	%eax
	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	KPTI_IRET


#ifdef CONFIG_SYS_POWER_MANAGEMENT
handle_idle:
	pushl	%eax
	pushl	%edx
	/* Populate 'ticks' argument to z_sys_power_save_idle_exit */
	push	_kernel_offset_to_idle(%ecx)
	/* Zero out _kernel.idle */
	movl	$0, _kernel_offset_to_idle(%ecx)

	/*
	 * Beware that a timer driver's z_sys_power_save_idle_exit() implementation might
	 * expect that interrupts are disabled when invoked.  This ensures that
	 * the calculation and programming of the device for the next timer
	 * deadline is not interrupted.
	 */

	call	z_sys_power_save_idle_exit
	/* discard 'ticks' argument passed on the stack */
	add	$0x4, %esp
	popl	%edx
	popl	%eax
	jmp	alreadyOnIntStack
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

/**
 *
 * z_SpuriousIntHandler -
 * @brief Spurious interrupt handler stubs
 *
 * Interrupt-gate descriptors are statically created for all slots in the IDT
 * that point to z_SpuriousIntHandler() or z_SpuriousIntNoErrCodeHandler().  The
 * former stub is connected to exception vectors where the processor pushes an
 * error code onto the stack (or kernel stack) in addition to the EFLAGS/CS/EIP
 * records.
 *
 * A spurious interrupt is considered a fatal condition; there is no provision
 * to return to the interrupted execution context and thus the volatile
 * registers are not saved.
 *
 * @return Never returns
 *
 * C function prototype:
 *
 * void z_SpuriousIntHandler (void);
 *
 * INTERNAL
 * The gen_idt tool creates an interrupt-gate descriptor for all
 * connections.  The processor will automatically clear the IF bit
 * in the EFLAGS register upon execution of the handler,
 * thus z_SpuriousIntNoErrCodeHandler()/z_SpuriousIntHandler() shall be
 * invoked with interrupts disabled.
 */
SECTION_FUNC(TEXT, z_SpuriousIntNoErrCodeHandler)

	pushl	$0			/* push dummy err code onto stk */

	/* fall through to z_SpuriousIntHandler */


SECTION_FUNC(TEXT, z_SpuriousIntHandler)

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

	pushl	%esp		/* push cur stack pointer: pEsf arg */

	/* re-enable interrupts */
	sti

	/* call the fatal error handler */
	call	z_x86_spurious_irq

	/* handler doesn't  return */

#if CONFIG_IRQ_OFFLOAD
SECTION_FUNC(TEXT, _irq_sw_handler)
	push $0
	push $z_irq_do_offload
	jmp _interrupt_enter

#endif

#if CONFIG_X86_DYNAMIC_IRQ_STUBS > 0
z_dynamic_irq_stub_common:
	/* stub number already pushed */
	push $z_x86_dynamic_irq_handler
	jmp _interrupt_enter

/* Create all the dynamic IRQ stubs
 *
 * NOTE: Please update DYN_STUB_SIZE in include/arch/x86/ia32/arch.h if you
 * change how large the generated stubs are, otherwise _get_dynamic_stub()
 * will be unable to correctly determine the offset
 */

/*
 * Create nice labels for all the stubs so we can see where we
 * are in a debugger
 */
.altmacro
.macro __INT_STUB_NUM id
z_dynamic_irq_stub_\id:
.endm
.macro INT_STUB_NUM id
__INT_STUB_NUM %id
.endm

z_dynamic_stubs_begin:
stub_num = 0
.rept ((CONFIG_X86_DYNAMIC_IRQ_STUBS + Z_DYN_STUB_PER_BLOCK - 1) / Z_DYN_STUB_PER_BLOCK)
	block_counter = 0
	.rept Z_DYN_STUB_PER_BLOCK
		.if stub_num < CONFIG_X86_DYNAMIC_IRQ_STUBS
			INT_STUB_NUM stub_num
			/*
			 * 2-byte push imm8.
			 */
			push $stub_num

			/*
			 * Check to make sure this isn't the last stub in
			 * a block, in which case we just fall through
			 */
			.if (block_counter <> (Z_DYN_STUB_PER_BLOCK - 1) && \
			     (stub_num <> CONFIG_X86_DYNAMIC_IRQ_STUBS - 1))
				/* This should always be a 2-byte jmp rel8 */
				jmp 1f
			.endif
			stub_num = stub_num + 1
			block_counter = block_counter + 1
		.endif
	.endr
	/*
	 * This must a 5-bvte jump rel32, which is why z_dynamic_irq_stub_common
	 * is before the actual stubs
	 */
1:	jmp z_dynamic_irq_stub_common
.endr
#endif /* CONFIG_X86_DYNAMIC_IRQ_STUBS > 0 */

