/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm64/tpidrro_el0.h>

#ifdef _ASMLANGUAGE

/*
 * Get CPU id
 */

.macro get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm

/*
 * Get CPU logic id by looking up cpu_node_list
 * returns
 *   xreg0: MPID
 *   xreg1: logic id (0 ~ CONFIG_MP_MAX_NUM_CPUS - 1)
 * clobbers: xreg0, xreg1, xreg2, xreg3
 */
.macro get_cpu_logic_id xreg0, xreg1, xreg2, xreg3
	get_cpu_id \xreg0
	ldr	\xreg3, =cpu_node_list
	mov	\xreg1, 0
1:	ldr	\xreg2, [\xreg3, \xreg1, lsl 3]
	cmp	\xreg2, \xreg0
	beq	2f
	add	\xreg1, \xreg1, 1
	cmp	\xreg1, #CONFIG_MP_MAX_NUM_CPUS
	bne	1b
	b	.
2:
.endm

/*
 * Get CPU pointer
 * Note: keep in sync with `arch_curr_cpu` in include/zephyr/arch/arm64/arch_inlines.h
 */

.macro get_cpu xreg0
	mrs	\xreg0, tpidrro_el0
	and	\xreg0, \xreg0, #TPIDRROEL0_CURR_CPU
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
