/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file
 * @brief Nordic suspend-to-RAM code (S2RAM)
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>


GTEXT(pm_s2ram_mark_set)
SECTION_FUNC(TEXT, pm_s2ram_mark_set)
	/*
	 * Restore the PC and continue
	 */
	mov r15, r1

GTEXT(pm_s2ram_mark_check_and_clear)
SECTION_FUNC(TEXT, pm_s2ram_mark_check_and_clear)
	/*
	 * Set return value to 0
	 */
	mov	r0, #0

	/*
	 * Load check RESETREAS register
	 */
	ldr	r2, =CONFIG_NRF_RESETINFO_PERIPH_ADDRESS
	ldr 	r5, =CONFIG_NRF_RESETREAS_LOCAL_REG_OFFSET
	ldr 	r3, [r5, r2]
	ldr 	r6, =CONFIG_NRF_RESETREAS_LOCAL_UNRETAINEDWAKE_FIELD
	cmp	r3, r6

	bne	exit

	/*
	 * Clear RESETREAS
	 */
	str	r0, [r5, r2]

	/*
	 * Load RESTOREVALID register
	 */
	ldr 	r5, =CONFIG_NRF_RESTOREVALID_REG_OFFSET
	ldr 	r3, [r5, r2]

	/*
	 * Clear RESTOREVALID
	 */
	str	r0, [r5, r2]

	/*
	 * Check RESTOREVALID register
	 */
	ldr 	r5, =CONFIG_NRF_RESTOREVALID_PRESENT_FIELD
	cmp	r3, r5
	bne	exit

	/*
	 * Set return value to 1
	 */
	mov	r0, #1
exit:
	/*
	 * Restore the PC and continue
	 */
	mov r15, r1
