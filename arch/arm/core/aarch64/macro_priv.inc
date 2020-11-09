/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifdef _ASMLANGUAGE

.macro z_arm64_get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm
/*
 * Increment nested counter
 */

.macro inc_nest_counter
	bl	z_arm64_curr_cpu
	ldr	x1, [x0, #___cpu_t_nested_OFFSET]
	add	x1, x1, #1
	str	x1, [x0, #___cpu_t_nested_OFFSET]
.endm

/*
 * Decrement nested counter and update condition flags
 */

.macro dec_nest_counter
	bl	z_arm64_curr_cpu
	ldr	x1, [x0, #___cpu_t_nested_OFFSET]
	subs	x1, x1, #1
	str	x1, [x0, #___cpu_t_nested_OFFSET]
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
