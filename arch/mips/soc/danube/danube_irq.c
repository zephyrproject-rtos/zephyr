/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief danube interrupt management code
 */

#include <irq.h>
#include <soc.h>
#include <generated_dts_board.h>

void _arch_irq_enable(unsigned int irq)
{
	int key;

	key = irq_lock();
	mips_bissr(1 << (irq + 8));
	irq_unlock(key);
};

void _arch_irq_disable(unsigned int irq)
{
	int key;

	key = irq_lock();
	mips_bicsr(1 << (irq + 8));
	irq_unlock(key);
};

int _arch_irq_is_enabled(unsigned int irq)
{
	return !!(mips_getsr() & (1 << (irq + 8)));
}

void sw0_isr(void *arg)
{	/* the context switch is handled in assembler */
	mips_biccr(CR_SINT0);
}

#if defined(CONFIG_MIPS_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void)
{
	/* ensure that all interrupts are disabled */
	(void)irq_lock();
	/* set BEV to allow modification of ebase */
	mips_bissr(SR_BEV);
	/* set wr in ebase to allow setting the top 2 bits of excbase */
	mips32_setebase(0x00000400);
	/* make sure ebase is set to CONFIG_SRAM_BASE_ADDRESS */
	mips32_setebase(CONFIG_SRAM_BASE_ADDRESS);
	/* enable single vector mode by setting IV in the cause register */
	mips_biscr(CR_IV);
	/* clear BEV to separate interrupts from exceptions */
	mips_bicsr(SR_BEV);

	mips32_setintctl((mips32_getintctl() & ~INTCTL_VS) | INTCTL_VS_0);

	/* init software interrupt for kernel */
	IRQ_CONNECT(MIPS_MACHINE_SOFT_IRQ, 0, sw0_isr, 0, 0);
	mips_bissr(SR_SINT0);
}
#endif
