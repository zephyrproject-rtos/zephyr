/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* Imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

/* Exports */
GTEXT(__start)

/*
 * Interrupts work the same way for both the RI5CY and ZERO-RISCY cores
 * in this SoC; the only difference is the location of the vectors section
 * on flash. We thus reuse this ivt definition for each core.
 *
 * On interrupt, the event unit sets pc to the address in this table
 * at byte offset 4 * (IRQ line number).
 *
 * The reset, illegal instruction, ecall, and load store unit error exceptions
 * are handled by the addresses right after the IRQ table.
 *
 * Note: Per RV32I restrictions, "j SOME_HANDLER" can jump within a +/- 1MiB
 *       range. This is not a problem on this SoC: RI5CY is allocated 1MiB flash
 *       and ZERO-RISCY is allocated 256 KiB, and these flash banks contain the
 *       text and vectors sections, so the limits are satisfied.
 */
SECTION_FUNC(vectors, ivt)
	.option norvc

	/* Interrupts */
	j _isr_wrapper	/* IRQ 0 */
	j _isr_wrapper	/* IRQ 1 */
	j _isr_wrapper	/* IRQ 2 */
	j _isr_wrapper	/* IRQ 3 */
	j _isr_wrapper	/* IRQ 4 */
	j _isr_wrapper	/* IRQ 5 */
	j _isr_wrapper	/* IRQ 6 */
	j _isr_wrapper	/* IRQ 7 */
	j _isr_wrapper	/* IRQ 8 */
	j _isr_wrapper	/* IRQ 9 */
	j _isr_wrapper	/* IRQ 10 */
	j _isr_wrapper	/* IRQ 11 */
	j _isr_wrapper	/* IRQ 12 */
	j _isr_wrapper	/* IRQ 13 */
	j _isr_wrapper	/* IRQ 14 */
	j _isr_wrapper	/* IRQ 15 */
	j _isr_wrapper	/* IRQ 16 */
	j _isr_wrapper	/* IRQ 17 */
	j _isr_wrapper	/* IRQ 18 */
	j _isr_wrapper	/* IRQ 19 */
	j _isr_wrapper	/* IRQ 20 */
	j _isr_wrapper	/* IRQ 21 */
	j _isr_wrapper	/* IRQ 22 */
	j _isr_wrapper	/* IRQ 23 */
	j _isr_wrapper	/* IRQ 24 */
	j _isr_wrapper	/* IRQ 25 */
	j _isr_wrapper	/* IRQ 26 */
	j _isr_wrapper	/* IRQ 27 */
	j _isr_wrapper	/* IRQ 28 */
	j _isr_wrapper	/* IRQ 29 */
	j _isr_wrapper	/* IRQ 30 */
	j _isr_wrapper	/* IRQ 31 */

	/* Exceptions */
	j __start		/* reset */
	j _isr_wrapper	/* illegal instruction */
	j _isr_wrapper	/* ecall */
	j _isr_wrapper	/* load store eunit error */

SECTION_FUNC(vectors, __start)
	/* Set mtvec to point at ivt. */
	la t0, ivt
	csrw 0x305, t0
	/* Call into Zephyr initialization. */
	tail __initialize
