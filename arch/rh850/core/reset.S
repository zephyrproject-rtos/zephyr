/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <kernel_arch_data.h>

GTEXT(__start)
GDATA(RH850_C_SYM(z_interrupt_stacks))
GDATA(__z_interrupt_stack_SIZEOF)
GDATA(z_rh850_vectors)
#ifdef CONFIG_FXU
GDATA(z_rh850_fxu_zero)
#endif

/* Registers that are undefined after reset are written before use */
SECTION_FUNC(TEXT, __start)
	.irp	n, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, \
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	mov	r0, r\n
	.endr

	ldsr	r0, RH850_SR_EIPC
	ldsr	r0, RH850_SR_FEPC
	ldsr	r0, RH850_SR_CTPC
	ldsr	r0, RH850_SR_EIWR
	ldsr	r0, RH850_SR_FEWR
	ldsr	r0, RH850_SR_INTBP
	ldsr	r0, RH850_SR_MEA
	ldsr	r0, RH850_SR_MEI
	ldsr	r0, RH850_SR_RBIP

	mov	hilo(z_rh850_vectors), r10
	ldsr	r10, RH850_SR_EBASE
	mov	hilo(RH850_PSW_ID | RH850_PSW_THREAD), r10
	ldsr	r10, RH850_SR_PSW

#if CONFIG_NUM_IRQ_PRIO_LEVELS > 16
	mov	hilo(RH850_INTCFG_EPL), r10
	ldsr	r10, RH850_SR_INTCFG
	mov	RH850_PLMR_ALL, r10
	ldsr	r10, RH850_SR_PLMR
#endif

	/* FPST and FPCC are views of FPSR and need PSW.CU0 */
	ldsr	r0, RH850_SR_FPST
	ldsr	r0, RH850_SR_FPCC
	mov	hilo(RH850_FPSR_FS), r10
	ldsr	r10, RH850_SR_FPSR
	ldsr	r0, RH850_SR_FPEPC

	ldsr	r0, RH850_SR_MCA
	ldsr	r0, RH850_SR_MCS
	ldsr	r0, RH850_SR_MCR
	ldsr	r0, RH850_SR_MCI
	mov	r0, r10
	mov	CONFIG_RH850_MPU_ENTRIES, r11
1:	ldsr	r10, RH850_SR_MPIDX
	ldsr	r0, RH850_SR_MPLA
	ldsr	r0, RH850_SR_MPUA
	ldsr	r0, RH850_SR_MPAT
	add	1, r10
	cmp	r11, r10
	bl	1b
	ldsr	r0, RH850_SR_MPIDX
	.irp	n, 24, 25, 26, 27, 28, 29, 30, 31
	ldsr	r0, \n, 5
	.endr

	ldsr	r0, RH850_SR_ICTAGL
	ldsr	r0, RH850_SR_ICTAGH
	ldsr	r0, RH850_SR_ICDATL
	ldsr	r0, RH850_SR_ICDATH
	ldsr	r0, RH850_SR_ICERR

#ifdef CONFIG_FXU
	mov	hilo(RH850_PSW_ID | RH850_PSW_THREAD | RH850_PSW_CU1), r10
	ldsr	r10, RH850_SR_PSW
	synci
	mov	hilo(z_rh850_fxu_zero), r10
	.irp	n, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, \
		18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	ldv.qw	0[r10], wr\n
	.endr
	mov	hilo(RH850_FXSR_INIT), r10
	ldsr	r10, RH850_SR_FXSR
	ldsr	r0, RH850_SR_FXST
	ldsr	r0, RH850_SR_FXXC
	ldsr	r0, RH850_SR_FXXP
	mov	hilo(RH850_PSW_ID | RH850_PSW_THREAD), r10
	ldsr	r10, RH850_SR_PSW
	synci
#endif

	mov	hilo(RH850_C_SYM(z_interrupt_stacks)), sp
	mov	hilo(__z_interrupt_stack_SIZEOF), r1
	add	r1, sp
	jarl	RH850_C_SYM(z_prep_c), lp
	di
2:	halt
	br	2b
