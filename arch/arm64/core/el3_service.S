/*
 * Copyright (c) 2023 Chad Karaginides <quic_chadk@quicinc.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements the logic to check that all criteria are met
 * to enter an EL3 service handler.
 * 
 * Service available:
 * Secure Monitor Call (SMC) handler.
 *
 * See https://developer.arm.com/docs/den0028/latest
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>

GTEXT(z_arm64_el3_service)
SECTION_FUNC(TEXT, z_arm64_el3_service)

	mrs	x0, currentEL
	lsr     x0, x0, #2
	cmp	x0, #3         /* Check if in EL3 */
	bne	1f
	mrs	x0, esr_el3
	lsr	x0, x0, #26
	cmp	x0, #0x17      /* 0x17 = SMC */
	bne	1f
 
	/*
	 * Copy of registers/SMC arguments already exists
	 * on the stack.  Pass pointer to them to SMC handler.
         *
         * Results are returned the same way (via stack) so no copying
         * is required when returning from exception.
	 * z_arm64_exit_exc handles copying from the stack to registers.
	 */
	add	x0, sp, ___esf_t_x0_x1_OFFSET
	bl	z_arm64_smc_handler
	b	z_arm64_exit_exc	
1:
        ret
