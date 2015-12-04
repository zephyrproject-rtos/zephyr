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


#include <asmPrv.h>

	/* exports (internal APIs) */

	GTEXT(_ExcEnt)
	GTEXT(_ExcEntNoErr)
	GTEXT(_ExcExit)
	GTEXT(_DynExcStubsBegin)
	GTEXT(_DynExcStubsNoErrBegin)

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

	/* Stash current value of EAX to free up the register */
	pushl	%eax

	/* Save the return address of the stub into EAX */
	movl	4(%esp), %eax

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
	xchgl	%eax, (%esp)

BRANCH_LABEL(_ExcEntSetupDone)

	/* By the time we get here, the stack should look like this:
	 * ESP -> EAX (excepting task)
	 *	  Exception Error code (or 0)
	 *	  EIP (excepting task)
	 *	  CS (excepting task)
	 *	  EFLAGS (excepting task)
	 *	  ...
	 *
	 * EAX now contains the EIP of the calling exception stub */

	/*
	 * Push the remaining volatile registers on the existing stack.
	 */

	pushl	%ecx
	pushl	%edx

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

	popl %ecx      /* discard the NANO_ESF * parameter */

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
	popl	%ecx
	popl	%eax

	addl	$4, %esp	/* "pop" error code */

	/* Pop of EFLAGS will re-enable interrupts and restore direction flag */
	iret

	/* Static exception handler stubs */
#if CONFIG_FP_SHARING
SYS_NANO_CPU_EXC_CONNECT(_FpNotAvailableExcHandler,IV_DEVICE_NOT_AVAILABLE)
#endif /* CONFIG_FP_SHARING */

#if CONFIG_EXCEPTION_DEBUG

#define EXC_HANDLER(vec) NANO_CPU_EXC_CONNECT_NO_ERR(handle_exc_##vec, vec, 0)
#define EXC_HANDLER_CODE(vec) NANO_CPU_EXC_CONNECT(handle_exc_##vec, vec, 0)

EXC_HANDLER(IV_DIVIDE_ERROR)
EXC_HANDLER(IV_NON_MASKABLE_INTERRUPT)
EXC_HANDLER(IV_OVERFLOW)
EXC_HANDLER(IV_BOUND_RANGE)
EXC_HANDLER(IV_INVALID_OPCODE)
#ifndef CONFIG_FP_SHARING
EXC_HANDLER(IV_DEVICE_NOT_AVAILABLE)
#endif
EXC_HANDLER_CODE(IV_DOUBLE_FAULT)
EXC_HANDLER_CODE(IV_INVALID_TSS)
EXC_HANDLER_CODE(IV_SEGMENT_NOT_PRESENT)
EXC_HANDLER_CODE(IV_STACK_FAULT)
EXC_HANDLER_CODE(IV_GENERAL_PROTECTION)
EXC_HANDLER_CODE(IV_PAGE_FAULT)
EXC_HANDLER(IV_X87_FPU_FP_ERROR)
EXC_HANDLER_CODE(IV_ALIGNMENT_CHECK)
EXC_HANDLER(IV_MACHINE_CHECK)
#endif /* CONFIG_EXCEPTION_DEBUG */

#if ALL_DYN_EXC_STUBS > 0
BRANCH_LABEL(_DynExcStubCommon)
	call _common_dynamic_exc_handler
	/* Cleanse the stack of stub_num */
	pop %eax
	/* Clean up and call IRET */
	jmp _ExcExit

/*
 * Create nice labels for all the stubs so we can see where we
 * are in a debugger
 */
.altmacro
.macro __EXC_STUB_NUM id
BRANCH_LABEL(_DynExcStub\id)
.endm
.macro EXC_STUB_NUM id
__EXC_STUB_NUM %id
.endm

stub_num = 0

SECTION_FUNC(TEXT, _DynExcStubsBegin)
#if CONFIG_NUM_DYNAMIC_EXC_STUBS > 0

/* Create all the dynamic IRQ stubs
 *
 * NOTE: Please update DYN_STUB_SIZE in include/arch/x86/arch.h if you change
 * how large the generated stubs are, otherwise _get_dynamic_stub() will
 * be unable to correctly determine the offset
 */
.rept ((CONFIG_NUM_DYNAMIC_EXC_STUBS + DYN_STUB_PER_BLOCK - 1) / DYN_STUB_PER_BLOCK)
	block_counter = 0
	.rept DYN_STUB_PER_BLOCK
		.if stub_num < CONFIG_NUM_DYNAMIC_EXC_STUBS
			EXC_STUB_NUM stub_num
			/*
			 * TODO: make this call in _DynExcStubCommon, saving
			 * 5 bytes per stub. Some voodoo will be necessary
			 * in _ExcEnt/_ExcExit to transplant the pushed
			 * stub_num to the irq stack
			 */
			call _ExcEnt

			/*
			 * 2-byte push imm8. Consumed by
			 * _common_dynamic_exc_handler(), see excconnect.c
			 */
			push $stub_num

			/*
			 * Check to make sure this isn't the last stub in
			 * a block, in which case we just fall through
			 */
			.if (block_counter <> (DYN_STUB_PER_BLOCK - 1) && \
			     (stub_num <> CONFIG_NUM_DYNAMIC_EXC_STUBS - 1))
				/* This should always be a 2-byte jmp rel8 */
				jmp 1f
			.endif
			stub_num = stub_num + 1
			block_counter = block_counter + 1
		.endif
	.endr
	/*
	 * This must a 5-bvte jump rel32, which is why _DynStubCommon
	 * is before the actual stubs
	 */
1:	jmp _DynExcStubCommon
.endr
#endif

SECTION_FUNC(TEXT, _DynExcStubsNoErrBegin)
#if CONFIG_NUM_DYNAMIC_EXC_NOERR_STUBS > 0

/* Same as above, but these stubs push a dummy error code as they will be
 * associated with exception that don't push one of their own.
 * Note that we don't reset stub_num to 0, we have a single set of indices
 * for error/non-error stubs */
.rept ((CONFIG_NUM_DYNAMIC_EXC_NOERR_STUBS + DYN_STUB_PER_BLOCK - 1) / DYN_STUB_PER_BLOCK)
	block_counter = 0
	.rept DYN_STUB_PER_BLOCK
		.if stub_num < ALL_DYN_EXC_STUBS
			EXC_STUB_NUM stub_num
			/*
			 * TODO: make this call in _DynExcStubCommon, saving
			 * 5 bytes per stub. Some voodoo will be necessary
			 * in _ExcEnt/_ExcExit to transplant the pushed
			 * stub_num to the irq stack
			 */
			call _ExcEntNoErr

			/*
			 * 2-byte push imm8. Consumed by
			 * _common_dynamic_exc_handler(), see excconnect.c
			 */
			push $stub_num

			/*
			 * Check to make sure this isn't the last stub in
			 * a block, in which case we just fall through
			 */
			.if (block_counter <> (DYN_STUB_PER_BLOCK - 1) && \
			     (stub_num <> ALL_DYN_EXC_STUBS - 1))
				/* This should always be a 2-byte jmp rel8 */
				jmp 1f
			.endif
			stub_num = stub_num + 1
			block_counter = block_counter + 1
		.endif
	.endr
	/*
	 * This must a 5-bvte jump rel32, which is why _DynStubCommon
	 * is before the actual stubs
	 */
1:	jmp _DynExcStubCommon
.endr

#endif /* CONFIG_NUM_DYNAMIC_EXC_NOERR_STUBS */
#endif /* ALL_DYN_EXC_STUBS */
