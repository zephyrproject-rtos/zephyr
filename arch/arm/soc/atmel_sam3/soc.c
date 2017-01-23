/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Atmel SAM3 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Atmel SAM3 family processor.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <arch/cpu.h>

/**
 * @brief Setup various clock on SoC.
 *
 * Setup the SoC clocks according to section 28.12 in datasheet.
 *
 * Assumption:
 * SLCK = 32.768kHz
 */
static ALWAYS_INLINE void clock_init(void)
{
	uint32_t tmp;

	/* Note:
	 * Magic numbers below are obtained by reading the registers
	 * when the SoC was running the SAM-BA bootloader
	 * (with reserved bits set to 0).
	 */

#ifdef CONFIG_SOC_ATMEL_SAM3_EXT_SLCK
	/* This part is to switch the slow clock to using
	 * the external 32 kHz crystal oscillator.
	 */

	/* Select external crystal */
	__SUPC->cr = SUPC_CR_KEY | SUPC_CR_XTALSEL;

	/* Wait for oscillator to be stablized */
	while (!(__SUPC->sr & SUPC_SR_OSCSEL))
		;
#endif /* CONFIG_SOC_ATMEL_SAM3_EXT_SLCK */

#ifdef CONFIG_SOC_ATMEL_SAM3_EXT_MAINCK
	/* Start the external main oscillator */
	__PMC->ckgr_mor = PMC_CKGR_MOR_KEY | PMC_CKGR_MOR_MOSCRCF_4MHZ
			  | PMC_CKGR_MOR_MOSCRCEN | PMC_CKGR_MOR_MOSCXTEN
			  | PMC_CKGR_MOR_MOSCXTST;

	/* Wait for main oscillator to be stablized */
	while (!(__PMC->sr & PMC_INT_MOSCXTS))
		;

	/* Select main oscillator as source since it is more accurate
	 * according to datasheet.
	 */
	__PMC->ckgr_mor = PMC_CKGR_MOR_KEY | PMC_CKGR_MOR_MOSCRCF_4MHZ
			  | PMC_CKGR_MOR_MOSCRCEN | PMC_CKGR_MOR_MOSCXTEN
			  | PMC_CKGR_MOR_MOSCXTST | PMC_CKGR_MOR_MOSCSEL;

	/* Wait for main oscillator to be selected */
	while (!(__PMC->sr & PMC_INT_MOSCSELS))
		;
#ifdef CONFIG_SOC_ATMEL_SAM3_WAIT_MODE
	/*
	 * Instruct CPU enter Wait mode instead of Sleep mode to
	 * keep Processor Clock (HCLK) and thus be able to debug
	 * CPU using JTAG
	 */
	__PMC->fsmr |= PMC_FSMR_LPM;
#endif
#else
	/* Set main fast RC oscillator to 12 MHz */
	__PMC->ckgr_mor = PMC_CKGR_MOR_KEY | PMC_CKGR_MOR_MOSCRCF_12MHZ
			  | PMC_CKGR_MOR_MOSCRCEN;

	/* Wait for main fast RC oscillator to be stablized */
	while (!(__PMC->sr & PMC_INT_MOSCRCS))
		;
#endif /* CONFIG_SOC_ATMEL_SAM3_EXT_MAINCK */

	/* Use PLLA as master clock.
	 * According to datasheet, PMC_MCKR must not be programmed in
	 * a single write operation. So it seems the safe way is to
	 * get the system to use main clock (by setting CSS). Then set
	 * the prescaler (PRES). Finally setting it back to using PLL.
	 */

	/* Switch to main clock first so we can setup PLL */
	tmp = __PMC->mckr & ~PMC_MCKR_CSS_MASK;
	__PMC->mckr = tmp | PMC_MCKR_CSS_MAIN;

	/* Wait for clock selection complete */
	while (!(__PMC->sr & PMC_INT_MCKRDY))
		;

	/* Setup PLLA */
	__PMC->ckgr_pllar = PMC_CKGR_PLLAR_DIVA | PMC_CKGR_PLLAR_ONE
			    | PMC_CKGR_PLLAR_MULA
			    | PMC_CKGR_PLLAR_PLLACOUNT;

	/* Wait for PLL lock */
	while (!(__PMC->sr & PMC_INT_LOCKA))
		;

	/* Setup prescaler */
	tmp = __PMC->mckr & ~PMC_MCKR_PRES_MASK;
	__PMC->mckr = tmp | PMC_MCKR_PRES_CLK;

	/* Wait for main clock setup complete */
	while (!(__PMC->sr & PMC_INT_MCKRDY))
		;

	/* Finally select PLL as clock source */
	tmp = __PMC->mckr & ~PMC_MCKR_CSS_MASK;
	__PMC->mckr = tmp | PMC_MCKR_CSS_PLLA;

	/* Wait for main clock setup complete */
	while (!(__PMC->sr & PMC_INT_MCKRDY))
		;
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int atmel_sam3_init(struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Note:
	 * Magic numbers below are obtained by reading the registers
	 * when the SoC was running the SAM-BA bootloader
	 * (with reserved bits set to 0).
	 */

	key = irq_lock();

	/* Setup the flash controller.
	 * The bootloader is running @ 48 MHz with
	 * FWS == 2.
	 * When running at 84 MHz, FWS == 4 seems
	 * to be more stable, and allows the board
	 * to boot.
	 */
	__EEFC0->fmr = 0x00000400;
	__EEFC1->fmr = 0x00000400;

	/* Clear all faults */
	_ScbMemFaultAllFaultsReset();
	_ScbBusFaultAllFaultsReset();
	_ScbUsageFaultAllFaultsReset();

	_ScbHardFaultAllFaultsReset();

	/* Setup master clock */
	clock_init();

	/* Disable watchdog timer, not used by system */
	__WDT->mr |= WDT_DISABLE;

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_sam3_init, PRE_KERNEL_1, 0);
