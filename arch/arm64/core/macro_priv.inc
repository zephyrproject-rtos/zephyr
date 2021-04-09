/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifdef _ASMLANGUAGE

GDATA(_kernel)

/*
 * Get CPU id
 */

.macro get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm

/*
 * Get CPU pointer
 */

.macro get_cpu xreg0, xreg1
	get_cpu_id \xreg1
	mov	\xreg0, #___cpu_t_SIZEOF
	mul	\xreg0, \xreg0, \xreg1
	ldr	\xreg1, =(_kernel + ___kernel_t_cpus_OFFSET)
	add	\xreg0, \xreg0, \xreg1
.endm

/*
 * Increment nested counter
 */

.macro inc_nest_counter xreg0, xreg1
#ifdef CONFIG_SMP
	get_cpu	\xreg0, \xreg1
	ldr	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
#else
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
#endif
.endm

/*
 * Decrement nested counter and update condition flags
 */

.macro dec_nest_counter xreg0, xreg1
#ifdef CONFIG_SMP
	get_cpu	\xreg0, \xreg1
	ldr	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
	subs	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
#else
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	subs	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
#endif
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
