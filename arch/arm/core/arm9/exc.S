/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 exception handlers
 *
 * Undefined instruction, supervisor call, prefetch abort, data abort, FIQ and
 * the reserved vector. All of them park the interrupted state in the per-mode
 * scratch area, switch to system mode and build a struct arch_esf on the
 * interrupted stack (see macro_priv.inc).
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_fault_undef_instruction)
GTEXT(z_arm_fault_prefetch)
GTEXT(z_arm_fault_data)
GTEXT(z_arm_fatal_error)
GTEXT(z_do_kernel_oops)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)
GTEXT(z_arm_svc)
GTEXT(z_arm_fiq)
GTEXT(z_arm_reserved)
GTEXT(z_armv5_exit_exc)

/**
 * @brief Return from an exception
 *
 * Called in system mode with sp pointing at the exception stack frame.
 *
 * ARMv5 has no RFE, and system mode has no SPSR, so the return address and
 * the target CPSR are handed over to supervisor mode (which does have an
 * SPSR) and the actual return is done with `movs pc, lr`. IRQs and FIQs must
 * stay masked all the way through, otherwise an interrupt taken in between
 * would clobber lr_svc/spsr_svc.
 */
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_armv5_exit_exc)

	/* decrement exception depth */
	get_cpu	r2
	ldrb	r1, [r2, #_cpu_offset_to_exc_depth]
	sub	r1, r1, #1
	strb	r1, [r2, #_cpu_offset_to_exc_depth]

	ldr	r0, [sp, #24]			/* return address */
	ldr	r1, [sp, #28]			/* CPSR to return to */

	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
	mov	lr, r0
	msr	spsr_cxsf, r1
	msr	cpsr_c, #(MODE_SYS | I_BIT | F_BIT)

	ldmia	sp!, {r0-r3, r12, lr}
	add	sp, sp, #8			/* drop saved pc/xpsr */

	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
	movs	pc, lr

/**
 * @brief Undefined instruction exception handler
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_undef_instruction)
	/*
	 * The faulting instruction is at lr-4 in ARM state and at lr-2 in
	 * Thumb state. sp already points at this mode's scratch area, so it
	 * can be borrowed to free up a register.
	 */
	str	r0, [sp]
	mrs	r0, spsr
	tst	r0, #T_BIT
	subeq	lr, lr, #4
	subne	lr, lr, #2
	ldr	r0, [sp]

	z_armv5_enter_exc
	bl	z_arm_fault_undef_instruction
	b	z_armv5_exit_exc

/**
 * @brief Prefetch abort exception handler
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_prefetch_abort)
	sub	lr, lr, #4
	z_armv5_enter_exc
	bl	z_arm_fault_prefetch
	b	z_armv5_exit_exc

/**
 * @brief Data abort exception handler
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_data_abort)
	sub	lr, lr, #8
	z_armv5_enter_exc
	bl	z_arm_fault_data
	b	z_armv5_exit_exc

/**
 * @brief FIQ handler
 *
 * FIQs are not used by this port; treat one as a fatal CPU exception rather
 * than silently returning into a system that cannot make progress.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_fiq)
	sub	lr, lr, #4
	z_armv5_enter_exc
	mov	r1, r0
	mov	r0, #0				/* K_ERR_CPU_EXCEPTION */
	bl	z_arm_fatal_error
	b	z_armv5_exit_exc

/**
 * @brief Reserved vector
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_reserved)
	z_armv5_enter_exc
	mov	r1, r0
	mov	r0, #0				/* K_ERR_CPU_EXCEPTION */
	bl	z_arm_fatal_error
	b	z_armv5_exit_exc

/**
 * @brief Supervisor call handler
 *
 * Under CONFIG_USE_SWITCH the svc instruction is only used for IRQ offloading
 * and for software generated fatal exceptions (k_oops()/k_panic()).
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_svc)
	z_armv5_enter_exc

	/* Recover the svc immediate from the instruction that trapped. */
	ldr	r1, [r0, #28]			/* interrupted CPSR */
	ldr	r2, [r0, #24]			/* address after the svc */
	tst	r1, #T_BIT
	bne	1f
	ldr	r1, [r2, #-4]			/* ARM: svc #imm24 */
	bic	r1, r1, #0xff000000
	b	2f
1:
	ldrb	r1, [r2, #-2]			/* Thumb: svc #imm8 */
2:
	cmp	r1, #_SVC_CALL_RUNTIME_EXCEPT
	beq	_oops

#ifdef CONFIG_IRQ_OFFLOAD
	cmp	r1, #_SVC_CALL_IRQ_OFFLOAD
	bne	_svc_inv

	/* Account the offloaded ISR like a real interrupt. */
	get_cpu	r2
	ldr	r3, [r2, #___cpu_t_nested_OFFSET]
	add	r3, r3, #1
	str	r3, [r2, #___cpu_t_nested_OFFSET]

	cmp	r3, #1
	bhi	1f
	mov	r0, sp
	ldr	sp, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	stmdb	sp!, {r0, r1}
1:
	bl	z_irq_do_offload
	b	z_armv5_irq_done
#endif /* CONFIG_IRQ_OFFLOAD */

_svc_inv:
	mov	r1, r0
	mov	r0, #0				/* K_ERR_CPU_EXCEPTION */
	bl	z_arm_fatal_error
	b	z_armv5_exit_exc

_oops:
	/* r0 already points at the exception stack frame */
	mov	r1, #0				/* callee_regs (Cortex-M only) */
	mov	r2, #0				/* exc_return (Cortex-M only) */
	bl	z_do_kernel_oops
	b	z_armv5_exit_exc
