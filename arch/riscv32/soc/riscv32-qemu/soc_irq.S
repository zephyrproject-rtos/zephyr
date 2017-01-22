/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <offsets.h>
#include <toolchain.h>
#include <sections.h>
#include <soc.h>

/* exports */
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 * Exception number is given as parameter via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	/* Clear exception number from CSR mip register */
	li t1, 1
	sll t0, t1, a0
	csrrc t1, mip, t0

	/* Return */
	jalr x0, ra


/*
 * SOC-specific function to determine if the exception is the result of a
 * an interrupt or an exception
 * return 1 (interrupt) or 0 (exception)
 */
SECTION_FUNC(exception.other, __soc_is_irq)
	/* Get exception number from the mcause CSR register. */
	csrr t0, mcause
	li t1, SOC_MCAUSE_IRQ_MASK
	and t0, t0, t1

	/* if IRQ number != RISCV_QEMU_TIMER_IRQ, not interrupt */
	li t1, RISCV_QEMU_TIMER_IRQ
	addi a0, x0, 0
	bne t0, t1, not_interrupt
	addi a0, a0, 1

not_interrupt:
	/* return */
	jalr x0, ra
