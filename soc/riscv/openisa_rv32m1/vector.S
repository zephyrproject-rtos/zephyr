/*
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* Imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)

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
	j __irq_wrapper	/* IRQ 0 */
	j __irq_wrapper	/* IRQ 1 */
	j __irq_wrapper	/* IRQ 2 */
	j __irq_wrapper	/* IRQ 3 */
	j __irq_wrapper	/* IRQ 4 */
	j __irq_wrapper	/* IRQ 5 */
	j __irq_wrapper	/* IRQ 6 */
	j __irq_wrapper	/* IRQ 7 */
	j __irq_wrapper	/* IRQ 8 */
	j __irq_wrapper	/* IRQ 9 */
	j __irq_wrapper	/* IRQ 10 */
	j __irq_wrapper	/* IRQ 11 */
	j __irq_wrapper	/* IRQ 12 */
	j __irq_wrapper	/* IRQ 13 */
	j __irq_wrapper	/* IRQ 14 */
	j __irq_wrapper	/* IRQ 15 */
	j __irq_wrapper	/* IRQ 16 */
	j __irq_wrapper	/* IRQ 17 */
	j __irq_wrapper	/* IRQ 18 */
	j __irq_wrapper	/* IRQ 19 */
	j __irq_wrapper	/* IRQ 20 */
	j __irq_wrapper	/* IRQ 21 */
	j __irq_wrapper	/* IRQ 22 */
	j __irq_wrapper	/* IRQ 23 */
	j __irq_wrapper	/* IRQ 24 */
	j __irq_wrapper	/* IRQ 25 */
	j __irq_wrapper	/* IRQ 26 */
	j __irq_wrapper	/* IRQ 27 */
	j __irq_wrapper	/* IRQ 28 */
	j __irq_wrapper	/* IRQ 29 */
	j __irq_wrapper	/* IRQ 30 */
	j __irq_wrapper	/* IRQ 31 */

	/* Exceptions */
	j __start		/* reset */
	j __irq_wrapper	/* illegal instruction */
	j __irq_wrapper	/* ecall */
	j __irq_wrapper	/* load store eunit error */

SECTION_FUNC(vectors, __start)
	/* Set mtvec to point at ivt. */
	la t0, ivt
	csrw 0x305, t0
	/* Call into Zephyr initialization. */
	tail __initialize
