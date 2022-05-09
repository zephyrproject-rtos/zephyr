/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include "asm_macros.inc"

#define CSR_PMPCFG_BASE  0x3a0
#define CSR_PMPADDR_BASE 0x3b0

/*
 * Prototype:
 *
 * void z_riscv_write_pmp_entries(unsigned int start,		// a0
 *                                unsigned int end,		// a1
 *                                bool clear_trailing_entries,	// a2
 *                                ulong_t *pmp_addr,		// a3
 *                                ulong_t *pmp_cfg)		// a4
 *
 * Called from pmp.c to write a range of PMP entries.
 *
 * PMP registers are accessed with the csr instruction which only takes an
 * immediate value as the actual register. In order to avoid traversing
 * the whole register list, we use the start index to jump directly to the
 * location corresponding to the start of the wanted range. For this to work
 * we disallow compressed instructions so the update block sizes are easily
 * known (luckily they're all power-of-2's simplifying the code further).
 *
 * start < end && end <= CONFIG_PMP_SLOTS must be true.
 */

GTEXT(z_riscv_write_pmp_entries)
SECTION_FUNC(TEXT, z_riscv_write_pmp_entries)

	la t0, pmpaddr_store
	slli t1, a0, 4  /* 16-byte instruction blocks */
	add t0, t0, t1
	jr t0

pmpaddr_store:

	.option push
	.option norvc
	.set _index, 0
	.rept CONFIG_PMP_SLOTS
	lr t0, (RV_REGSIZE * _index)(a3)
	li t1, _index + 1
	csrw (CSR_PMPADDR_BASE + _index), t0
	beq t1, a1, pmpaddr_done
	.set _index, _index + 1
	.endr
	.option pop

pmpaddr_done:

	/*
	 * Move to the pmpcfg space:
	 * a0 = a0 / RV_REGSIZE
	 * a1 = (a1 + RV_REGSIZE - 1) / RV_REGSIZE
	 */
	la t0, pmpcfg_store
	srli a0, a0, RV_REGSHIFT
	slli t1, a0, 4  /* 16-byte instruction blocks */
	add t0, t0, t1
	addi a1, a1, RV_REGSIZE - 1
	srli a1, a1, RV_REGSHIFT
	jr t0

pmpcfg_store:

	.option push
	.option norvc
	.set _index, 0
	.rept (CONFIG_PMP_SLOTS / RV_REGSIZE)
	lr t0, (RV_REGSIZE * _index)(a4)
	addi a0, a0, 1
	csrw (CSR_PMPCFG_BASE + RV_REGSIZE/4 * _index), t0
	beq a0, a1, pmpcfg_done
	.set _index, _index + 1
	.endr
	.option pop

pmpcfg_done:

	beqz a2, done

	la t0, pmpcfg_zerotail
	slli a0, a0, 2  /* 4-byte instruction blocks */
	add t0, t0, a0
	jr t0

pmpcfg_zerotail:

	.option push
	.option norvc
	.set _index, 0
	.rept (CONFIG_PMP_SLOTS / RV_REGSIZE)
	csrw (CSR_PMPCFG_BASE + RV_REGSIZE/4 * _index), zero
	.set _index, _index + 1
	.endr
	.option pop

done:	ret
