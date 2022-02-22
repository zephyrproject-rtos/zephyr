/*
 * Assembly macros and helpers
 *
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_64BIT
	/* register-wide load/store based on ld/sd (XLEN = 64) */

	.macro lr, rd, mem
	ld \rd, \mem
	.endm

	.macro sr, rs, mem
	sd \rs, \mem
	.endm

#else
	/* register-wide load/store based on lw/sw (XLEN = 32) */

	.macro lr, rd, mem
	lw \rd, \mem
	.endm

	.macro sr, rs, mem
	sw \rs, \mem
	.endm

#endif

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION

	.macro flr, rd, mem
	fld \rd, \mem
	.endm

	.macro fsr, rs, mem
	fsd \rs, \mem
	.endm

#else

	.macro flr, rd, mem
	flw \rd, \mem
	.endm

	.macro fsr, rs, mem
	fsw \rs, \mem
	.endm

#endif
