/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#ifdef _ASMLANGUAGE

/*
 * Increment nested counter
 */

.macro inc_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

/*
 * Decrement nested counter and update condition flags
 */

.macro dec_nest_counter xreg0, xreg1
	ldr	\xreg0, =_kernel
	ldr	\xreg1, [\xreg0, #_kernel_offset_to_nested]
	subs	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #_kernel_offset_to_nested]
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
