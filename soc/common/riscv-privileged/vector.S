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
#if defined(CONFIG_GEN_SW_ISR_TABLE)
GTEXT(_isr_wrapper)
#endif

SECTION_FUNC(vectors, __start)
#if defined(CONFIG_RISCV_GP)
	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	.option norvc;

#if defined(CONFIG_SOC_RESET_HOOK)
	call soc_reset_hook
#endif

#if defined(CONFIG_RISCV_VECTORED_MODE)
#if defined(CONFIG_RISCV_HAS_CLIC)

	/*
	 * CLIC vectored mode
	 *
	 * CLIC vectored mode uses mtvec exclusively for exception handling and
	 * mtvec.base must be aligned to 64 bytes (this is done using
	 * CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT)
	 */
#if defined(CONFIG_GEN_SW_ISR_TABLE)
	la t0, _isr_wrapper
#else
	add t0, zero, zero
#endif
	addi t0, t0, 0x03 /* Enable CLIC vectored mode by setting LSB */
	csrw mtvec, t0

	/*
	 * CLIC vectored mode has a similar concept to CLINT vectored mode,
	 * where an interrupt vector table is used for specific interrupts.
	 * However, in CLIC vectored mode, the handler table contains the
	 * address of the interrupt handler instead of an opcode containing a
	 * jump instruction, this is done by leveraging
	 * CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS.
	 * When an interrupt occurs in CLIC vectored mode, the address of the
	 * handler entry from the vector table is loaded and then jumped to in
	 * hardware. This time mtvt is used as the base address for the
	 * interrupt table.
	 */
	la t0, _irq_vector_table
	csrw 0x307, t0 /* mtvt */

#else /* !CONFIG_RISCV_HAS_CLIC */

	/*
	 * CLINT vectored mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _irq_vector_table (interrupt vector table). Add 1 to base
	 * address of _irq_vector_table to indicate that vectored mode
	 * is used (LSB = 0x1). CPU will mask the LSB out of
	 * the address so that base address of _irq_vector_table is used.
	 *
	 * NOTE: _irq_vector_table is 256-byte aligned. Incorrect alignment
	 *        of _irq_vector_table breaks this code.
	 */
	la t0, _irq_vector_table /* Load address of interrupt vector table */
	addi t0, t0, 0x01 /* Enable vectored mode by setting LSB */
	csrw mtvec, t0

#endif /* CONFIG_RISCV_HAS_CLIC */

#else /* !CONFIG_RISCV_VECTORED_MODE */

#if defined(CONFIG_RISCV_HAS_CLIC) && !defined(CONFIG_LEGACY_CLIC)

	la t0, _isr_wrapper
	addi t0, t0, 0x03 /* Set mode bits to 3, signifying CLIC. Everything else is reserved. */
	csrw mtvec, t0

#else /* !CONFIG_RISCV_HAS_CLIC || CONFIG_LEGACY_CLIC */

	/*
	 * CLINT direct mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0

#endif /* CONFIG_RISCV_HAS_CLIC&& !CONFIG_LEGACY_CLIC */

#endif /* CONFIG_RISCV_VECTORED_MODE */

	/* Jump to __reset */
	tail __reset
