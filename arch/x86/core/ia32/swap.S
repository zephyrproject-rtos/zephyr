/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel swapper code for IA-32
 *
 * This module implements the arch_swap() routine for the IA-32 architecture.
 */

#include <zephyr/arch/x86/ia32/asm.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <kernel_arch_data.h>
#include <offsets_short.h>

	/* exports (internal APIs) */

	GTEXT(arch_swap)
	GTEXT(z_x86_thread_entry_wrapper)
	GTEXT(_x86_user_thread_entry_wrapper)

	/* externs */
#if !defined(CONFIG_X86_KPTI) && defined(CONFIG_X86_USERSPACE)
	GTEXT(z_x86_swap_update_page_tables)
#endif
	GDATA(_k_neg_eagain)

/*
 * Given that arch_swap() is called to effect a cooperative context switch,
 * only the non-volatile integer registers need to be saved in the TCS of the
 * outgoing thread.  The restoration of the integer registers of the incoming
 * thread depends on whether that thread was preemptively context switched out.
 * The X86_THREAD_FLAG_INT and _EXC bits in the k_thread->arch.flags field will
 * signify that the thread was preemptively context switched out, and thus both
 * the volatile and non-volatile integer registers need to be restored.
 *
 * The non-volatile registers need to be scrubbed to ensure they contain no
 * sensitive information that could compromise system security.  This is to
 * make sure that information will not be leaked from one application to
 * another via these volatile registers.
 *
 * Here, the integer registers (EAX, ECX, EDX) have been scrubbed.  Any changes
 * to this routine that alter the values of these registers MUST be reviewed
 * for potential security impacts.
 *
 * Floating point registers are handled using a lazy save/restore mechanism
 * since it's expected relatively few threads will be created with the
 * K_FP_REGS or K_SSE_REGS option bits.  The kernel data structure maintains a
 * 'current_fp' field to keep track of the thread that "owns" the floating
 * point registers.  Floating point registers consist of ST0->ST7 (x87 FPU and
 * MMX registers) and XMM0 -> XMM7.
 *
 * All floating point registers are considered 'volatile' thus they will only
 * be saved/restored when a preemptive context switch occurs.
 *
 * Floating point registers are currently NOT scrubbed, and are subject to
 * potential security leaks.
 *
 * C function prototype:
 *
 * unsigned int arch_swap (unsigned int eflags);
 */

SECTION_FUNC(PINNED_TEXT, arch_swap)
#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	pushl	%eax
	call	z_thread_mark_switched_out
	popl	%eax
#endif
	/*
	 * Push all non-volatile registers onto the stack; do not copy
	 * any of these registers into the k_thread.  Only the 'esp' register
	 * after all the pushes have been performed) will be stored in the
	 * k_thread.
	 */

	pushl	%edi

	movl	$_kernel, %edi

	pushl	%esi
	pushl	%ebx
	pushl	%ebp

	/*
	 * Carve space for the return value. Setting it to a default of
	 * -EAGAIN eliminates the need for the timeout code to set it.
	 * If another value is ever needed, it can be modified with
	 * arch_thread_return_value_set().
	 */

	pushl   _k_neg_eagain


	/* save esp into k_thread structure */

	movl	_kernel_offset_to_current(%edi), %edx
	movl	%esp, _thread_offset_to_esp(%edx)
	movl	_kernel_offset_to_ready_q_cache(%edi), %eax

	/*
	 * At this point, the %eax register contains the 'k_thread *' of the
	 * thread to be swapped in, and %edi still contains &_kernel. %edx
	 * has the pointer to the outgoing thread.
	 */
#if defined(CONFIG_X86_USERSPACE) && !defined(CONFIG_X86_KPTI)

	push	%eax
	call	z_x86_swap_update_page_tables
	pop	%eax

	/* Page tables updated. All memory access after this point needs to be
	 * to memory that has the same mappings and access attributes wrt
	 * supervisor mode!
	 */
#endif

#ifdef CONFIG_EAGER_FPU_SHARING
	/* Eager floating point state restore logic
	 *
	 * Addresses CVE-2018-3665
	 * Used as an alternate to CONFIG_LAZY_FPU_SHARING if there is any
	 * sensitive data in the floating point/SIMD registers in a system
	 * with untrusted threads.
	 *
	 * Unconditionally save/restore floating point registers on context
	 * switch.
	 */
	/* Save outgpoing thread context */
