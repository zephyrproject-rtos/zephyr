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

	mrs	r2, msp
	str	r2, [r1, #___cpu_context_t_msp_OFFSET]

	mrs	r2, msplim
	str	r2, [r1, #___cpu_context_t_msplim_OFFSET]

	mrs	r2, psp
	str	r2, [r1, #___cpu_context_t_psp_OFFSET]

	mrs	r2, psplim
	str	r2, [r1, #___cpu_context_t_psplim_OFFSET]

	mrs	r2, primask
	str	r2, [r1, #___cpu_context_t_primask_OFFSET]

	mrs	r2, faultmask
	str	r2, [r1, #___cpu_context_t_faultmask_OFFSET]

	mrs	r2, basepri
	str	r2, [r1, #___cpu_context_t_basepri_OFFSET]

	mrs	r2, control
	str	r2, [r1, #___cpu_context_t_control_OFFSET]

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

	ldr	r1, [r0, #___cpu_context_t_msp_OFFSET]
	msr	msp, r1

	ldr	r1, [r0, #___cpu_context_t_msplim_OFFSET]
	msr	msplim, r1

	ldr	r1, [r0, #___cpu_context_t_psp_OFFSET]
	msr	psp, r1

	ldr	r1, [r0, #___cpu_context_t_psplim_OFFSET]
	msr	psplim, r1

	ldr	r1, [r0, #___cpu_context_t_primask_OFFSET]
	msr	primask, r1

	ldr	r1, [r0, #___cpu_context_t_faultmask_OFFSET]
	msr	faultmask, r1

	ldr	r1, [r0, #___cpu_context_t_basepri_OFFSET]
	msr	basepri, r1

	ldr	r1, [r0, #___cpu_context_t_control_OFFSET]
	msr	control, r1
	isb

	pop	{r4-r12, lr}

	/*
	 * Set the return value and return
	 */
	mov	r0, #0
	bx	lr
