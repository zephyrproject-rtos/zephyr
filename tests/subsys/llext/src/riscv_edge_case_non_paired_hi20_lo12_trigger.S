/*
 * Copyright (c) 2025 CISPA Helmholtz Center for Information Security
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* 32-bit global defined in C */
GDATA(_data_segment_symbol)

GTEXT(_riscv_edge_case_non_paired_hi20_lo12)

/*
 * Tests an edge case in the RISC-V PSABI: In the medany and the medlow code models,
 * the compiler emits auipc/lui (U-type) and ld/sw (I-type/S-type) instruction pairs
 * for accessing a non-local symbol.
 * The U-type instruction sets the upper 20 bits, the I/S-type the lower 12.
 * Thus, any address in a 32-bit range from 0 (medlow) / the PC (medany) can be reached.
 * Often, the U-type and I-type/S-type instruction pairs are adjacent in code.
 * However, this need not be the case - compilers can re-use the upper 20 bits set by
 * the U-type instruction with multiple I/S-type instructions, which is a useful optimization
 * for multiple loads/stores of or within the same symbol.
 * The U-type instruction can also appear after the I-type in code, e.g., due to control flow.
 * When the U-type and I/S-type instructions are not in sequence, this triggers an edge case
 * in the llext loader.
 * This test triggers this edge case by loading a global, modifying it and storing it back.
 */
SECTION_FUNC(TEXT, _riscv_edge_case_non_paired_hi20_lo12)
	/* jump beyond the I-type/load instruction initially to break sequence assumption */
	j _do_utype

_do_load:
	/* re-use the upper-bit value set by the U-type below for a load */
        lw    a0, %pcrel_lo(.LUtype)(a1)

	addi t1, a0, 42

	j _do_store

_do_utype:
	/* this u-type sets the higher 20 bits of the global */
	.LUtype:  auipc a1, %pcrel_hi(_data_segment_symbol)

	/* backwards jump to test loading */
	j _do_load

_do_store:

	/* write the modified value back for the C code to check */
	sw t1, %pcrel_lo(.LUtype)(a1)

	/* return a0, i.e., the value we read, for the C code to check */
	ret