#ifdef CONFIG_X86_SSE
	fxsave	_thread_offset_to_preempFloatReg(%edx)
	fninit
#else
	fnsave	 _thread_offset_to_preempFloatReg(%edx)
#endif
	/* Restore incoming thread context */
#ifdef CONFIG_X86_SSE
	fxrstor _thread_offset_to_preempFloatReg(%eax)
#else
	frstor _thread_offset_to_preempFloatReg(%eax)
#endif /* CONFIG_X86_SSE */
#elif defined(CONFIG_LAZY_FPU_SHARING)
	/*
	 * Clear the CR0[TS] bit (in the event the current thread
	 * doesn't have floating point enabled) to prevent the "device not
	 * available" exception when executing the subsequent fxsave/fnsave
	 * and/or fxrstor/frstor instructions.
	 *
	 * Indeed, it's possible that none of the aforementioned instructions
	 * need to be executed, for example, the incoming thread doesn't
	 * utilize floating point operations.  However, the code responsible
	 * for setting the CR0[TS] bit appropriately for the incoming thread
	 * (just after the 'restoreContext_NoFloatSwap' label) will leverage
	 * the fact that the following 'clts' was performed already.
	 */

	clts


	/*
	 * Determine whether the incoming thread utilizes floating point regs
	 * _and_ whether the thread was context switched out preemptively.
	 */

	testb	$_FP_USER_MASK, _thread_offset_to_user_options(%eax)
	je 	restoreContext_NoFloatSwap


	/*
	 * The incoming thread uses floating point registers:
	 * Was it the last thread to use floating point registers?
	 * If so, there there is no need to restore the floating point context.
	 */

	movl	_kernel_offset_to_current_fp(%edi), %ebx
	cmpl	%ebx, %eax
	je	restoreContext_NoFloatSwap


	/*
	 * The incoming thread uses floating point registers and it was _not_
	 * the last thread to use those registers:
	 * Check whether the current FP context actually needs to be saved
	 * before swapping in the context of the incoming thread.
	 */

	testl	%ebx, %ebx
	jz	restoreContext_NoFloatSave


	/*
	 * The incoming thread uses floating point registers and it was _not_
	 * the last thread to use those registers _and_ the current FP context
	 * needs to be saved.
	 *
	 * Given that the ST[0] -> ST[7] and XMM0 -> XMM7 registers are all
	 * 'volatile', only save the registers if the "current FP context"
	 * was preemptively context switched.
	 */

	testb	$X86_THREAD_FLAG_ALL, _thread_offset_to_flags(%ebx)
	je	restoreContext_NoFloatSave


#ifdef CONFIG_X86_SSE
	testb	$K_SSE_REGS, _thread_offset_to_user_options(%ebx)
	je	x87FloatSave

	/*
	 * 'fxsave' does NOT perform an implicit 'fninit', therefore issue an
	 * 'fninit' to ensure a "clean" FPU state for the incoming thread
	 * (for the case when the fxrstor is not executed).
	 */

	fxsave	_thread_offset_to_preempFloatReg(%ebx)
	fninit
	jmp	floatSaveDone

x87FloatSave:
#endif /* CONFIG_X86_SSE */

	/* 'fnsave' performs an implicit 'fninit' after saving state! */

	fnsave	 _thread_offset_to_preempFloatReg(%ebx)

	/* fall through to 'floatSaveDone' */

floatSaveDone:
restoreContext_NoFloatSave:

	/*********************************************************
	 * Restore floating point context of the incoming thread.
	 *********************************************************/

	/*
	 * Again, given that the ST[0] -> ST[7] and XMM0 -> XMM7 registers are
	 * all 'volatile', only restore the registers if the incoming thread
	 * was previously preemptively context switched out.
	 */

	testb   $X86_THREAD_FLAG_ALL, _thread_offset_to_flags(%eax)
	je 	restoreContext_NoFloatRestore

#ifdef CONFIG_X86_SSE
	testb	$K_SSE_REGS, _thread_offset_to_user_options(%eax)
	je	x87FloatRestore

	fxrstor	_thread_offset_to_preempFloatReg(%eax)
	jmp	floatRestoreDone

x87FloatRestore:

#endif /* CONFIG_X86_SSE */

	frstor	_thread_offset_to_preempFloatReg(%eax)

	/* fall through to 'floatRestoreDone' */

