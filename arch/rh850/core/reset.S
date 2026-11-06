/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <kernel_arch_data.h>

/* MPCFG.NMPUE + 1 on U2C */
#define RH850_MPU_ENTRIES 24

GTEXT(__start)
GDATA(RH850_C_SYM(z_interrupt_stacks))
GDATA(__z_interrupt_stack_SIZEOF)
GDATA(z_rh850_vectors)

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
	mov	hilo(RH850_PSW_ID | RH850_PSW_EBV | RH850_PSW_CU0), r10
	ldsr	r10, RH850_SR_PSW

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
	mov	RH850_MPU_ENTRIES, r11
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

	mov	hilo(RH850_C_SYM(z_interrupt_stacks)), sp
	mov	hilo(__z_interrupt_stack_SIZEOF), r1
	add	r1, sp
	jarl	RH850_C_SYM(z_prep_c), lp
	di
2:	halt
	br	2b
