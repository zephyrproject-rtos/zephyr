/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/irq.h>

/* exports */
GTEXT(__start)
GTEXT(__ivt)
GTEXT(__rv32_isr_wrapper)

/* imports */
GTEXT(__initialize)
#if defined(CONFIG_GEN_SW_ISR_TABLE)
GTEXT(_isr_wrapper)
#endif

SECTION_FUNC(vectors, __ivt)
	.option push
	.option norvc
	j __start
	j _isr_wrapper
	j _isr_wrapper
	j _isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper
	j __rv32_isr_wrapper

SECTION_FUNC(vectors, __rv32_isr_wrapper)
	addi sp, sp, -8
	sw  t0, 0x00(sp)
	sw  t1, 0x04(sp)
	csrr t0, mcause
	/* Synthesize interrupt cause by setting 31st bit */
	li t1, 1 << RISCV_MCAUSE_IRQ_POS
	or t0, t0, t1
	csrw mcause, t0
	lw  t0, 0x00(sp)
	lw  t1, 0x04(sp)
	addi sp, sp, 8
	j _isr_wrapper

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

	la t0, __ivt /* Load address of interrupt vector table */
	addi t0, t0, 0x01 /* Enable vectored mode by setting LSB */
	csrw mtvec, t0

	/* Enable IRQs */
	csrwi mstatus, 8

	/* Jump to __reset */
	tail __reset
