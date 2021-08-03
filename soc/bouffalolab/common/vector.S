/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
	.cfi_startproc

	.option norvc

	/* Inform the debugger that there is nowhere to backtrace */
	.cfi_undefined ra

	/* Disable interrupts */
	li t0, MSTATUS_MIE
	csrc mstatus, t0

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 *	CLINT Direct mode
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0

	/* Jump to __initialize */
	tail __initialize

	.cfi_endproc
