/*
 * Copyright 2020 Broadcom.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_MACRO_INC_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_MACRO_INC_

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

/*
 * macro to support mov of immediate constant to 64 bit register
 * It will generate instruction sequence of 'mov'/ 'movz' and one
 * to three 'movk' depending on the immediate value.
 */
.macro mov_imm, xreg, imm
	.if ((\imm) == 0)
		mov	\xreg, \imm
	.else
		.if (((\imm) >> 31) == 0 || ((\imm) >> 31) == 0x1ffffffff)
			movz    \xreg, (\imm >> 16) & 0xffff, lsl 16
		.else
			.if (((\imm) >> 47) == 0 || ((\imm) >> 47) == 0x1ffff)
				movz    \xreg, (\imm >> 32) & 0xffff, lsl 32
			.else
				movz    \xreg, (\imm >> 48) & 0xffff, lsl 48
				movk    \xreg, (\imm >> 32) & 0xffff, lsl 32
			.endif
			movk    \xreg, (\imm >> 16) & 0xffff, lsl 16
		.endif
		movk    \xreg, (\imm) & 0xffff, lsl 0
	.endif
.endm
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_MACRO_INC_ */
