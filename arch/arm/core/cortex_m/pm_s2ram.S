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
 * Macro expanding to an integer literal equal to the offset of
 * field `sr_name` in `struct __cpu_context`. This macro has to
 * be implemented in C, because GEN_OFFSET_SYM provides offsets
 * as C preprocessor definitions - there are not visible to the
 * assembler.
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

SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/*
	 * Save the CPU context
	 *
	 * r0: address of the system_off function
	 */
	push	{r4-r12, lr}

	/* Move system_off to protected register. */
	mov 	r4, r0

	/* Store CPU context */
	ldr	r1, =_cpu_context

	SAVE_SPECIAL_REG(msp, r1, r2)

	SAVE_SPECIAL_REG(msplim, r1, r2)

	SAVE_SPECIAL_REG(psp, r1, r2)

	SAVE_SPECIAL_REG(psplim, r1, r2)

	SAVE_SPECIAL_REG(primask, r1, r2)

	SAVE_SPECIAL_REG(faultmask, r1, r2)

	SAVE_SPECIAL_REG(basepri, r1, r2)

	SAVE_SPECIAL_REG(control, r1, r2)

	/*
	 * Mark entering suspend to RAM.
	 */
	mov 	r1, lr
	bl 	pm_s2ram_mark_set
	mov	lr, r1

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
	mov 	r1, lr
	bl	pm_s2ram_mark_check_and_clear
	mov 	lr, r1

	/* Move system_off back to r0 as return value */
	mov	r0, r4

	pop	{r4-r12, lr}
	bx	lr


GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if reset occurred after suspending to RAM.
	 */
	mov 	r1, lr
	bl 	pm_s2ram_mark_check_and_clear
	mov 	lr, r1
	cmp     r0, #0x1
	beq     resume
	bx      lr

resume:
	/*
	 * Restore the CPU context
	 */
	ldr	r0, =_cpu_context

	RESTORE_SPECIAL_REG(msp, r0, r1)

	RESTORE_SPECIAL_REG(msplim, r0, r1)

	RESTORE_SPECIAL_REG(psp, r0, r1)

	RESTORE_SPECIAL_REG(psplim, r0, r1)

	RESTORE_SPECIAL_REG(primask, r0, r1)

	RESTORE_SPECIAL_REG(faultmask, r0, r1)

	RESTORE_SPECIAL_REG(basepri, r0, r1)

	RESTORE_SPECIAL_REG(control, r0, r1)
	isb

	pop	{r4-r12, lr}

	/*
	 * Set the return value and return
	 */
	mov	r0, #0
	bx	lr
