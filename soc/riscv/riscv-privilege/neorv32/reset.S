/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__reset)

/* imports */
GTEXT(__initialize)

SECTION_FUNC(reset, __reset)
	/* Zerorize zero register */
	lui x0, 0

	/* Disable insterrupts */
	csrw mstatus, x0
	csrw mie, x0

#ifdef CONFIG_USERSPACE
	/* Disable counter access outside M-mode */
	csrw mcounteren, x0
#endif /* CONFIG_USERSPACE */

	/* Allow mcycle and minstret counters to increment */
	li x11, ~5
	csrw mcountinhibit, x11

	/* Zerorize counters */
	csrw mcycle, x0
	csrw mcycleh, x0
	csrw minstret, x0
	csrw minstreth, x0

	/*
	 * Simplify dummy machine trap code by not having to decode
	 * instruction width.
	 */
	.option push
	.option norvc

	/*
	 * Temporarily setup a dummy machine trap vector to catch (and ignore)
	 * Store Access faults due to unimplemented peripherals.
	 */
	csrr x6, mtvec
	la x7, __dummy_trap_handler
	csrw mtvec, x7

	/* Attempt to zerorize all IO peripheral registers */
	la x8, __io_start
	la x9, __io_end
1:	sw x0, 0(x8)
	addi x8, x8, 4
	bne x8, x9, 1b

	/* Restore previous machine trap vector */
	csrw mtvec, x6

	.option pop

	/* Jump to __initialize */
	call __initialize

.balign 4
SECTION_FUNC(reset, __dummy_trap_handler)
	csrr x5, mepc
	addi x5, x5, 4
	csrw mepc, x5
	mret
