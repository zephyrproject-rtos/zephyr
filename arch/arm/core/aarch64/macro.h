/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_H_
#define _MACRO_H_

#ifdef _ASMLANGUAGE

.macro	switch_el, xreg, el3_label, el2_label, el1_label
	mrs	\xreg, CurrentEL
	cmp	\xreg, 0xc
	beq	\el3_label
	cmp	\xreg, 0x8
	beq	\el2_label
	cmp	\xreg, 0x4
	beq	\el1_label
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_H_ */
