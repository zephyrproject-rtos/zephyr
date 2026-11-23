/*
 * Copyright (c) 2021 BayLibre SAS
 * Written by: Nicolas Pitre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/offsets.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm64_fpu_save)
SECTION_FUNC(TEXT, z_arm64_fpu_save)

	mrs	x1, fpsr
	mrs	x2, fpcr
	str	w1, [x0, #__z_arm64_fp_context_fpsr_OFFSET]
	str	w2, [x0, #__z_arm64_fp_context_fpcr_OFFSET]

	/* Save NEON registers */
	add	x0, x0, #__z_arm64_fp_context_neon_OFFSET
	stp	q0,  q1,  [x0, #(16 *  0)]
	stp	q2,  q3,  [x0, #(16 *  2)]
	stp	q4,  q5,  [x0, #(16 *  4)]
	stp	q6,  q7,  [x0, #(16 *  6)]
	stp	q8,  q9,  [x0, #(16 *  8)]
	stp	q10, q11, [x0, #(16 * 10)]
	stp	q12, q13, [x0, #(16 * 12)]
	stp	q14, q15, [x0, #(16 * 14)]
	stp	q16, q17, [x0, #(16 * 16)]
	stp	q18, q19, [x0, #(16 * 18)]
	stp	q20, q21, [x0, #(16 * 20)]
	stp	q22, q23, [x0, #(16 * 22)]
	stp	q24, q25, [x0, #(16 * 24)]
	stp	q26, q27, [x0, #(16 * 26)]
	stp	q28, q29, [x0, #(16 * 28)]
	stp	q30, q31, [x0, #(16 * 30)]

	ret

GTEXT(z_arm64_fpu_restore)
SECTION_FUNC(TEXT, z_arm64_fpu_restore)

	ldr	w1, [x0, #__z_arm64_fp_context_fpsr_OFFSET]
	ldr	w2, [x0, #__z_arm64_fp_context_fpcr_OFFSET]
	msr	fpsr, x1
	msr	fpcr, x2

	/* Restore NEON registers */
	add	x0, x0, #__z_arm64_fp_context_neon_OFFSET
	ldp	q0,  q1,  [x0, #(16 *  0)]
	ldp	q2,  q3,  [x0, #(16 *  2)]
	ldp	q4,  q5,  [x0, #(16 *  4)]
	ldp	q6,  q7,  [x0, #(16 *  6)]
	ldp	q8,  q9,  [x0, #(16 *  8)]
	ldp	q10, q11, [x0, #(16 * 10)]
	ldp	q12, q13, [x0, #(16 * 12)]
	ldp	q14, q15, [x0, #(16 * 14)]
	ldp	q16, q17, [x0, #(16 * 16)]
	ldp	q18, q19, [x0, #(16 * 18)]
	ldp	q20, q21, [x0, #(16 * 20)]
	ldp	q22, q23, [x0, #(16 * 22)]
	ldp	q24, q25, [x0, #(16 * 24)]
	ldp	q26, q27, [x0, #(16 * 26)]
	ldp	q28, q29, [x0, #(16 * 28)]
	ldp	q30, q31, [x0, #(16 * 30)]

	ret

#ifdef CONFIG_ARM64_SVE

GTEXT(z_arm64_sve_save)
SECTION_FUNC(TEXT, z_arm64_sve_save)

	/* Save control registers */
	mrs	x2, fpsr
	mrs	x3, fpcr
	str	w2, [x0, #__z_arm64_fp_context_fpsr_OFFSET]
	str	w3, [x0, #__z_arm64_fp_context_fpcr_OFFSET]

	/* Get Z registers base address */
	add	x2, x0, #__z_arm64_fp_context_sve_z_regs_OFFSET

	/* Save Z registers */
	str	z0, [x2, #0, MUL VL]
	str	z1, [x2, #1, MUL VL]
	str	z2, [x2, #2, MUL VL]
	str	z3, [x2, #3, MUL VL]
	str	z4, [x2, #4, MUL VL]
	str	z5, [x2, #5, MUL VL]
	str	z6, [x2, #6, MUL VL]
	str	z7, [x2, #7, MUL VL]
	str	z8, [x2, #8, MUL VL]
	str	z9, [x2, #9, MUL VL]
	str	z10, [x2, #10, MUL VL]
	str	z11, [x2, #11, MUL VL]
	str	z12, [x2, #12, MUL VL]
	str	z13, [x2, #13, MUL VL]
	str	z14, [x2, #14, MUL VL]
	str	z15, [x2, #15, MUL VL]
	str	z16, [x2, #16, MUL VL]
	str	z17, [x2, #17, MUL VL]
	str	z18, [x2, #18, MUL VL]
	str	z19, [x2, #19, MUL VL]
	str	z20, [x2, #20, MUL VL]
	str	z21, [x2, #21, MUL VL]
	str	z22, [x2, #22, MUL VL]
	str	z23, [x2, #23, MUL VL]
	str	z24, [x2, #24, MUL VL]
	str	z25, [x2, #25, MUL VL]
	str	z26, [x2, #26, MUL VL]
	str	z27, [x2, #27, MUL VL]
	str	z28, [x2, #28, MUL VL]
	str	z29, [x2, #29, MUL VL]
	str	z30, [x2, #30, MUL VL]
	str	z31, [x2, #31, MUL VL]

	/* Get P registers base address */
	mov	x3, #__z_arm64_fp_context_sve_p_regs_OFFSET
	add	x3, x0, x3

	/* Save P registers */
	str	p0, [x3, #0, MUL VL]
	str	p1, [x3, #1, MUL VL]
	str	p2, [x3, #2, MUL VL]
	str	p3, [x3, #3, MUL VL]
	str	p4, [x3, #4, MUL VL]
	str	p5, [x3, #5, MUL VL]
	str	p6, [x3, #6, MUL VL]
	str	p7, [x3, #7, MUL VL]
	str	p8, [x3, #8, MUL VL]
	str	p9, [x3, #9, MUL VL]
	str	p10, [x3, #10, MUL VL]
	str	p11, [x3, #11, MUL VL]
	str	p12, [x3, #12, MUL VL]
	str	p13, [x3, #13, MUL VL]
	str	p14, [x3, #14, MUL VL]
	str	p15, [x3, #15, MUL VL]

	/* Get FFR base address */
	mov	x4, #__z_arm64_fp_context_sve_ffr_OFFSET
	add	x4, x0, x4

	/* Save FFR */
	rdffr	p0.b
	str	p0, [x4]

	ret

GTEXT(z_arm64_sve_restore)
SECTION_FUNC(TEXT, z_arm64_sve_restore)

	/* Get Z registers base address */
	add	x2, x0, #__z_arm64_fp_context_sve_z_regs_OFFSET

	/* Restore Z registers */
	ldr	z0, [x2, #0, MUL VL]
	ldr	z1, [x2, #1, MUL VL]
	ldr	z2, [x2, #2, MUL VL]
	ldr	z3, [x2, #3, MUL VL]
	ldr	z4, [x2, #4, MUL VL]
	ldr	z5, [x2, #5, MUL VL]
	ldr	z6, [x2, #6, MUL VL]
	ldr	z7, [x2, #7, MUL VL]
	ldr	z8, [x2, #8, MUL VL]
	ldr	z9, [x2, #9, MUL VL]
	ldr	z10, [x2, #10, MUL VL]
	ldr	z11, [x2, #11, MUL VL]
	ldr	z12, [x2, #12, MUL VL]
	ldr	z13, [x2, #13, MUL VL]
	ldr	z14, [x2, #14, MUL VL]
	ldr	z15, [x2, #15, MUL VL]
	ldr	z16, [x2, #16, MUL VL]
	ldr	z17, [x2, #17, MUL VL]
	ldr	z18, [x2, #18, MUL VL]
	ldr	z19, [x2, #19, MUL VL]
	ldr	z20, [x2, #20, MUL VL]
	ldr	z21, [x2, #21, MUL VL]
	ldr	z22, [x2, #22, MUL VL]
	ldr	z23, [x2, #23, MUL VL]
	ldr	z24, [x2, #24, MUL VL]
	ldr	z25, [x2, #25, MUL VL]
	ldr	z26, [x2, #26, MUL VL]
	ldr	z27, [x2, #27, MUL VL]
	ldr	z28, [x2, #28, MUL VL]
	ldr	z29, [x2, #29, MUL VL]
	ldr	z30, [x2, #30, MUL VL]
	ldr	z31, [x2, #31, MUL VL]

	/* Get FFR base address */
	mov	x4, #__z_arm64_fp_context_sve_ffr_OFFSET
	add	x4, x0, x4

	/* Restore FFR */
	ldr	p0, [x4]
	wrffr	p0.b

	/* Get P registers base address */
	mov	x3, #__z_arm64_fp_context_sve_p_regs_OFFSET
	add	x3, x0, x3

	/* Restore P registers intervals */
	ldr	p0, [x3, #0, MUL VL]
	ldr	p1, [x3, #1, MUL VL]
	ldr	p2, [x3, #2, MUL VL]
	ldr	p3, [x3, #3, MUL VL]
	ldr	p4, [x3, #4, MUL VL]
	ldr	p5, [x3, #5, MUL VL]
	ldr	p6, [x3, #6, MUL VL]
	ldr	p7, [x3, #7, MUL VL]
	ldr	p8, [x3, #8, MUL VL]
	ldr	p9, [x3, #9, MUL VL]
	ldr	p10, [x3, #10, MUL VL]
	ldr	p11, [x3, #11, MUL VL]
	ldr	p12, [x3, #12, MUL VL]
	ldr	p13, [x3, #13, MUL VL]
	ldr	p14, [x3, #14, MUL VL]
	ldr	p15, [x3, #15, MUL VL]

	/* Restore control registers */
	ldr	w2, [x0, #__z_arm64_fp_context_fpsr_OFFSET]
	ldr	w3, [x0, #__z_arm64_fp_context_fpcr_OFFSET]
	msr	fpsr, x2
	msr	fpcr, x3

	ret

#endif /* CONFIG_ARM64_SVE */
