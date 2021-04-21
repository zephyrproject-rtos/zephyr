/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <soc_pinmap.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

void eos_s3_lock_enable(void)
{
	MISC_CTRL->LOCK_KEY_CTRL = MISC_LOCK_KEY;
}

void eos_s3_lock_disable(void)
{
	MISC_CTRL->LOCK_KEY_CTRL = 1;
}

int eos_s3_io_mux(uint32_t pad_nr, uint32_t pad_cfg)
{
	volatile uint32_t *p = (uint32_t *)IO_MUX_BASE;

	if (pad_nr > EOS_S3_MAX_PAD_NR) {
		return -EINVAL;
	}

	p += pad_nr;
	*p = pad_cfg;

	return 0;
}

static void eos_s3_cru_init(void)
{
	/* Set desired frequency */
	AIP->OSC_CTRL_0 |= AIP_OSC_CTRL_EN;
	AIP->OSC_CTRL_0 &= ~AIP_OSC_CTRL_FRE_SEL;
	OSC_SET_FREQ_INC(HSOSC_60MHZ);

	while (!OSC_CLK_LOCKED()) {
		;
	}

	/* Enable all clocks for every domain */
	CRU->CLK_DIVIDER_CLK_GATING = (CLK_DIVIDER_A_CG | CLK_DIVIDER_B_CG
		| CLK_DIVIDER_C_CG | CLK_DIVIDER_D_CG | CLK_DIVIDER_F_CG
		| CLK_DIVIDER_G_CG | CLK_DIVIDER_H_CG | CLK_DIVIDER_I_CG
		| CLK_DIVIDER_J_CG);

	/* Turn off divisor for A0 domain */
	CRU->CLK_CTRL_A_0 = 0;

	/* Enable UART, WDT and TIMER peripherals */
	CRU->C11_CLK_GATE = C11_CLK_GATE_PATH_0_ON;

	/* Set divider for domain C11 to ~ 5.12MHz */
	CRU->CLK_CTRL_D_0 = (CLK_CTRL_CLK_DIVIDER_ENABLE |
		CLK_CTRL_CLK_DIVIDER_RATIO_12);
}



static int eos_s3_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Clocks setup */
	eos_s3_lock_enable();
	eos_s3_cru_init();
	eos_s3_lock_disable();

	SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk;

	/* Clear all interrupts */
	INTR_CTRL->OTHER_INTR = 0xFFFFFF;

	/* Enable UART interrupt */
	INTR_CTRL->OTHER_INTR_EN_M4 = UART_INTR_EN_M4;

	key = irq_lock();

	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(eos_s3_init, PRE_KERNEL_1, 0);
