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

_ASM_FILE_PROLOGUE

GTEXT(pm_s2ram_mark_set)
GTEXT(pm_s2ram_mark_check_and_clear)
GDATA(_cpu_context)

/**
 * The following macros are used to save / load a single
 * special register, interacting with the __cpu_context
 * structure. The ___cpu_context_t_##_OFFSET definitions
 * are provided by C defines, so these MUST be C macros.
 */
#define SR_CTX_OFFSET(sr_name)	___cpu_context_t_ ## sr_name ## _OFFSET

#define SAVE_CPU_SR(ctx, tmp, sr_name)			\
	mrs	tmp, sr_name;				\
	str	tmp, [ ctx , # SR_CTX_OFFSET(sr_name) ];

#define RESTORE_CPU_SR(ctx, tmp, sr_name) 		\
        ldr     tmp, [ ctx , # SR_CTX_OFFSET(sr_name)];	\
        msr     sr_name, tmp;

/*
 * Save all special registers of the CPU to __cpu_context.
 *
 * Arguments:
 *	- ctx:	register containing `__cpu_context` address
 *	- tmp:	scratch register for internal usage in macro
 */
.macro save_special_registers ctx, tmp
	SAVE_CPU_SR(\ctx, \tmp, msp)
	SAVE_CPU_SR(\ctx, \tmp, psp)
	SAVE_CPU_SR(\ctx, \tmp, apsr)
	SAVE_CPU_SR(\ctx, \tmp, ipsr)
	SAVE_CPU_SR(\ctx, \tmp, epsr)
	SAVE_CPU_SR(\ctx, \tmp, primask)
	SAVE_CPU_SR(\ctx, \tmp, control)

#ifdef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	SAVE_CPU_SR(\ctx, \tmp, basepri)
	SAVE_CPU_SR(\ctx, \tmp, faultmask)
# ifdef CONFIG_ARMV8_M_MAINLINE
	SAVE_CPU_SR(\ctx, \tmp, msplim)
	SAVE_CPU_SR(\ctx, \tmp, psplim)
# endif
#endif
.endm

/*
 * Restore all special registers of the CPU from __cpu_context.
 *
 * Arguments:
 *	- ctx:	register containing `__cpu_context` address
 *	- tmp:	scratch register for internal usage in macro
 */
.macro restore_special_registers ctx, tmp
	RESTORE_CPU_SR(\ctx, \tmp, msp)
	RESTORE_CPU_SR(\ctx, \tmp, psp)
	RESTORE_CPU_SR(\ctx, \tmp, ipsr)
	RESTORE_CPU_SR(\ctx, \tmp, epsr)
	RESTORE_CPU_SR(\ctx, \tmp, primask)

	/*
	 * APSR can't be handled by the macro because
	 * the register name differs from the field
	 * name in the __cpu_context structure
	 */
	ldr	\tmp, [\ctx, # SR_CTX_OFFSET(apsr)]
	msr	apsr_nzcvq, \tmp

#ifdef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	RESTORE_CPU_SR(\ctx, \tmp, basepri)
	RESTORE_CPU_SR(\ctx, \tmp, faultmask)
# ifdef CONFIG_ARMV8_M_MAINLINE
	RESTORE_CPU_SR(\ctx, \tmp, msplim)
	RESTORE_CPU_SR(\ctx, \tmp, psplim)
# endif
#endif
	/* Restore CONTROL register last,
	 * as it must be followed by ISB.
	 */
	RESTORE_CPU_SR(\ctx, \tmp, control)
	isb
.endm

/*
 * WARNING: the push_gprs and pop_gprs macros are tightly
 * coupled. Do not modify one without adapting the other.
 */

/* Push registers r4~r12 and lr on the stack */
.macro push_gprs
#ifndef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* ARMv6-M `push` only supports r0~r7 and LR.
	 * r0 must be preserved (holds `system_off`).
	 */
	push	{ r4-r7 }
	mov	r1, r8
	mov	r2, r9
	mov	r3, r10
	mov	r4, r11
	mov	r5, r12
	push	{ r1-r5, lr }
#else
	/* ARMv7-M and ARMv8-M have no limitation on `push` */
	push	{ r5-r12 }
#endif
.endm

/* Pop registers r4-r12 and lr from the stack */
.macro pop_gprs
#ifndef CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	/* ARMv6-M `push` only supports r0~r7 and LR.
	 * r0 must be preserved (holds return value).
	 */
	pop	{ r1-r6 } /* can't pop lr to itself... */
	mov 	lr, r6
	mov 	r12, r5
	mov 	r11, r4
	mov 	r10, r3
	mov 	r9, r2
	mov 	r8, r1
	pop	{ r4-r7 }
#else
	/* ARMv7-M and ARMv8-M have no limitation on `push` */
	push	{ r5-r12 }
#endif
.endm

/*
 * arch_pm_s2ram_suspend: save CPU context before calling
 * the `system_off` function, whose address is in r0.
 *
 * Does not return if system enters suspended state.
 * Otherwise, must return what `system_off` returned.
 */
SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/* Save general-purpose registers on the current stack */
	push_gprs

	/* Save special registers in _cpu_context */
	ldr	r1, =_cpu_context
	save_special_registers r1, r2

	/* Mark that we are entering suspend-to-RAM */
	bl	pm_s2ram_mark_set

	/*
	 * Call the `system_off` function passed as parameter.
	 * This call will not return if system is suspended.
	 */
	blx	r0

	/*
	 * `system_off` failed and returned an error code in r0.
	 * System was not suspended, so the suspend-to-RAM mark
	 * needs to be cleared. Since we need to call a function,
	 * the error code must be saved somewhere other than r0.
	 * As r4 has been pushed on the stack, and is preserved,
	 * we can use it as temporary storage.
	 */
	mov	r4, r0

	/* Clear the suspend-to-RAM mark. Return value is ignored. */
	bl	pm_s2ram_mark_check_and_clear

	/* Restore error code saved earlier */
	mov	r0, r4

	/* Restore CPU registers from the stack and return */
	pop_gprs
	bx	lr

GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if reset occurred after suspending to RAM.
	 */
	push    { lr }
	bl      pm_s2ram_mark_check_and_clear
	cmp	r0, #0x1
	beq	resume
	pop 	{ pc }	/* not resume reset; return to caller */

resume:
	/* Pop the previously pushed `lr` (value ignored) */
	pop	{ r0 }

	/*
	 * Restore special registers from _cpu_context.
	 * This includes reloading of the stack pointer.
	 */
	ldr	r0, =_cpu_context
	restore_special_registers r0, r1

	/* Restore CPU registers from the stack */
	pop_gprs

	/* Set the return value and return */
	movs	r0, #0
	bx	lr
