/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 */

#include <kernel_internal.h>
#include <irq.h>
#include <string.h>
#include <soc.h>

static void sw0_isr(void *arg)
{	/* the context switch is handled in assembler */
	mips_biccr(CR_SINT0);
}

static void interrupt_init(void)
{
	extern char __isr_vec[];
	extern uint32_t mips_cp0_status_int_mask;
	unsigned long ebase;

	irq_lock();

	mips_cp0_status_int_mask = 0;

	ebase = 0x80000000;

	memcpy((void *)(ebase + 0x180), __isr_vec, 0x80);

	/*
	 * disable boot exception vector in BOOTROM,
	 * use exception vector in RAM
	 */
	mips_bicsr(SR_BEV);

	/* init software interrupt for kernel */
	IRQ_CONNECT(MIPS_MACHINE_SOFT_IRQ, 0, sw0_isr, 0, 0);
	arch_irq_enable(MIPS_MACHINE_SOFT_IRQ);
}

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void _PrepC(void)
{
	z_bss_zero();

	interrupt_init();

	z_cstart();
	CODE_UNREACHABLE;
}
