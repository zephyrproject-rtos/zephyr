/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <kernel_arch_data.h>

GTEXT(RH850_C_SYM(z_rh850_fxu_save))
GTEXT(RH850_C_SYM(z_rh850_fxu_load))
GDATA(z_rh850_fxu_zero)

/* PSW.CU1 must be set, followed by SYNCI, before the first FXU access */
.macro z_rh850_fxu_access
	rh850_stsr	RH850_SR_PSW, r10
	mov	hilo(RH850_PSW_CU1), r11
	or	r11, r10
	ldsr	r10, RH850_SR_PSW
	synci
.endm

/* r6: 16-byte aligned wr0-wr31 area, r7: FXSR and FXXC */
SECTION_FUNC(TEXT, RH850_C_SYM(z_rh850_fxu_save))
	z_rh850_fxu_access
	.irp	n, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, \
		18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	stv.qw	wr\n, 16 * \n[r6]
	.endr
	rh850_stsr	RH850_SR_FXSR, r10
	st.w	r10, 0[r7]
	rh850_stsr	RH850_SR_FXXC, r10
	st.w	r10, 4[r7]
	jmp	[lp]

SECTION_FUNC(TEXT, RH850_C_SYM(z_rh850_fxu_load))
	z_rh850_fxu_access
	.irp	n, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, \
		18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	ldv.qw	16 * \n[r6], wr\n
	.endr
	ld.w	0[r7], r10
	ldsr	r10, RH850_SR_FXSR
	ld.w	4[r7], r10
	ldsr	r10, RH850_SR_FXXC
	jmp	[lp]

	.section .rodata.z_rh850_fxu_zero, "a"
	.balign	16
z_rh850_fxu_zero:
	.fill	16, 1, 0
