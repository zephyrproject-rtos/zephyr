/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/rh850/sysreg.h>
#include <offsets_short.h>

/*
 * CU0 makes the FPSR read safe after UCPOP; EP keeps EIRET from clearing
 * ISPR after FEINT and FENMI, which enter with EP = 0.
 */
.macro z_rh850_frame_save fe=0, exc=0
	addi	-___esf_t_SIZEOF, sp, sp
	st.w	r1, _esf_r1[sp]
	st.w	r2, _esf_r2[sp]
	.irp	n, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, \
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	st.w	r\n, _esf_r4 + 4 * (\n - 4)[sp]
	.endr
	.if \fe
	rh850_stsr	RH850_SR_FEPC, r10
	rh850_stsr	RH850_SR_FEPSW, r11
	.else
	rh850_stsr	RH850_SR_EIPC, r10
	rh850_stsr	RH850_SR_EIPSW, r11
	.endif
	st.w	r10, _esf_eipc[sp]
	st.w	r11, _esf_eipsw[sp]
	.if \exc
	rh850_stsr	RH850_SR_PSW, r10
	mov	hilo(RH850_PSW_CU0 | RH850_PSW_EP), r11
	or	r11, r10
	ldsr	r10, RH850_SR_PSW
	.endif
	rh850_stsr	RH850_SR_FPSR, r10
	st.w	r10, _esf_fpsr[sp]
	rh850_stsr	RH850_SR_FPEPC, r10
	st.w	r10, _esf_fpepc[sp]
	rh850_stsr	RH850_SR_CTPC, r10
	st.w	r10, _esf_ctpc[sp]
	rh850_stsr	RH850_SR_CTPSW, r10
	st.w	r10, _esf_ctpsw[sp]
.endm

/* r20 and r21 are callee-saved, so they survive the C calls that follow */
.macro z_rh850_irq_enter
	mov	sp, r20
	mov	hilo(RH850_C_SYM(_kernel) + ___kernel_t_cpus_OFFSET), r21
	ld.w	___cpu_t_nested_OFFSET[r21], r10
	cmp	0, r10
	bne	.Lz_rh850_irq_nested_\@
	ld.w	___cpu_t_irq_stack_OFFSET[r21], sp
	st.w	r20, _cpu_offset_to_esf[r21]
.Lz_rh850_irq_nested_\@:
	add	1, r10
	st.w	r10, ___cpu_t_nested_OFFSET[r21]
#ifdef CONFIG_FXU
	/* interrupts keep PSW.CU1, so FXU use in interrupt context traps */
	rh850_stsr	RH850_SR_PSW, r10
	mov	hilo(~RH850_PSW_CU1), r11
	and	r11, r10
	ldsr	r10, RH850_SR_PSW
	synci
#endif
.endm
