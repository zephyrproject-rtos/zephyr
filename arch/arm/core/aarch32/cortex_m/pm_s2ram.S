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

#define MAGIC (0xDABBAD00)

_ASM_FILE_PROLOGUE

GDATA(_cpu_context)
GDATA(marker)

SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/*
	 * Save the CPU context
	 *
	 * r0: address of the system_off function
	 */
	push	{r4-r12, r14}
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
	 * Set the marker to MAGIC value
	 */
	ldr	r1, =marker
	ldr	r2, =MAGIC
	str	r2, [r1]

	/*
	 * Call the system_off function passed as parameter
	 */
	bx	r0

GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if the marker is set
	 */
	ldr	r0, =marker
	ldr	r0, [r0]
	ldr	r1, =MAGIC
	cmp	r0, r1
	beq	resume
	bx	lr

resume:
	/*
	 * Reset the marker
	 */
	ldr	r0, =marker
	mov	r1, #0x0
	str	r1, [r0]

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

	pop	{r4-r12, r14}

	/*
	 * Set the return value and return
	 */
	mov	r0, #0
	bx	lr
