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

#if !(CONFIG_PM_S2RAM_CUSTOM_MARKING)
#define MAGIC (0xDABBAD00)
GDATA(marker)

SECTION_FUNC(TEXT, pm_s2ram_mark_set)
	/*
	 * Set the marker to MAGIC value
	 */
	ldr	r0, =marker
	ldr	r2, =MAGIC
	str	r2, [r0]

	/*
	 * Restore the PC and continue
	 */
	mov r15, r1

SECTION_FUNC(TEXT, pm_s2ram_mark_check_and_clear)
	/* Set return value to 0 */
	mov	r0, #0

	/*
	 * Check the marker
	 */
	ldr	r2, =marker
	ldr	r5, [r2]
	ldr	r3, =MAGIC
	cmp	r5, r3
	bne	exit

	/*
	 * Reset the marker
	 */
	str	r0, [r2]

	/*
	 * Set return value to 1
	 */
	mov	r0, #1

exit:
	/*
	 * Restore the PC and continue
	 */
	mov r15, r1
#endif

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

	mrs	r2, apsr
	str	r2, [r1, #___cpu_context_t_apsr_OFFSET]

	mrs	r2, ipsr
	str	r2, [r1, #___cpu_context_t_ipsr_OFFSET]

	mrs	r2, epsr
	str	r2, [r1, #___cpu_context_t_epsr_OFFSET]

	mrs	r2, primask
	str	r2, [r1, #___cpu_context_t_primask_OFFSET]

	mrs	r2, faultmask
	str	r2, [r1, #___cpu_context_t_faultmask_OFFSET]

	mrs	r2, basepri
	str	r2, [r1, #___cpu_context_t_basepri_OFFSET]

	mrs	r2, control
	str	r2, [r1, #___cpu_context_t_control_OFFSET]

	/*
	 * Save the return address from `pm_s2ram_mark_set`
	 */
	mov r1, r15
	add r1, #6

	/*
	 * Mark entering suspend to RAM.
	 */
	b pm_s2ram_mark_set

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
	 * Save the return address from `pm_s2ram_mark_check_and_clear`
	 */
	mov r1, r15
	add r1, #6

	/*
	 * Reset the marking of suspend to RAM, return is ignored.
	 */
	b pm_s2ram_mark_check_and_clear

	/* Move system_off back to r0 as return value */
	mov	r0, r4

	pop	{r4-r12, lr}
	bx	lr


GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)


	/*
	 * Save the return address from `pm_s2ram_mark_check_and_clear`
	 */
	mov 	r1, r15
	add 	r1, #6

	/*
	 * Check if reset occurred after suspending to RAM.
	 */
	b 	pm_s2ram_mark_check_and_clear
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

	ldr	r1, [r0, #___cpu_context_t_apsr_OFFSET]
	msr	apsr_nzcvq, r1

	ldr	r1, [r0, #___cpu_context_t_ipsr_OFFSET]
	msr	ipsr, r1

	ldr	r1, [r0, #___cpu_context_t_epsr_OFFSET]
	msr	epsr, r1

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