floatRestoreDone:
restoreContext_NoFloatRestore:

	/* record that the incoming thread "owns" the floating point registers */

	movl	%eax, _kernel_offset_to_current_fp(%edi)


	/*
	 * Branch point when none of the floating point registers need to be
	 * swapped because: a) the incoming thread does not use them OR
	 * b) the incoming thread is the last thread that used those registers.
	 */

restoreContext_NoFloatSwap:

	/*
	 * Leave CR0[TS] clear if incoming thread utilizes the floating point
	 * registers
	 */

	testb	$_FP_USER_MASK, _thread_offset_to_user_options(%eax)
	jne	CROHandlingDone

	/*
	 * The incoming thread does NOT currently utilize the floating point
	 * registers, so set CR0[TS] to ensure the "device not available"
	 * exception occurs on the first attempt to access a x87 FPU, MMX,
	 * or XMM register.
	 */

	movl %cr0, %edx
	orl $0x8, %edx
	movl %edx, %cr0

CROHandlingDone:

#endif /* CONFIG_LAZY_FPU_SHARING */

	/* update _kernel.current to reflect incoming thread */

	movl    %eax, _kernel_offset_to_current(%edi)

#if defined(CONFIG_X86_USE_THREAD_LOCAL_STORAGE)
	pushl	%eax

	call	z_x86_tls_update_gdt

	/* Since segment descriptor has changed, need to reload */
	movw	$GS_TLS_SEG, %ax
	movw	%ax, %gs

	popl	%eax
#endif

	/* recover thread stack pointer from k_thread */

	movl	_thread_offset_to_esp(%eax), %esp


	/* load return value from a possible arch_thread_return_value_set() */

	popl	%eax

	/* pop the non-volatile registers from the stack */

	popl	%ebp
	popl	%ebx
	popl	%esi
	popl	%edi

	/*
	 * %eax may contain one of these values:
	 *
	 * - the return value for arch_swap() that was set up by a call to
	 * arch_thread_return_value_set()
	 * - -EINVAL
	 */

	/* Utilize the 'eflags' parameter to arch_swap() */

	pushl	4(%esp)
	popfl

#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
	pushl	%eax
	call	z_thread_mark_switched_in
	popl	%eax
#endif
	ret

#ifdef _THREAD_WRAPPER_REQUIRED
/**
 *
 * @brief Adjust stack/parameters before invoking thread entry function
 *
 * This function adjusts the initial stack frame created by arch_new_thread()
 * such that the GDB stack frame unwinders recognize it as the outermost frame
 * in the thread's stack.
 *
 * GDB normally stops unwinding a stack when it detects that it has
 * reached a function called main().  Kernel threads, however, do not have
 * a main() function, and there does not appear to be a simple way of stopping
 * the unwinding of the stack.
 *
 * Given the initial thread created by arch_new_thread(), GDB expects to find
 * a return address on the stack immediately above the thread entry routine
 * z_thread_entry, in the location occupied by the initial EFLAGS.  GDB
 * attempts to examine the memory at this return address, which typically
 * results in an invalid access to page 0 of memory.
 *
 * This function overwrites the initial EFLAGS with zero.  When GDB subsequently
 * attempts to examine memory at address zero, the PeekPoke driver detects
 * an invalid access to address zero and returns an error, which causes the
 * GDB stack unwinder to stop somewhat gracefully.
 *
 * The initial EFLAGS cannot be overwritten until after z_swap() has swapped in
 * the new thread for the first time.  This routine is called by z_swap() the
 * first time that the new thread is swapped in, and it jumps to
 * z_thread_entry after it has done its work.
 *
 *       __________________
 *      |      param3      |   <------ Top of the stack
 *      |__________________|
 *      |      param2      |           Stack Grows Down
 *      |__________________|                  |
 *      |      param1      |                  V
 *      |__________________|
 *      |      pEntry      |
 *      |__________________|
 *      | initial EFLAGS   |  <----   ESP when invoked by z_swap()
 *      |__________________|             (Zeroed by this routine)
 *
 * The address of the thread entry function needs to be in %edi when this is
 * invoked. It will either be z_thread_entry, or if userspace is enabled,
 * _arch_drop_to_user_mode if this is a user thread.
 *
 * @return this routine does NOT return.
 */

SECTION_FUNC(PINNED_TEXT, z_x86_thread_entry_wrapper)
	movl	$0, (%esp)
	jmp	*%edi
#endif /* _THREAD_WRAPPER_REQUIRED */
