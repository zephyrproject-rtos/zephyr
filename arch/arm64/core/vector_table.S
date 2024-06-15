/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Populated vector table
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/offsets.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/tpidrro_el0.h>
#include <offsets_short.h>
#include "macro_priv.inc"


_ASM_FILE_PROLOGUE

/*
 * Save volatile registers, LR, SPSR_EL1 and ELR_EL1
 *
 * Save the volatile registers and LR on the process stack. This is
 * needed if the thread is switched out because they can be clobbered by the
 * ISR and/or context switch.
 */

.macro z_arm64_enter_exc xreg0, xreg1, el
	/*
	 * Two things can happen to the remaining registers:
	 *
	 * - No context-switch: in this case x19-x28 are callee-saved register
	 *   so we can be sure they are not going to be clobbered by ISR.
	 * - Context-switch: the callee-saved registers are saved by
	 *   z_arm64_context_switch() in the kernel structure.
	 */

	sub	sp, sp, ___esf_t_SIZEOF

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	.if	\el == el1
	/*
	 * EL1t mode cannot access sp_el1, so set x0 to sp_el1 without corrupt
	 * other registers
	 */
	add	sp, sp, x0			// sp' = sp + x0
	sub	x0, sp, x0			// x0' = sp' - x0 = sp
	msr	SPSel, #0
	stp	x16, x17, [sp, -(___esf_t_SIZEOF - ___esf_t_x16_x17_OFFSET)]
	stp	x18, lr,  [sp, -(___esf_t_SIZEOF - ___esf_t_x18_lr_OFFSET)]
	bl	z_arm64_quick_stack_check
	.endif
#endif

	stp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	stp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	stp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]
	stp	x6, x7, [sp, ___esf_t_x6_x7_OFFSET]
	stp	x8, x9, [sp, ___esf_t_x8_x9_OFFSET]
	stp	x10, x11, [sp, ___esf_t_x10_x11_OFFSET]
	stp	x12, x13, [sp, ___esf_t_x12_x13_OFFSET]
	stp	x14, x15, [sp, ___esf_t_x14_x15_OFFSET]
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	/* The expection from el1 does not need to save x16, x17, x18 and lr */
	.if	\el == el0
#endif
	stp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	stp	x18, lr, [sp, ___esf_t_x18_lr_OFFSET]
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	.endif
#endif

#ifdef CONFIG_FRAME_POINTER
	str	x29, [sp, ___esf_t_fp_OFFSET]
