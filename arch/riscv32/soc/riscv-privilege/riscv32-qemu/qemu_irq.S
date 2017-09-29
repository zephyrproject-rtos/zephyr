/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <offsets.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <soc.h>

/* exports */
GTEXT(__soc_is_irq)

/*
 * riscv32-qemu does not truely follow riscv privilege specification
 * when determining if exception is the result of an interrupt or an
 * exception. Hence, reimplement __soc_is_irq here.
 *
 * return 1 (interrupt) or 0 (exception)
 */
SECTION_FUNC(exception.other, __soc_is_irq)
	/* Get exception number from the mcause CSR register. */
	csrr t0, mcause
	li t1, SOC_MCAUSE_EXP_MASK
	and t0, t0, t1

	/*
	 * If corresponding exception bit is set in the
	 * Machine Interrupt Pending register (mip),
	 * the exception is an interrupt, otherwise it
	 * is an unexpected exception.
	 */
	csrr t2, mip
	li t3, 1
	sll t4, t3, t0

	addi a0, x0, 0
	bne t2, t4, not_interrupt
	addi a0, a0, 1

not_interrupt:
	/* return */
	jalr x0, ra
