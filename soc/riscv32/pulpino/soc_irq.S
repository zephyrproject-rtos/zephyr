/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <offsets.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <soc.h>

/* exports */
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(__soc_irq_unlock)

/* Use ABI name of registers for the sake of simplicity */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
/*
 * The Pulpino core has ISA extensions for faster loop performance
 * that use extra registers.
 *
 * If the toolchain generates instructions that use them, they must be saved
 * prior to handling an interrupt/exception. This case is handled using
 * Zephyr's generic RISC-V mechanism for soc-specific context.
 *
 * For details, see the Kconfig help for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */
SECTION_FUNC(exception.other, __soc_save_context)
	/* Save hardware loop registers to the soc_esf_t passed in a0. */
	csrr t0, PULP_LPSTART0
	csrr t1, PULP_LPEND0
	csrr t2, PULP_LPCOUNT0
	sw t0, __soc_esf_t_lpstart0_OFFSET(a0)
	sw t1, __soc_esf_t_lpend0_OFFSET(a0)
	sw t2, __soc_esf_t_lpcount0_OFFSET(a0)
	csrr t0, PULP_LPSTART1
	csrr t1, PULP_LPEND1
	csrr t2, PULP_LPCOUNT1
	sw t0, __soc_esf_t_lpstart1_OFFSET(a0)
	sw t1, __soc_esf_t_lpend1_OFFSET(a0)
	sw t2, __soc_esf_t_lpcount1_OFFSET(a0)

	/* Return */
	jalr x0, ra


SECTION_FUNC(exception.other, __soc_restore_context)
	/* Restore hardware loop registers from soc_esf_t in a0. */
	lw t0, __soc_esf_t_lpstart0_OFFSET(a0)
	lw t1, __soc_esf_t_lpend0_OFFSET(a0)
	lw t2, __soc_esf_t_lpcount0_OFFSET(a0)
	csrw PULP_LPSTART0, t0
	csrw PULP_LPEND0, t1
	csrw PULP_LPCOUNT0, t2
	lw t0, __soc_esf_t_lpstart1_OFFSET(a0)
	lw t1, __soc_esf_t_lpend1_OFFSET(a0)
	lw t2, __soc_esf_t_lpcount1_OFFSET(a0)
	csrw PULP_LPSTART1, t0
	csrw PULP_LPEND1, t1
	csrw PULP_LPCOUNT1, t2

	/* Return */
	jalr x0, ra
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 *
 * The pulpino core has:
 * 1) an ICP register, which is used to clear the pending
 *    IRQ number upon an interrupt.
 * 2) an ECP register, which is used to clear the pending IRQ number
 *    that has woken up the CPU from sleep state.
 *
 * Exception number is given as parameter via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	/* Clear exception number from the Interrupt pending register */
	li t1, PULP_ICP_ADDR
	li t2, 1
	sll t3, t2, a0
	sw t3, 0x00(t1)

	/* Clear exception number from the Event pending register */
	li t1, PULP_ECP_ADDR
	sw t3, 0x00(t1)

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
	andi t0, t0, SOC_MCAUSE_EXP_MASK

	/* if IRQ number < PULP_MIN_IRQ, not interrupt */
	li t1, PULP_MIN_IRQ
	addi a0, x0, 0
	blt t0, t1, not_interrupt
	addi a0, a0, 1

not_interrupt:
	/* return */
	jalr x0, ra
