/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M suspend-to-RAM code (S2RAM)
 */

#include <zephyr/toolchain.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>

/**
 * Macro expanding to the (integer literal) offset of field
 * `sr_name` in the __cpu_context structure. This macro has
 * to be implemented in C because GEN_OFFSET_SYM provides the
 * offsets as C preprocessor definitions; they are not available
 * by the time the assembler macros are processed.
 *
 * See also: `arch/arm/core/offsets/offsets_aarch32.c`
 */
#define CPU_CTX_SR_OFFSET(sr_name) \
	___cpu_context_t_ ## sr_name ## _OFFSET

/**
 * Macros used to save / load a special register in __cpu_context.
 * These also have to be implemented in C due to CPU_CTX_SR_OFFSET.
 */
#define SAVE_SPECIAL_REG(sr_name, cpu_ctx_reg, tmp_reg)	\
	mrs	tmp_reg, sr_name;			\
	str	tmp_reg, [cpu_ctx_reg, # CPU_CTX_SR_OFFSET(sr_name)];

#define RESTORE_SPECIAL_REG(sr_name, cpu_ctx_reg, tmp_reg)		\
	ldr	tmp_reg, [cpu_ctx_reg, # CPU_CTX_SR_OFFSET(sr_name)];	\
	msr	sr_name, tmp_reg;

_ASM_FILE_PROLOGUE

GTEXT(pm_s2ram_mark_set)
GTEXT(pm_s2ram_mark_check_and_clear)
GDATA(_cpu_context)

# Pushes registers r4~r12 and lr on the stack.
# r0 is unmodified, other GPRs may be overwritten.
.macro push_gprs
#ifndef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* `push` on ARMv6-M / ARMv8-M Baseline:
	 * only r0~r7 and lr may be pushed
	 */
	push	{ r4-r7 }
	mov	r1, r8
	mov	r2, r9
	mov	r3, r10
	mov	r4, r11
	mov	r5, r12
	push	{ r1-r5, lr }
#else
	/* `push` on ARMv7-M and ARMv8-M Mainline: no limitation */
	push	{r4-r12, lr}
#endif /* !CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
.endm

# Pops registers r4~r12 and lr from the stack
# r0 is unmodified, other GPRs may be overwritten.
.macro pop_gprs
#ifndef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* `pop` on ARMv6-M / ARMv8-M Baseline:
	 * can only pop to r0~r7 and pc (not lr!)
	 */
	pop	{ r1-r6 } /* pop lr through r6 */
	mov 	lr, r6
	mov 	r12, r5
	mov 	r11, r4
	mov 	r10, r3
	mov 	r9, r2
	mov 	r8, r1
	pop	{ r4-r7 }
#else
	/* `pop` on ARMv7-M and ARMv8-M Mainline: no limitation */
	pop	{r4-r12, lr}
#endif /* !CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
.endm

# Saves all special registers in the provided __cpu_context structure.
.macro save_special_registers cpu_ctx, tmp_reg
	SAVE_SPECIAL_REG(msp,		\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(psp,		\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(apsr,		\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(primask,	\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(control,	\cpu_ctx, \tmp_reg)
#ifdef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* Registers present on ARMv7-M and ARMv8-M Mainline */
	SAVE_SPECIAL_REG(faultmask,	\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(basepri,	\cpu_ctx, \tmp_reg)
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
	/* Registers present only on ARMv8-M (Baseline / Mainline) */
	SAVE_SPECIAL_REG(msplim,	\cpu_ctx, \tmp_reg)
	SAVE_SPECIAL_REG(psplim,	\cpu_ctx, \tmp_reg)
#endif /* CONFIG_ARMV8_M_BASELINE || CONFIG_ARMV8_M_MAINLINE */
.endm

# Restores all special registers from the provided __cpu_context structure.
.macro restore_special_registers cpu_ctx, tmp_reg
#ifdef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* Registers present on ARMv7-M and ARMv8-M Mainline */
	RESTORE_SPECIAL_REG(faultmask,	\cpu_ctx, \tmp_reg)
	RESTORE_SPECIAL_REG(basepri,	\cpu_ctx, \tmp_reg)
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
	/* Registers present only on ARMv8-M (Baseline / Mainline) */
	RESTORE_SPECIAL_REG(msplim,	\cpu_ctx, \tmp_reg)
	RESTORE_SPECIAL_REG(psplim,	\cpu_ctx, \tmp_reg)
#endif /* CONFIG_ARMV8_M_BASELINE || CONFIG_ARMV8_M_MAINLINE */

	RESTORE_SPECIAL_REG(msp,	\cpu_ctx, \tmp_reg)
	RESTORE_SPECIAL_REG(psp,	\cpu_ctx, \tmp_reg)

	# APSR has different name between MRS and MSR
	ldr	\tmp_reg, [\cpu_ctx, # CPU_CTX_SR_OFFSET(apsr)]
	msr	apsr_nzcvq, \tmp_reg

	RESTORE_SPECIAL_REG(primask,	\cpu_ctx, \tmp_reg)
	RESTORE_SPECIAL_REG(control,	\cpu_ctx, \tmp_reg)
	isb
.endm

SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/*
	 * Save the CPU context
	 *
	 * r0: address of the system_off function
	 */
	push_gprs

	/* Move system_off to protected register. */
	mov 	r4, r0

	/* Store CPU context */
	ldr	r1, =_cpu_context

	save_special_registers r1, r2

	/*
	 * Mark entering suspend to RAM.
	 */
	bl pm_s2ram_mark_set

	/*
	 * Call the system_off function passed as parameter. This should never
	 * return.
	 */
	blx	r4

	/*
	 * The system_off function returns here only when the powering off was
	 * not successful (in r0 the return value).
	 */

	/*
	 * Reset the marking of suspend to RAM, return is ignored.
	 */
	bl pm_s2ram_mark_check_and_clear

	/* Move system_off back to r0 as return value */
	mov	r0, r4

	pop_gprs
	bx	lr


GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if reset occurred after suspending to RAM.
	 * Store LR to ensure we can continue boot when we are not suspended
	 * to RAM. In addition to LR, R0 is pushed too, to ensure "SP mod 8 = 0",
	 * as stated by ARM rule 6.2.1.2 for AAPCS32.
	 */
	push    {r0, lr}
	bl      pm_s2ram_mark_check_and_clear
	cmp	r0, #0x1
	beq	resume
	pop	{r0, pc}

resume:
	/* Revert effect of prologue's `push { r0, lr }` */
	add	sp, #8

	/*
	 * Restore the CPU context
	 */
	ldr	r0, =_cpu_context

	restore_special_registers r0, r1

	pop_gprs

	/*
	 * Set the return value and return
	 */
	movs	r0, #0
	bx	lr
