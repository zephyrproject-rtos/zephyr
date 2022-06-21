/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
#if defined(CONFIG_RISCV_GP)
	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	.option norvc;

#if defined(CONFIG_RISCV_MTVEC_VECTORED_MODE)
	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to __ivt (interrupt vector table). Add 1 to base
	 * address of __ivt to indicate that vectored mode
	 * is used (LSB = 0x1). CPU will mask the LSB out of
	 * the address so that base address of __ivt is used.
	 *
	 * NOTE: __ivt is 256-byte aligned. Incorrect alignment
	 *        of __ivt breaks this code.
	 */
	la t0, __ivt /* Load address of interrupt vector table */
	addi t0, t0, 1 /* Enable vectored mode by setting LSB */

/* MTVEC_DIRECT_MODE */
#else
	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
#endif

	csrw mtvec, t0

	/* Jump to __reset */
	tail __reset

#if defined(CONFIG_RISCV_MTVEC_VECTORED_MODE)
SECTION_FUNC(reset,  __ivt)
	.option push
	.option norvc
	.balign 0x100 /* must be 256 byte aligned per specification */
	.rept (CONFIG_NUM_IRQS)
	j _isr_wrapper
	.endr
#endif
