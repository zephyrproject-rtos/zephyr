/*
 * Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <arch/riscv/csr.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)

SECTION_FUNC(vectors, __start)
	.cfi_startproc

	.option norvc

	/* Inform the debugger that there is nowhere to backtrace */
	.cfi_undefined ra

	/* Disable interrupts */
	li t0, MSTATUS_MIE
	csrc mstatus, t0

#ifdef CONFIG_RISCV_GP
	/* The absolute first thing that must happen is configuring the global
	 * pointer register, which must be done with relaxation disabled
	 * because it's not valid to obtain the address of any symbol without
	 * GP configured.  The C environment might go ahead and do this again,
	 * but that's safe as it's a fixed register. */
	.option push
	.option norelax
		la gp, __global_pointer$
	.option pop
#endif

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 *	CLINT Direct mode
	 */
	la t0, __irq_wrapper
	csrw mtvec, t0

	/* Jump to __initialize */
	tail __initialize

	.cfi_endproc