#endif

	mrs	\xreg0, spsr_el1
	mrs	\xreg1, elr_el1
	stp	\xreg0, \xreg1, [sp, ___esf_t_spsr_elr_OFFSET]

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	.if	\el == el0
	mrs	x0, sp_el0
	str	x0, [sp, ___esf_t_sp_el0_OFFSET]

	/* Retrieving safe exception stack */
	get_cpu	x0
	ldr	x1, [x0, #_cpu_offset_to_safe_exception_stack]
	msr	sp_el0, x1
	.endif
#endif

	/* Clear usermode flag and increment exception depth */
	mrs	\xreg0, tpidrro_el0
	mov	\xreg1, #TPIDRROEL0_EXC_UNIT
	bic	\xreg0, \xreg0, #TPIDRROEL0_IN_EL0
	add	\xreg0, \xreg0, \xreg1
	msr	tpidrro_el0, \xreg0

#ifdef CONFIG_FPU_SHARING
	bl	z_arm64_fpu_enter_exc
#endif

.endm

/*
 * Four types of exceptions:
 * - synchronous: aborts from MMU, SP/CP alignment checking, unallocated
 *   instructions, SVCs/SMCs/HVCs, ...)
 * - IRQ: group 1 (normal) interrupts
 * - FIQ: group 0 or secure interrupts
 * - SError: fatal system errors
 *
 * Four different contexts:
 * - from same exception level, when using the SP_EL0 stack pointer
 * - from same exception level, when using the SP_ELx stack pointer
 * - from lower exception level, when this is AArch64
 * - from lower exception level, when this is AArch32
 *
 * +------------------+------------------+-------------------------+
 * |     Address      |  Exception type  |       Description       |
 * +------------------+------------------+-------------------------+
 * | VBAR_ELn + 0x000 | Synchronous      | Current EL with SP0     |
 * |          + 0x080 | IRQ / vIRQ       |                         |
 * |          + 0x100 | FIQ / vFIQ       |                         |
 * |          + 0x180 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x200 | Synchronous      | Current EL with SPx     |
 * |          + 0x280 | IRQ / vIRQ       |                         |
 * |          + 0x300 | FIQ / vFIQ       |                         |
 * |          + 0x380 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x400 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch32  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

GDATA(_vector_table)
SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)

	/* The whole table must be 2K aligned */
	.align 11

	/* Current EL with SP0 / Synchronous */
	.align 7
	z_arm64_enter_exc x0, x1, el1
	b	z_arm64_sync_exc

	/* Current EL with SP0 / IRQ */
	.align 7
	z_arm64_enter_exc x0, x1, el1
#ifdef CONFIG_GEN_SW_ISR_TABLE
	b 	_isr_wrapper
#else
	b	z_irq_spurious
#endif

	/* Current EL with SP0 / FIQ */
	.align 7
	b 	.

	/* Current EL with SP0 / SError */
	.align 7
	z_arm64_enter_exc x0, x1, el1
	b	z_arm64_serror

	/* Current EL with SPx / Synchronous */
	.align 7
	z_arm64_enter_exc x0, x1, el1
	b	z_arm64_sync_exc

	/* Current EL with SPx / IRQ */
	.align 7
	z_arm64_enter_exc x0, x1, el1
#ifdef CONFIG_GEN_SW_ISR_TABLE
	b 	_isr_wrapper
#else
	b	z_irq_spurious
#endif

	/* Current EL with SPx / FIQ */
	.align 7
	b	.

	/* Current EL with SPx / SError */
	.align 7
	z_arm64_enter_exc x0, x1, el1
	b	z_arm64_serror

	/* Lower EL using AArch64 / Synchronous */
	.align 7
	z_arm64_enter_exc x0, x1, el0
	b	z_arm64_sync_exc

	/* Lower EL using AArch64 / IRQ */
	.align 7
	z_arm64_enter_exc x0, x1, el0
#ifdef CONFIG_GEN_SW_ISR_TABLE
	b 	_isr_wrapper
#else
	b	z_irq_spurious
#endif

	/* Lower EL using AArch64 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch64 / SError */
	.align 7
	z_arm64_enter_exc x0, x1, el0
	b	z_arm64_serror

	/* Lower EL using AArch32 / Synchronous */
	.align 7
	b	.

	/* Lower EL using AArch32 / IRQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / SError */
	.align 7
	b	.

GTEXT(z_arm64_serror)
SECTION_FUNC(TEXT, z_arm64_serror)

	mov	x1, sp
	mov	x0, #0 /* K_ERR_CPU_EXCEPTION */

	bl	z_arm64_fatal_error
	/* Return here only in case of recoverable error */
	b	z_arm64_exit_exc

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
GTEXT(z_arm64_quick_stack_check)
SECTION_FUNC(TEXT, z_arm64_quick_stack_check)
	/*
	 * x0 is SP_EL1
	 * Retrieve the current stack limit
	 */
	get_cpu	x16
	ldr	x17, [x16, #_cpu_offset_to_current_stack_limit]
	/*
	 * If priv sp <= the stack limit, then keep the safe exception stack
	 * go to the stack overflow process.
	 */
	cmp	x0, x17
	/* Restore the sp_el1 */
	msr	SPSel, #1			// switch sp to sp_el1
	sub	x0, sp, x0			// x0'' = sp' - x0' = x0
	sub	sp, sp, x0			// sp'' = sp' - x0 = sp
	ble	1f
	/*
	 * If the stack does not overflow, keep using sp_el1, copy the original
	 * x16, x17, x18, lr from sp_el0 (safe_exception_stack) to sp_el1. So
	 * the four registers can be restroed directly from sp_el1 without a
	 * stack mode switch.
	 */
	mrs	x18, sp_el0
	ldp	x16, x17, [x18, -(___esf_t_SIZEOF - ___esf_t_x16_x17_OFFSET)]
	stp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	ldp	x16, x17, [x18, -(___esf_t_SIZEOF - ___esf_t_x18_lr_OFFSET)]
	stp	x16, x17, [sp, ___esf_t_x18_lr_OFFSET]
	ret
1:	/*
	 * If stack overflow, save the current sp and then switch sp to safe
	 * exception stack
	 * x16 is still the current _cpu
	 */
	mrs	x18, sp_el0
	mov	x17, sp
	str	x17, [x16, #_cpu_offset_to_corrupted_sp]
	/*
	 * switch sp to safe exception stack, which means we handle the fatal
	 * error with safe exception stack.
	 */
	sub	sp, x18, ___esf_t_SIZEOF
	ret
#endif

/*
 * Restore volatile registers, LR, SPSR_EL1 and ELR_EL1
 *
 * This is the common exit point for z_arm64_sync_exc() and _isr_wrapper().
 */

GTEXT(z_arm64_exit_exc)
SECTION_FUNC(TEXT, z_arm64_exit_exc)

#ifdef CONFIG_FPU_SHARING
	bl	z_arm64_fpu_exit_exc

 GTEXT(z_arm64_exit_exc_fpu_done)
 z_arm64_exit_exc_fpu_done:
#endif

	ldp	x0, x1, [sp, ___esf_t_spsr_elr_OFFSET]
	msr	spsr_el1, x0
	msr	elr_el1, x1

	/* Restore the kernel/user mode flag and decrement exception depth */
	tst	x0, #SPSR_MODE_MASK	/* EL0 == 0 */
	mrs	x0, tpidrro_el0
	mov	x1, #TPIDRROEL0_EXC_UNIT
	orr	x2, x0, #TPIDRROEL0_IN_EL0
	csel	x0, x2, x0, eq
	sub	x0, x0, x1
	msr	tpidrro_el0, x0

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	bne	1f
	ldr	x0, [sp, ___esf_t_sp_el0_OFFSET]
	msr	sp_el0, x0
1:
#endif

	ldp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	ldp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	ldp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]
	ldp	x6, x7, [sp, ___esf_t_x6_x7_OFFSET]
	ldp	x8, x9, [sp, ___esf_t_x8_x9_OFFSET]
	ldp	x10, x11, [sp, ___esf_t_x10_x11_OFFSET]
	ldp	x12, x13, [sp, ___esf_t_x12_x13_OFFSET]
	ldp	x14, x15, [sp, ___esf_t_x14_x15_OFFSET]
	ldp	x16, x17, [sp, ___esf_t_x16_x17_OFFSET]
	ldp	x18, lr, [sp, ___esf_t_x18_lr_OFFSET]

#ifdef CONFIG_FRAME_POINTER
	ldr	x29, [sp,  ___esf_t_fp_OFFSET]
#endif

	add	sp, sp, ___esf_t_SIZEOF

	/*
	 * In general in the ELR_EL1 register we can find:
	 *
	 * - The address of ret in z_arm64_call_svc()
	 * - The address of the next instruction at the time of the IRQ when the
	 *   thread was switched out.
	 * - The address of z_thread_entry() for new threads (see thread.c).
	 */
	eret

