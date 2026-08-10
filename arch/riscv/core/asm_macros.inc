/*
 * Assembly macros and helpers
 *
 * Copyright (c) 2022 BayLibre, SAS
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

/*
 * la_abs: load the absolute address of a data symbol.
 *
 * The assembler "la" pseudo always emits auipc+addi (PCREL_HI20 +
 * PCREL_LO12), which only reaches symbols within +-2 GB of the PC.
 * Under CONFIG_RISCV_CMODEL_LARGE (64-bit only), data symbols placed
 * in DDR above 4 GB may be beyond that range, causing a link-time
 * "relocation truncated to fit: R_RISCV_PCREL_HI20" error.
 *
 * GCC handles this automatically for C code by emitting a literal
 * pool entry (.dword sym) in .rodata and loading the 64-bit address
 * indirectly via ld.  Assembly files must use this macro explicitly
 * for any data symbol whose section may be placed far from .text.
 * Symbols in .text or .rodata (always near .text) can use plain la.
 *
 * On 32-bit or non-large-model builds the macro reduces to a plain la,
 * so there is no cost on those configurations.
 */
#ifdef CONFIG_RISCV_CMODEL_LARGE
	.macro la_abs, rd, sym
	.pushsection .rodata.la_pool, "a"
991:
	.dword \sym
	.popsection
	lla \rd, 991b
	ld \rd, 0(\rd)
	.endm
#else
	.macro la_abs, rd, sym
	la \rd, \sym
	.endm
#endif /* CONFIG_RISCV_CMODEL_LARGE */

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

	/*
	 * Perform rd += rs * mult using only shifts and adds.
	 * Useful when the mul instruction isn't available.
	 * mult must be a constant. rs will be clobbered.
	 */
	.macro shiftmul_add rd, rs, mult

	beqz \rs, 999f

	.set _bitpos, 0
	.set _lastbitpos, 0

	.rept 32
	.if ((\mult) & (1 << _bitpos))
	.if (_bitpos - _lastbitpos) != 0
	slli \rs, \rs, (_bitpos - _lastbitpos)
	.set _lastbitpos, _bitpos
	.endif
	add \rd, \rd, \rs
	.endif
	.set _bitpos, _bitpos + 1
	.endr
999:
	.endm

/* lowest common denominator for register availability */
#if defined(CONFIG_RISCV_ISA_RV32E)
#define RV_E(op...) op
#define RV_I(op...) /* unavailable */
#else
#define RV_E(op...) op
#define RV_I(op...) op
#endif
