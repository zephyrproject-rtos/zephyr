/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>


/*
 * Make sure PCR sleep enables are clear except for crypto
 * which do not have internal clock gating.
 */
static int soc_pcr_init(void)
{
	PCR_REGS->SLP_EN0 = 0;
	PCR_REGS->SLP_EN1 = 0;
	PCR_REGS->SLP_EN2 = 0;
	PCR_REGS->SLP_EN4 = 0;
	PCR_REGS->SLP_EN3 = MCHP_PCR3_CRYPTO_MASK;

	return 0;
}

/*
 * Select 32KHz clock source used for PLL reference.
 * Options are:
 * Internal 32KHz silicon oscillator.
 * External parallel resonant crystal between XTAL1 and XTAL2 pins.
 * External single ended crystal connected to XTAL2 pin.
 * External 32KHz square wave from Host chipset/board on 32KHZ_IN pin.
 * NOTES:
 *   PLL can take up to 3 ms to lock. Before lock the PLL output
 *   will be ramping up from ~20MHz.
 *   32KHZ_IN pin must be configured for 32KHZ_IN function.
 *   Crystals vary and may take longer time to stabilize this will
 *   affect PLL lock time.
 *   Crystal do not like to be power cycled. If using a crystal
 *   the board should supply a battery backed (VBAT) power rail.
 *   The VBAT clock control register selecting 32KHz source is
 *   connected to the VBAT power rail. If using a battery one can
 *   check the VBAT Power Fail and Reset Status register for a VBAT POR.
 */
static void clk32_change(uint8_t new_clk32)
{
	new_clk32 &= MCHP_VBATR_CLKEN_MASK;

	if ((VBATR_REGS->CLK32_EN & MCHP_VBATR_CLKEN_MASK)
		== (uint32_t)new_clk32) {
		return;
	}

	if (new_clk32 == MCHP_VBATR_USE_SIL_OSC) {
		VBATR_REGS->CLK32_EN = new_clk32;
	} else {
		/* 1. switch to internal oscillator */
		VBATR_REGS->CLK32_EN = MCHP_VBATR_USE_SIL_OSC;
		/* 2. delay for PLL */
		while ((PCR_REGS->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK) == 0)
			;
		/* 3. switch to desired source */
		VBATR_REGS->CLK32_EN = new_clk32;
	}
}

static int soc_clk32_init(void)
{
	uint8_t new_clk32;

#ifdef CONFIG_SOC_MEC1501_EXT_32K
  #ifdef CONFIG_SOC_MEC1501_EXT_32K_CRYSTAL
    #ifdef CONFIG_SOC_MEC1501_EXT_32K_PARALLEL_CRYSTAL
	new_clk32 = MCHP_VBATR_USE_PAR_CRYSTAL;
    #else
	new_clk32 = MCHP_VBATR_USE_SE_CRYSTAL;
    #endif
  #else
	/* Use 32KHZ_PIN as 32KHz source */
	new_clk32 = MCHP_VBATR_USE_32KIN_PIN;
  #endif
#else
	/* Use internal 32KHz +/-2% silicon oscillator
	 * if required performed OTP value override
	 */
	if (MCHP_REVISION_ID() == MCHP_GCFG_REV_B0) {
		VBATR_REGS->CKK32_TRIM = 0x06;
	}
	new_clk32 = MCHP_VBATR_USE_SIL_OSC;
#endif
	clk32_change(new_clk32);

	return 0;
}

/*
 * Initialize MEC1501 EC Interrupt Aggregator (ECIA) and external NVIC
 * inputs.
 */
static int soc_ecia_init(void)
{
	GIRQ_Type *pg;
	uint32_t n;

	mchp_pcr_periph_slp_ctrl(PCR_ECIA, MCHP_PCR_SLEEP_DIS);

	ECS_REGS->INTR_CTRL |= MCHP_ECS_ICTRL_DIRECT_EN;

	/* gate off all aggregated outputs */
	ECIA_REGS->BLK_EN_CLR = 0xFFFFFFFFul;
	/* gate on GIRQ's that are aggregated only */
	ECIA_REGS->BLK_EN_SET = MCHP_ECIA_AGGR_BITMAP;

	/* Clear all GIRQn source enables and source status */
	pg = &ECIA_REGS->GIRQ08;
	for (n = MCHP_FIRST_GIRQ; n <= MCHP_LAST_GIRQ; n++) {
		pg->EN_CLR = 0xFFFFFFFFul;
		pg->SRC = 0xFFFFFFFFul;
		pg++;
	}

	/* Clear all external NVIC enables and pending status */
	for (n = 0u; n < MCHP_NUM_NVIC_REGS; n++) {
		NVIC->ICER[n] = 0xFFFFFFFFul;
		NVIC->ICPR[n] = 0xFFFFFFFFul;
	}

	return 0;
}

static int soc_init(const struct device *dev)
{
	uint32_t isave;

	ARG_UNUSED(dev);

	isave = __get_PRIMASK();
	__disable_irq();

	soc_pcr_init();

	soc_clk32_init();

	/*
	 * On HW reset PCR Processor Clock Divider = 4 for 48/4 = 12 MHz.
	 * Set clock divider = 1 for maximum speed.
	 * NOTE1: This clock divider affects all Cortex-M4 core clocks.
	 * If you change it you must repogram SYSTICK to maintain the
	 * same absolute time interval.
	 */
	PCR_REGS->PROC_CLK_CTRL = CONFIG_SOC_MEC1501_PROC_CLK_DIV;

	soc_ecia_init();

	if (!isave) {
		__enable_irq();
	}

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
