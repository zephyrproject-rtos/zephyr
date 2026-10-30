/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <offsets_short.h>
#include <zephyr/arch/rh850/error.h>
#include <kernel_arch_data.h>
#include "asm_macros.inc"

GTEXT(z_rh850_trap0)
GTEXT(z_rh850_frame_restore)
GTEXT(z_rh850_irq_exit)
GTEXT(z_rh850_exc_ei_saved)
GTEXT(RH850_C_SYM(z_rh850_except))
#ifdef CONFIG_FXU
GTEXT(RH850_C_SYM(z_rh850_fxu_arch_switch))
#endif

/* TRAP0 enters with EIPC = next instruction, PSW.ID = 1 and EP = 1 */
SECTION_FUNC(TEXT, z_rh850_trap0)
	z_rh850_frame_save

	rh850_stsr	RH850_SR_EIIC, r10
	mov	RH850_EIIC_TRAP0 + RH850_TRAP_SWITCH, r11
	cmp	r11, r10
	bne	1f

#ifdef CONFIG_FXU
	mov	r6, r20
	mov	r7, r21
	mov	r7, r6
	jarl	RH850_C_SYM(z_rh850_fxu_arch_switch), lp
	mov	r20, r6
	mov	r21, r7
#endif
	st.w	sp, 0[r7]
	mov	r6, sp
	jr	z_rh850_frame_restore

1:	mov	RH850_EIIC_TRAP0 + RH850_TRAP_EXCEPT, r11
	cmp	r11, r10
	bne	2f
	z_rh850_irq_enter
	mov	r20, r7
	jarl	RH850_C_SYM(z_rh850_except), lp
	jr	z_rh850_irq_exit

2:
#ifdef CONFIG_IRQ_OFFLOAD
	mov	RH850_EIIC_TRAP0 + RH850_TRAP_IRQ_OFFLOAD, r11
	cmp	r11, r10
	be	3f
#endif
	jr	z_rh850_exc_ei_saved

#ifdef CONFIG_IRQ_OFFLOAD
3:	z_rh850_irq_enter
	mov	r6, r11
	mov	r7, r6
	jarl	[r11], lp
	jr	z_rh850_irq_exit
#endif

/* A pending FPI keeps its FPEPC and FPSR cause bits (HW 3.2.6.1 (11) p361) */
z_rh850_frame_restore:
	ld.w	_esf_fpsr[sp], r10
#ifdef CONFIG_CPU_G4KH
	rh850_stsr	RH850_SR_FPEC, r11
	andi	RH850_FPEC_FPIVD, r11, r11
	bz	4f
	rh850_stsr	RH850_SR_FPSR, r11
	andi	RH850_FPSR_XC_MASK, r11, r11
	andi	RH850_FPSR_XC_MASK, r10, r12
	xor	r12, r10
	or	r11, r10
	ldsr	r10, RH850_SR_FPSR
	br	5f
4:
#endif
	ldsr	r10, RH850_SR_FPSR
	ld.w	_esf_fpepc[sp], r10
	ldsr	r10, RH850_SR_FPEPC
5:
	ld.w	_esf_ctpc[sp], r10
	ldsr	r10, RH850_SR_CTPC
	ld.w	_esf_ctpsw[sp], r10
	ldsr	r10, RH850_SR_CTPSW
	ld.w	_esf_eipc[sp], r10
	ldsr	r10, RH850_SR_EIPC
	ld.w	_esf_eipsw[sp], r10
	ldsr	r10, RH850_SR_EIPSW
	ld.w	_esf_r1[sp], r1
	ld.w	_esf_r2[sp], r2
	.irp	n, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, \
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	ld.w	_esf_r4 + 4 * (\n - 4)[sp], r\n
	.endr
	addi	___esf_t_SIZEOF, sp, sp
	eiret
